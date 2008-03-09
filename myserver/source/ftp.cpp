/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../include/ftp.h"
#include "../include/ftp_common.h"
#include "../include/ftp_parser.h"
#include "../include/clients_thread.h"
#include "../include/server.h"
#include "../include/files_utility.h"
#include "../include/file.h"
#include "../include/files_utility.h"
#include "../include/lfind.h"
#include "../include/stringutils.h"
#include <assert.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//////////////////////////////////////////////////////////////////////////////
// FtpHost class
void SetFtpHost(FtpHost &out, const FtpHost &in)
{
	out.h1 = in.h1;
	out.h2 = in.h2;
	out.h3 = in.h3;
	out.h4 = in.h4;
	out.p1 = in.p1;
	out.p2 = in.p2;
}

const char *GetIpAddr(const FtpHost &host)
{
	std::ostringstream sRet;
	sRet << host.h1 << '.' << host.h2 << '.' << host.h3 << '.' << host.h4;
	return sRet.str().c_str();
}

int GetPortNo(const FtpHost &host)
{
	return ((host.p1 << 8) + host.p2);
}

//////////////////////////////////////////////////////////////////////////////
// Used at Ftp access control
/*! Cache for security files. */
SecurityCache Ftp::secCache;

/*! Access the security cache safely. */
Mutex Ftp::secCacheMutex;

//////////////////////////////////////////////////////////////////////////////
// FtpUserData class
FtpUserData::FtpUserData()
{
	reset();
}

FtpUserData::~FtpUserData()
{
	delete m_pDataConnection;
	m_pDataConnection = NULL;
}

void FtpUserData::reset()
{
	m_nFtpState = NO_CONTROL_CONNECTION;
	m_pDataConnection = new Connection();
	m_sUserName = m_sPass = "";
	m_nFtpRepresentation = REPR_ASCII;
	m_nFtpFormatControl = NON_PRINT;
	m_nFtpFileStructure = STRU_FILE;
	m_nTransferMode = MODE_STREAM;
	m_cwd = "";
	m_nLocalDataPort = 0;
	m_bBreakDataConnection = false;
	m_dataThreadId = 0;
	m_bPassiveSrv = false;
	m_nRestartOffset = 0;
	m_sCurrentFileName = "";
	m_nFileSize = 0;
	m_nBytesSent = 0;
}

