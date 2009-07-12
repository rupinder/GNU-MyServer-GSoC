/*
MyServer
Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
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

#include <include/protocol/ftp/ftp.h>
#include <include/protocol/ftp/ftp_common.h>
#include <include/protocol/ftp/ftp_parser.h>
#include <include/base/thread/thread.h>
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/string/securestr.h>
#include <include/base/find_data/find_data.h>
#include <include/base/string/stringutils.h>
#include <include/base/mem_buff/mem_buff.h>

#include <include/conf/security/auth_domain.h>

#ifndef WIN32
#include <netinet/in.h>
#ifdef SENDFILE
#include <sys/sendfile.h>
#endif
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

static DEFINE_THREAD(SendAsciiFile,pParam);
static DEFINE_THREAD(SendImageFile,pParam);

static DEFINE_THREAD(ReceiveAsciiFile,pParam);
static DEFINE_THREAD(ReceiveImageFile,pParam);

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

void SetFtpHost(FtpHost &out, const char *szIn)
{
  std::stringstream ss;
  char *szLocalIn = strdup(szIn);
  char *tok = strtok(szLocalIn, ",.");
  while ( tok != NULL )
  {
    ss << tok << " ";
    tok = strtok(NULL, ",.");
  }
  ss >> out.h1 >> out.h2 >> out.h3 >> out.h4 >> out.p1 >> out.p2;
  free(szLocalIn);
}

void GetIpAddr(const FtpHost &host, char *pOut, const int &nBuffSize)
{
  if ( pOut == NULL )
    return;
  std::ostringstream sRet;
  sRet << host.h1 << '.' << host.h2 << '.' << host.h3 << '.' << host.h4;
  memset(pOut, 0, nBuffSize);
  strncpy(pOut, sRet.str().c_str(), nBuffSize-1);
}

int GetPortNo(const FtpHost &host)
{
  return ((host.p1 << 8) + host.p2);
}

std::string GetPortNo(unsigned int nPort)
{
  unsigned int hiByte = (nPort & 0x0000ff00) >> 8;
  unsigned int loByte = nPort & 0x000000ff;
  std::ostringstream out;
  out << hiByte << "," << loByte;
  return out.str();
}

std::string GetHost(const FtpHost &host)
{
  std::ostringstream s;
  s << host.h1 << ',' << host.h2 << ',' << host.h3 << ',' << host.h4 << ',' << host.p1 << ',' << host.p2;
  return s.str().c_str();
}

//////////////////////////////////////////////////////////////////////////////
// FtpUserData class
FtpUserData::FtpUserData()
{
  reset();
  m_DataConnBusy.init();
}

bool FtpUserData::allowDelete(bool wait)
{
  if ( wait )
  {
    //wait for data connection to finish
    m_DataConnBusy.lock();
    m_DataConnBusy.unlock();

  }
  if ( m_pDataConnection != NULL )
    return !m_pDataConnection->isScheduled();
  else
      return true;
}


FtpUserData::~FtpUserData()
{
  delete m_pDataConnection;
  m_pDataConnection = NULL;
  m_DataConnBusy.destroy();
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
  m_cdh.h1 = 0;
  m_cdh.h2 = 0;
  m_cdh.h3 = 0;
  m_cdh.h4 = 0;
  m_cdh.p1 = 0;
  m_cdh.p2 = 0;
}

int FtpUserData::CloseDataConnection()
{
  if ( m_nFtpState < DATA_CONNECTION_UP )
    return 1;

  if ( m_pDataConnection != NULL && m_pDataConnection->socket != NULL )
  {
    m_pDataConnection->socket->shutdown(SD_BOTH);
    m_pDataConnection->socket->close();
    delete m_pDataConnection->socket;
    m_pDataConnection->socket = NULL;
    m_pDataConnection->setScheduled(0);
  }
  
  //m_DataConnBusy.unlock();
  m_nFtpState = USER_LOGGED_IN;
  return 1;
}

//////////////////////////////////////////////////////////////////////////////
// FtpUserData class
DataConnectionWorkerThreadData::DataConnectionWorkerThreadData()
{
  m_pConnection = NULL;
  m_bAppend = false;
}

DataConnectionWorkerThreadData::~DataConnectionWorkerThreadData()
{
}

//////////////////////////////////////////////////////////////////////////////
// FtpThreadContext helper structure
FtpThreadContext::FtpThreadContext()
{
  pConnection = NULL;
  buffer = NULL;
  secondaryBuffer = NULL;
  buffersize = 0;
  secondaryBufferSize = 0;
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
USER, PASS, PORT, PASV, TYPE, REST, RETR, LIST, NLST, ABOR, \r\n\
CWD, CDUP, PWD, ALLO, STOR, STOU, DELE, APPE, MKD, RMD, \r\n\
RNFR, RNTO, SYST, STAT, QUIT\r\n\
214 Detailed help on commands will soon be available." },
  { 215, "%s system type." },
  { 220, "Service ready for new user." },
  { 221, "Goodbye." },
  { 226, "Closing data connection." },
  { 227, "Entering passive mode. %s" },
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
  { 500, "Syntax error, command unrecognized." },
  { 501, "Syntax error in parameters or arguments." },
  { 502, "Command not implemented." },
  { 503, "Bad sequence of commands." },
  { 504, "Command not implemented for that parameter."},
  { 530, "Not logged in." },
  { 532, "Need account for storing files." },
  { 550, "Requested action not taken. File unavailable." },
  { 0, "" }
};

int get_ftp_reply(int nReplyCode, std::string &sReply)
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

void ftp_reply(ConnectionPtr pConnection, int nReplyCode,
               const std::string &sCustomText/* = ""*/)
{
  if ( pConnection == NULL || pConnection->socket == NULL )
    return;

  std::string sLocalCustomText(sCustomText);
  Server *pInstance = Server::getInstance();
  if ( pInstance != NULL && pInstance->stopServer() == 1 )
  {
    nReplyCode = 421;
    sLocalCustomText = "";
  }

  std::ostringstream buffer;
  if ( !sLocalCustomText.empty() )
    {
        if ( nReplyCode >= 0 )
        buffer << nReplyCode << " " << sLocalCustomText << "\r\n";
        else
            buffer << sLocalCustomText << "\r\n";
        }
  else
  {
    std::string sReplyText;
    get_ftp_reply(nReplyCode, sReplyText);
    if ( sReplyText.find('\n') == std::string::npos )
      buffer << nReplyCode << " " << sReplyText << "\r\n";
    else
      buffer << nReplyCode << "-" << sReplyText << "\r\n";
  }
  pConnection->socket->send (buffer.str ().c_str (),
                             strlen (buffer.str ().c_str ()), 0);
}

//////////////////////////////////////////////////////////////////////////////
// Ftp class

bool Ftp::m_bAllowAnonymous = false;
bool Ftp::m_bAnonymousNeedPass = true;
bool Ftp::m_bAllowAsynchronousCmds = true;
bool Ftp::m_bEnablePipelining = true;
bool Ftp::m_bEnableStoreCmds = true;

