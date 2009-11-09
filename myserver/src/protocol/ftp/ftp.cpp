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

#include "stdafx.h"
#include <include/protocol/ftp/ftp.h>
#include <include/protocol/ftp/ftp_common.h>
#include <include/protocol/ftp/ftp_parser.h>
#include <include/base/thread/thread.h>
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/file/files_utility.h>
#include <include/base/file/file.h>
#include <include/base/string/securestr.h>
#include <include/base/read_directory/read_directory.h>
#include <include/base/string/stringutils.h>
#include <include/base/mem_buff/mem_buff.h>

#include <include/conf/security/auth_domain.h>

#ifndef WIN32
# include <netinet/in.h>
# include <sys/socket.h>
# include <arpa/inet.h>
#endif

static DEFINE_THREAD (SendAsciiFile, pParam);
static DEFINE_THREAD (SendImageFile, pParam);

static DEFINE_THREAD (ReceiveAsciiFile, pParam);
static DEFINE_THREAD (ReceiveImageFile, pParam);

void setFtpHost (FtpHost & out, const FtpHost & in)
{
  out.h1 = in.h1;
  out.h2 = in.h2;
  out.h3 = in.h3;
  out.h4 = in.h4;
  out.p1 = in.p1;
  out.p2 = in.p2;
}

void setFtpHost (FtpHost & out, const char *szIn)
{
  std::stringstream ss;
  char *szLocalIn = strdup (szIn);
  char *tok = strtok (szLocalIn, ",.");
  while (tok != NULL)
    {
      ss << tok << " ";
      tok = strtok (NULL, ",.");
    }
  ss >> out.h1 >> out.h2 >> out.h3 >> out.h4 >> out.p1 >> out.p2;
  free (szLocalIn);
}

void getIpAddr (const FtpHost & host, char *pOut, const int &nBuffSize)
{
  if (pOut == NULL)
    return;
  std::ostringstream sRet;
  sRet << host.h1 << '.' << host.h2 << '.' << host.h3 << '.' << host.h4;
  memset (pOut, 0, nBuffSize);
  strncpy (pOut, sRet.str ().c_str (), nBuffSize - 1);
}

int getPortNo (const FtpHost & host)
{
  return ((host.p1 << 8) + host.p2);
}

std::string getPortNo (unsigned int nport)
{
  unsigned int hiByte = (nport & 0x0000ff00) >> 8;
  unsigned int loByte = nport & 0x000000ff;
  std::ostringstream out;
  out << hiByte << "," << loByte;
  return out.str ();
}

std::string getHost (const FtpHost & host)
{
  std::ostringstream s;
  s << host.h1 << ',' << host.h2 << ',' << host.h3 << ',' << host.
    h4 << ',' << host.p1 << ',' << host.p2;
  return s.str ().c_str ();
}

FtpuserData::FtpuserData ()
{
  reset ();
  m_DataConnBusy.init ();
}

bool FtpuserData::allowdelete (bool wait)
{
  if (wait)
    {
      /* Wait for data connection to finish.  */
      m_DataConnBusy.lock ();
      m_DataConnBusy.unlock ();

    }
  if (m_pDataConnection != NULL)
    return !m_pDataConnection->isScheduled ();
  else
    return true;
}


FtpuserData::~FtpuserData ()
{
  delete m_pDataConnection;
  m_pDataConnection = NULL;
  m_DataConnBusy.destroy ();
}