//////////////////////////////////////////////////////////////////////////////
// FtpThreadContext helper structure
FtpThreadContext::FtpThreadContext()
{
	pConnection = NULL;
	buffer = NULL;
	buffer2 = NULL;
	buffersize = 0;
	buffersize2 = 0;
	m_nParseLength = 0;
	pProtocolInterpreter = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Ftp helper structure
struct reply
{
	int nCode;
	std::string sText;
};

struct reply reply_table[] = 
{
	{ 120, "Service ready in %s minutes." },
	{ 125, "Data connection already open; transfer starting." },
	{ 150, "File status okay; about to open data connection." },
	{ 200, "Command okay." },
	{ 213, "File status." },
	{ 214, "The following commands are recognized:\r\n\
USER, PASS, PORT, TYPE, RETR, QUIT\r\n\
214 Detailed help on commands will soon be available." },
	{ 215, "%s system type." },
	{ 220, "Service ready for new user." },
	{ 221, "Goodbye." },
	{ 226, "Closing data connection." },
	{ 230, "User logged in, proceed."},
	{ 250, "Requested file action okay, completed."},
	{ 257, "PATHNAME created" },
	{ 331, "User name okay, need password." },
	{ 350, "Requested file action pending further information." },
	{ 421, "Service not available, closing control connection." },
	{ 425, "Can't open data connection." },
	{ 426, "Connection closed; Transfer aborted." },
	{ 450, "Requested file action not taken. File unavailable." },
	{ 451, "Requested action aborted: local error in processing." },
	{ 502, "Command not implemented." },
	{ 503, "Bad sequence of commands." },
	{ 500, "Syntax error, command unrecognized." },
	{ 501, "Syntax error in parameters or arguments."},
	{ 504, "Command not implemented for that parameter."},
	{ 530, "Not logged in." },
	{ 550, "Requested action not taken. File unavailable." },
	{ 0, "" }
};

//////////////////////////////////////////////////////////////////////////////
// Ftp class

bool Ftp::m_bAnonymousNeedPass = false;

Ftp::Ftp()
{
}

Ftp::~Ftp()
{
}

int Ftp::controlConnection(ConnectionPtr pConnection, char *b1, char *b2,
			int bs1, int bs2, u_long nbtr, u_long id)
{
	if ( pConnection == NULL )
		return ClientsThread::DELETE_CONNECTION;

	FtpUserData *pFtpUserData = NULL;
	if ( pConnection->protocolBuffer == NULL )
		pConnection->protocolBuffer = new FtpUserData();
	pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
	if ( pFtpUserData == NULL )
		return ClientsThread::DELETE_CONNECTION;

	// check if ftp is buisy(return 120) or unavailable(return 421)
	if ( pFtpUserData->m_nFtpState == FtpUserData::BUISY )
	{
		// TODO: really compute buisy time interval
		std::string sTempText;
		get_ftp_reply(120, sTempText);
		std::string::size_type n = sTempText.find("%s");
		if ( n != std::string::npos )
			sTempText.replace(n, 2, "10");
		ftp_reply(120, sTempText);
	}
	if ( pFtpUserData->m_nFtpState == FtpUserData::UNAVAILABLE )
		ftp_reply(421);

	// init default local ports
	m_nLocalControlPort = pConnection->getLocalPort();
	pFtpUserData->m_nLocalDataPort = m_nLocalControlPort - 1;

	if ( pFtpUserData->m_cwd.empty() && pConnection->host != NULL )//current dir not initialized
		pFtpUserData->m_cwd = pConnection->host->getDocumentRoot();

	//switch context
	td.pConnection = pConnection;
	td.buffer = pConnection->getActiveThread()->getBuffer();
	td.buffer2 = pConnection->getActiveThread()->getBuffer2();
	td.buffersize = bs1;
	td.buffersize2 = bs2;
	td.nBytesToRead = nbtr;
	td.pProtocolInterpreter = this;
	td.m_nParseLength = 0;

	return ParseControlConnection();
}

int Ftp::loadProtocol(XmlParser*)
{
	// load custom messages from cfg here
	XmlParser *configurationFileManager = Server::getInstance()->getConfiguration();
	if ( configurationFileManager == NULL )
		return 0;
	char *pData = configurationFileManager->getValue("ANONYMOUS_NEED_PASS");
	int nAnonymousNeedPass = 0;
	if ( pData != NULL )
		nAnonymousNeedPass = atoi(pData);
	m_bAnonymousNeedPass = nAnonymousNeedPass != 0;
	return 1;
}

int Ftp::unLoadProtocol(XmlParser*)
{
	if ( true/*everything is ok*/ )
		return 1;
	else
		return 0;
}

void Ftp::ftp_reply(int nReplyCode, const std::string &sCustomText /*= ""*/)
{
	if ( td.pConnection == NULL || td.pConnection->socket == NULL ||
			td.buffer2 == NULL )
		return;

	if ( !sCustomText.empty() )
	{
		sprintf(td.buffer2->getBuffer(), "%d %s", nReplyCode, sCustomText.c_str());
	}
	else
	{
		std::string sReplyText;
		get_ftp_reply(nReplyCode, sReplyText);
		if ( sReplyText.find('\n') == std::string::npos )
			sprintf(td.buffer2->getBuffer(), "%d %s", nReplyCode, sReplyText.c_str());
		else
			sprintf(td.buffer2->getBuffer(), "%d-%s", nReplyCode, sReplyText.c_str());
	}
	td.buffer2->setLength(strlen(td.buffer2->getBuffer()));
	*td.buffer2 << (const char*)"\r\n";
	td.pConnection->socket->send(td.buffer2->getBuffer(), td.buffer2->getLength(), 0);
}

int Ftp::get_ftp_reply(int nReplyCode, std::string &sReply)
{
	for ( int i = 0; reply_table[i].nCode != 0; i++ )
	{
		if ( reply_table[i].nCode != nReplyCode )
			continue;
		sReply = reply_table[i].sText;
		return 1;
	}
	return 0;
}

int Ftp::PrintError(const char *msg)
{
	printf("eroare: %s", msg);
	return 1;
}

void Ftp::User(const std::string &sParam)
{
	assert(td.pConnection != NULL);
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);

	switch ( pFtpUserData->m_nFtpState )
	{
		case FtpUserData::CONTROL_CONNECTION_UP:
		case FtpUserData::USER_LOGGED_IN:
			pFtpUserData->m_sUserName = sParam;
			if ( !m_bAnonymousNeedPass )
			{
				pFtpUserData->m_nFtpState = FtpUserData::USER_LOGGED_IN;
				ftp_reply(230);
			}
			else
			{
				pFtpUserData->m_nFtpState = FtpUserData::CONTROL_CONNECTION_UP;
				ftp_reply(331);
			}
			break;
		case FtpUserData::UNAVAILABLE:
			ftp_reply(421);
		default://error
			{
				//assert(false);
				ftp_reply(530);
			}
	}
}

void Ftp::Password(const std::string &sParam)
{
	assert(td.pConnection != NULL);
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);

	switch ( pFtpUserData->m_nFtpState )
	{
		case FtpUserData::CONTROL_CONNECTION_UP:
			pFtpUserData->m_sPass = sParam;
			pFtpUserData->m_nFtpState = FtpUserData::USER_LOGGED_IN;
			ftp_reply(230);
			break;
		case FtpUserData::USER_LOGGED_IN:
			if ( m_bAnonymousNeedPass )
				ftp_reply(503);
			else
				ftp_reply(230);
			break;
		default://error
			assert(false);
	}
}

void Ftp::Port(const FtpHost &host)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	SetFtpHost(pFtpUserData->m_cdh, host);
	ftp_reply(200);
}

void Ftp::Pasv(const FtpHost &host)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	SetFtpHost(pFtpUserData->m_cdh, host);
	ftp_reply(200);
}