int Ftp::FIRST_PASV_PORT = 60000;
int Ftp::LAST_PASV_PORT = 65000;

Ftp::Ftp()
{
  m_nPassivePort = Ftp::FIRST_PASV_PORT;
  protocolOptions = PROTOCOL_FAST_CHECK | PROTOCOL_DENY_DELETE;
  protocolPrefix.assign("ftp://");
}

Ftp::~Ftp()
{
}

int Ftp::controlConnection(ConnectionPtr pConnection, char *b1, char *b2,
                           int bs1, int bs2, u_long nbtr, u_long id)
{
  if ( pConnection == NULL )
    return ClientsThread::DELETE_CONNECTION;
  Server* server = Server::getInstance();
  if ( server == NULL )
    return ClientsThread::DELETE_CONNECTION;

  FtpUserData *pFtpUserData = NULL;
  if ( pConnection->protocolBuffer == NULL )
    pConnection->protocolBuffer = new FtpUserData();
  pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
  if ( pFtpUserData == NULL )
    return ClientsThread::DELETE_CONNECTION;

  // check if ftp is busy(return 120) or unavailable(return 421)
  if ( pConnection->getToRemove() == CONNECTION_REMOVE_OVERLOAD )
  {
    pFtpUserData->m_nFtpState = FtpUserData::BUISY;
    // TODO: really compute busy time interval
    std::string sTempText;
    get_ftp_reply(120, sTempText);
    std::string::size_type n = sTempText.find("%s");
    if ( n != std::string::npos )
      sTempText.replace(n, 2, "10");
    ftp_reply(120, sTempText);
  }

  if ( server->isRebooting() != 0 )
  {
    pFtpUserData->m_nFtpState = FtpUserData::UNAVAILABLE;
    ftp_reply(421);
    return 0;
  }

  // init default local ports
  m_nLocalControlPort = pConnection->getLocalPort();
  pFtpUserData->m_nLocalDataPort = m_nLocalControlPort - 1;

  if ( pFtpUserData->m_cwd.empty() && pConnection->host != NULL )//current dir not initialized
  {
    pFtpUserData->m_cwd = "";
  }

  //switch context
  td.pConnection = pConnection;
  td.buffer = pConnection->getActiveThread()->getBuffer();
  td.secondaryBuffer = pConnection->getActiveThread()->getSecondaryBuffer();
  td.buffersize = bs1;
  td.secondaryBufferSize = bs2;
  td.nBytesToRead = nbtr;
  td.pProtocolInterpreter = this;
  td.m_nParseLength = 0;

  return ParseControlConnection();
}


char* Ftp::registerName(char* out, int len)
{
  return registerNameImpl(out, len);
}

char* Ftp::registerNameImpl(char* out, int len)
{
  if(out)
  {
    myserver_strlcpy(out, "FTP", len);
  }
  return (char*) "FTP";
}


int Ftp::loadProtocolStatic(XmlParser*)
{
  // load custom messages from cfg here
  Server *server = Server::getInstance ();

  // allow anonymous access
  const char *pData = server->getHashedData ("ftp.allow_anonymous");
  if ( pData != NULL )
    m_bAllowAnonymous = strcmpi("Yes", pData) == 0 ? true : false;

  // request password for anonymous
  pData = server->getHashedData ("ftp.anonymous_need_pass");
  if ( pData != NULL )
    m_bAnonymousNeedPass = strcmpi("Yes", pData) == 0 ? true : false;

  // enable asyncronous cmds
  pData = server->getHashedData ("ftp.allow_asynchronous_cmds");
  if ( pData != NULL )
    m_bAllowAsynchronousCmds = strcmpi("Yes", pData) == 0 ? true : false;

  // enable pipelining
  pData = server->getHashedData ("ftp.allow_pipelining");
  if ( pData != NULL )
    m_bEnablePipelining = strcmpi("Yes", pData) == 0 ? true : false;

  // enable write commands
  pData = server->getHashedData ("ftp.allow_store");
  if ( pData != NULL )
    m_bEnableStoreCmds = strcmpi("Yes", pData) == 0 ? true : false;

  return 1;
}

int Ftp::unLoadProtocolStatic(XmlParser*)
{
  if ( true/*everything is ok*/ )
    return 1;
  else
    return 0;
}

void Ftp::ftp_reply(int nReplyCode, const std::string &sCustomText /*= ""*/)
{
  if ( td.pConnection == NULL || td.pConnection->socket == NULL )
    return;

  ::ftp_reply(td.pConnection, nReplyCode, sCustomText);

  logAccess (nReplyCode, sCustomText);
}

void Ftp::logAccess (int nReplyCode, const std::string &sCustomText)
{
  /* Log the reply.  */
  string time;
  char msgCode[12];
  sprintf (msgCode, "%i", nReplyCode);
  getLocalLogFormatDate (time, 32);

  td.secondaryBuffer->setLength (0);
  *td.secondaryBuffer << time
                      << " " << td.pConnection->getIpAddr ()
                      << " " << msgCode
                      << " " << sCustomText;

#ifdef WIN32
  *td.secondaryBuffer  << "\r\n" << end_str;
#else
  *td.secondaryBuffer  << "\n" << end_str;
#endif

  if (td.pConnection->host)
  td.pConnection->host->accessesLogWrite (td.secondaryBuffer->getBuffer ());
}

int Ftp::CloseDataConnection()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  int nRet = pFtpUserData->CloseDataConnection();
  return nRet;
}

int Ftp::PrintError(const char *msg)
{
  /* Log the reply.  */
  string time;
  getLocalLogFormatDate (time, 32);

  td.secondaryBuffer->setLength (0);
  *td.secondaryBuffer << td.pConnection->getIpAddr ();
  *td.secondaryBuffer << " " << time
                      << " " << msg;

  if (td.pConnection->host)
    td.pConnection->host->warningsLogWrite (td.secondaryBuffer->getBuffer ());

  return 1;
}

void Ftp::User(const std::string &sParam)
{
  WaitDataConnection();

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  switch ( pFtpUserData->m_nFtpState )
  {
    case FtpUserData::CONTROL_CONNECTION_UP:
    case FtpUserData::USER_LOGGED_IN:
      pFtpUserData->m_sUserName = sParam;
      if ( strcmpi(sParam.c_str(), "anonymous") == 0 && !m_bAnonymousNeedPass )
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
        ftp_reply(530);
      }
  }
}

