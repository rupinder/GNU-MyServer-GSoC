/* -*- mode: c++ -*- */
/*
MyServer
Copyright (C) 2008 Free Software Foundation, Inc.
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
#define FTP_H
#include "stdafx.h"
#include <include/protocol/protocol.h>
#include <include/connection/connection.h>
#include <include/base/mem_buff/mem_buff.h>
#include <include/base/xml/xml_parser.h>
#include <include/protocol/ftp/ftp_common.h>

#include <include/protocol/ftp/ftp_parser.h>
#include <include/protocol/ftp/ftp_lexer.h>

class Ftp;

class FtpUserData : public ProtocolBuffer
{
public:
	FtpUserData();
	~FtpUserData();
	void reset();
	int CloseDataConnection();
  virtual bool allowDelete(bool);

	enum FtpState
	{
		NO_CONTROL_CONNECTION,
		BUISY,
		UNAVAILABLE,
		CONTROL_CONNECTION_UP,
		USER_LOGGED_IN,
		DATA_CONNECTION_UP
	} m_nFtpState;

	std::string m_sUserName, m_sPass;
	enum FtpRepresentation
	{
		REPR_ASCII,
		REPR_IMAGE
	} m_nFtpRepresentation;
	enum FtpFormatControl
	{
		NON_PRINT
	} m_nFtpFormatControl;
	enum FtpFileStructure
	{
		STRU_FILE
	} m_nFtpFileStructure;
	enum FtpTransferMode
	{
		MODE_STREAM
	} m_nTransferMode;
	ConnectionPtr m_pDataConnection;
	Mutex m_DataConnBusy;
	FtpHost m_cdh;
	std::string m_cwd;
	int m_nLocalDataPort;
	bool m_bBreakDataConnection;
	ThreadID m_dataThreadId;
	bool m_bPassiveSrv;
	u_long m_nRestartOffset;
	std::string m_sRenameFrom;

	// STAT data
	std::string m_sCurrentFileName;
	u_long m_nFileSize, m_nBytesSent;
};

struct FtpThreadContext
{
	FtpThreadContext();
	ConnectionPtr pConnection;
	MemBuf *buffer;
	MemBuf *buffer2;
	u_long buffersize;
	u_long buffersize2;
	u_long m_nParseLength;
	u_long nBytesToRead;
	Ftp *pProtocolInterpreter;
};

class Ftp : public Protocol
{
public:
	Ftp();
	virtual ~Ftp();
	virtual int controlConnection(ConnectionPtr pConnection, char *b1, char *b2,
			int bs1, int bs2, u_long nbtr, u_long id);
	static int loadProtocolStatic(XmlParser*);
	static int unLoadProtocolStatic(XmlParser*);
// Ftp helpers
	int ParseControlConnection();
	yyscan_t GetScanner() { return m_scanner; }
	u_long ComputeParseLength(const YYLTYPE &location);

	void ftp_reply(int nReplyCode, const std::string &sCustomText = "");
	//int get_ftp_reply(int nReplyCode, std::string &sReply);
	int PrintError(const char *msg);//TODO: change this fnc !!!
	FtpThreadContext td;
	int CloseDataConnection();

	static int FIRST_PASV_PORT;
	static int LAST_PASV_PORT;

	int CheckRights(const std::string &sUser, const std::string &sPass, const std::string &sPath, int mask);
	void WaitDataConnection();
	
	int OpenDataConnection();
	int OpenDataPassive();
	int OpenDataActive();

  virtual char* registerName(char* out, int len);
  static char* registerNameImpl(char* out, int len);

protected:
	yyscan_t	m_scanner;
	bool UserLoggedIn();
	bool BuildLocalPath(const std::string &sPath, std::string &sOutPath);
	bool GetLocalPath(const std::string &sPath, std::string &sOutPath);
	void RetrStor(bool bRetr, bool bAppend, const std::string &sPath);
	void RemovePipelinedCmds(MemBuf &In, MemBuf &Out);

public:
	void EscapeTelnet(MemBuf &In, MemBuf &Out);

// Ftp commands Handlers
public:
	void User(const std::string &sParam);
	void Password(const std::string &sParam);
	void Port(const FtpHost &host);
	void Pasv();
	int Type(int nTypeCode, int nFormatControlCode = -1);
	void Retr(const std::string &sPath);
	void Quit();
	void Help(const std::string &sCmd = "");
	void Noop();
	void Stru(int nStructure);
	void Mode(int nMode);
	void List(const std::string &sParam = "");
	void Nlst(const std::string &sParam = "");
	void Abor();
	void Cwd(const std::string &sPath);
	void Pwd();
	void Rest(const std::string &sRestPoint);
	void Syst();
	void Stat(const std::string &sParam = "");
	void Allo(int nSize, int nRecordSize = -1);

	void Stor(const std::string &sPath);
	void Stou(const std::string &sPath);
	void Dele(const std::string &sPath);
	void Appe(const std::string &sPath);
	void Mkd(const std::string &sPath);
	void Rmd(const std::string &sPath);
	void Rnfr(const std::string &sPath);
	void Rnto(const std::string &sPath);

	/* RFC 3659 commands*/
	void Size(const std::string &sPath);

	static bool m_bAllowAnonymous, m_bAnonymousNeedPass, m_bAllowAsynchronousCmds, m_bEnablePipelining, m_bEnableStoreCmds;
	int m_nLocalControlPort;
	int m_nPassivePort;
};

class DataConnectionWorkerThreadData
{
public:
	DataConnectionWorkerThreadData();
	~DataConnectionWorkerThreadData();
	ConnectionPtr m_pConnection;
	std::string m_sFilePath;
	bool m_bAppend;
	Ftp *m_pFtp;
};

int get_ftp_reply(int nReplyCode, std::string &sReply);
void ftp_reply(ConnectionPtr pConnection, int nReplyCode, const std::string &sCustomText = "");

#ifdef WIN32
unsigned int __stdcall SendAsciiFile(void* pParam);
unsigned int __stdcall SendImageFile(void* pParam);
unsigned int __stdcall ReceiveAsciiFile(void* pParam);
unsigned int __stdcall ReceiveImageFile(void* pParam);
#elif HAVE_PTHREAD
void* SendAsciiFile(void* pParam);
void* SendImageFile(void* pParam);
void* ReceiveAsciiFile(void* pParam);
void* ReceiveImageFile(void* pParam);
#endif //HAVE_PTHREAD

void yyerror(YYLTYPE *pLoc, Ftp *pContext, const char *msg);



/*!
 *Adapter class to make Ftp reentrant.
 */
class FtpProtocol : public Protocol
{
public:
	FtpProtocol()
  {
    protocolOptions = PROTOCOL_FAST_CHECK;
  }

  virtual ~FtpProtocol()
  {

  }

  virtual char* registerName(char* out, int len)
  {
    return Ftp::registerNameImpl(out, len);
  }

	virtual int controlConnection(ConnectionPtr a, char *b1, char *b2,
                                int bs1, int bs2, u_long nbtr, u_long id)
  {
    Ftp ftp;
    return ftp.controlConnection(a, b1, b2, bs1, bs2, nbtr, id);
  }

  virtual int loadProtocol(XmlParser* parser)
  {
    return Ftp::loadProtocolStatic(parser);
  }
  
	virtual int unLoadProtocol(XmlParser* parser)
  {
    return Ftp::unLoadProtocolStatic(parser);

  }

  int getProtocolOptions()
  {
    return protocolOptions;
  }

};


#endif // FTP_H