void Ftp::Retr(const std::string &sPath)
{
	std::string sLocalPath;
	if ( !UserLoggedIn() || OpenDataConnection() == 0 || !GetLocalPath(sPath, sLocalPath) )
	{
		CloseDataConnection();
		return;
	}

	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	if ( pFtpUserData == NULL )
	{
		assert(pFtpUserData != NULL);
		ftp_reply(451);
		CloseDataConnection();
		return;
	}
	std::string sLocalDir, sLocalFileName;
	FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);

	/* The security file doesn't exist in any case.  */
    	if( !strcmpi(sLocalFileName.c_str(), "security") )
	{
		ftp_reply(550);
		CloseDataConnection();
		return;
	}

	SecurityToken st;
	if ( strcmpi(pFtpUserData->m_sUserName.c_str(), "anonymous") == 0 )
	{
		st.user = "Guest";
		st.password = "";
	}
	else
	{
		st.user = pFtpUserData->m_sUserName.c_str();
		st.password = pFtpUserData->m_sPass.c_str();
	}
	st.directory = sLocalDir.c_str();
	st.sysdirectory = pFtpUserData->m_pDataConnection->host->getSystemRoot();
	st.authType = 0;
	st.filename = sLocalFileName.c_str();
	//st.providedMask = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE;
	int permMask = -1;
	secCacheMutex.lock();
	try
	{
		permMask = secCache.getPermissionMask (&st);
          	secCacheMutex.unlock();
	}
	catch ( ... )
	{
		secCacheMutex.unlock();
		throw;
	}
	if ( permMask == -1 || (permMask & (MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE)) == 0 )
	{
		ftp_reply(550);
		CloseDataConnection();
		return;
	}

	RetrWorkerThreadData *pRetrData = new RetrWorkerThreadData();
	pRetrData->m_pFtp = this;
	pRetrData->m_sFilePath = sLocalPath;

	pFtpUserData->m_sCurrentFileName = "";
	pFtpUserData->m_nFileSize = 0;
	pFtpUserData->m_nBytesSent = 0;

	switch ( pFtpUserData->m_nFtpRepresentation )
	{
		case FtpUserData::REPR_ASCII:
			      Thread::create(&pFtpUserData->m_dataThreadId, SendAsciiFile, pRetrData);
			break;
		case FtpUserData::REPR_IMAGE:
			      Thread::create(&pFtpUserData->m_dataThreadId, SendImageFile, pRetrData);
			break;
		default:
			;
	}

	//ftp_reply(226);
	//CloseDataConnection();

}

#ifdef WIN32
unsigned int __stdcall SendAsciiFile(void* pParam)
#elif HAVE_PTHREAD
void* SendAsciiFile(void* pParam)
#endif //HAVE_PTHREAD
{
	RetrWorkerThreadData *pWtpd = reinterpret_cast<RetrWorkerThreadData *>(pParam);
	if ( pWtpd == NULL )
	{
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}
	Ftp *pFtp = pWtpd->m_pFtp;
	if ( pFtp == NULL )
	{
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}

	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pFtp->td.pConnection->protocolBuffer);
	if ( pFtpUserData == NULL )
	{
		assert(pFtpUserData != NULL);
		pFtp->CloseDataConnection();
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}
      	if( pFtpUserData->m_pDataConnection == NULL || 
			pFtpUserData->m_pDataConnection->socket == NULL)
	{
		pFtp->CloseDataConnection();
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}

	File *file = NULL;
	try
	{
		file = Server::getInstance()->getCachedFiles()->open(pWtpd->m_sFilePath.c_str());
	    	u_long filesize = file->getFileSize();
		if ( pFtpUserData->m_nRestartOffset > 0 )
			pFtpUserData->m_nRestartOffset = 0;// don't implement restart for ASCII

		pFtpUserData->m_sCurrentFileName = pWtpd->m_sFilePath;
		pFtpUserData->m_nFileSize = filesize;

		u_long nbr, nBufferSize = 0;
		char *pLine = NULL;
		int nLineLength = 0;
		std::string sLine;
		while ( filesize != 0 )
		{
			nBufferSize = std::min(static_cast<u_long>(filesize), static_cast<u_long>(pFtp->td.buffer->getRealLength()/2));
			if ( file->readFromFile(pFtp->td.buffer->getBuffer(), nBufferSize, &nbr) )
			{
				pFtp->ftp_reply(451);
				file->closeFile();
				delete file;
				pFtp->CloseDataConnection();
				delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
			}
			filesize -= nbr;
			pFtpUserData->m_nBytesSent += nbr;

			MemBuf &buffer2 = *pFtp->td.buffer2;
			buffer2.setLength(0);
			pLine = pFtp->td.buffer->getBuffer();
			if ( pLine == NULL )
			{
				pFtp->ftp_reply(451);
				file->closeFile();
				delete file;
				pFtp->CloseDataConnection();
				delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
			}
			while ( *pLine != 0 )
			{
				nLineLength = getEndLine(pLine, 0);
				if ( nLineLength < 0 )//last line
					nLineLength = strlen(pLine);
				sLine.assign(pLine, nLineLength);
				if ( !sLine.empty() )
					buffer2 << sLine << "\r\n";
				while ( *(pLine + nLineLength)  == '\r' || *(pLine + nLineLength) == '\n' )
					nLineLength++;
				pLine += nLineLength;
			}
      			if ( pFtpUserData->m_pDataConnection->socket->send(pFtp->td.buffer2->getBuffer(), 
					(u_long)pFtp->td.buffer2->getLength(), 0) == SOCKET_ERROR )
      			{
       				file->closeFile();
				delete file;
       				// report error
      			}
			if ( pFtpUserData->m_bBreakDataConnection )
			{
				pFtpUserData->m_bBreakDataConnection = false;
       				file->closeFile();
				delete file;
				pFtp->CloseDataConnection();
				delete pWtpd;
#ifdef WIN32
	return 1;
#elif HAVE_PTHREAD
	return (void*)1;
#endif
			}
		}
	}
	catch (bad_alloc &ba)
	{
		if ( file != NULL )
			file->closeFile();
		delete file;
		//report error
	}

	pFtpUserData->m_sCurrentFileName = "";
	pFtpUserData->m_nFileSize = 0;
	pFtpUserData->m_nBytesSent = 0;
	pFtpUserData->m_nRestartOffset = 0;
	pFtp->ftp_reply(226);
	pFtp->CloseDataConnection();
	delete pWtpd;