void Ftp::Password(const std::string &sParam)
{
  WaitDataConnection();

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( !m_bAllowAnonymous && strcmpi(pFtpUserData->m_sUserName.c_str(), "anonymous") == 0 )
  {
    ftp_reply(530);
    return;
  }

  switch ( pFtpUserData->m_nFtpState )
  {
    case FtpUserData::CONTROL_CONNECTION_UP:
      if ( CheckRights( pFtpUserData->m_sUserName, sParam, pFtpUserData->m_cwd, 
        MYSERVER_PERMISSION_BROWSE) != 0 )
      {
        pFtpUserData->m_sPass = sParam;
        pFtpUserData->m_nFtpState = FtpUserData::USER_LOGGED_IN;
        ftp_reply(230);
      }
      else
        ftp_reply(530);
      break;
    case FtpUserData::USER_LOGGED_IN:
      //if ( m_bAnonymousNeedPass )
        ftp_reply(503);
      //else
      //  ftp_reply(230);
      break;
  }
}

void Ftp::Port(const FtpHost &host)
{
  WaitDataConnection();

  if ( !UserLoggedIn() )
    return;
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  SetFtpHost(pFtpUserData->m_cdh, host);
  ftp_reply(200);
}

void Ftp::Pasv()
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  std::string sHost = td.pConnection->getLocalIpAddr();
  if ( m_nPassivePort < Ftp::LAST_PASV_PORT )
    sHost += "," + GetPortNo(m_nPassivePort++);
  else
  {
    m_nPassivePort = Ftp::FIRST_PASV_PORT;
    sHost += "," + GetPortNo(m_nPassivePort++);
  }
  SetFtpHost(pFtpUserData->m_cdh, sHost.c_str());

  pFtpUserData->m_bPassiveSrv = true;
    if ( OpenDataConnection() == 0 )
      {
	ftp_reply(425);//RFC959 command replay exception
	return;
      }

  std::string sTempText;
  get_ftp_reply(227, sTempText);
  std::string::size_type n = sTempText.find("%s");
  if ( n != std::string::npos )
#ifdef WIN32
    sTempText.replace(n, 2, GetHost(pFtpUserData->m_cdh));
#else
    sTempText.replace(n, 2, GetHost(pFtpUserData->m_cdh));
#endif //WIN32
  ftp_reply(227, sTempText);

  //wait for incoming connection
  int timeoutvalue = 3;
#ifdef __linux__
  timeoutvalue = 1;
#endif
#ifdef __HURD__
  timeoutvalue = 5;
#endif
  MYSERVER_SOCKADDRIN asockIn;
  int asockInLen = 0;
  Socket asock;
  if ( pFtpUserData->m_pDataConnection->socket->dataOnRead(timeoutvalue, 0) == 1 )
    {
    asockInLen = sizeof(sockaddr_in);
    asock = pFtpUserData->m_pDataConnection->socket->accept(&asockIn, &asockInLen);
    if ( asock.getHandle() == (Handle)INVALID_SOCKET )
      return;

    pFtpUserData->m_pDataConnection->socket->shutdown(SD_BOTH);
    pFtpUserData->m_pDataConnection->socket->close();
    delete pFtpUserData->m_pDataConnection->socket;
    pFtpUserData->m_pDataConnection->socket = new Socket(asock);
  }
  pFtpUserData->m_bPassiveSrv = false;
}

void Ftp::RetrStor(bool bRetr, bool bAppend, const std::string &sPath)
{    
  std::string sLocalPath;
  if ( !UserLoggedIn() )
  {
    //CloseDataConnection();
    return;
  }
  if ( (bRetr && !GetLocalPath(sPath, sLocalPath)) || 
    (!bRetr && !BuildLocalPath(sPath, sLocalPath)) )
  {
    //CloseDataConnection();
    return;
  }

  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");
  if( !strcmpi(sLocalFileName.c_str(), secName))
  {
    ftp_reply(550);
    //CloseDataConnection();
    return;
  }

  int nMask = 0;
  if ( bRetr )
    nMask = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE;
  else
    nMask = MYSERVER_PERMISSION_WRITE;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( CheckRights(pFtpUserData->m_sUserName, pFtpUserData->m_sPass, sLocalPath, nMask) == 0 )
  {
    ftp_reply(550);
    //CloseDataConnection();
    return;
  }

  if (FilesUtility::isDirectory(sLocalPath.c_str ()))
    {
      ftp_reply(550);
      return;
    }

  /* FIXME: Log after the file is sent, not before.  */
  logAccess (0, sLocalPath);

  DataConnectionWorkerThreadData *pData = new DataConnectionWorkerThreadData();
  pData->m_pConnection = td.pConnection;
  pData->m_bAppend = bAppend || pFtpUserData->m_nRestartOffset > 0;
  pData->m_sFilePath = sLocalPath;
  pData->m_pFtp = this;

  pFtpUserData->m_sCurrentFileName = "";
  pFtpUserData->m_nFileSize = 0;
  pFtpUserData->m_nBytesSent = 0;
  
  switch ( pFtpUserData->m_nFtpRepresentation )
  {
    case FtpUserData::REPR_ASCII:
            Thread::create(&pFtpUserData->m_dataThreadId, bRetr?SendAsciiFile:ReceiveAsciiFile, pData);
      break;
    case FtpUserData::REPR_IMAGE:
            Thread::create(&pFtpUserData->m_dataThreadId, bRetr?SendImageFile:ReceiveImageFile, pData);
      break;
  }
}