void FtpuserData::reset ()
{
  m_nFtpstate = NO_CONTROL_CONNECTION;
  m_pDataConnection = new Connection ();
  m_suserName = m_sPass = "";
  m_nFtpRepresentation = REPR_ASCII;
  m_nFtpFormatControl = NON_PRINT;
  m_nFtpFilestructure = STRU_FILE;
  m_nTransfermode = MODE_STREAM;
  m_cwd = "";
  m_nLocalDataport = 0;
  m_bBreakDataConnection = false;
  m_dataThreadId = 0;
  m_bPassiveSrv = false;
  m_nrestartOffset = 0;
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

int FtpuserData::closeDataConnection ()
{
  if (m_nFtpstate < DATA_CONNECTION_UP)
    return 1;

  if (m_pDataConnection != NULL && m_pDataConnection->socket != NULL)
    {
      m_pDataConnection->socket->shutdown (SD_BOTH);
      m_pDataConnection->socket->close ();
      delete m_pDataConnection->socket;
      m_pDataConnection->socket = NULL;
      m_pDataConnection->setScheduled (0);
    }
  m_nFtpstate = USER_LOGGED_IN;
  return 1;
}

DataConnectionWorkerThreadData::DataConnectionWorkerThreadData ()
{
  m_pConnection = NULL;
  m_bappend = false;
}

DataConnectionWorkerThreadData::~DataConnectionWorkerThreadData ()
{
}

FtpThreadContext::FtpThreadContext ()
{
  pConnection = NULL;
  buffer = NULL;
  auxiliaryBuffer = NULL;
  buffersize = 0;
  m_nParseLength = 0;
  pProtocolInterpreter = NULL;
}

struct reply
{
  int nCode;
  std::string sText;
};

static struct reply reply_table[] = {
  {120, "Service ready in %s minutes."},
  {125, "Data connection already open; transfer starting."},
  {150, "File status okay; about to open data connection."},
  {200, "Command okay."},
  {213, "File status."},
  {214, "The following commands are recognized:\r\n\
USER, PASS, PORT, PASV, TYPE, REST, RETR, LIST, NLST, ABOR, \r\n\
CWD, CDUP, PWD, ALLO, STOR, STOU, DELE, APPE, MKD, RMD, \r\n\
RNFR, RNTO, SYST, STAT, QUIT\r\n\
214 Detailed help on commands will soon be available."},
  {215, "%s system type."},
  {220, "Service ready for new user."},
  {221, "Goodbye."},
  {226, "Closing data connection."},
  {227, "Entering passive mode. %s"},
  {230, "user logged in, proceed."},
  {250, "Requested file action okay, completed."},
  {257, "PATHNAME created"},
  {331, "user name okay, need password."},
  {350, "Requested file action pending further information."},
  {421, "Service not available, closing control connection."},
  {425, "Can't open data connection."},
  {426, "Connection closed; Transfer aborted."},
  {450, "Requested file action not taken. File unavailable."},
  {451, "Requested action aborted: local error in processing."},
  {500, "Syntax error, command unrecognized."},
  {501, "Syntax error in parameters or arguments."},
  {502, "Command not implemented."},
  {503, "Bad sequence of commands."},
  {504, "Command not implemented for that parameter."},
  {530, "Not logged in."},
  {532, "Need account for storing files."},
  {550, "Requested action not taken. File unavailable."},
  {0, ""}
};

int getFtpReply (int nReplyCode, std::string & sReply)
{
  for (int i = 0; reply_table[i].nCode != 0; i++)
    {
      if (reply_table[i].nCode != nReplyCode)
        continue;
      sReply = reply_table[i].sText;
      return 1;
    }
  return 0;
}

void ftpReply (ConnectionPtr pConnection, int nReplyCode,
               const std::string & sCustomText /* = "" */ )
{
  if (pConnection == NULL || pConnection->socket == NULL)
    return;

  std::string sLocalCustomText (sCustomText);
  Server *pInstance = Server::getInstance ();
  if (pInstance != NULL && pInstance->stopServer () == 1)
    {
      nReplyCode = 421;
      sLocalCustomText = "";
    }

  std::ostringstream buffer;
  if (!sLocalCustomText.empty ())
    {
      if (nReplyCode >= 0)
        buffer << nReplyCode << " " << sLocalCustomText << "\r\n";
      else
        buffer << sLocalCustomText << "\r\n";
    }
  else
    {
      std::string sReplyText;
      getFtpReply (nReplyCode, sReplyText);
      if (sReplyText.find ('\n') == std::string::npos)
        buffer << nReplyCode << " " << sReplyText << "\r\n";
      else
        buffer << nReplyCode << "-" << sReplyText << "\r\n";
    }

  pConnection->socket->send (buffer.str ().c_str (),
                             strlen (buffer.str ().c_str ()), 0);
}

bool Ftp::m_ballowAnonymous = false;
bool Ftp::m_bAnonymousNeedPass = true;
bool Ftp::m_ballowAsynchronousCmds = true;
bool Ftp::m_bEnablePipelining = true;
bool Ftp::m_bEnablestoreCmds = true;

int Ftp::FIRST_PASV_PORT = 60000;
int Ftp::LAST_PASV_PORT = 65000;

Ftp::Ftp ()
{
  m_nPassiveport = Ftp::FIRST_PASV_PORT;
  protocolOptions = Protocol::FAST_CHECK | Protocol::DENY_DELETE;
  protocolPrefix.assign ("ftp://");
}

Ftp::~Ftp ()
{
}

int Ftp::controlConnection (ConnectionPtr pConnection, char *request,
                            char *auxBuf, u_long reqBufLen, u_long auxBufLen,
                            u_long reqLen, u_long tid)
{
  if (pConnection == NULL)
    return ClientsThread::DELETE_CONNECTION;
  Server *server = Server::getInstance ();
  if (server == NULL)
    return ClientsThread::DELETE_CONNECTION;

  FtpuserData *pFtpuserData = NULL;
  if (pConnection->protocolBuffer == NULL)
    pConnection->protocolBuffer = new FtpuserData ();
  pFtpuserData = static_cast < FtpuserData * >(pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    return ClientsThread::DELETE_CONNECTION;

  /* Check if ftp is busy (return 120) or unavailable (return 421).  */
  if (pConnection->getToRemove () == Connection::REMOVE_OVERLOAD)
    {
      pFtpuserData->m_nFtpstate = FtpuserData::BUISY;
      /* TODO: compute busy time interval.  */
      std::string sTempText;
      getFtpReply (120, sTempText);
      std::string::size_type n = sTempText.find ("%s");
      if (n != std::string::npos)
        sTempText.replace (n, 2, "10");
      ftpReply (120, sTempText);
    }

  if (server->isRebooting () != 0)
    {
      pFtpuserData->m_nFtpstate = FtpuserData::UNAVAILABLE;
      ftpReply (421);
      return 0;
    }

  m_nLocalControlport = pConnection->getLocalPort ();
  pFtpuserData->m_nLocalDataport = m_nLocalControlport - 1;

  if (pFtpuserData->m_cwd.empty () && pConnection->host != NULL)
    pFtpuserData->m_cwd = "";

  td.pConnection = pConnection;
  td.buffer = pConnection->getActiveThread ()->getBuffer ();
  td.auxiliaryBuffer = pConnection->getActiveThread ()->getAuxiliaryBuffer ();
  td.buffersize = pConnection->getActiveThread ()->getBufferSize ();
  td.nBytesToRead = reqLen;
  td.pProtocolInterpreter = this;
  td.m_nParseLength = 0;

  return parseControlConnection ();
}


const char * Ftp::getName ()
{
  return getNameImpl ();
}

const char * Ftp::getNameImpl ()
{
  return "FTP";
}


int Ftp::loadProtocolstatic ()
{
  Server *server = Server::getInstance ();

  const char *pData = server->getData ("ftp.allow_anonymous");
  if (pData != NULL)
    m_ballowAnonymous = strcmpi ("Yes", pData) == 0 ? true : false;

  pData = server->getData ("ftp.anonymous_need_pass");
  if (pData != NULL)
    m_bAnonymousNeedPass = strcmpi ("Yes", pData) == 0 ? true : false;

  pData = server->getData ("ftp.allow_asynchronous_cmds");
  if (pData != NULL)
    m_ballowAsynchronousCmds = strcmpi ("Yes", pData) == 0 ? true : false;

  pData = server->getData ("ftp.allow_pipelining");
  if (pData != NULL)
    m_bEnablePipelining = strcmpi ("Yes", pData) == 0 ? true : false;

  pData = server->getData ("ftp.allow_store");
  if (pData != NULL)
    m_bEnablestoreCmds = strcmpi ("Yes", pData) == 0 ? true : false;

  return 1;
}

int Ftp::unLoadProtocolstatic ()
{
  return 1;
}

void Ftp::ftpReply (int nReplyCode, const std::string & sCustomText /*= ""*/ )
{
  if (td.pConnection == NULL || td.pConnection->socket == NULL)
    return;

  ::ftpReply (td.pConnection, nReplyCode, sCustomText);

  logAccess (nReplyCode, sCustomText);
}

void Ftp::logAccess (int nReplyCode, const std::string & sCustomText)
{
  string time;
  char msgCode[12];
  sprintf (msgCode, "%i", nReplyCode);
  getLocalLogFormatDate (time, 32);

  td.auxiliaryBuffer->setLength (0);
  *td.auxiliaryBuffer << time
                      << " " << td.pConnection->getIpAddr ()
                      << " " << msgCode << " " << sCustomText << end_str;

  if (td.pConnection->host)
    td.pConnection->host->accessesLogWrite ("%s", td.auxiliaryBuffer->getBuffer ());
}

int Ftp::closeDataConnection ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  int nRet = pFtpuserData->closeDataConnection ();
  return nRet;
}

int Ftp::printError (const char *msg)
{
  td.pConnection->host->warningsLogWrite ("%s", msg);
  return 1;
}

void
Ftp::user (const std::string & sParam)
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (!m_ballowAnonymous
      && strcmpi (pFtpuserData->m_suserName.c_str (), "anonymous") == 0)
    {
      ftpReply (530);
      return;
    }

  pFtpuserData->m_suserName = sParam;
  ftpReply (331);
}

void Ftp::password (const std::string & sParam)
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (!m_ballowAnonymous
      && strcmpi (pFtpuserData->m_suserName.c_str (), "anonymous") == 0)
    {
      ftpReply (530);
      return;
    }

  pFtpuserData->m_sPass = sParam;
  pFtpuserData->m_nFtpstate = FtpuserData::USER_LOGGED_IN;
  ftpReply (230);
}

void Ftp::port (const FtpHost & host)
{
  waitDataConnection ();

  if (!userLoggedIn ())
    return;
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  setFtpHost (pFtpuserData->m_cdh, host);
  ftpReply (200);
}

void Ftp::pasv ()
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  std::string sHost = td.pConnection->getLocalIpAddr ();
  if (m_nPassiveport < Ftp::LAST_PASV_PORT)
    sHost += "," + getPortNo (m_nPassiveport++);
  else
    {
      m_nPassiveport = Ftp::FIRST_PASV_PORT;
      sHost += "," + getPortNo (m_nPassiveport++);
    }
  setFtpHost (pFtpuserData->m_cdh, sHost.c_str ());

  pFtpuserData->m_bPassiveSrv = true;
  if (OpenDataConnection () == 0)
    {
      /* RFC959 command replay exception.  */
      ftpReply (425);
      return;
    }

  std::string sTempText;
  getFtpReply (227, sTempText);
  std::string::size_type n = sTempText.find ("%s");
  if (n != std::string::npos)