#ifdef WIN32
	return 1;
#elif HAVE_PTHREAD
	return (void*)1;
#endif

}

#ifdef WIN32
unsigned int __stdcall SendImageFile(void* pParam)
#elif HAVE_PTHREAD
void* SendImageFile(void* pParam)
#endif //HAVE_PTHREAD
{
	RetrWorkerThreadData *pWtpd = reinterpret_cast<RetrWorkerThreadData *>(pParam);
	if ( pWtpd == NULL )
	{
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}

	Ftp *pFtp = pWtpd->m_pFtp;
	if ( pFtp == NULL )
	{
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}

	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pFtp->td.pConnection->protocolBuffer);
	if ( pFtpUserData == NULL )
	{
		assert(pFtpUserData != NULL);
		pFtp->CloseDataConnection();
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}
      	if( pFtpUserData->m_pDataConnection == NULL || 
			pFtpUserData->m_pDataConnection->socket == NULL)
	{
		pFtp->CloseDataConnection();
		delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
	}

	File *file = NULL;
	try
	{
		file = Server::getInstance()->getCachedFiles()->open(pWtpd->m_sFilePath.c_str());
		if( file == NULL )
		{
			pFtp->ftp_reply(451);
			pFtp->CloseDataConnection();
			delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
		}
		u_long filesize = file->getFileSize();
		u_long nbr, nBufferSize = 0;
		//sendfile(pFtpUserData->m_pDataConnection->socket->getHandle(), file->getHandle(), &offset, filesize);
		if ( pFtpUserData->m_nRestartOffset > 0 && pFtpUserData->m_nRestartOffset < filesize )
		{
			file->setFilePointer(pFtpUserData->m_nRestartOffset);
			filesize -= pFtpUserData->m_nRestartOffset;
		}

		pFtpUserData->m_sCurrentFileName = pWtpd->m_sFilePath;
		pFtpUserData->m_nFileSize = filesize;

		while ( filesize != 0 )
		{
			pFtp->td.buffer2->setLength(0);
			nBufferSize = std::min(static_cast<u_long>(filesize), static_cast<u_long>(pFtp->td.buffer2->getRealLength()/2));
			if ( file->readFromFile(pFtp->td.buffer2->getBuffer(), nBufferSize, &nbr) ||
				pFtpUserData->m_pDataConnection->socket->send(pFtp->td.buffer2->getBuffer(), 
					(u_long)nBufferSize, 0) == SOCKET_ERROR )
			{
				pFtp->ftp_reply(451);
				file->closeFile();
				delete file;
				pFtp->CloseDataConnection();
				delete pWtpd;
#ifdef WIN32
	return 0;
#elif HAVE_PTHREAD
	return (void*)0;
#endif
			}
			filesize -= nbr;
			pFtpUserData->m_nBytesSent += nbr;
			pFtpUserData->m_nRestartOffset += nbr;
			if ( pFtpUserData->m_bBreakDataConnection )
			{
				pFtpUserData->m_bBreakDataConnection = false;
       				file->closeFile();
				delete file;
				pFtp->CloseDataConnection();
				delete pWtpd;
#ifdef WIN32
	return 1;
#elif HAVE_PTHREAD
	return (void*)1;
#endif
			}
		}
		file->closeFile();
		delete file;
	}
	catch (bad_alloc &ba)
	{
		if ( file != NULL )
			file->closeFile();
		delete file;
		//report error
	}

	pFtpUserData->m_sCurrentFileName = "";
	pFtpUserData->m_nFileSize = 0;
	pFtpUserData->m_nBytesSent = 0;
	pFtpUserData->m_nRestartOffset = 0;
	pFtp->ftp_reply(226);
	pFtp->CloseDataConnection();
	delete pWtpd;
#ifdef WIN32
	return 1;
#elif HAVE_PTHREAD
	return (void*)1;
#endif
}

bool Ftp::UserLoggedIn()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	if ( pFtpUserData == NULL )
	{
		assert(pFtpUserData != NULL);
		return false;
	}
	if ( pFtpUserData->m_nFtpState < FtpUserData::USER_LOGGED_IN )
	{
		ftp_reply(530);
		return false;
	}
	if ( pFtpUserData->m_nFtpState != FtpUserData::USER_LOGGED_IN)
		return false;//TODO: handle all cases and reply accordingly
	return true;
}

/*!
 *Converts from relative client's path to local path.
 *\param sPath client's relative path
 *\param sOutPath local path
 *\return Return true if path exist, file is a normal one and is into the ftp's root folder
 */