static DEFINE_THREAD(SendAsciiFile, pParam)
{
  DataConnectionWorkerThreadData *pWt = reinterpret_cast<DataConnectionWorkerThreadData *>(pParam);
  if ( pWt == NULL )
  {
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if ( pConnection == NULL )
  {
    ftp_reply(pConnection, 451);
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
  if ( pFtpUserData == NULL )
  {
    ftp_reply(pConnection, 451);
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  pFtpUserData->m_DataConnBusy.lock();
  
  if ( pWt->m_pFtp == NULL )
  {
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(pConnection, 125);
    else 
    {
      ftp_reply(pConnection, 150);
        if ( pWt->m_pFtp->OpenDataConnection() == 0 )
      {
        ftp_reply(pConnection, 425);
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
      }
    }

     if( pFtpUserData->m_pDataConnection == NULL || 
    pFtpUserData->m_pDataConnection->socket == NULL)
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  File *file = NULL;//new File();
  try
  {
    file = Server::getInstance()->getCachedFiles()->open(pWt->m_sFilePath.c_str());
    if ( file == NULL )
    {
      ftp_reply(pConnection, 451);
      pFtpUserData->CloseDataConnection();
      pFtpUserData->m_DataConnBusy.unlock();
      delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
    }
        u_long filesize = file->getFileSize();
    if ( pFtpUserData->m_nRestartOffset > 0 )
      pFtpUserData->m_nRestartOffset = 0;// don't implement restart for ASCII

    pFtpUserData->m_sCurrentFileName = pWt->m_sFilePath;
    pFtpUserData->m_nFileSize = filesize;

    u_long nbr, nBufferSize = 0;
    char *pLine = NULL;
    int nLineLength = 0;
    std::string sLine;
    MemBuf buffer, secondaryBuffer;
    buffer.setLength(1024);
    while ( filesize != 0 )
    {
      memset(buffer.getBuffer(), 0, buffer.getRealLength());
      nBufferSize = std::min(static_cast<u_long>(filesize), static_cast<u_long>(buffer.getRealLength()/2));
      if ( file->read(buffer.getBuffer(), nBufferSize, &nbr) )
      {
        ftp_reply(pConnection, 451);
        file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
      }
      filesize -= nbr;
      pFtpUserData->m_nBytesSent += nbr;

      secondaryBuffer.setLength(0);
      pLine = buffer.getBuffer();
      if ( pLine == NULL )
      {
        ftp_reply(pConnection, 451);
        file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
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
        {
          sLine.assign(pLine, strlen(pLine));
          if ( !sLine.empty() )
            secondaryBuffer << sLine;
          pLine += strlen(pLine);
        }
        else
        {
          sLine.assign(pLine, nLineLength);
          secondaryBuffer << sLine << "\r\n";
          if ( *(pLine + nLineLength) == '\r' )
            nLineLength++;
          if ( *(pLine + nLineLength) == '\n' )
            nLineLength++;
          pLine += nLineLength;
        }
      }
            if ( pFtpUserData->m_pDataConnection->socket->send(secondaryBuffer.getBuffer(), 
          (u_long)secondaryBuffer.getLength(), 0) == SOCKET_ERROR )
            {
        ftp_reply(pConnection, 451);
               file->close();
        file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
            }
      if ( pFtpUserData->m_bBreakDataConnection )
      {
        pFtpUserData->m_bBreakDataConnection = false;
               file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
      }
    }
    file->close();
    delete file;
  }
  catch (bad_alloc &ba)
  {
    if ( file != NULL )
      file->close();
    delete file;
    //report error
  }

  pFtpUserData->m_sCurrentFileName = "";
  pFtpUserData->m_nFileSize = 0;
  pFtpUserData->m_nBytesSent = 0;
  pFtpUserData->m_nRestartOffset = 0;
  ftp_reply(pConnection, 226);
  pFtpUserData->CloseDataConnection();
  pFtpUserData->m_DataConnBusy.unlock();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif

}

static DEFINE_THREAD(SendImageFile, pParam)
{
  DataConnectionWorkerThreadData *pWt = reinterpret_cast<DataConnectionWorkerThreadData *>(pParam);
  if ( pWt == NULL )
  {
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if ( pConnection == NULL )
  {
    ftp_reply(pConnection, 451);
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
  if ( pFtpUserData == NULL )
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  pFtpUserData->m_DataConnBusy.lock();
  
  if ( pWt->m_pFtp == NULL )
  {
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }
  
  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(pConnection, 125);
    else 
    {
      ftp_reply(pConnection, 150);
        if ( pWt->m_pFtp->OpenDataConnection() == 0 )
      {
        ftp_reply(pConnection, 425);
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
        }
  }

        if( pFtpUserData->m_pDataConnection == NULL || 
      pFtpUserData->m_pDataConnection->socket == NULL)
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  File *file = NULL;
  try
  {
    file = Server::getInstance()->getCachedFiles()->open(pWt->m_sFilePath.c_str());
    if( file == NULL )
    {
      ftp_reply(pConnection, 451);
      pFtpUserData->CloseDataConnection();
      pFtpUserData->m_DataConnBusy.unlock();
      delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
    }
    u_long filesize = file->getFileSize();
    u_long nbr, nBufferSize = 0;
    if ( pWt->m_bAppend && pFtpUserData->m_nRestartOffset < filesize )
    {
      file->seek (pFtpUserData->m_nRestartOffset);
      filesize -= pFtpUserData->m_nRestartOffset;
    }

    pFtpUserData->m_sCurrentFileName = pWt->m_sFilePath;
    pFtpUserData->m_nFileSize = filesize;

    MemBuf secondaryBuffer;
    secondaryBuffer.setLength(1024);
    while ( filesize != 0 )
    {
      nBufferSize = std::min(static_cast<u_long>(filesize), static_cast<u_long>(secondaryBuffer.getRealLength()/2));
      if ( file->read(secondaryBuffer.getBuffer(), nBufferSize, &nbr) ||
        pFtpUserData->m_pDataConnection->socket->send(secondaryBuffer.getBuffer(), 
          (u_long)nBufferSize, 0) == SOCKET_ERROR )
      {
        ftp_reply(pConnection, 451);
        file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
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
               file->close();
        delete file;
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
      }
    }
    file->close();
    delete file;
  }
  catch (bad_alloc &ba)
  {
    if ( file != NULL )
      file->close();
    delete file;
    //report error
  }

  pFtpUserData->m_sCurrentFileName = "";
  pFtpUserData->m_nFileSize = 0;
  pFtpUserData->m_nBytesSent = 0;
  pFtpUserData->m_nRestartOffset = 0;
  ftp_reply(pConnection, 226);
  pFtpUserData->CloseDataConnection();
  pFtpUserData->m_DataConnBusy.unlock();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
}

static DEFINE_THREAD(ReceiveAsciiFile, pParam)
{
  DataConnectionWorkerThreadData *pWt =
    reinterpret_cast<DataConnectionWorkerThreadData *>(pParam);

  if ( pWt == NULL )
  {
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }
  ConnectionPtr pConnection = pWt->m_pConnection;
  if ( pConnection == NULL )
  {
    ftp_reply(pConnection, 451);
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
  if ( pFtpUserData == NULL )
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  pFtpUserData->m_DataConnBusy.lock();
  
  if ( pWt->m_pFtp == NULL )
  {
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(pConnection, 125);
    else 
    {
      ftp_reply(pConnection, 150);
        if ( pWt->m_pFtp->OpenDataConnection() == 0 )
      {
        ftp_reply(pConnection, 425);
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
        }
  }
  
        if( pFtpUserData->m_pDataConnection == NULL || 
      pFtpUserData->m_pDataConnection->socket == NULL)
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  File file;
  try
  {
    u_long flags = 0;
    if ( pWt->m_bAppend )
      flags = File::APPEND | File::WRITE;
    else
      flags = File::FILE_CREATE_ALWAYS | File::WRITE;
    if ( file.openFile(pWt->m_sFilePath.c_str(), flags) )
    {
      ftp_reply(pConnection, 451);
      pFtpUserData->CloseDataConnection();
      pFtpUserData->m_DataConnBusy.unlock();
      delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
    }

    MemBuf buffer, secondaryBuffer;
    buffer.setLength(1024);
    memset(buffer.getBuffer(), 0, buffer.getRealLength());
    char *pLine = NULL;
    int nLineLength = 0;
    std::string sLine;
    u_long nbr;
    while ( pFtpUserData->m_pDataConnection->socket->read(buffer.getBuffer(), 
          (u_long)buffer.getRealLength()-1, &nbr) != SOCKET_ERROR && nbr != 0 )
    {
      memset(secondaryBuffer.getBuffer(), 0, secondaryBuffer.getRealLength());
      secondaryBuffer.setLength(0);
      pLine = buffer.getBuffer();
      if ( pLine == NULL )
      {
        ftp_reply(pConnection, 451);
        file.close();
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
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
        {
          sLine.assign(pLine, strlen(pLine));
          if ( !sLine.empty() )
            secondaryBuffer << sLine;
          pLine += strlen(pLine);
        }
        else
        {
          sLine.assign(pLine, nLineLength);
#ifdef WIN32
          secondaryBuffer << sLine << "\r\n";
#else
          secondaryBuffer << sLine << "\n";
#endif
          if ( *(pLine + nLineLength) == '\r' )
            nLineLength++;
          if ( *(pLine + nLineLength) == '\n' )
            nLineLength++;
          pLine += nLineLength;
        }
      }
      file.write(secondaryBuffer.getBuffer(), (u_long)secondaryBuffer.getLength(), &nbr);

      if ( pFtpUserData->m_bBreakDataConnection )
      {
        pFtpUserData->m_bBreakDataConnection = false;
               file.close();
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
      }
      memset(buffer.getBuffer(), 0, buffer.getRealLength());
    }
    file.close();
  }
  catch (bad_alloc &ba)
  {
    file.close();
    //report error
  }

  ftp_reply(pConnection, 226);
  pFtpUserData->CloseDataConnection();
  pFtpUserData->m_DataConnBusy.unlock();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
}

static DEFINE_THREAD(ReceiveImageFile, pParam)
{
  DataConnectionWorkerThreadData *pWt = reinterpret_cast<DataConnectionWorkerThreadData *>(pParam);
  if ( pWt == NULL )
  {
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)-1;
#endif
  }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if ( pConnection == NULL )
  {
    ftp_reply(pConnection, 451);
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(pConnection->protocolBuffer);
  if ( pFtpUserData == NULL )
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  pFtpUserData->m_DataConnBusy.lock();
  
  if ( pWt->m_pFtp == NULL )
  {
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(pConnection, 125);
    else 
    {
      ftp_reply(pConnection, 150);
        if ( pWt->m_pFtp->OpenDataConnection() == 0 )
      {
        ftp_reply(pConnection, 425);
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
        }
  }
  
        if( pFtpUserData->m_pDataConnection == NULL || 
      pFtpUserData->m_pDataConnection->socket == NULL)
  {
    ftp_reply(pConnection, 451);
    pFtpUserData->CloseDataConnection();
    pFtpUserData->m_DataConnBusy.unlock();
    delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
  }

  File file;
  try
  {
    u_long flags = 0;
    if ( pWt->m_bAppend )
      flags = File::APPEND | File::WRITE;
    else
      flags = File::FILE_CREATE_ALWAYS | File::WRITE;
    if ( file.openFile(pWt->m_sFilePath.c_str(), flags) )
    {
      ftp_reply(pConnection, 451);
      pFtpUserData->CloseDataConnection();
      pFtpUserData->m_DataConnBusy.unlock();
      delete pWt;
#ifdef WIN32
  return 0;
#elif HAVE_PTHREAD
  return (void*)0;
#endif
    }
    u_long nbr;
    MemBuf buffer;
    buffer.setLength(1024);
    memset(buffer.getBuffer(), 0, buffer.getRealLength());
    while ( pFtpUserData->m_pDataConnection->socket->read(buffer.getBuffer(), 
          (u_long)buffer.getRealLength()-1, &nbr) != SOCKET_ERROR && nbr != 0 )
    {
      file.write(buffer.getBuffer(), nbr, &nbr);
      if ( pFtpUserData->m_bBreakDataConnection )
      {
        pFtpUserData->m_bBreakDataConnection = false;
               file.close();
        pFtpUserData->CloseDataConnection();
        pFtpUserData->m_DataConnBusy.unlock();
        delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void*)1;
#endif
      }
    }
    file.close();
  }
  catch (bad_alloc &ba)
  {
    //report error
    file.close();
  }

  ftp_reply(pConnection, 226);
  pFtpUserData->CloseDataConnection();
  pFtpUserData->m_DataConnBusy.unlock();
  delete pWt;
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
    return false;
  }
  if ( pFtpUserData->m_nFtpState < FtpUserData::USER_LOGGED_IN )
  {
    ftp_reply(530);
    return false;
  }
  return true;
}

/*!
 *Converts from relative client's path to local path(out path may not exist).
 *\param sPathIn client's relative path
 *\param sOutPath local path
 *\return Return true if path exist, file is a normal one and is into the ftp's root folder
 */
bool Ftp::BuildLocalPath(const std::string &sPathIn, std::string &sOutPath)
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  sOutPath = td.pConnection->host->getDocumentRoot ();

  if (sOutPath.length ())
    sOutPath.append ("/");

  if (sPathIn[0] != '/')
    sOutPath.append (pFtpUserData->m_cwd);

  if ( sPathIn[0] != '-' ) // ls params not handled
    sOutPath.append (sPathIn);

  FilesUtility::completePath (sOutPath);

  std::string sDocRoot(td.pConnection->host->getDocumentRoot());
  FilesUtility::completePath(sDocRoot);
  if ( FilesUtility::getPathRecursionLevel(sDocRoot) > FilesUtility::getPathRecursionLevel(sOutPath) )
  {
    ftp_reply(550);
    return false;
  }
  ///////////////////////////////////////
  if ( FilesUtility::isDirectory(sOutPath) &&
       (sOutPath[sOutPath.length() - 1] != '/' && sOutPath[sOutPath.length() - 1] != '\\') )
    sOutPath.append("/");
  return true;
}

/*!
 *Converts from relative client's path to local path and checks if the path is available.
 *\param sPath client's relative path
 *\param sOutPath local path
 *\return Return true if path exist, file is a normal one and is into the ftp's root folder
 */
bool Ftp::GetLocalPath(const std::string &sPath, std::string &sOutPath)
{
  if ( !BuildLocalPath(sPath, sOutPath) )
    return false;

  if ( sOutPath.empty() || 
     !FilesUtility::fileExists(sOutPath) || 
     FilesUtility::isLink(sOutPath.c_str()) )
  {
    ftp_reply(550);
    return false;
  }

  return true;
}

void Ftp::Quit()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  //wait to finish data transfer
  if ( !m_bAllowAsynchronousCmds )
    WaitDataConnection();

  pFtpUserData->m_nFtpState = FtpUserData::NO_CONTROL_CONNECTION;
  ftp_reply(221);
}

void Ftp::Help(const std::string &sCmd/* = ""*/)
{
  WaitDataConnection();
  // treat SITE the same as HELP
  if ( sCmd.empty() || stringcmpi(sCmd, "SITE") == 0 )
    ftp_reply(214);
  else
    ftp_reply(502);
}

void Ftp::Noop()
{
  WaitDataConnection();
  ftp_reply(200);
}

int Ftp::OpenDataConnection()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    return 1;

  //pFtpUserData->m_DataConnBusy.lock();
  int nRet = pFtpUserData->m_bPassiveSrv ? OpenDataPassive() : OpenDataActive();
  if ( nRet != 0 )
    pFtpUserData->m_nFtpState = FtpUserData::DATA_CONNECTION_UP;
  return nRet;
}

int Ftp::OpenDataPassive()
{
  Server* server = Server::getInstance();
  if ( server == NULL )
    return 0;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  Socket *pSocket = new Socket();
  pSocket->socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if ( pSocket->getHandle() == (Handle)INVALID_SOCKET )
    return 0;
  int nReuseAddr = 1;
  MYSERVER_SOCKADDR_STORAGE storage = { 0 };
  ((sockaddr_in*)(&storage))->sin_family = AF_INET;
  char szIpAddr[16];
  memset(szIpAddr, 0, 16);
  GetIpAddr(pFtpUserData->m_cdh, szIpAddr, 16);
#ifdef WIN32
  ((sockaddr_in*)(&storage))->sin_addr.s_addr = inet_addr(szIpAddr);
#else
  inet_aton(szIpAddr, &((sockaddr_in*)(&storage))->sin_addr);
#endif // WIN32
  ((sockaddr_in*)(&storage))->sin_port = htons(GetPortNo(pFtpUserData->m_cdh));
  if ( pSocket->setsockopt(SOL_SOCKET, SO_REUSEADDR, (const char*)&nReuseAddr, sizeof(nReuseAddr)) < 0 )
    return 0;
  if ( pSocket->bind(&storage, sizeof(sockaddr_in)) != 0 || pSocket->listen(SOMAXCONN) != 0 )
    return 0;

  pFtpUserData->m_pDataConnection->setPort(GetPortNo(pFtpUserData->m_cdh));
  pFtpUserData->m_pDataConnection->setLocalPort(pFtpUserData->m_nLocalDataPort);
  pFtpUserData->m_pDataConnection->setIpAddr(td.pConnection->getIpAddr());
  pFtpUserData->m_pDataConnection->setLocalIpAddr(td.pConnection->getLocalIpAddr());
  pFtpUserData->m_pDataConnection->host = td.pConnection->host;
  pFtpUserData->m_pDataConnection->socket = pSocket;
  pFtpUserData->m_pDataConnection->setScheduled(1);
  return 1;
}

int Ftp::OpenDataActive()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  Socket dataSocket;
  dataSocket.socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char szIpAddr[16];
  memset(szIpAddr, 0, 16);
  GetIpAddr(pFtpUserData->m_cdh, szIpAddr, 16);
  if ( dataSocket.connect(szIpAddr, GetPortNo(pFtpUserData->m_cdh)) < 0 )
    return 0;

  pFtpUserData->m_pDataConnection->setPort(GetPortNo(pFtpUserData->m_cdh));
  pFtpUserData->m_pDataConnection->setLocalPort(pFtpUserData->m_nLocalDataPort);
  pFtpUserData->m_pDataConnection->setIpAddr(td.pConnection->getIpAddr());
  pFtpUserData->m_pDataConnection->setLocalIpAddr(td.pConnection->getLocalIpAddr());
  pFtpUserData->m_pDataConnection->host = td.pConnection->host;
  pFtpUserData->m_pDataConnection->socket = new Socket(dataSocket);
  pFtpUserData->m_pDataConnection->setScheduled(1);

  return 1;
}

int Ftp::Type(int nTypeCode, int nFormatControlCode/* = -1*/)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return 0;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  pFtpUserData->m_nFtpRepresentation = (FtpUserData::FtpRepresentation)nTypeCode;
  switch ( nFormatControlCode )
  {
    case FtpUserData::NON_PRINT:
      if ( nTypeCode == FtpUserData::REPR_ASCII )
        pFtpUserData->m_nFtpFormatControl = (FtpUserData::FtpFormatControl)nFormatControlCode;
      else
      {
        ftp_reply(501);
        return 0;
      }
      break;
    case FtpUserData::REPR_IMAGE:
      pFtpUserData->m_nFtpFormatControl = (FtpUserData::FtpFormatControl)nFormatControlCode;
      break;
  }

  ftp_reply(200);
  return 1;
}

void Ftp::Stru(int nStructure)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  if ( nStructure < 0 )
    ftp_reply(504);

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  pFtpUserData->m_nFtpFileStructure = (FtpUserData::FtpFileStructure)nStructure;
  ftp_reply(200);
}

void Ftp::Mode(int nMode)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  if ( nMode < 0 )
    ftp_reply(504);

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  pFtpUserData->m_nTransferMode = (FtpUserData::FtpTransferMode)nMode;
}

