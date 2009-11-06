/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#ifndef FTP_H
# define FTP_H
# include "stdafx.h"
# include <include/protocol/protocol.h>
# include <include/connection/connection.h>
# include <include/base/mem_buff/mem_buff.h>
# include <include/base/xml/xml_parser.h>
# include <include/protocol/ftp/ftp_common.h>

# include <include/protocol/ftp/ftp_parser.h>
# include <include/protocol/ftp/ftp_lexer.h>

# include <include/conf/security/security_token.h>
class Ftp;

class FtpuserData:public ProtocolBuffer
{
public:
  FtpuserData ();
  ~FtpuserData ();
  void reset ();
  int closeDataConnection ();
  virtual bool allowdelete (bool);

  enum Ftpstate
    {
      NO_CONTROL_CONNECTION,
      BUISY,
      UNAVAILABLE,
      CONTROL_CONNECTION_UP,
      USER_LOGGED_IN,
      DATA_CONNECTION_UP
    } m_nFtpstate;

  std::string m_suserName, m_sPass;
  enum FtpRepresentation
    {
      REPR_ASCII,
      REPR_IMAGE
    } m_nFtpRepresentation;
  enum FtpFormatControl
    {
      NON_PRINT
    } m_nFtpFormatControl;
  enum FtpFilestructure
    {
      STRU_FILE
    } m_nFtpFilestructure;
  enum FtpTransfermode
    {
      MODE_STREAM
    } m_nTransfermode;
  ConnectionPtr m_pDataConnection;
  Mutex m_DataConnBusy;
  FtpHost m_cdh;
  std::string m_cwd;
  int m_nLocalDataport;
  bool m_bBreakDataConnection;
  ThreadID m_dataThreadId;
  bool m_bPassiveSrv;
  u_long m_nrestartOffset;
  std::string m_sRenameFrom;

  // STAT data
  std::string m_sCurrentFileName;
  u_long m_nFileSize, m_nBytesSent;
};

struct FtpThreadContext
{
  FtpThreadContext ();
  ConnectionPtr pConnection;
  MemBuf *buffer;
  MemBuf *secondaryBuffer;
  u_long buffersize;
  u_long m_nParseLength;
  u_long nBytesToRead;
  Ftp *pProtocolInterpreter;
  SecurityToken st;
};

class Ftp:public Protocol
{
public:
  Ftp ();
  virtual ~ Ftp ();
  virtual int controlConnection (ConnectionPtr con, char *request, char *auxBuf,
                                 u_long reqBufLen, u_long auxBufLen, u_long reqLen,
                                 u_long tid);

  static int loadProtocolstatic ();
  static int unLoadProtocolstatic ();

  int parseControlConnection ();
  yyscan_t getScanner ()
  {
    return m_scanner;
  }
  u_long computeParseLength (const YYLTYPE & location);

  void logAccess (int nReplyCode, const std::string & sCustomText);
  void ftpReply (int nReplyCode, const std::string & sCustomText = "");

  int printError (const char *msg);
  FtpThreadContext td;
  int closeDataConnection ();

  static int FIRST_PASV_PORT;
  static int LAST_PASV_PORT;

  int checkRights (const std::string & suser, const std::string & sPass,
		   const std::string & sPath, int mask);
  void waitDataConnection ();

  int OpenDataConnection ();
  int openDataPassive ();
  int openDataActive ();

  virtual const char *getName ();
  static const char *getNameImpl ();

  void escapeTelnet (MemBuf & In, MemBuf & Out);
  void user (const std::string & sParam);
  void password (const std::string & sParam);
  void port (const FtpHost & host);
  void pasv ();
  int type (int ntypeCode, int nFormatControlCode = -1);
  void retr (const std::string & sPath);
  void quit ();
  void help (const std::string & sCmd = "");
  void noop ();
  void stru (int nstructure);
  void mode (int nmode);
  void list (const std::string & sParam = "");
  void nlst (const std::string & sParam = "");
  void abor ();
  void cwd (const std::string & sPath);
  void pwd ();
  void rest (const std::string & srestPoint);
  void syst ();
  void statCmd (const std::string & sParam = "");
  void allo (int nSize, int nRecordSize = -1);

  void stor (const std::string & sPath);
  void stou (const std::string & sPath);
  void dele (const std::string & sPath);
  void appe (const std::string & sPath);
  void mkd (const std::string & sPath);
  void rmd (const std::string & sPath);
  void rnfr (const std::string & sPath);
  void Rnto (const std::string & sPath);

  /* RFC 3659 commands */
  void size (const std::string & sPath);


protected:
  yyscan_t m_scanner;
  bool userLoggedIn ();
  bool buildLocalPath (const std::string & sPath, std::string & sOutPath);
  bool getLocalPath (const std::string & sPath, std::string & sOutPath);
  void retrstor (bool bretr, bool bappend, const std::string & sPath);
  void removePipelinedCmds (MemBuf & In, MemBuf & Out);

  static bool m_ballowAnonymous, m_bAnonymousNeedPass,
    m_ballowAsynchronousCmds, m_bEnablePipelining, m_bEnablestoreCmds;

  int m_nLocalControlport;
  int m_nPassiveport;
};

class DataConnectionWorkerThreadData
{
public:
  DataConnectionWorkerThreadData ();
  ~DataConnectionWorkerThreadData ();
  ConnectionPtr m_pConnection;
  std::string m_sFilePath;
  bool m_bappend;
  Ftp *m_pFtp;
};

int getFtpReply (int nReplyCode, std::string & sReply);
void ftpReply (ConnectionPtr pConnection, int nReplyCode,
               const std::string & sCustomText = "");
void yyerror (YYLTYPE * pLoc, Ftp * pContext, const char *msg);



/*!
 *Adapter class to make Ftp reentrant.
 */
class FtpProtocol:public Protocol
{
public:
  FtpProtocol ()
  {
    protocolOptions = Protocol::FAST_CHECK;
  }

  virtual ~ FtpProtocol ()
  {

  }

  virtual const char *getName ()
  {
    return Ftp::getNameImpl ();
  }

  virtual int controlConnection (ConnectionPtr con, char *request,
                                 char *auxBuf, u_long reqBufLen,
                                 u_long auxBufLen, u_long reqLen,
                                 u_long tid)
  {
    Ftp ftp;
    int ret = 0;

    return ftp.controlConnection (con, request, auxBuf, reqBufLen, auxBufLen,
                                  reqLen, tid);
  }

  virtual int loadProtocol ()
  {
    return Ftp::loadProtocolstatic ();
  }

  virtual int unLoadProtocol ()
  {
    return Ftp::unLoadProtocolstatic ();

  }

  int getProtocolOptions ()
  {
    return protocolOptions;
  }

};


#endif