bool Ftp::GetLocalPath(const std::string &sPath, std::string &sOutPath)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	if ( sPath[0] == '/' )
		sOutPath = sPath;// UNIX absolute path
	else if ( sPath[0] == '-' ) // ls params not handled
		sOutPath = pFtpUserData->m_cwd;
	else
		sOutPath = pFtpUserData->m_cwd + "/" + sPath;
	//FilesUtility::completePath(sOutPath);
	if ( sOutPath.empty() || !FilesUtility::fileExists(sOutPath) || FilesUtility::isLink(sOutPath.c_str()) )
	{
		ftp_reply(550);
		return false;
	}

	///////////////////////////////////////
	// verify if file is in ftp root folder
	if ( pFtpUserData->m_cwd.empty() )//current dir not initialized
	{
		if ( pFtpUserData->m_pDataConnection != NULL && pFtpUserData->m_pDataConnection->host != NULL )
			pFtpUserData->m_cwd = pFtpUserData->m_pDataConnection->host->getDocumentRoot();
	}

	std::string sDocRoot(td.pConnection->host->getDocumentRoot());
	FilesUtility::completePath(sDocRoot);
	if ( FilesUtility::getPathRecursionLevel(sDocRoot) > FilesUtility::getPathRecursionLevel(sOutPath) )
	{
		ftp_reply(550);
		return false;
	}
	///////////////////////////////////////
	
	return true;
}

void Ftp::Quit()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	pFtpUserData->m_nFtpState = FtpUserData::NO_CONTROL_CONNECTION;
	ftp_reply(221);
}

void Ftp::Help(const std::string &sCmd/* = ""*/)
{
	// treat SITE the same as HELP
	if ( sCmd.empty() || stringcmpi(sCmd, "SITE") == 0 )
		ftp_reply(214);
	else
		ftp_reply(502);
}

void Ftp::Noop()
{
	;//do nothing :)
}

int Ftp::OpenDataConnection()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
	{
		ftp_reply(125);
		return 1;
	}

	ftp_reply(150);
	int nRet = pFtpUserData->m_bPassiveSrv ? OpenDataPassive() : OpenDataActive();
	if ( nRet == 0 )
		ftp_reply(425);
	else
		pFtpUserData->m_nFtpState = FtpUserData::DATA_CONNECTION_UP;
	return nRet;
}

#ifdef WIN32
unsigned int __stdcall listenData(void *argv)
#else
void *listenData(void *argv)
#endif //WIN32
{
	return 0;
}

int Ftp::OpenDataPassive()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	Server* server = Server::getInstance();

	Socket dataSocket;
	dataSocket.socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (dataSocket.getHandle() == (SocketHandle)INVALID_SOCKET)
	{
		server->logPreparePrintError();
		//TODO: create an error message for this condition
		//server->logWriteln(languageParser->getValue("ERR_OPENP"));
		server->logEndPrintError();
	}

	MYSERVER_SOCKADDR_STORAGE sockServerSocketIPv4 = { 0 };
	//server->logWriteln(languageParser->getValue("MSG_SSOCKRUN"));
	((sockaddr_in*)(&sockServerSocketIPv4))->sin_family = AF_INET;
	((sockaddr_in*)(&sockServerSocketIPv4))->sin_addr.s_addr = 
		inet_addr(GetIpAddr(pFtpUserData->m_cdh));
	((sockaddr_in*)(&sockServerSocketIPv4))->sin_port =
		htons((u_short)GetPortNo(pFtpUserData->m_cdh));

#ifdef NOT_WIN
	/*
	 *Under the unix environment the application needs some time before
	 * create a new socket for the same address.
	 *To avoid this behavior we use the current code.
	 */
	int optvalReuseAddr = 1;
	if(dataSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 
		(const char *)&optvalReuseAddr, sizeof(optvalReuseAddr)) < 0)
	{
		server->logPreparePrintError();
		//TODO: create an error message for this condition
		//server->logWriteln(languageParser->getValue("ERR_ERROR"));
		server->logEndPrintError();
	}
#endif
	/*
	 *Bind data port.
	 */
	//server->logWriteln(languageParser->getValue("MSG_BIND_PORT"));
	if (dataSocket.bind(&sockServerSocketIPv4, sizeof(sockaddr_in)) != 0)
	{
		server->logPreparePrintError();
		//TODO: create an error message for this condition
		//server->logWriteln(languageParser->getValue("ERR_BIND"));
		server->logEndPrintError();
	}
	else
		;//server->logWriteln(languageParser->getValue("MSG_PORT_BOUND"));
	if ( dataSocket.listen(GetPortNo(pFtpUserData->m_cdh)) < 0 )
	{
		//TODO: add errno code
		ftp_reply(425);
		return 0;
	}
	/*
	 *Set connections listen queque to max allowable.
	 */
	//server->logWriteln(languageParser->getValue("MSG_SLISTEN"));
	if (dataSocket.listen(SOMAXCONN))
	{
		server->logPreparePrintError();
		//server->logWriteln(languageParser->getValue("ERR_LISTEN"));
		server->logEndPrintError();
	}

	ostringstream portBuff;
	portBuff << (u_int)GetPortNo(pFtpUserData->m_cdh);
	string listenPortMsg;
	//listenPortMsg.assign(languageParser->getValue("MSG_LISTEN"));
	listenPortMsg.append(": ");
	listenPortMsg.append(portBuff.str());
	server->logWriteln(listenPortMsg.c_str());

	pFtpUserData->m_pDataConnection->socket = new Socket(dataSocket);
	ThreadID threadId = 0;
	void *dataArgv = NULL;
	Thread::create(&threadId, &::listenData, dataArgv);

	return 1;
}