#ifdef WIN32
    sTempText.replace (n, 2, getHost (pFtpuserData->m_cdh));
#else
  sTempText.replace (n, 2, getHost (pFtpuserData->m_cdh));
#endif
  ftpReply (227, sTempText);

  int timeoutvalue = 3;
#ifdef __linux__
  timeoutvalue = 1;
#endif
#ifdef __HURD__
  timeoutvalue = 5;
#endif
  MYSERVER_SOCKADDRIN asockIn;
  socklen_t asockInLen = 0;
  Socket *asock;
  if (pFtpuserData->m_pDataConnection->socket->dataOnRead (timeoutvalue, 0)
      == 1)
    {
      asockInLen = sizeof (sockaddr_in);
      asock = pFtpuserData->m_pDataConnection->socket->accept (&asockIn,
                                                               &asockInLen);
      if (asock->getHandle () == (Handle) INVALID_SOCKET)
        return;

      pFtpuserData->m_pDataConnection->socket->shutdown (SD_BOTH);
      pFtpuserData->m_pDataConnection->socket->close ();
      delete pFtpuserData->m_pDataConnection->socket;
      pFtpuserData->m_pDataConnection->socket = asock;
    }

  pFtpuserData->m_bPassiveSrv = false;
}

void Ftp::retrstor (bool bretr, bool bappend, const std::string & sPath)
{
  std::string sLocalPath;
  if (!userLoggedIn ())
    return;

  if ((bretr && !getLocalPath (sPath, sLocalPath)) ||
      (!bretr && !buildLocalPath (sPath, sLocalPath)))
    return;

  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath (sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF,
                                       ".security.xml");
  if (!strcmpi (sLocalFileName.c_str (), secName))
    {
      ftpReply (550);
      return;
    }

  int nMask = 0;
  if (bretr)
    nMask = MYSERVER_PERMISSION_READ | MYSERVER_PERMISSION_BROWSE;
  else
    nMask = MYSERVER_PERMISSION_WRITE;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (checkRights (pFtpuserData->m_suserName, pFtpuserData->m_sPass,
                   sLocalPath, nMask) == 0)
    {
      ftpReply (530);
      return;
    }

  if (FilesUtility::isDirectory (sLocalPath.c_str ()))
    {
      ftpReply (550);
      return;
    }

  /* FIXME: Log after the file is sent, not before.  */
  logAccess (0, sLocalPath);

  DataConnectionWorkerThreadData *pData =
    new DataConnectionWorkerThreadData ();
  pData->m_pConnection = td.pConnection;
  pData->m_bappend = bappend || pFtpuserData->m_nrestartOffset > 0;
  pData->m_sFilePath = sLocalPath;
  pData->m_pFtp = this;

  pFtpuserData->m_sCurrentFileName = "";
  pFtpuserData->m_nFileSize = 0;
  pFtpuserData->m_nBytesSent = 0;

  switch (pFtpuserData->m_nFtpRepresentation)
    {
    case FtpuserData::REPR_ASCII:
      Thread::create (&pFtpuserData->m_dataThreadId,
                      bretr ? SendAsciiFile : ReceiveAsciiFile, pData);
      break;
    case FtpuserData::REPR_IMAGE:
      Thread::create (&pFtpuserData->m_dataThreadId,
                      bretr ? SendImageFile : ReceiveImageFile, pData);
      break;
    }
}

