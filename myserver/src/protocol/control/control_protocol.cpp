/*
MyServer
Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <include/protocol/protocol.h>
#include <include/protocol/control/control_protocol.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/md5/md5.h>
#include <include/server/server.h>
#include <include/base/find_data/find_data.h>
#include <include/base/string/securestr.h>
#include <include/protocol/control/control_errors.h>
#include <include/base/string/stringutils.h>
#include <include/base/file/files_utility.h>
#include <string.h>

extern "C"
{
#ifdef WIN32
# include <direct.h>
# include <errno.h>
#else
# include <string.h>
# include <errno.h>
#endif
}
#include <string>
#include <sstream>

using namespace std;

char ControlProtocol::adminLogin[64] = "";
char ControlProtocol::adminPassword[64] = "";
int  ControlProtocol::controlEnabled = 0;

struct ControlProtocolVisitorArg
{
  static const int SHOW_CONNECTIONS = 1;
  static const int KILL_CONNECTION = 2;
  int command;

  ConnectionPtr connection;
  File* out;
  char *b1;
  int bs1;
  ControlHeader* header;

  u_long id;
};

/*!
 *Returns the name of the protocol. If an out buffer is defined
 *fullfill it with the name too.
 */
char* ControlProtocol::registerName (char* out,int len)
{
  if (out)
    strncpy (out, "CONTROL", len);

  return (char*)"CONTROL";
}

/*!
 * Class constructor.
 */
ControlProtocol::ControlProtocol ()
{
  protocolOptions = PROTOCOL_USES_SSL;
  protocolPrefix.assign ("control://");
}

/*!
 * Destructor for the class.
 */
ControlProtocol::~ControlProtocol ()
{

}

/*!
 * Load the control protocol.
 */
int ControlProtocol::loadProtocol ()
{
  char tmpName[64];
  char tmpPassword[64];
  tmpName[0]='\0';
  tmpPassword[0]='\0';
  Md5 md5;

  /* Is the value in the config file still in MD5?  */
  int adminNameMD5ized = 0;

  /* Is the value in the config file still in MD5?  */
  int adminPasswordMD5ized = 0;

  const char *data = 0;

  data = Server::getInstance ()->getData ("control.enabled");
  if (data && (!strcmpi (data, "YES")))
    controlEnabled = 1;
  else
    controlEnabled = 0;

  data = Server::getInstance ()->getData ("control.admin");
  if (data)
    strncpy (tmpName, data, 64);

  data = Server::getInstance ()->getData ("control.password");
  if (data)
    strncpy (tmpPassword, data, 64);

  data = Server::getInstance ()->getData ("control.admin.md5");
  if (data)
    if (strcmpi (data, "YES") == 0)
      adminNameMD5ized = 1;

  data = Server::getInstance ()->getData ("control.password.md5");
  if (data)
    if (strcmpi (data, "YES") == 0)
      adminPasswordMD5ized = 1;

  if (adminNameMD5ized)
    strncpy (adminLogin, tmpName, 64);
  else
  {
    md5.init ();
    md5.update ((unsigned char const*)tmpName, (unsigned int)strlen (tmpName) );
    md5.end ( adminLogin);
  }

  if (adminPasswordMD5ized)
    strncpy (adminPassword, tmpPassword, 64);
  else
  {
    md5.init ();
    md5.update ((unsigned char const*)tmpPassword,(unsigned int)strlen (tmpPassword) );
    md5.end (adminPassword);
  }

  return 0;
}

/*!
 *Check if the client is allowed to connect to.
 *Return 1 if the client is allowed.
 */
int ControlProtocol::checkAuth (ControlHeader& header)
{
  char authLoginHeaderMD5[64];
  char authPasswordHeaderMD5[64];
  char *headerLogin;
  char *headerPassword;
  Md5 md5;
  /*! Return 0 if we haven't enabled the service. */
  if (!controlEnabled)
    return 0;
  headerLogin = header.getAuthLogin ();
  headerPassword = header.getAuthPassword ();
  authLoginHeaderMD5[0] = authPasswordHeaderMD5[0] = '\0';
  md5.init ();
  md5.update ((unsigned char const*) headerLogin, (unsigned int)strlen ( headerLogin ) );
  md5.end ( authLoginHeaderMD5);

  md5.init ();
  md5.update ((unsigned char const*) headerPassword,(unsigned int)strlen (headerPassword) );
  md5.end (authPasswordHeaderMD5);

  if ((!strcmpi (adminLogin, authLoginHeaderMD5)) &&
     (!strcmpi (adminPassword, authPasswordHeaderMD5)) )
    return 1;
  else
    return 0;
}