int Ftp::OpenDataActive()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);

	Socket dataSocket;
	dataSocket.socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( dataSocket.connect(GetIpAddr(pFtpUserData->m_cdh), GetPortNo(pFtpUserData->m_cdh)) < 0 )
	{
		//TODO: add errno code
		ftp_reply(425);
		return 0;
	}

	char localIp[MAX_IP_STRING_LEN];
	memset(localIp, 0, MAX_IP_STRING_LEN);
	MYSERVER_SOCKADDRIN  localSockIn = { 0 };
	int nDim = sizeof(sockaddr_in);
	dataSocket.getsockname((MYSERVER_SOCKADDRIN *)&localSockIn, &nDim);
	int nNameRet = getnameinfo(reinterpret_cast<const sockaddr *>(&localSockIn), 
		sizeof(sockaddr_in), localIp, MAX_IP_STRING_LEN, NULL, 0, NI_NUMERICHOST);
	if ( nNameRet != 0 )
	{
		//TODO: errno code
		ftp_reply(425);
		return 0;
	}

	pFtpUserData->m_pDataConnection->setPort(GetPortNo(pFtpUserData->m_cdh));
	pFtpUserData->m_pDataConnection->setLocalPort(pFtpUserData->m_nLocalDataPort);
	pFtpUserData->m_pDataConnection->setIpAddr(td.pConnection->getIpAddr());
	pFtpUserData->m_pDataConnection->setLocalIpAddr(localIp);
	pFtpUserData->m_pDataConnection->host = 
		Server::getInstance()->getVhosts()->getVHost(0, localIp,
                                                            (u_short)m_nLocalControlPort);
	pFtpUserData->m_pDataConnection->socket = new Socket(dataSocket);

	return 1;
}

int Ftp::CloseDataConnection()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	if ( pFtpUserData->m_nFtpState < FtpUserData::DATA_CONNECTION_UP )
		return 1;

	if ( pFtpUserData->m_pDataConnection != NULL 
			&& pFtpUserData->m_pDataConnection->socket != NULL )
	{
		pFtpUserData->m_pDataConnection->socket->shutdown(SD_BOTH);
		pFtpUserData->m_pDataConnection->socket->closesocket();
		delete pFtpUserData->m_pDataConnection->socket;
		pFtpUserData->m_pDataConnection->socket = NULL;
	}
	
	pFtpUserData->m_nFtpState = FtpUserData::USER_LOGGED_IN;
	return 1;
}

int Ftp::Type(const char chParam)
{
	std::string sParam;//(chParam);
	sParam += chParam;
	return Type(sParam);
}

int Ftp::Type(const std::string &sParam)
{
	const char *pParam = sParam.c_str();
	if ( pParam == 0 )
	{
		ftp_reply(501);
		return 0;
	}
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	switch ( *pParam )
	{
		case 'a':
		case 'A':
			pFtpUserData->m_nFtpRepresentation = FtpUserData::REPR_ASCII;
			break;
		case 'i':
		case 'I':
			pFtpUserData->m_nFtpRepresentation = FtpUserData::REPR_IMAGE;
			break;
		case 'e':
		case 'E':
		case 'l':
		case 'L':
		default:
			ftp_reply(504);
			return 1;
	}
	if ( pParam++ == NULL || *pParam != ' ' || pParam++ == NULL )
	{
		ftp_reply(200);
		return 1;
	}
	switch ( *pParam )
	{
		case 'n':
		case 'N':
			if ( pFtpUserData->m_nFtpRepresentation == FtpUserData::REPR_ASCII )
				pFtpUserData->m_nFtpFormatControl = FtpUserData::NON_PRINT;
			else
			{
				ftp_reply(501);
				return 0;
			}
			break;
		case 't':
		case 'T':
		case 'c':
		case 'C':
		default:
			ftp_reply(504);
			return 1;
	}
	ftp_reply(200);
	return 1;
}

void Ftp::Stru(const char s)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	switch ( s )
	{
		case 'f':
			pFtpUserData->m_nFtpFileStructure = FtpUserData::STRU_FILE;
			break;
		case 'r':
		case 'p':
		default:
			ftp_reply(504);
	}
	ftp_reply(200);
}

void Ftp::Mode(const char m)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	switch ( m )
	{
		case 's':
			pFtpUserData->m_nTransferMode = FtpUserData::MODE_STREAM;
			break;
		case 'b':
		case 'c':
		default:
			ftp_reply(504);
	}
}