static
DEFINE_THREAD (SendAsciiFile, pParam)
{
  DataConnectionWorkerThreadData *pWt =
    reinterpret_cast < DataConnectionWorkerThreadData * >(pParam);
  if (pWt == NULL)
    {
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if (pConnection == NULL)
    {
      ftpReply (pConnection, 451);
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    {
      ftpReply (pConnection, 451);
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  pFtpuserData->m_DataConnBusy.lock ();

  if (pWt->m_pFtp == NULL)
    {
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (pConnection, 125);
  else
    {
      ftpReply (pConnection, 150);
      if (pWt->m_pFtp->OpenDataConnection () == 0)
        {
          ftpReply (pConnection, 425);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
    }

  if (pFtpuserData->m_pDataConnection == NULL ||
      pFtpuserData->m_pDataConnection->socket == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }
  File *file = NULL;
  try
    {
      file =
        Server::getInstance ()->getCachedFiles ()->open (pWt->m_sFilePath.
                                                         c_str ());
      if (file == NULL)
        {
          ftpReply (pConnection, 451);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
      u_long filesize = file->getFileSize ();

      /* don't implement restart for ASCII.  */
      if (pFtpuserData->m_nrestartOffset > 0)
        pFtpuserData->m_nrestartOffset = 0;
      pFtpuserData->m_sCurrentFileName = pWt->m_sFilePath;
      pFtpuserData->m_nFileSize = filesize;

      u_long nbr, nBufferSize = 0;
      char *pLine = NULL;
      int nLineLength = 0;
      std::string sLine;
      MemBuf buffer, auxiliaryBuffer;
      buffer.setRealLength (1024);
      while (filesize != 0)
        {
          memset (buffer.getBuffer (), 0, buffer.getRealLength ());
          nBufferSize =
            std::min (static_cast < u_long > (filesize),
                      static_cast < u_long > (buffer.getRealLength () / 2));
          if (file->read (buffer.getBuffer (), nBufferSize, &nbr))
            {
              ftpReply (pConnection, 451);
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 0;
#elif HAVE_PTHREAD
              return (void *) 0;
#endif
            }
          filesize -= nbr;
          pFtpuserData->m_nBytesSent += nbr;

          auxiliaryBuffer.setLength (0);
          pLine = buffer.getBuffer ();
          if (pLine == NULL)
            {
              ftpReply (pConnection, 451);
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 0;
#elif HAVE_PTHREAD
              return (void *) 0;
#endif
            }
          while (*pLine != 0)
            {
              nLineLength = getEndLine (pLine, 0);
              if (nLineLength < 0)
                {
                  sLine.assign (pLine, strlen (pLine));
                  if (!sLine.empty ())
                    auxiliaryBuffer << sLine;
                  pLine += strlen (pLine);
                }
              else
                {
                  sLine.assign (pLine, nLineLength);
                  auxiliaryBuffer << sLine << "\r\n";
                  if (*(pLine + nLineLength) == '\r')
                    nLineLength++;
                  if (*(pLine + nLineLength) == '\n')
                    nLineLength++;
                  pLine += nLineLength;
                }
            }
          if (pFtpuserData->m_pDataConnection->socket->
              send (auxiliaryBuffer.getBuffer (),
                    (u_long) auxiliaryBuffer.getLength (), 0) == SOCKET_ERROR)
            {
              ftpReply (pConnection, 451);
              file->close ();
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 0;
#elif HAVE_PTHREAD
              return (void *) 0;
#endif
            }
          if (pFtpuserData->m_bBreakDataConnection)
            {
              pFtpuserData->m_bBreakDataConnection = false;
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 1;
#elif HAVE_PTHREAD
              return (void *) 1;
#endif
            }
        }
      file->close ();
      delete file;
    }
  catch (bad_alloc & ba)
    {
      if (file != NULL)
        file->close ();
      delete file;
    }

  pFtpuserData->m_sCurrentFileName = "";
  pFtpuserData->m_nFileSize = 0;
  pFtpuserData->m_nBytesSent = 0;
  pFtpuserData->m_nrestartOffset = 0;
  ftpReply (pConnection, 226);
  pFtpuserData->closeDataConnection ();
  pFtpuserData->m_DataConnBusy.unlock ();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void *) 1;
#endif

}

static
DEFINE_THREAD (SendImageFile, pParam)
{
  DataConnectionWorkerThreadData *pWt =
    reinterpret_cast < DataConnectionWorkerThreadData * >(pParam);
  if (pWt == NULL)
    {
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if (pConnection == NULL)
    {
      ftpReply (pConnection, 451);
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  pFtpuserData->m_DataConnBusy.lock ();

  if (pWt->m_pFtp == NULL)
    {
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (pConnection, 125);
  else
    {
      ftpReply (pConnection, 150);
      if (pWt->m_pFtp->OpenDataConnection () == 0)
        {
          ftpReply (pConnection, 425);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
    }

  if (pFtpuserData->m_pDataConnection == NULL ||
      pFtpuserData->m_pDataConnection->socket == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  File *file = NULL;
  try
    {
      file =
        Server::getInstance ()->getCachedFiles ()->open (pWt->m_sFilePath.
                                                         c_str ());
      if (file == NULL)
        {
          ftpReply (pConnection, 451);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
      u_long filesize = file->getFileSize ();
      u_long nbr, nBufferSize = 0;
      if (pWt->m_bappend && pFtpuserData->m_nrestartOffset < filesize)
        {
          file->seek (pFtpuserData->m_nrestartOffset);
          filesize -= pFtpuserData->m_nrestartOffset;
        }

      pFtpuserData->m_sCurrentFileName = pWt->m_sFilePath;
      pFtpuserData->m_nFileSize = filesize;

      MemBuf auxiliaryBuffer;
      auxiliaryBuffer.setRealLength (1024);
      while (filesize != 0)
        {
          nBufferSize =
            std::min (static_cast < u_long > (filesize),
                      static_cast < u_long >
                      (auxiliaryBuffer.getRealLength () / 2));

          if (file->read (auxiliaryBuffer.getBuffer (), nBufferSize, &nbr)
              || pFtpuserData->m_pDataConnection->socket->send (auxiliaryBuffer.getBuffer (),
                                                                (u_long)nBufferSize, 0)
              == SOCKET_ERROR)
            {
              ftpReply (pConnection, 451);
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 0;
#elif HAVE_PTHREAD
              return (void *) 0;
#endif
            }
          filesize -= nbr;
          pFtpuserData->m_nBytesSent += nbr;
          pFtpuserData->m_nrestartOffset += nbr;
          if (pFtpuserData->m_bBreakDataConnection)
            {
              pFtpuserData->m_bBreakDataConnection = false;
              file->close ();
              delete file;
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 1;
#elif HAVE_PTHREAD
              return (void *) 1;
#endif
            }
        }
      file->close ();
      delete file;
    }
  catch (bad_alloc & ba)
    {
      if (file != NULL)
        file->close ();
      delete file;
    }

  pFtpuserData->m_sCurrentFileName = "";
  pFtpuserData->m_nFileSize = 0;
  pFtpuserData->m_nBytesSent = 0;
  pFtpuserData->m_nrestartOffset = 0;
  ftpReply (pConnection, 226);
  pFtpuserData->closeDataConnection ();
  pFtpuserData->m_DataConnBusy.unlock ();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void *) 1;
#endif
}

static
DEFINE_THREAD (ReceiveAsciiFile, pParam)
{
  DataConnectionWorkerThreadData *pWt =
    reinterpret_cast < DataConnectionWorkerThreadData * >(pParam);

  if (pWt == NULL)
    {
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }
  ConnectionPtr pConnection = pWt->m_pConnection;
  if (pConnection == NULL)
    {
      ftpReply (pConnection, 451);
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  pFtpuserData->m_DataConnBusy.lock ();

  if (pWt->m_pFtp == NULL)
    {
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (pConnection, 125);
  else
    {
      ftpReply (pConnection, 150);
      if (pWt->m_pFtp->OpenDataConnection () == 0)
        {
          ftpReply (pConnection, 425);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
    }

  if (pFtpuserData->m_pDataConnection == NULL ||
      pFtpuserData->m_pDataConnection->socket == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  File file;
  try
    {
      u_long flags = 0;
      if (pWt->m_bappend)
        flags = File::APPEND | File::WRITE;
      else
        flags = File::FILE_CREATE_ALWAYS | File::WRITE;
      if (file.openFile (pWt->m_sFilePath.c_str (), flags))
        {
          ftpReply (pConnection, 451);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }

      MemBuf buffer, auxiliaryBuffer;
      buffer.setRealLength (1024);
      memset (buffer.getBuffer (), 0, buffer.getRealLength ());
      char *pLine = NULL;
      int nLineLength = 0;
      std::string sLine;
      u_long nbr;
      while (pFtpuserData->m_pDataConnection->socket->read (buffer.getBuffer (),
                                                            (u_long) buffer.getRealLength () - 1, &nbr)
             != SOCKET_ERROR && nbr != 0)
        {
          memset (auxiliaryBuffer.getBuffer (), 0,
                  auxiliaryBuffer.getRealLength ());
          auxiliaryBuffer.setLength (0);
          pLine = buffer.getBuffer ();
          if (pLine == NULL)
            {
              ftpReply (pConnection, 451);
              file.close ();
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 0;
#elif HAVE_PTHREAD
              return (void *) 0;
#endif
            }
          while (*pLine != 0)
            {
              nLineLength = getEndLine (pLine, 0);
              if (nLineLength < 0)
                {
                  sLine.assign (pLine, strlen (pLine));
                  if (!sLine.empty ())
                    auxiliaryBuffer << sLine;
                  pLine += strlen (pLine);
                }
              else
                {
                  sLine.assign (pLine, nLineLength);
#ifdef WIN32
                  auxiliaryBuffer << sLine << "\r\n";
#else
                  auxiliaryBuffer << sLine << "\n";
#endif
                  if (*(pLine + nLineLength) == '\r')
                    nLineLength++;
                  if (*(pLine + nLineLength) == '\n')
                    nLineLength++;
                  pLine += nLineLength;
                }
            }
          file.write (auxiliaryBuffer.getBuffer (),
                      (u_long) auxiliaryBuffer.getLength (), &nbr);

          if (pFtpuserData->m_bBreakDataConnection)
            {
              pFtpuserData->m_bBreakDataConnection = false;
              file.close ();
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 1;
#elif HAVE_PTHREAD
              return (void *) 1;
#endif
            }
          memset (buffer.getBuffer (), 0, buffer.getRealLength ());
        }
      file.close ();
    }
  catch (bad_alloc & ba)
    {
      file.close ();
    }

  ftpReply (pConnection, 226);
  pFtpuserData->closeDataConnection ();
  pFtpuserData->m_DataConnBusy.unlock ();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void *) 1;
#endif
}

static
DEFINE_THREAD (ReceiveImageFile, pParam)
{
  DataConnectionWorkerThreadData *pWt =
    reinterpret_cast < DataConnectionWorkerThreadData * >(pParam);
  if (pWt == NULL)
    {
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) -1;
#endif
    }

  ConnectionPtr pConnection = pWt->m_pConnection;
  if (pConnection == NULL)
    {
      ftpReply (pConnection, 451);
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  pFtpuserData->m_DataConnBusy.lock ();

  if (pWt->m_pFtp == NULL)
    {
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (pConnection, 125);
  else
    {
      ftpReply (pConnection, 150);
      if (pWt->m_pFtp->OpenDataConnection () == 0)
        {
          ftpReply (pConnection, 425);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
    }

  if (pFtpuserData->m_pDataConnection == NULL ||
      pFtpuserData->m_pDataConnection->socket == NULL)
    {
      ftpReply (pConnection, 451);
      pFtpuserData->closeDataConnection ();
      pFtpuserData->m_DataConnBusy.unlock ();
      delete pWt;
#ifdef WIN32
      return 0;
#elif HAVE_PTHREAD
      return (void *) 0;
#endif
    }

  File file;
  try
    {
      u_long flags = 0;
      if (pWt->m_bappend)
        flags = File::APPEND | File::WRITE;
      else
        flags = File::FILE_CREATE_ALWAYS | File::WRITE;
      if (file.openFile (pWt->m_sFilePath.c_str (), flags))
        {
          ftpReply (pConnection, 451);
          pFtpuserData->closeDataConnection ();
          pFtpuserData->m_DataConnBusy.unlock ();
          delete pWt;
#ifdef WIN32
          return 0;
#elif HAVE_PTHREAD
          return (void *) 0;
#endif
        }
      u_long nbr;
      MemBuf buffer;
      buffer.setRealLength (1024);
      memset (buffer.getBuffer (), 0, buffer.getRealLength ());
      while (pFtpuserData->m_pDataConnection->socket->read (buffer.getBuffer (),
                                                            (u_long) buffer.
                                                            getRealLength () -
                                                            1,
                                                            &nbr) !=
             SOCKET_ERROR && nbr != 0)
        {
          file.write (buffer.getBuffer (), nbr, &nbr);
          if (pFtpuserData->m_bBreakDataConnection)
            {
              pFtpuserData->m_bBreakDataConnection = false;
              file.close ();
              pFtpuserData->closeDataConnection ();
              pFtpuserData->m_DataConnBusy.unlock ();
              delete pWt;
#ifdef WIN32
              return 1;
#elif HAVE_PTHREAD
              return (void *) 1;
#endif
            }
        }
      file.close ();
    }
  catch (bad_alloc & ba)
    {
      file.close ();
    }

  ftpReply (pConnection, 226);
  pFtpuserData->closeDataConnection ();
  pFtpuserData->m_DataConnBusy.unlock ();
  delete pWt;
#ifdef WIN32
  return 1;
#elif HAVE_PTHREAD
  return (void *) 1;
#endif
}

bool Ftp::userLoggedIn ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  if (pFtpuserData == NULL)
    {
      return false;
    }
  if (pFtpuserData->m_nFtpstate < FtpuserData::USER_LOGGED_IN)
    {
      ftpReply (530);
      return false;
    }
  return true;
}

/*!
 *Converts from relative client's path to local path (out path may not exist).
 *\param sPathIn client's relative path
 *\param sOutPath local path
 *\return Return true if path exist, file is a normal one and is into the ftp's root folder
 */
bool
Ftp::buildLocalPath (const std::string & sPathIn, std::string & sOutPath)
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  sOutPath = td.pConnection->host->getDocumentRoot ();

  if (sOutPath.length ())
    sOutPath.append ("/");

  if (sPathIn[0] != '/')
    sOutPath.append (pFtpuserData->m_cwd);

  if (sPathIn[0] != '-')
    sOutPath.append (sPathIn);

  FilesUtility::completePath (sOutPath);

  std::string sDocRoot (td.pConnection->host->getDocumentRoot ());
  FilesUtility::completePath (sDocRoot);
  if (FilesUtility::getPathRecursionLevel (sDocRoot) >
      FilesUtility::getPathRecursionLevel (sOutPath))
    {
      ftpReply (550);
      return false;
    }
  if (FilesUtility::isDirectory (sOutPath) &&
      (sOutPath[sOutPath.length () - 1] != '/'
       && sOutPath[sOutPath.length () - 1] != '\\'))
    sOutPath.append ("/");
  return true;
}

/*!
 *Converts from relative client's path to local path and checks if the path is available.
 *\param sPath client's relative path
 *\param sOutPath local path
 *\return Return true if path exist, file is a normal one and is into the ftp's root folder
 */
bool Ftp::getLocalPath (const std::string & sPath, std::string & sOutPath)
{
  if (!buildLocalPath (sPath, sOutPath))
    return false;

  if (sOutPath.empty () ||
      !FilesUtility::fileExists (sOutPath) ||
      FilesUtility::isLink (sOutPath.c_str ()))
    {
      ftpReply (550);
      return false;
    }
  return true;
}

void
Ftp::quit ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (!m_ballowAsynchronousCmds)
    waitDataConnection ();

  pFtpuserData->m_nFtpstate = FtpuserData::NO_CONTROL_CONNECTION;
  ftpReply (221);
}

void
Ftp::help (const std::string & sCmd /* = "" */ )
{
  waitDataConnection ();
  if (sCmd.empty () || stringcmpi (sCmd, "SITE") == 0)
    ftpReply (214);
  else
    ftpReply (502);
}

void
Ftp::noop ()
{
  waitDataConnection ();
  ftpReply (200);
}

int
Ftp::OpenDataConnection ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    return 1;

  int nRet = pFtpuserData->m_bPassiveSrv ? openDataPassive ()
    : openDataActive ();
  if (nRet != 0)
    pFtpuserData->m_nFtpstate = FtpuserData::DATA_CONNECTION_UP;
  return nRet;
}

int
Ftp::openDataPassive ()
{
  Server *server = Server::getInstance ();
  if (server == NULL)
    return 0;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  Socket *pSocket = new Socket ();
  pSocket->socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (pSocket->getHandle () == (Handle) INVALID_SOCKET)
    return 0;
  int nReuseAddr = 1;
  MYSERVER_SOCKADDR_STORAGE storage = { 0 };
  ((sockaddr_in *) (&storage))->sin_family = AF_INET;
  char szIpAddr[16];
  memset (szIpAddr, 0, 16);
  getIpAddr (pFtpuserData->m_cdh, szIpAddr, 16);
#ifdef WIN32
  ((sockaddr_in *) (&storage))->sin_addr.s_addr = inet_addr (szIpAddr);
#else
  inet_aton (szIpAddr, &((sockaddr_in *) (&storage))->sin_addr);
#endif
  ((sockaddr_in *) (&storage))->sin_port =
    htons (getPortNo (pFtpuserData->m_cdh));
  if (pSocket->setsockopt (SOL_SOCKET, SO_REUSEADDR, (const char *) &nReuseAddr,
                           sizeof (nReuseAddr)) < 0)
    return 0;

  if (pSocket->bind (&storage, sizeof (sockaddr_in)) != 0
      || pSocket->listen (SOMAXCONN) != 0)
    return 0;

  pFtpuserData->m_pDataConnection->setPort (getPortNo (pFtpuserData->m_cdh));
  pFtpuserData->m_pDataConnection->setLocalPort (pFtpuserData->m_nLocalDataport);
  pFtpuserData->m_pDataConnection->setIpAddr (td.pConnection->getIpAddr ());
  pFtpuserData->m_pDataConnection->setLocalIpAddr (td.pConnection->getLocalIpAddr ());
  pFtpuserData->m_pDataConnection->host = td.pConnection->host;
  pFtpuserData->m_pDataConnection->socket = pSocket;
  pFtpuserData->m_pDataConnection->setScheduled (1);
  return 1;
}

int
Ftp::openDataActive ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  Socket *dataSocket = new Socket ();
  dataSocket->socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char szIpAddr[16];
  memset (szIpAddr, 0, 16);
  getIpAddr (pFtpuserData->m_cdh, szIpAddr, 16);
  if (dataSocket->connect (szIpAddr, getPortNo (pFtpuserData->m_cdh)) < 0)
    {
      delete dataSocket;
      return 0;
    }

  pFtpuserData->m_pDataConnection->setPort (getPortNo (pFtpuserData->m_cdh));
  pFtpuserData->m_pDataConnection->setLocalPort (pFtpuserData->m_nLocalDataport);
  pFtpuserData->m_pDataConnection->setIpAddr (td.pConnection->getIpAddr ());
  pFtpuserData->m_pDataConnection->setLocalIpAddr (td.pConnection->
                                                   getLocalIpAddr ());
  pFtpuserData->m_pDataConnection->host = td.pConnection->host;
  pFtpuserData->m_pDataConnection->socket = dataSocket;
  pFtpuserData->m_pDataConnection->setScheduled (1);

  return 1;
}

int
Ftp::type (int ntypeCode, int nFormatControlCode /* = -1 */ )
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return 0;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  pFtpuserData->m_nFtpRepresentation =
    (FtpuserData::FtpRepresentation) ntypeCode;
  switch (nFormatControlCode)
    {
    case FtpuserData::NON_PRINT:
      if (ntypeCode == FtpuserData::REPR_ASCII)
        pFtpuserData->m_nFtpFormatControl =
          (FtpuserData::FtpFormatControl) nFormatControlCode;
      else
        {
          ftpReply (501);
          return 0;
        }
      break;

    case FtpuserData::REPR_IMAGE:
      pFtpuserData->m_nFtpFormatControl =
        (FtpuserData::FtpFormatControl) nFormatControlCode;
      break;
    }

  ftpReply (200);
  return 1;
}

void
Ftp::stru (int nstructure)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  if (nstructure < 0)
    ftpReply (504);

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  pFtpuserData->m_nFtpFilestructure =
    (FtpuserData::FtpFilestructure) nstructure;
  ftpReply (200);
}

void
Ftp::mode (int nmode)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  if (nmode < 0)
    ftpReply (504);

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  pFtpuserData->m_nTransfermode = (FtpuserData::FtpTransfermode) nmode;
}

void
Ftp::list (const std::string & sParam /*= ""*/ )
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  waitDataConnection ();

  std::string sLocalPath;
  if (!userLoggedIn () || !getLocalPath (sParam, sLocalPath))
    return;

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (125);
  else
    {
      ftpReply (150);
      if (OpenDataConnection () == 0)
        {
          ftpReply (425);
          return;
        }
    }

  std::string sPath (sLocalPath);
  if (sPath.empty ())
    sPath = pFtpuserData->m_cwd;

  const char *username = pFtpuserData->m_suserName.c_str ();
  const char *password = pFtpuserData->m_sPass.c_str ();

  if (checkRights (username, password, sLocalPath,
                   MYSERVER_PERMISSION_BROWSE) == 0)
    {
      ftpReply (530);
      return;
    }

  time_t now;
  time (&now);

  MemBuf & auxiliaryBuffer = *td.auxiliaryBuffer;
  auxiliaryBuffer.setLength (0);

  char perm[11];
  if (FilesUtility::isDirectory (sPath))
    {
      ReadDirectory fd;

      if (fd.findfirst (sPath))
        {
          ftpReply (450);
          closeDataConnection ();
          return;
        }

      const char *secName = td.st.getData ("security.filename",
                                           MYSERVER_VHOST_CONF |
                                           MYSERVER_SERVER_CONF,
                                           ".security.xml");
      do
        {
          if (fd.name[0] == '.' || !strcmpi (fd.name.c_str (), secName))
            continue;

          perm[10] = '\0';
          perm[0] = fd.attrib == FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-';
          string completeFileName (sPath);
          completeFileName.append (fd.name);
          int guestMask = checkRights ("Guest", "", completeFileName, -1);
          int pMask = checkRights (username, password, completeFileName, -1);

          perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
          perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
          perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

          perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
          perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
          perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

          string date;
          const char *datePtr = getRFC822LocalTime (fd.time_write, date, 32);

          char dateFtpFormat[13];

          dateFtpFormat[3] = ' ';
          dateFtpFormat[6] = ' ';
          dateFtpFormat[7] = ' ';
          dateFtpFormat[12] = '\0';

          int offset = datePtr[6] == ' ' ? 0 : 1;

          dateFtpFormat[4] = datePtr[5];
          dateFtpFormat[5] = datePtr[6];

          dateFtpFormat[0] = datePtr[7 + offset];
          dateFtpFormat[1] = datePtr[8 + offset];
          dateFtpFormat[2] = datePtr[9 + offset];

          /* If the file was modified in the last 6 months
           * show the hour instead of the year.  */
          if (now - fd.time_write < 60 * 60 * 183 && now > fd.time_write)
            {
              dateFtpFormat[7] = datePtr[16 + offset];
              dateFtpFormat[8] = datePtr[17 + offset];
              dateFtpFormat[9] = ':';
              dateFtpFormat[10] = datePtr[19 + offset];
              dateFtpFormat[11] = datePtr[20 + offset];
            }
          else
            {
              dateFtpFormat[8] = datePtr[11 + offset];
              dateFtpFormat[9] = datePtr[12 + offset];
              dateFtpFormat[10] = datePtr[13 + offset];
              dateFtpFormat[11] = datePtr[14 + offset];
            }

          char nlinkStr[12];
          sprintf (nlinkStr, "%lu", fd.st_nlink);

          char fdSizeStr[12];
          sprintf (fdSizeStr, "%li", fd.size);

          auxiliaryBuffer << (const char *) perm << " " << nlinkStr << " "
                          << username << " " << username << " " << fdSizeStr
                          << " " << (const char *) dateFtpFormat << " "
                          << fd.name << "\r\n";

        }
      while (!fd.findnext ());
      fd.findclose ();
    }
  else if (!FilesUtility::isLink (sPath))
    {
      /* TODO: implement * selection.  */
      std::string sDir, sFileName;
      FilesUtility::splitPath (sLocalPath, sDir, sFileName);
      ReadDirectory fd;
      if (fd.findfirst (sDir))
        {
          ftpReply (450);
          closeDataConnection ();
          return;
        }
      do
        {
          if (strcmp (fd.name.c_str (), sFileName.c_str ()) != 0)
            continue;

          perm[10] = '\0';
          perm[0] = fd.attrib == FILE_ATTRIBUTE_DIRECTORY ? 'd' : '-';

          string completeFileName (sDir);
          completeFileName.append (fd.name);

          int guestMask = checkRights ("guest", "", completeFileName, -1);
          int pMask = checkRights (username, password, completeFileName, -1);

          perm[1] = perm[4] = pMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
          perm[2] = perm[5] = pMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
          perm[3] = perm[6] = pMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

          perm[7] = guestMask & MYSERVER_PERMISSION_READ ? 'r' : '-';
          perm[8] = guestMask & MYSERVER_PERMISSION_WRITE ? 'w' : '-';
          perm[9] = guestMask & MYSERVER_PERMISSION_EXECUTE ? 'x' : '-';

          string date;
          const char *datePtr = getRFC822LocalTime (fd.time_write, date, 32);

          char dateFtpFormat[13];

          dateFtpFormat[3] = ' ';
          dateFtpFormat[6] = ' ';
          dateFtpFormat[7] = ' ';
          dateFtpFormat[12] = '\0';

          int offset = datePtr[6] == ' ' ? 0 : 1;

          dateFtpFormat[4] = datePtr[5];
          dateFtpFormat[5] = datePtr[6];

          dateFtpFormat[0] = datePtr[7 + offset];
          dateFtpFormat[1] = datePtr[8 + offset];
          dateFtpFormat[2] = datePtr[9 + offset];

          /* If the file was modified in the last 6 months
           * show the hour instead of the year.  */
          if (now - fd.time_write < 60 * 60 * 183 && now > fd.time_write)
            {
              dateFtpFormat[7] = datePtr[16 + offset];
              dateFtpFormat[8] = datePtr[17 + offset];
              dateFtpFormat[9] = ':';
              dateFtpFormat[10] = datePtr[19 + offset];
              dateFtpFormat[11] = datePtr[20 + offset];
            }
          else
            {
              dateFtpFormat[8] = datePtr[11 + offset];
              dateFtpFormat[9] = datePtr[12 + offset];
              dateFtpFormat[10] = datePtr[13 + offset];
              dateFtpFormat[11] = datePtr[14 + offset];
            }

          char nlinkStr[12];
          sprintf (nlinkStr, "%lu", fd.st_nlink);

          char fdSizeStr[12];
          sprintf (fdSizeStr, "%li", fd.size);

          auxiliaryBuffer << (const char *) perm << " " << nlinkStr << " "
                          << username << " " << username << " " << fdSizeStr
                          << " " << (const char *) dateFtpFormat << " "
                          << fd.name << "\r\n";
        }
      while (!fd.findnext ());
      fd.findclose ();
    }

  if (pFtpuserData->m_pDataConnection->socket->
      send (td.auxiliaryBuffer->getBuffer (),
            (u_long) td.auxiliaryBuffer->getLength (), 0) == SOCKET_ERROR)
    {
      ftpReply (451);
    }

  ftpReply (226);
  closeDataConnection ();
}

void
Ftp::nlst (const std::string & sParam /* = "" */ )
{
  waitDataConnection ();
  std::string sLocalPath;
  if (!userLoggedIn () || !getLocalPath (sParam, sLocalPath))
    return;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    ftpReply (125);
  else
    {
      ftpReply (150);
      if (OpenDataConnection () == 0)
        {
          ftpReply (425);
          return;
        }
    }

  std::string sPath (sLocalPath);
  if (sPath.empty ())
    sPath = pFtpuserData->m_cwd;

  if (!FilesUtility::isDirectory (sPath))
    {
      ftpReply (450);
      closeDataConnection ();
      return;
    }
  ReadDirectory fd;
  if (fd.findfirst (sPath))
    {
      ftpReply (450);
      closeDataConnection ();
      return;
    }

  MemBuf & auxiliaryBuffer = *td.auxiliaryBuffer;
  auxiliaryBuffer.setLength (0);

  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF,
                                       ".security.xml");
  do
    {
      if (fd.name[0] == '.' || !strcmpi (fd.name.c_str (), secName))
        continue;

      if (!sParam.empty ())
        auxiliaryBuffer << sParam << "/";
      auxiliaryBuffer << fd.name << "\r\n";
    }
  while (!fd.findnext ());
  fd.findclose ();

  if (pFtpuserData->m_pDataConnection->socket->
      send (td.auxiliaryBuffer->getBuffer (),
            (u_long) td.auxiliaryBuffer->getLength (), 0) == SOCKET_ERROR)
    {
      ftpReply (451);
    }
  ftpReply (226);
  closeDataConnection ();
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
void
Ftp::escapeTelnet (MemBuf & In, MemBuf & Out)
{
  Out.setLength (0);

  if (In.getRealLength () == 0)
    return;

  char *pIn = In.getBuffer ();
  char szReply[3];

  while ((u_int) (pIn - In.getBuffer ()) < In.getLength ())
    {
      if (*pIn == '\377')
        {
          szReply[0] = *pIn++;
          if (*pIn == '\0')
            break;
          switch (*pIn)
            {
            case '\375':
            case '\376':
              szReply[1] = '\374';
              pIn++;
              break;
            case '\373':
            case '\374':
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
          ftpReply (-1, szReply);
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
void Ftp::removePipelinedCmds (MemBuf & In, MemBuf & Out)
{
  Out.setLength (0);
  if (In.getRealLength () == 0)
    return;
  int i = 0;
  for (char c = In[i]; c != '\0'; i++, c = In[i])
    {
      Out << c;
      if (c == '\n')
        break;
    }
  Out << '\0';
}

void Ftp::abor ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (!m_ballowAsynchronousCmds)
    waitDataConnection ();

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    {
      pFtpuserData->m_bBreakDataConnection = true;
      Thread::join (pFtpuserData->m_dataThreadId);
      ftpReply (426);
    }
  else
    ftpReply (226);
  pFtpuserData->m_bBreakDataConnection = false;
}

void Ftp::pwd ()
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  std::string sCurrentPath = "\"";
  sCurrentPath += pFtpuserData->m_cwd + "\"";
  ftpReply (257, sCurrentPath);
}

void Ftp::cwd (const std::string & sPath)
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  waitDataConnection ();
  if (!userLoggedIn ())
    return;

  std::string sLocalPath;
  if (!getLocalPath (sPath, sLocalPath))
    return;
  if (!FilesUtility::isDirectory (sLocalPath))
    {
      ftpReply (550);
      return;
    }

  if (sPath[0] == '/')
    pFtpuserData->m_cwd.assign ("");

  if (sPath.size () == 2 && sPath[0] == '.' && sPath[1] == '.')
    {
      size_t ind = pFtpuserData->m_cwd.find_last_of ('/');
      if (ind != string::npos)
        pFtpuserData->m_cwd.erase (ind + 1);
    }
  else if (sPath.size () != 1 || sPath[0] != '.')
    {
      if (pFtpuserData->m_cwd.size () &&
          pFtpuserData->m_cwd[pFtpuserData->m_cwd.size () - 1] != '/')
        pFtpuserData->m_cwd += "/";
      pFtpuserData->m_cwd += sPath;
    }

  ftpReply (250);
}

void Ftp::rest (const std::string & srestPoint)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  pFtpuserData->m_nrestartOffset = strtoul (srestPoint.c_str (), NULL, 10);
  ftpReply (350);
}

void Ftp::syst ()
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;

  std::string sTempText;
  getFtpReply (215, sTempText);
  std::string::size_type n = sTempText.find ("%s");
  if (n != std::string::npos)
#ifdef WIN32
    sTempText.replace (n, 2, "WIN32");
#else
  sTempText.replace (n, 2, "UNIX type: L8");
#endif
  ftpReply (215, sTempText);
}

void Ftp::statCmd (const std::string & sParam /* = "" */ )
{
  if (!userLoggedIn ())
    return;

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (!m_ballowAsynchronousCmds)
    waitDataConnection ();

  if (pFtpuserData->m_nFtpstate == FtpuserData::DATA_CONNECTION_UP)
    {
      std::ostringstream sstat;
      sstat << "Transferring file: " << pFtpuserData->m_sCurrentFileName;
      sstat << " " << pFtpuserData->
        m_nBytesSent << " bytes transferred from " << pFtpuserData->
        m_nFileSize;
      ftpReply (213, sstat.str ());
    }
  else
    {
      /* TODO.  */
      ftpReply (502);
    }
}

void Ftp::retr (const std::string & sPath)
{
  retrstor (true, false, sPath);
}

void Ftp::stor (const std::string & sPath)
{
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }
  retrstor (false, false, sPath);
}

void Ftp::stou (const std::string & sPath)
{
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }
  std::string sOutPath, sTempPath (sPath);
  int nCount = -1;
  do
    {
      if (nCount >= 0)
        {
          std::ostringstream sRename;
          sRename << nCount;
          sTempPath = sPath + sRename.str ();
        }
      if (!buildLocalPath (sTempPath, sOutPath))
        return;
      nCount++;
    }
  while (FilesUtility::fileExists (sOutPath));
  retrstor (false, false, sOutPath);
}

void Ftp::dele (const std::string & sPath)
{
  waitDataConnection ();
  std::string sLocalPath;
  if (!userLoggedIn () || !getLocalPath (sPath, sLocalPath))
    return;
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }
  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath (sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF,
                                       ".security.xml");
  if (!strcmpi (sLocalFileName.c_str (), secName))
    {
      ftpReply (550);
      closeDataConnection ();
      return;
    }
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (strcmpi (pFtpuserData->m_suserName.c_str (), "anonymous") == 0)
    {
      if (checkRights ("Guest", "", sLocalFileName, MYSERVER_PERMISSION_WRITE)
          == 0)
        {
          ftpReply (550);
          return;
        }
    }
  else
    {
      if (checkRights (pFtpuserData->m_suserName, pFtpuserData->m_sPass,
                       sLocalFileName, MYSERVER_PERMISSION_WRITE) == 0)
        {
          ftpReply (550);
          return;
        }
    }

  if (FilesUtility::deleteFile (sLocalPath) != 0)
    ftpReply (450);
  ftpReply (250);
}

void Ftp::appe (const std::string & sPath)
{
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }
  retrstor (false, true, sPath);
}

void Ftp::mkd (const std::string & sPath)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  std::string sLocalPath;
  if (!buildLocalPath (sPath, sLocalPath))
    return;

  if (checkRights (pFtpuserData->m_suserName, pFtpuserData->m_sPass,
                   sLocalPath, MYSERVER_PERMISSION_WRITE) == 0)
    {
      ftpReply (550);
      return;
    }
  if (FilesUtility::mkdir (sLocalPath) == 0)
    ftpReply (250);
  else
    ftpReply (501);
}