/*!
 *Control the connection.
 */
int ControlProtocol::controlConnection (ConnectionPtr a, char *b1, char *b2,
                                        int bs1, int bs2, u_long nbtr, u_long id)
{
  int ret;
  int realHeaderLength;
  int dataWritten = 0;
  ostringstream inFilePath;
  ostringstream outFilePath;
  u_long nbw;
  int specifiedLength;
  char *version = 0;
  u_long timeout;
  int authorized;
  char *command = 0;
  char *opt = 0 ;
  /* Input file. */
  File *inFile = 0;
  /* Output file. */
  File *outFile = 0;

  /* Use control_header to parse the request. */
  ControlHeader header;

  /* Is the specified command a know one? */
  int knownCommand;
  if (a->getToRemove ())
  {
    switch (a->getToRemove ())
    {
      /* Remove the connection from the list. */
    case Connection::REMOVE_OVERLOAD:
      sendResponse (b2, bs2, a, CONTROL_SERVER_BUSY, header, 0);
      return 0;
    default:
      return 0;
    }
  }

  ret = header.parse_header (b1, nbtr, &realHeaderLength);

  /*
   *On errors remove the connection from the connections list.
   *For return values look at protocol/control/control_errors.h.
   *Returning 0 from the controlConnection we will remove the connection
   *from the active connections list.
   */
  if (ret != CONTROL_OK)
  {
    /* parse_header returns -1 on an incomplete header.  */
    if (ret == -1)
    {
      return 2;
    }
    sendResponse (b2, bs2, a, ret, header, 0);
    return 0;
  }
  specifiedLength = header.getLength ();
  version = header.getVersion ();
  timeout=getTicks ();
  if (specifiedLength)
  {
    inFile = new File ();
    if (inFile == 0)
    {
      a->host->warningsLogWrite (_("Control: internal error"));
      return 0;
    }

    inFilePath << getdefaultwd (0,0) << "/ControlInput_" << (u_int) id;

    ret = inFile->createTemporaryFile (inFilePath.str ().c_str ());
    if (ret)
      {
        a->host->warningsLogWrite (_("Control: internal error"));
        sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
        inFile->close ();
        FilesUtility::deleteFile (inFilePath.str ().c_str ());
        delete inFile;
        return 0;
      }
    if (nbtr - realHeaderLength)
      {
        ret = inFile->writeToFile (b1 + realHeaderLength, nbtr - realHeaderLength,
                                   &nbw);
        dataWritten += nbw;
        if (ret)
          {
            a->host->warningsLogWrite (_("Control: internal error"));
            sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
            inFile->close ();
            FilesUtility::deleteFile (inFilePath.str ().c_str ());
            delete inFile;
            inFile=0;
            return 0;
          }
      }
  }

  /* Check if there are other bytes waiting to be read. */
  if (specifiedLength && (specifiedLength != static_cast<int>(nbtr - realHeaderLength) ))
    {
      /* Check if we can read all the specified data. */
      while (specifiedLength != static_cast<int>(nbtr - realHeaderLength))
        {
          if (a->socket->bytesToRead ())
            {
              ret = a->socket->recv (b2, bs2, 0);
              if (ret == -1)
                {
                  a->host->warningsLogWrite (_("Control: internal error"));
                  sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
                  inFile->close ();
                  FilesUtility::deleteFile (inFilePath.str ().c_str ());
                  delete inFile;
                  inFile=0;
                  return -1;
                }
              ret = inFile->writeToFile (b2, ret, &nbw);
              dataWritten += nbw;
              if (ret)
                {
                  a->host->warningsLogWrite (_("Control: internal error"));
                  sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
                  inFile->close ();
                  FilesUtility::deleteFile (inFilePath.str ().c_str ());
                  delete inFile;
                  inFile=0;
                }

              if (dataWritten >=  specifiedLength)
                break;
              timeout = getTicks ();
            }
          else if (getTicks () - timeout > MYSERVER_SEC (5))
            {
              sendResponse (b2, bs2, a, CONTROL_BAD_LEN, header, 0);
              inFile->close ();
              FilesUtility::deleteFile (inFilePath.str ().c_str ());
              delete inFile;
              inFile=0;
              return 0;
            }
          else
            {
              /* Wait a bit.  */
              Thread::wait (2);
            }
        }
    }
  if (inFile)
    {
      inFile->seek (0);
    }

  if (strcmpi (version, "CONTROL/1.0"))
    {
      a->host->warningsLogWrite (_("Control: specified version not supported"));
      if (inFile)
        {
          inFile->close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          delete inFile;
          inFile = 0;
        }
      sendResponse (b2, bs2, a, CONTROL_BAD_VERSION, header, 0);
      return 0;
    }

  authorized = checkAuth (header);

  /*
   *If the client is not authorized remove the connection.
   */
  if (authorized == 0)
    {
      if (inFile)
        {
          inFile->close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          delete inFile;
          inFile=0;
        }
      sendResponse (b2, bs2, a, CONTROL_AUTH, header, 0);
      return 0;
    }
  /*
   *If the specified length is different from the length that the
   *server can read, remove the connection.
   */
  if (dataWritten != specifiedLength)
    {
      if (inFile)
        {
          inFile->close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          delete inFile;
          inFile = 0;
        }
      sendResponse (b2, bs2, a, CONTROL_BAD_LEN, header, 0);
      return 0;
    }

  command = header.getCommand ();
  opt     = header.getOptions ();

  knownCommand = 0;

  /*
   *Create an out file. This can be used by commands that
   *needs it.
   */
  outFile = new File ();
  outFilePath << getdefaultwd (0, 0) << "/ControlOutput_" << (u_int) id;

  ret = outFile->createTemporaryFile (outFilePath.str ().c_str ());
  if (ret)
    {
      a->host->warningsLogWrite (_("Control: internal error"));
      sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
      delete outFile;
      inFile->close ();
      delete inFile;

      FilesUtility::deleteFile (inFilePath.str ().c_str ());
      FilesUtility::deleteFile (outFilePath.str ().c_str ());

      inFile=0;
      outFile=0;
      return 0;
    }
  if (!strcmp (command, "SHOWCONNECTIONS"))
    {
      knownCommand = 1;
      ret = showConnections (a, outFile, b1, bs1, header);
    }
  else if (!strcmp (command, "KILLCONNECTION"))
    {
      char buff[11];
      u_long id;
      knownCommand = 1;
      strncpy (buff, header.getOptions (), 10 );
      buff[10] = '\0';
      id = header.getOptions () ? atol (buff) : 0;
      ret = killConnection (a, id, outFile, b1, bs1, header);
    }
  else if (!strcmp (command, "REBOOT"))
    {
      knownCommand = 1;
      Server::getInstance ()->rebootOnNextLoop ();
      ret = 0;
    }
  else if (!strcmp (command, "GETFILE"))
    {
      knownCommand = 1;
      ret = getFile (a,header.getOptions (), inFile, outFile, b1, bs1, header);
    }
  else if (!strcmp (command, "PUTFILE"))
    {
      knownCommand = 1;
      ret = putFile (a,header.getOptions (), inFile, outFile, b1, bs1, header);
    }
  else if (!strcmp (command, "DISABLEREBOOT"))
    {
      Server::getInstance ()->disableAutoReboot ();
      knownCommand = 1;

    }
  else if (!strcmp (command, "ENABLEREBOOT"))
    {
      Server::getInstance ()->enableAutoReboot ();
      knownCommand = 1;

    }
  else if (!strcmp (command, "VERSION"))
    {
      knownCommand = 1;
      ret = getVersion (a, outFile, b1, bs1, header);
    }

  if (knownCommand)
    {
      char *connection;
      outFile->seek (0);
      if (ret)
        {
          sendResponse (b2, bs2, a, CONTROL_INTERNAL, header, 0);
        }
      else
        sendResponse (b2, bs2, a, CONTROL_OK, header, outFile);
      if (inFile)
        {
          inFile->close ();
          delete inFile;
          inFile=0;
        }
      if (outFile)
        {
          outFile->close ();
          delete outFile;
          outFile=0;
        }
      FilesUtility::deleteFile (inFilePath.str ().c_str ());
      FilesUtility::deleteFile (outFilePath.str ().c_str ());
      connection = header.getConnection ();
      /*
       *If the Keep-Alive was specified keep the connection in the
       *active connections list.
       */
      if (!strcmpi (connection,"keep-alive"))
        return 1;
      else
        return 0;
    }
  else
    {
      sendResponse (b2, bs2, a, CONTROL_CMD_NOT_FOUND, header, 0);

      if (inFile)
        {
          inFile->close ();
          delete inFile;
          inFile = NULL;
        }
      if (outFile)
        {
          outFile->close ();
          delete outFile;
          outFile = NULL;
        }
      FilesUtility::deleteFile (inFilePath.str ().c_str ());
      FilesUtility::deleteFile (outFilePath.str ().c_str ());
      return 0;
    }
}