void Ftp::List(const std::string &sParam/*= ""*/)
{
	std::string sLocalPath;
	if ( !UserLoggedIn() || !GetLocalPath(sParam, sLocalPath) || OpenDataConnection() == 0 )
		return;

	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);

	std::string sPath(sLocalPath);
	if ( sPath.empty() )
		sPath = pFtpUserData->m_cwd;

	SecurityToken st;
	//Map these with real username/password.
	const char *username = pFtpUserData->m_sUserName.c_str();
	const char *password = pFtpUserData->m_sPass.c_str();

	time_t now;
	time(&now);

	MemBuf &buffer2 = *td.buffer2;
	buffer2.setLength(0);
	char perm[11];
	if ( FilesUtility::isDirectory(sPath) )
	{
		FindData fd;
		//dir MUST ends with '/'
		if ( fd.findfirst(sPath) )
		{
			ftp_reply(450);
			CloseDataConnection();
			return;
		}

		do
		{
			if(fd.name[0] == '.' || !strcmpi(fd.name, "security") )
				continue;

			perm[10] = '\0';
			perm[0] = fd.attrib == FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-';

			string completeFileName(sPath);
			completeFileName.append(fd.name);

			st.directory = sPath.c_str();
			st.authType = 0;
			st.filename = fd.name;
			st.sysdirectory = pFtpUserData->m_pDataConnection->host->getSystemRoot();

			st.user = "guest";
			st.password = "";

			int guestMask = -1;
			secCacheMutex.lock();
			try
			{
				guestMask = secCache.getPermissionMask (&st);
	          		secCacheMutex.unlock();
			}
			catch ( ... )
			{
				secCacheMutex.unlock();
				throw;
			}
	
			/////////////////Put here the real user!/////////
			st.user = username;
			st.password =password;

			int pMask = -1;
			secCacheMutex.lock();
			try
			{
				pMask = secCache.getPermissionMask (&st);
	          		secCacheMutex.unlock();
			}
			catch ( ... )
			{
				secCacheMutex.unlock();
				throw;
			}

			//Owner and group permissions are the same.
			perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
			perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
			perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

			//Permission for All are the permission for Guest
			perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
			perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
			perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

			string date;
			const char* datePtr =	getRFC822LocalTime(fd.time_write, date, 32);

			char dateFtpFormat[13];

			dateFtpFormat[3] = ' ';
			dateFtpFormat[6] = ' ';
			dateFtpFormat[7] = ' ';
			dateFtpFormat[12] = '\0';

			int offset = datePtr[6] == ' ' ? 0 : 1;

    			//Day
			dateFtpFormat[4] = datePtr[5];
			dateFtpFormat[5] = datePtr[6];

			//Month
			dateFtpFormat[0] = datePtr[7 + offset];
			dateFtpFormat[1] = datePtr[8 + offset];
			dateFtpFormat[2] = datePtr[9 + offset];

			//If the file was modified in the last 6 months
			//show the hour instead of the year
			if(now - fd.time_write < 60 * 60 * 183 && 
				 now > fd.time_write)
			{
				//Hour
				dateFtpFormat[7] = datePtr[16 + offset];
				dateFtpFormat[8] = datePtr[17 + offset];
				dateFtpFormat[9] = ':';
				dateFtpFormat[10] = datePtr[19 + offset];
				dateFtpFormat[11] = datePtr[20 + offset];
			}
			else
			{
				//Year
				dateFtpFormat[8] = datePtr[11 + offset];
				dateFtpFormat[9] = datePtr[12 + offset];
				dateFtpFormat[10] = datePtr[13 + offset];
				dateFtpFormat[11] = datePtr[14 + offset];
			}

			char nlinkStr[12];
			memset(nlinkStr, 0, sizeof(char)*12);
#ifdef NOT_WIN
			nlink_t nlink = 1;
			nlink = fd.getStatStruct()->st_nlink;
			sprintf(nlinkStr, "%lu", nlink);
#endif

			char fdSizeStr[12];
			sprintf(fdSizeStr, "%li", fd.size);

			buffer2 << (const char *)perm << " " << nlinkStr << " " 
							<< username << " " << username << " " << fdSizeStr 
							<< " " << (const char *)dateFtpFormat << " " 
							<< fd.name << "\r\n";

        }
      while (!fd.findnext());
		fd.findclose();
	}
	else if ( !FilesUtility::isLink(sPath) )
	{
		// TODO: implement * selection
		std::string sDir, sFileName;
		FilesUtility::splitPath(sLocalPath, sDir, sFileName);
		FindData fd;
		if ( fd.findfirst(sDir) )
		{
			ftp_reply(450);
			CloseDataConnection();
			return;
		}
		do
		{
			if ( strcmp(fd.name, sFileName.c_str()) != 0 )
				continue;

			perm[10] = '\0';
			perm[0] = fd.attrib == FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-';

			string completeFileName(sDir);
			completeFileName.append(fd.name);

			st.directory = sDir.c_str();
			st.authType = 0;
			st.filename = fd.name;
			st.sysdirectory = pFtpUserData->m_pDataConnection->host->getSystemRoot();

			st.user = "guest";
			st.password = "";

			int guestMask = -1;
			secCacheMutex.lock();
			try
			{
				guestMask = secCache.getPermissionMask (&st);
	          		secCacheMutex.unlock();
			}
			catch ( ... )
			{
				secCacheMutex.unlock();
				throw;
			}
	
			/////////////////Put here the real user!/////////
			st.user = username;
			st.password =password;

			int pMask = -1;
			secCacheMutex.lock();
			try
			{
				pMask = secCache.getPermissionMask (&st);
	          		secCacheMutex.unlock();
			}
			catch ( ... )
			{
				secCacheMutex.unlock();
				throw;
			}

			//Owner and group permissions are the same.
			perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
			perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
			perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

			//Permission for All are the permission for Guest
			perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
			perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
			perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

			string date;
			const char* datePtr =	getRFC822LocalTime(fd.time_write, date, 32);

			char dateFtpFormat[13];

			dateFtpFormat[3] = ' ';
			dateFtpFormat[6] = ' ';
			dateFtpFormat[7] = ' ';
			dateFtpFormat[12] = '\0';

			int offset = datePtr[6] == ' ' ? 0 : 1;

    			//Day
			dateFtpFormat[4] = datePtr[5];
			dateFtpFormat[5] = datePtr[6];

			//Month
			dateFtpFormat[0] = datePtr[7 + offset];
			dateFtpFormat[1] = datePtr[8 + offset];
			dateFtpFormat[2] = datePtr[9 + offset];

			//If the file was modified in the last 6 months
			//show the hour instead of the year
			if(now - fd.time_write < 60 * 60 * 183 && 
				 now > fd.time_write)
			{
				//Hour
				dateFtpFormat[7] = datePtr[16 + offset];
				dateFtpFormat[8] = datePtr[17 + offset];
				dateFtpFormat[9] = ':';
				dateFtpFormat[10] = datePtr[19 + offset];
				dateFtpFormat[11] = datePtr[20 + offset];
			}
			else
			{
				//Year
				dateFtpFormat[8] = datePtr[11 + offset];
				dateFtpFormat[9] = datePtr[12 + offset];
				dateFtpFormat[10] = datePtr[13 + offset];
				dateFtpFormat[11] = datePtr[14 + offset];
			}

			char nlinkStr[12];
			memset(nlinkStr, 0, sizeof(char)*12);
#ifdef NOT_WIN
			nlink_t nlink = 1;
			nlink = fd.getStatStruct()->st_nlink;
			sprintf(nlinkStr, "%lu", nlink);
#endif

			char fdSizeStr[12];
			sprintf(fdSizeStr, "%li", fd.size);

			buffer2 << (const char *)perm << " " << nlinkStr << " " 
							<< username << " " << username << " " << fdSizeStr 
							<< " " << (const char *)dateFtpFormat << " " 
							<< fd.name << "\r\n";
        }
      while (!fd.findnext());
		fd.findclose();
	}
	if( pFtpUserData->m_pDataConnection->socket->send(td.buffer2->getBuffer(), 
			(u_long)td.buffer2->getLength(), 0) == SOCKET_ERROR)
	{
		ftp_reply(451);
	}

	ftp_reply(226);
	CloseDataConnection();
}