void Ftp::List(const std::string &sParam/*= ""*/)
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  WaitDataConnection();

  std::string sLocalPath;
  if ( !UserLoggedIn() || !GetLocalPath(sParam, sLocalPath) /*|| */ )
    return;

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(125);
    else
    {
      ftp_reply(150);
        if ( OpenDataConnection() == 0 )
	  {
	    ftp_reply(425);
	    return;
	  }
    }

  std::string sPath(sLocalPath);
  if ( sPath.empty() )
    sPath = pFtpUserData->m_cwd;

  const char *username = pFtpUserData->m_sUserName.c_str();
  const char *password = pFtpUserData->m_sPass.c_str();

  time_t now;
  time(&now);

  MemBuf &secondaryBuffer = *td.secondaryBuffer;
  secondaryBuffer.setLength(0);
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

    const char *secName = td.st.getHashedData ("security.filename",
                                               MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                               ".security.xml");
    do
    {
      if(fd.name[0] == '.' || !strcmpi(fd.name, secName) )
        continue;

      perm[10] = '\0';
      perm[0] = fd.attrib == FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-';
      string completeFileName(sPath);
      completeFileName.append(fd.name);
      int guestMask = CheckRights("guest", "", completeFileName, -1);
      int pMask = CheckRights(username, password, completeFileName, -1);

      //Owner and group permissions are the same.
      perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
      perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
      perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

      //Permission for All are the permission for Guest
      perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
      perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
      perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

      string date;
      const char* datePtr =  getRFC822LocalTime(fd.time_write, date, 32);

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
#ifndef WIN32
      nlink_t nlink = 1;
      nlink = fd.getStatStruct()->st_nlink;
      sprintf(nlinkStr, "%lu", (u_long) nlink);
#endif

      char fdSizeStr[12];
      sprintf(fdSizeStr, "%li", fd.size);

      secondaryBuffer << (const char *)perm << " " << nlinkStr << " " 
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

      int guestMask = CheckRights("guest", "", completeFileName, -1);
      int pMask = CheckRights(username, password, completeFileName, -1);
      //Owner and group permissions are the same.
      perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
      perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
      perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

      //Permission for All are the permission for Guest
      perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
      perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
      perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

      string date;
      const char* datePtr =  getRFC822LocalTime(fd.time_write, date, 32);

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
#ifndef WIN32
      nlink_t nlink = 1;
      nlink = fd.getStatStruct()->st_nlink;
      sprintf(nlinkStr, "%lu", (u_long) nlink);
#endif

      char fdSizeStr[12];
      sprintf(fdSizeStr, "%li", fd.size);

      secondaryBuffer << (const char *)perm << " " << nlinkStr << " " 
              << username << " " << username << " " << fdSizeStr 
              << " " << (const char *)dateFtpFormat << " " 
              << fd.name << "\r\n";
        }
      while (!fd.findnext());
    fd.findclose();
  }
  if( pFtpUserData->m_pDataConnection->socket->send(td.secondaryBuffer->getBuffer(), 
      (u_long)td.secondaryBuffer->getLength(), 0) == SOCKET_ERROR)
  {
    ftp_reply(451);
  }

  ftp_reply(226);
  CloseDataConnection();
}