/*!
 *Add the entry to the log file.
 */
int ControlProtocol::addToLog (int retCode, ConnectionPtr con, char *b1,
                               int bs1, ControlHeader& header)
{
  string time;
  getRFC822GMTTime (time, 32);

#ifdef HAVE_SNPRINTF
  snprintf (b1, bs1,
#else
  sprintf (b1,
#endif
  "%s [%s] %s:%s:%s - %s - %i", con->getIpAddr (), time.c_str (),
          header.getCommand (),  header.getVersion (), header.getOptions (),
          header.getAuthLogin (), retCode);
  con->host->accessesLogWrite ("%s", b1);
  return 0;
}

/*!
 *Send the response with status=errID and the data contained in the outFile.
 *Return nonzero on errors.
 */
int ControlProtocol::sendResponse (char *buffer, int buffersize,
                                   ConnectionPtr a, int errID,
                                   ControlHeader& header, File* outFile)
{
  u_long dataLength = 0;
  int err;
  err = addToLog (errID, a, buffer, buffersize, header);
  if (err)
    return err;

  if (outFile)
    dataLength = outFile->getFileSize ();
  /* Build and send the first line.  */
#ifdef HAVE_SNPRINTF
  snprintf (buffer, buffersize,
#else
  sprintf (buffer,
#endif
          "/%i\r\n", errID);

  err = a->socket->send (buffer, strlen (buffer), 0);
  if (err == -1)
  {
    a->host->warningsLogWrite (_("Control: socket error"));
    return -1;
  }

  /* Build and send the Length line.  */
#ifdef HAVE_SNPRINTF
  snprintf (buffer, buffersize,
#else
  sprintf (buffer,
#endif
          "/LEN %u\r\n", (u_int)dataLength);

  err = a->socket->send (buffer, strlen (buffer), 0);
           if (err == -1)
  {
    a->host->warningsLogWrite (_("Control: socket error"));
    return -1;
  }

  /* Send the end of the header.  */
  err = a->socket->send ("\r\n", 2, 0);
  if (err == -1)
  {
    a->host->warningsLogWrite (_("Control: socket error"));
    return -1;
  }

  /* Flush the content of the file if any.  */
  if (dataLength)
  {
    int dataToSend = dataLength;
    u_long nbr;
    for ( ; ; )
    {
      err = outFile->read (buffer, min (dataToSend, buffersize), &nbr);
      if (err)
      {
        a->host->warningsLogWrite (_("Control: internal error"));
        return -1;
      }
      dataToSend -= nbr;
      err = a->socket->send (buffer, nbr, 0);
      if (dataToSend == 0)
        break;
      if (err == -1)
      {
        a->host->warningsLogWrite (_("Control: socket error"));
        return -1;
      }
    }
  }

  return 0;
}

/*!
 *Show the currect active connections.
 */
int  ControlProtocol::showConnections (ConnectionPtr a,File* out, char *b1,
                                      int bs1, ControlHeader& header)
{
  ControlProtocolVisitorArg args;
  args.command = ControlProtocolVisitorArg::SHOW_CONNECTIONS;
  args.connection = a;
  args.out = out;
  args.b1 = b1;
  args.bs1 = bs1;
  args.header = &header;

  return Server::getInstance ()->getConnectionsScheduler ()->accept (this, &args);
}

/*!
 *Kill a connection by its ID.
 */
int ControlProtocol::killConnection (ConnectionPtr a, u_long id, File* out,
                                    char *b1, int bs1, ControlHeader& header)
{
  if (id == 0)
    return -1;

  ControlProtocolVisitorArg args;
  args.command = ControlProtocolVisitorArg::KILL_CONNECTION;
  args.connection = a;
  args.id = id;
  args.out = out;
  args.b1 = b1;
  args.bs1 = bs1;
  args.header = &header;

  Server::getInstance ()->getConnectionsScheduler ()->accept (this, &args);

  return 0;
}

/*!
 *Visitor.
 */
int ControlProtocol::visitConnection (ConnectionPtr con, void* argP)
{
  int ret = 0;
  u_long nbw;
  ControlProtocolVisitorArg* arg = static_cast<ControlProtocolVisitorArg*>(argP);

  if (arg->command == ControlProtocolVisitorArg::SHOW_CONNECTIONS)
  {
#ifdef HAVE_SNPRINTF
    snprintf (arg->b1, arg->bs1, "%i - %s - %i - %s - %i - %s - %s\r\n",
             static_cast<int>(con->getID ()),  con->getIpAddr (),
             static_cast<int>(con->getPort ()),
             con->getLocalIpAddr (),  static_cast<int>(con->getLocalPort ()),
             con->getLogin (), con->getPassword ());
#else
    sprintf (arg->b1, "%i - %s - %i - %s - %i - %s - %s\r\n",
            static_cast<int>(con->getID ()),  con->getIpAddr (),
            static_cast<int>(con->getPort ()),
            con->getLocalIpAddr (),  static_cast<int>(con->getLocalPort ()),
            con->getLogin (), con->getPassword ());
#endif

    ret = arg->out->writeToFile (arg->b1, strlen (arg->b1), &nbw);

    if (ret)
      {
        con->host->warningsLogWrite (_("Control: internal error"));
        return ret;
      }
  }
  else if (arg->command == ControlProtocolVisitorArg::KILL_CONNECTION)
    {
      if (con->getID () == arg->id)
        {
          /* Define why the connection is killed.  */
          con->setToRemove (Connection::USER_KILL);
          return 1;
        }
    }

  return 0;
}



/*!
 * Return the requested file to the client.
 */
int ControlProtocol::getFile (ConnectionPtr a, char* fn, File* in, File* out,
                              char *b1,int bs1, ControlHeader& header)
{
  const char *filename = 0;
  File localfile;
  int ret = 0;

  /* # of bytes read.  */
  u_long nbr = 0;

  /* # of bytes written.  */
  u_long nbw = 0;

  filename = fn;

  ret = localfile.openFile (filename, File::READ | File::OPEN_IF_EXISTS);

  /* An internal server error happens.  */
  if (ret)
    {
    a->host->warningsLogWrite (_("Control: internal error"));
    return CONTROL_INTERNAL;
    }
  for (;;)
    {
    ret = localfile.read (b1, bs1, &nbr);
    if (ret)
      {
        a->host->warningsLogWrite (_("Control: internal error"));
        return CONTROL_INTERNAL;
      }

    /* Break the loop when we can't read no more data.  */
    if (!nbr)
      break;

    ret = out->writeToFile (b1, nbr, &nbw);

    if (ret)
      {
        a->host->warningsLogWrite (_("Control: internal error"));
        return CONTROL_INTERNAL;
      }

    }

  localfile.close ();
  return 0;
}

/*!
 * Save the file on the local FS.
 */
int ControlProtocol::putFile (ConnectionPtr a, char* fn, File* in,
                              File* out, char *b1,int bs1, ControlHeader& header)
{
  const char *filename = 0;
  File localfile;
  int isAutoRebootToEnable = Server::getInstance ()->isAutorebootEnabled ();
  int ret = 0;
  /*! # of bytes read.  */
  u_long nbr = 0;
  /*! # of bytes written.  */
  u_long nbw = 0;
  Server::getInstance ()->disableAutoReboot ();

  filename = fn;

  /* Remove the file before create it.  */
  ret = FilesUtility::deleteFile (filename);

  /* An internal server error happens.  */
  if (ret)
    {
      a->host->warningsLogWrite (_("Control: internal error"));
      return CONTROL_INTERNAL;
    }

  ret = localfile.openFile (filename, File::WRITE | File::FILE_OPEN_ALWAYS);

  /* An internal server error happens.  */
  if (ret)
    {
      a->host->warningsLogWrite (_("Control: internal error"));
      return CONTROL_INTERNAL;
    }

  if (in == NULL)
    {
      a->host->warningsLogWrite (_("Control: internal error"));
      return CONTROL_INTERNAL;
    }

  for (;;)
    {
      ret = in->read (b1, bs1, &nbr);

      if (ret)
        {
          a->host->warningsLogWrite (_("Control: internal error"));
          return CONTROL_INTERNAL;
        }

      /* Break the loop when we can't read no more data.  */
      if (!nbr)
        break;

      ret = localfile.writeToFile (b1, nbr, &nbw);

      if (ret)
        {
          a->host->warningsLogWrite (_("Control: internal error"));
          return CONTROL_INTERNAL;
        }
    }
  localfile.close ();

  return 0;
}

/*!
 *Return the current MyServer version.
 */
int ControlProtocol::getVersion (ConnectionPtr a, File* out, char *b1, int bs1,
                                 ControlHeader& header)
{
  u_long nbw;

  myserver_strlcpy (b1, MYSERVER_VERSION, bs1);

  return out->writeToFile (b1, strlen (b1), &nbw);
}