void
Ftp::rmd (const std::string & sPath)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  if (sPath.empty ())
    {
      ftpReply (550);
      return;
    }
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }

  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  std::string sLocalPath;
  if (!getLocalPath (sPath, sLocalPath))
    return;

  if (checkRights (pFtpuserData->m_suserName, pFtpuserData->m_sPass,
                   sLocalPath, MYSERVER_PERMISSION_WRITE) == 0)
    {
      ftpReply (550);
      return;
    }
  if (FilesUtility::rmdir (sLocalPath) == 0)
    ftpReply (250);
  else
    ftpReply (501);
}

void Ftp::rnfr (const std::string & sPath)
{
  waitDataConnection ();
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);
  pFtpuserData->m_sRenameFrom = "";
  if (!userLoggedIn ())
    return;
  std::string sLocalPath;
  if (sPath.empty () || !getLocalPath (sPath, sLocalPath))
    {
      ftpReply (550);
      return;
    }
  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath (sLocalPath, sLocalDir, sLocalFileName);

  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF | MYSERVER_SERVER_CONF,
                                       ".security.xml");

  /* The security file doesn't exist in any case.  */
  if (!strcmpi (sLocalFileName.c_str (), secName))
    {
      ftpReply (550);
      return;
    }
  pFtpuserData->m_sRenameFrom = sLocalPath;
  ftpReply (350);
}