void Ftp::Nlst(const std::string &sParam/* = ""*/)
{
  WaitDataConnection();
  std::string sLocalPath;
  if ( !UserLoggedIn() || !GetLocalPath(sParam, sLocalPath) )
    return;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
    ftp_reply(125);
    else
    {
      ftp_reply(150);
        if ( OpenDataConnection() == 0 )
	  {
	    ftp_reply(425);
	    return;
	  }
    }

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

  MemBuf &secondaryBuffer = *td.secondaryBuffer;
  secondaryBuffer.setLength(0);

  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");
  do
    {
      if(fd.name[0] == '.' || !strcmpi(fd.name, secName) )
        continue;

      if ( !sParam.empty() )
        secondaryBuffer << sParam << "/";
      secondaryBuffer << fd.name << "\r\n";
    }
  while (!fd.findnext());

  fd.findclose();

  if( pFtpUserData->m_pDataConnection->socket->send(td.secondaryBuffer->getBuffer(), 
      (u_long)td.secondaryBuffer->getLength(), 0) == SOCKET_ERROR)
  {
    ftp_reply(451);
  }
  ftp_reply(226);
  CloseDataConnection();
}

/*!
 *Handle telnet commands:
 * DOx      -> WONT
 * DONTx    -> ignore
 * WILLx    -> DONT
 * WONTx    -> ignore
 * IACx     -> ignore
 * IACIAC   -> IAC
 *\param In client's requests
 *\param Out client's requests without telnet codes
 */