void Ftp::Nlst(const std::string &sParam/* = ""*/)
{
	std::string sLocalPath;
	if ( !UserLoggedIn() || !GetLocalPath(sParam, sLocalPath) || OpenDataConnection() == 0 )
		return;

	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);

	std::string sPath(sLocalPath);
	if ( sPath.empty() )
		sPath = pFtpUserData->m_cwd;

	if ( !FilesUtility::isDirectory(sPath) )
	{
		ftp_reply(450);
		CloseDataConnection();
		return;
	}
	FindData fd;
	if ( fd.findfirst(sPath) )
	{
		ftp_reply(450);
		CloseDataConnection();
		return;
	}

	MemBuf &buffer2 = *td.buffer2;
	buffer2.setLength(0);
	do
	{
		if(fd.name[0] == '.' || !strcmpi(fd.name, "security") )
			continue;
		if ( !sParam.empty() )
			buffer2 << sParam << "/";
		buffer2 << fd.name << "\r\n";
	} while (!fd.findnext());
	fd.findclose();

	if( pFtpUserData->m_pDataConnection->socket->send(td.buffer2->getBuffer(), 
			(u_long)td.buffer2->getLength(), 0) == SOCKET_ERROR)
	{
		ftp_reply(451);
	}
	ftp_reply(226);
	CloseDataConnection();
}

/*!
 *Clean all telnet sequences.
 *\param szIn client's requests
 *\param szOut client's requests without telnet codes
 */
void Ftp::EscapeTelnet(MemBuf &In, MemBuf &Out)
{
	Out.setLength(0);
	if ( In.getRealLength() == 0 )
		return;
	int i = 0;
	for ( char c = In[i]; c != '\0'; i++, c = In[i] )
	{
		if ( c == (char)0377 || c == (char)0364 || c == (char)0362 )
			continue;
		Out << c;
	}
	Out << '\0';
}

void Ftp::Abor()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
	{
		pFtpUserData->m_bBreakDataConnection = true;
      		Thread::join(pFtpUserData->m_dataThreadId);// wait for data connection to end
		ftp_reply(426);
	}
	ftp_reply(226);
	pFtpUserData->m_bBreakDataConnection = false;
}

void Ftp::Pwd()
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	std::string sCurrentPath = "\"";
	sCurrentPath += pFtpUserData->m_cwd + "\"";
	ftp_reply(257, sCurrentPath);
}

void Ftp::Cwd(const std::string &sPath)
{
	if ( !UserLoggedIn() )
	{
		ftp_reply(530);
		return;
	}
	std::string sLocalPath;
	if ( !GetLocalPath(sPath, sLocalPath) )
	{
		ftp_reply(501);
		return;
	}
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	pFtpUserData->m_cwd = sLocalPath;
	ftp_reply(250);
}

void Ftp::Rest(const std::string &sRestPoint)
{
	if ( !UserLoggedIn() )
	{
		ftp_reply(530);
		return;
	}
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	pFtpUserData->m_nRestartOffset = strtoul(sRestPoint.c_str(), NULL, 10);
	ftp_reply(350);
}

void Ftp::Syst()
{
	std::string sTempText;
	get_ftp_reply(215, sTempText);
	std::string::size_type n = sTempText.find("%s");
	if ( n != std::string::npos )
#ifdef WIN32
		sTempText.replace(n, 2, "WIN32");
#else
		sTempText.replace(n, 2, "UNIX Type: L8");
#endif //WIN32
	ftp_reply(215, sTempText);
}

void Ftp::Stat(const std::string &sParam/* = ""*/)
{
	FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
	assert(pFtpUserData != NULL);
	if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
	{
		std::ostringstream sStat;
		sStat << "Transfering file: " << pFtpUserData->m_sCurrentFileName;
		sStat << " " << pFtpUserData->m_nBytesSent << " bytes transferred from " << pFtpUserData->m_nFileSize;
		ftp_reply(213, sStat.str());
	}
	else
	{
		//TODO: will be implemented later
		ftp_reply(502);
	}
}