void Ftp::Rnto (const std::string & sPath)
{
  waitDataConnection ();
  if (!userLoggedIn ())
    return;
  std::string sLocalPath;
  if (sPath.empty () || !buildLocalPath (sPath, sLocalPath))
    {
      ftpReply (550);
      return;
    }
  if (!m_bEnablestoreCmds)
    {
      ftpReply (532);
      return;
    }

  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath (sLocalPath, sLocalDir, sLocalFileName);
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  if (checkRights (pFtpuserData->m_suserName, pFtpuserData->m_sPass,
                   pFtpuserData->m_sRenameFrom,
                   MYSERVER_PERMISSION_WRITE) == 0)
    {
      ftpReply (550);
      return;
    }

  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF,
                                       ".security.xml");

  /* The security file doesn't exist in any case.  */
  if (!strcmpi (sLocalFileName.c_str (), secName))
    {
      ftpReply (550);
      return;
    }
  FilesUtility::isLink (pFtpuserData->m_sRenameFrom);
  if (FilesUtility::renameFile (pFtpuserData->m_sRenameFrom, sLocalPath))
    ftpReply (550);
  else
    ftpReply (250);
}

int Ftp::checkRights (const std::string & suser, const std::string & sPass,
                      const std::string & sPath, int mask)
{
  if (sPath.empty ())
    return 0;
  std::string sDir (""), sFileName ("");
  if (!FilesUtility::isDirectory (sPath.c_str ()))
    FilesUtility::splitPath (sPath, sDir, sFileName);
  else
    sDir = sPath;

  string user;
  string password;
  if (strcmpi (suser.c_str (), "anonymous") == 0)
    {
      user.assign ("Guest");
      password.assign ("");
    }
  else
    {
      user.assign (suser);
      password.assign (sPass);
    }

  td.st.setUser (user);
  td.st.setPassword (password);

  td.st.setDirectory (&sDir);
  td.st.setSysDirectory ((string *) & (td.pConnection->host->getSystemRoot ()));
  td.st.setResource (&sFileName);

  AuthDomain auth (&td.st);
  string validator (td.st.getData ("sec.validator", MYSERVER_VHOST_CONF
                                   | MYSERVER_SERVER_CONF, "xml"));
  string authMethod (td.st.getData ("sec.auth_method", MYSERVER_VHOST_CONF
                                    | MYSERVER_SERVER_CONF, "xml"));

  SecurityDomain *domains[] = { &auth, NULL };

  Server::getInstance ()->getSecurityManager ()->getPermissionMask (&td.st,
                                                                    domains,
                                                                    validator,
                                                                    authMethod);

  return (td.st.getMask () & mask);
}