void Ftp::EscapeTelnet(MemBuf &In, MemBuf &Out)
{
  Out.setLength(0);

  if ( In.getRealLength() == 0 )
    return;

  char *pIn = In.getBuffer();
  char szReply[3];

  while ( (u_int) (pIn - In.getBuffer()) < In.getLength() )
    {
        if ( *pIn == '\377' )
        {
            szReply[0] = *pIn++;
            if ( *pIn == '\0' )
                break;
            switch ( *pIn )
            {
                case '\375'://DO
                case '\376'://DONT
                    szReply[1] = '\374';
                    pIn++;
                    break;
                case '\373'://WILL
                case '\374'://WONT
                    szReply[1] = '\376';
                    pIn++;
                    break;
                case '\377':
                    szReply[1] = '\0';
                Out << *pIn;
                    break;

                default:
                    pIn++;
                    continue;
            }
            szReply[2] = *pIn++;
            ftp_reply(-1, szReply);
            continue;
        }
    Out << *pIn++;
    }
  Out << '\0';
}

/*!
 *Let only first cmd to be handled.
 *\param In client's requests
 *\param Out client's requests without telnet codes
 */
void Ftp::RemovePipelinedCmds(MemBuf &In, MemBuf &Out)
{
  Out.setLength(0);
  if ( In.getRealLength() == 0 )
    return;
  int i = 0;
  for ( char c = In[i]; c != '\0'; i++, c = In[i] )
  {
    Out << c;
    if ( c == '\n' )
      break;
  }
  Out << '\0';
}

void Ftp::Abor()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  //wait to finish data transfer
  if ( !m_bAllowAsynchronousCmds )
    WaitDataConnection();

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
  {
    pFtpUserData->m_bBreakDataConnection = true;
          Thread::join(pFtpUserData->m_dataThreadId);// wait for data connection to end
    ftp_reply(426);
  }
  else
    ftp_reply(226);
  pFtpUserData->m_bBreakDataConnection = false;
}

void Ftp::Pwd()
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  std::string sCurrentPath = "\"";
  sCurrentPath += pFtpUserData->m_cwd + "\"";
  ftp_reply(257, sCurrentPath);
}

void Ftp::Cwd(const std::string &sPath)
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;

  std::string sLocalPath;
  if ( !GetLocalPath(sPath, sLocalPath) )
    return;
  if ( !FilesUtility::isDirectory(sLocalPath) )
    {
      ftp_reply(550);
      return;
    }

  if (sPath[0] == '/')
    pFtpUserData->m_cwd.assign ("");

  if (sPath.size () == 2 && sPath[0] == '.' && sPath[1] == '.')
    {
      size_t ind = pFtpUserData->m_cwd.find_last_of ('/');
      if (ind != string::npos)
        pFtpUserData->m_cwd.erase (ind + 1);
    }
  else if (sPath.size () != 1 || sPath[0] != '.')
    {
      if (pFtpUserData->m_cwd.size () &&
          pFtpUserData->m_cwd[pFtpUserData->m_cwd.size () - 1] != '/')
        pFtpUserData->m_cwd += "/";
      pFtpUserData->m_cwd += sPath;
    }

  ftp_reply(250);
}

void Ftp::Rest(const std::string &sRestPoint)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  pFtpUserData->m_nRestartOffset = strtoul(sRestPoint.c_str(), NULL, 10);
  ftp_reply(350);
}

void Ftp::Syst()
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;

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
  if ( !UserLoggedIn() )
    return;

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  //wait to finish data transfer
  if ( !m_bAllowAsynchronousCmds )
    WaitDataConnection();

  if ( pFtpUserData->m_nFtpState == FtpUserData::DATA_CONNECTION_UP )
  {
    std::ostringstream sStat;
    sStat << "Transferring file: " << pFtpUserData->m_sCurrentFileName;
    sStat << " " << pFtpUserData->m_nBytesSent << " bytes transferred from " << pFtpUserData->m_nFileSize;
    ftp_reply(213, sStat.str());
  }
  else
  {
    //TODO: will be implemented later
    ftp_reply(502);
  }
}

void Ftp::Retr(const std::string &sPath)
{
  RetrStor(true, false, sPath);
}

void Ftp::Stor(const std::string &sPath)
{
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }
  RetrStor(false, false, sPath);
}

void Ftp::Stou(const std::string &sPath)
{
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }
  std::string sOutPath, sTempPath(sPath);
  int nCount = -1;
  do
  {
    if ( nCount >= 0 )
    {
      std::ostringstream sRename;
      sRename << nCount;
      sTempPath = sPath + sRename.str();
    }
    if ( !BuildLocalPath(sTempPath, sOutPath) )
      return;
    nCount++;
  } while ( FilesUtility::fileExists(sOutPath) );
  RetrStor(false, false, sOutPath);
}