void Ftp::size (const std::string & sPath)
{
  waitDataConnection ();
  std::string sLocalPath;

  if (!userLoggedIn () || !getLocalPath (sPath, sLocalPath))
    return;

  std::string sLocalDir, sLocalFileName;
  FilesUtility::splitPath (sLocalPath, sLocalDir, sLocalFileName);

  /* The security file doesn't exist in any case.  */
  const char *secName = td.st.getData ("security.filename",
                                       MYSERVER_VHOST_CONF |
                                       MYSERVER_SERVER_CONF,
                                       ".security.xml");

  if (!strcmpi (sLocalFileName.c_str (), secName))
    {
      ftpReply (550);
      closeDataConnection ();
      return;
    }

  if (sPath.empty () || !getLocalPath (sPath, sLocalPath))
    {
      ftpReply (550);
      return;
    }

  if (FilesUtility::isDirectory (sLocalPath.c_str ()))
    {
      ftpReply (550);
      return;
    }

  File f;
  if (f.openFile (sLocalPath.c_str (), File::OPEN_IF_EXISTS | File::READ))
    {
      ftpReply (550);
      return;
    }

  char size[12];
  sprintf (size, "%l", f.getFileSize ());
  f.close ();

  ftpReply (213, size);
}

void Ftp::allo (int nSize, int nRecordSize /* = -1 */ )
{
  /* TODO.  */
  noop ();
}

void Ftp::waitDataConnection ()
{
  FtpuserData *pFtpuserData =
    static_cast < FtpuserData * >(td.pConnection->protocolBuffer);

  pFtpuserData->m_DataConnBusy.lock ();
  pFtpuserData->m_DataConnBusy.unlock ();
}