void Ftp::Dele(const std::string &sPath)
{
  WaitDataConnection();
  std::string sLocalPath;
  if ( !UserLoggedIn() || !GetLocalPath(sPath, sLocalPath) )
    return;
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }
  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");

  if( !strcmpi(sLocalFileName.c_str(), secName) )
  {
    ftp_reply(550);
    CloseDataConnection();
    return;
  }
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( strcmpi(pFtpUserData->m_sUserName.c_str(), "anonymous") == 0 )
  {
    if ( CheckRights("Guest", "", sLocalFileName, MYSERVER_PERMISSION_WRITE) == 0 )
    {
      ftp_reply(550);
      return;
    }
  }
  else
  {
    if ( CheckRights(pFtpUserData->m_sUserName, pFtpUserData->m_sPass, 
      sLocalFileName, MYSERVER_PERMISSION_WRITE) == 0 )
    {
      ftp_reply(550);
      return;
    }
  }
  if ( FilesUtility::deleteFile(sLocalPath) != 0 )
    ftp_reply(450);
  ftp_reply(250);
}

void Ftp::Appe(const std::string &sPath)
{
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }
  RetrStor(false, true, sPath);
}

void Ftp::Mkd(const std::string &sPath)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  std::string sLocalPath;// = pFtpUserData->m_cwd + "/" + sPath;
  if ( !BuildLocalPath(sPath, sLocalPath) )
    return;

  if ( CheckRights(pFtpUserData->m_sUserName, pFtpUserData->m_sPass, 
    sLocalPath, MYSERVER_PERMISSION_WRITE) == 0 )
  {
    ftp_reply(550);
    return;
  }
  if ( FilesUtility::mkdir (sLocalPath) == 0 )
    ftp_reply(250);
  else
    ftp_reply(501);
}

void Ftp::Rmd(const std::string &sPath)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  if ( sPath.empty() )
  {
    ftp_reply(550);
    return;
  }
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }

  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  std::string sLocalPath;
  if ( !GetLocalPath(sPath, sLocalPath) )
    return;

  if ( CheckRights(pFtpUserData->m_sUserName, pFtpUserData->m_sPass, 
    sLocalPath, MYSERVER_PERMISSION_WRITE) == 0 )
  {
    ftp_reply(550);
    return;
  }
  if ( FilesUtility::rmdir (sLocalPath) == 0 )
    ftp_reply(250);
  else
    ftp_reply(501);
}

void Ftp::Rnfr(const std::string &sPath)
{
  WaitDataConnection();
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);
  pFtpUserData->m_sRenameFrom = "";
  if ( !UserLoggedIn() )
    return;
  std::string sLocalPath;
  if ( sPath.empty() || !GetLocalPath(sPath, sLocalPath) )
  {
    ftp_reply(550);
    return;
  }
  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);

  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");

  /* The security file doesn't exist in any case.  */
  if( !strcmpi(sLocalFileName.c_str(), secName) )
  {
    ftp_reply(550);
    return;
  }
  pFtpUserData->m_sRenameFrom = sLocalPath;
  ftp_reply(350);
}

void Ftp::Rnto(const std::string &sPath)
{
  WaitDataConnection();
  if ( !UserLoggedIn() )
    return;
  std::string sLocalPath;
  if ( sPath.empty() || !BuildLocalPath(sPath, sLocalPath) )
  {
    ftp_reply(550);
    return;
  }
  if ( !m_bEnableStoreCmds )
  {
    ftp_reply(532);
    return;
  }

  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( CheckRights(pFtpUserData->m_sUserName, pFtpUserData->m_sPass, 
    pFtpUserData->m_sRenameFrom, MYSERVER_PERMISSION_WRITE) == 0)
  {
    ftp_reply(550);
    return;
  }

  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");

  /* The security file doesn't exist in any case.  */
  if( !strcmpi(sLocalFileName.c_str(), secName) )
  {
    ftp_reply(550);
    return;
  }
  FilesUtility::isLink(pFtpUserData->m_sRenameFrom);
  if ( FilesUtility::renameFile(pFtpUserData->m_sRenameFrom, sLocalPath) )
    ftp_reply(550);
  else
    ftp_reply(250);
}

int Ftp::CheckRights(const std::string &sUser, const std::string &sPass, const std::string &sPath, int mask)
{
  if ( sPath.empty() )
    return 0;
  std::string sDir(""), sFileName("");
  if ( !FilesUtility::isDirectory(sPath.c_str()) )
    FilesUtility::splitPath(sPath, sDir, sFileName);
  else
    sDir = sPath;

  string user;
  string password;
  if ( strcmpi(sUser.c_str(), "anonymous") == 0 )
  {
    user.assign ("Guest");
    password.assign("");
  }
  else
  {
    user.assign (sUser);
    password.assign (sPass);
  }

  td.st.setUser (user);
  td.st.setPassword (password);


  td.st.setDirectory (&sDir);
  td.st.setSysDirectory ((string *)&(td.pConnection->host->getSystemRoot ()));
  td.st.setResource (&sFileName);

  AuthDomain auth (&td.st);
  string validator (td.st.getHashedData ("sec.validator", MYSERVER_VHOST_CONF |
                                      MYSERVER_SERVER_CONF, "xml"));
  string authMethod (td.st.getHashedData ("sec.auth_method", MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF, "xml"));

  SecurityDomain* domains[] = {&auth, NULL};

  Server::getInstance()->getSecurityManager ()->getPermissionMask (&td.st, domains,
                                                                   validator, authMethod);

  return (td.st.getMask () & mask);
}

void Ftp::Size(const std::string &sPath)
{
  WaitDataConnection();
  std::string sLocalPath;
  if ( !UserLoggedIn() || !GetLocalPath(sPath, sLocalPath) )
    return;
  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath(sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getHashedData ("security.filename",
                                             MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                             ".security.xml");
  if( !strcmpi(sLocalFileName.c_str(), secName))
  {
    ftp_reply(550);
    CloseDataConnection();
    return;
  }
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  if ( sPath.empty() || !GetLocalPath(sPath, sLocalPath) )
    {
      ftp_reply(550);
      return;
    }

  if (FilesUtility::isDirectory(sLocalPath.c_str ()))
    {
      ftp_reply(550);
      return;
    }

  File f;
  if (f.openFile (sLocalPath.c_str (), File::OPEN_IF_EXISTS | File::READ))
    {
      ftp_reply (550);
      return;
    }

  char size [12];
  sprintf (size, "%l", f.getFileSize ());
  f.close ();

  ftp_reply(213, size);
}

void Ftp::Allo(int nSize, int nRecordSize/* = -1*/)
{
  //TODO: implement
  Noop();
}

void Ftp::WaitDataConnection()
{
  FtpUserData *pFtpUserData = static_cast<FtpUserData *>(td.pConnection->protocolBuffer);

  pFtpUserData->m_DataConnBusy.lock();
  pFtpUserData->m_DataConnBusy.unlock();
}
