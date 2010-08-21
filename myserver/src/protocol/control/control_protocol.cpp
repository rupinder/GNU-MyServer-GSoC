/*
  MyServer
  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free Software
  Foundation, Inc.
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

#include "myserver.h"

#include <include/protocol/protocol.h>
#include <include/protocol/control/control_protocol.h>
#include <include/base/xml/xml_parser.h>
#include <include/base/crypt/md5.h>
#include <include/server/server.h>
#include <include/server/clients_thread.h>
#include <include/base/string/securestr.h>
#include <include/protocol/control/control_errors.h>
#include <include/base/string/stringutils.h>
#include <include/base/file/files_utility.h>
#include <string.h>

#include <stdio.h>

#ifdef WIN32
# include <direct.h>
# include <errno.h>
#else
# include <string.h>
# include <errno.h>
#endif

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
  File *out;
  char *buffer;
  int bufferSize;
  ControlHeader *header;
  u_long id;
};

/*!
  Returns the name of the protocol. If an out buffer is defined
  fullfill it with the name too.
 */
const char* ControlProtocol::getName ()
{
  return "CONTROL";
}

/*!
  Class constructor.
 */
ControlProtocol::ControlProtocol ()
{
  protocolOptions = Protocol::SSL;
  protocolPrefix.assign ("control://");
}

/*!
  Destructor for the class.
 */
ControlProtocol::~ControlProtocol ()
{

}

/*!
  Load the control protocol.
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
  if (data && (! strcasecmp (data, "YES")))
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
    if (strcasecmp (data, "YES") == 0)
      adminNameMD5ized = 1;

  data = Server::getInstance ()->getData ("control.password.md5");
  if (data)
    if (strcasecmp (data, "YES") == 0)
      adminPasswordMD5ized = 1;

  if (adminNameMD5ized)
    strncpy (adminLogin, tmpName, 64);
  else
  {
    md5.init ();
    md5.update (tmpName, (unsigned int) strlen (tmpName));
    md5.end ( adminLogin);
  }

  if (adminPasswordMD5ized)
    strncpy (adminPassword, tmpPassword, 64);
  else
  {
    md5.init ();
    md5.update (tmpPassword,(unsigned int) strlen (tmpPassword));
    md5.end (adminPassword);
  }

  return 0;
}

/*!
  Check if the client is allowed to connect to.
  Return 1 if the client is allowed.
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
  md5.update (headerLogin, (unsigned int) strlen (headerLogin));
  md5.end (authLoginHeaderMD5);

  md5.init ();
  md5.update (headerPassword, (unsigned int) strlen (headerPassword));
  md5.end (authPasswordHeaderMD5);

  if ((! strcasecmp (adminLogin, authLoginHeaderMD5)) &&
     (! strcasecmp (adminPassword, authPasswordHeaderMD5)))
    return 1;
  else
    return 0;
}

/*!
  Control the connection.
 */
int ControlProtocol::controlConnection (ConnectionPtr a, char *request,
                                        char *auxBuffer, u_long, u_long,
                                        u_long nbtr, u_long id)
{
  int ret;
  int realHeaderLength;
  int dataWritten = 0;
  ostringstream inFilePath;
  ostringstream outFilePath;
  size_t nbw;
  int specifiedLength;
  char *version = 0;
  u_long timeout;
  int authorized;
  char *command = 0;
  char *opt = 0 ;
  int bufferSize = a->getActiveThread ()->getBufferSize ();
  File inFile;
  File outFile;

  /* Use control_header to parse the request. */
  ControlHeader header;

  /* Is the specified command a know one? */
  int knownCommand;

  try
    {
      if (a->getToRemove ())
        {
          switch (a->getToRemove ())
            {
              /* Remove the connection from the list. */
            case Connection::REMOVE_OVERLOAD:
              sendResponse (auxBuffer, bufferSize, a, CONTROL_SERVER_BUSY,
                            header, 0);
              return 0;
            default:
              return 0;
            }
        }

      /*
        On errors remove the connection from the connections list.
        For return values look at protocol/control/control_errors.h.
        Returning 0 from the controlConnection we will remove the connection
        from the active connections list.
       */
      ret = header.parse_header (request, nbtr, &realHeaderLength);
      if (ret != CONTROL_OK)
        {
          /* parse_header returns -1 on an incomplete header.  */
          if (ret == -1)
            return 2;

          sendResponse (auxBuffer, bufferSize, a, ret, header, 0);
          return 0;
        }

      specifiedLength = header.getLength ();
      version = header.getVersion ();
      timeout = getTicks ();
      if (specifiedLength)
        {
          inFilePath << getdefaultwd (0,0) << "/ControlInput_" << (u_int) id;

          inFile.createTemporaryFile (inFilePath.str ().c_str ());

          if (nbtr - realHeaderLength)
            {
              inFile.writeToFile (request + realHeaderLength,
                                  nbtr - realHeaderLength, &nbw);
              dataWritten += nbw;
            }
        }

      /* Check if there are other bytes waiting to be read. */
      if (specifiedLength && (specifiedLength
                              != static_cast<int>(nbtr - realHeaderLength)))
        {
          /* Check if we can read all the specified data. */
          while (specifiedLength != static_cast<int>(nbtr - realHeaderLength))
            {
              if (a->socket->bytesToRead ())
                {
                  u_long ret;
                  ret = a->socket->recv (auxBuffer, bufferSize, 0);
                  inFile.writeToFile (auxBuffer, ret, &nbw);
                  dataWritten += nbw;
                  if (dataWritten >=  specifiedLength)
                    break;
                  timeout = getTicks ();
                }
              else if (getTicks () - timeout > MYSERVER_SEC (5))
                {
                  sendResponse (auxBuffer, bufferSize, a, CONTROL_BAD_LEN, header,
                                0);
                  inFile.close ();
                  FilesUtility::deleteFile (inFilePath.str ().c_str ());
                  return 0;
                }
              else
                Thread::wait (2);
            }
        }

      inFile.seek (0);

      if (strcasecmp (version, "CONTROL/1.0"))
        {
          a->host->warningsLogWrite (_("Control: specified version not supported"));
          inFile.close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          sendResponse (auxBuffer, bufferSize, a, CONTROL_BAD_VERSION, header, 0);
          return 0;
        }

      authorized = checkAuth (header);

      /*
        If the client is not authorized remove the connection.
       */
      if (authorized == 0)
        {
          inFile.close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          sendResponse (auxBuffer, bufferSize, a, CONTROL_AUTH, header, 0);
          return 0;
        }
      /*
        If the specified length is different from the length that the
        server can read, remove the connection.
       */
      if (dataWritten != specifiedLength)
        {
          inFile.close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          sendResponse (auxBuffer, bufferSize, a, CONTROL_BAD_LEN, header, 0);
          return 0;
        }

      command = header.getCommand ();
      opt = header.getOptions ();

      knownCommand = 0;

      /* Create an out file. This can be used by commands needing it. */
      outFilePath << getdefaultwd (0, 0) << "/ControlOutput_" << (u_int) id;

      outFile.createTemporaryFile (outFilePath.str ().c_str ());

      if (!strcmp (command, "SHOWCONNECTIONS"))
        {
          knownCommand = 1;
          ret = showConnections (a, &outFile, request, bufferSize, header);
        }
      else if (!strcmp (command, "KILLCONNECTION"))
        {
          char buff[11];
          u_long id;
          knownCommand = 1;
          strncpy (buff, header.getOptions (), 10 );
          buff[10] = '\0';
          id = header.getOptions () ? atol (buff) : 0;
          ret = killConnection (a, id, &outFile, request, bufferSize, header);
        }
      else if (!strcmp (command, "REBOOT"))
        {
          knownCommand = 1;
          Server::getInstance ()->delayedReboot ();
          ret = 0;
        }
      else if (!strcmp (command, "GETFILE"))
        {
          knownCommand = 1;
          ret = getFile (a, header.getOptions (), &inFile, &outFile, request,
                         bufferSize, header);
        }
      else if (!strcmp (command, "PUTFILE"))
        {
          knownCommand = 1;
          ret = putFile (a,header.getOptions (), &inFile, &outFile, request,
                         bufferSize, header);
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
          ret = getVersion (a, &outFile, request, bufferSize, header);
        }

      if (knownCommand)
        {
          char *connection;
          outFile.seek (0);
          if (ret)
            sendResponse (auxBuffer, bufferSize, a, CONTROL_INTERNAL, header, 0);
          else
            sendResponse (auxBuffer, bufferSize, a, CONTROL_OK, header, &outFile);

          inFile.close ();
          outFile.close ();

          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          FilesUtility::deleteFile (outFilePath.str ().c_str ());
          connection = header.getConnection ();

          /*
            If the Keep-Alive was specified keep the connection in the
            active connections list.
           */
          if (! strcasecmp (connection, "keep-alive"))
            return ClientsThread::KEEP_CONNECTION;
          else
            return ClientsThread::DELETE_CONNECTION;
        }
      else
        {
          sendResponse (auxBuffer, bufferSize, a, CONTROL_CMD_NOT_FOUND,
                        header, 0);

          inFile.close ();
          outFile.close ();
          FilesUtility::deleteFile (inFilePath.str ().c_str ());
          FilesUtility::deleteFile (outFilePath.str ().c_str ());

          return ClientsThread::DELETE_CONNECTION;
        }
    }
  catch (exception & e)
    {
      inFile.close ();
      outFile.close ();
      FilesUtility::deleteFile (inFilePath.str ().c_str ());
      FilesUtility::deleteFile (outFilePath.str ().c_str ());
      a->host->warningsLogWrite (_E ("Control: internal error"), &e);
      return ClientsThread::DELETE_CONNECTION;
    }

  return ClientsThread::DELETE_CONNECTION;
}

/*!
  Add the entry to the log file.
 */
int ControlProtocol::addToLog (int retCode, ConnectionPtr con, char *buffer,
                               int bufferSize, ControlHeader &header)
{
  string time;
  getRFC822GMTTime (time, 32);
  gnulib::snprintf (buffer, bufferSize, "%s [%s] %s:%s:%s - %s - %i",
                    con->getIpAddr (), time.c_str (), header.getCommand (),
                    header.getVersion (), header.getOptions (),
                    header.getAuthLogin (), retCode);
  con->host->accessesLogWrite ("%s", buffer);
  return 0;
}

/*!
  Send the response with status=errID and the data contained in the outFile.
  Return nonzero on errors.
 */
int ControlProtocol::sendResponse (char *buffer, int buffersize,
                                   ConnectionPtr a, int errID,
                                   ControlHeader &header, File *outFile)
{
  u_long dataLength = 0;

  if (addToLog (errID, a, buffer, buffersize, header))
    return -1;

  if (outFile)
    dataLength = outFile->getFileSize ();

  /* Build and send the first line.  */
  gnulib::snprintf (buffer, buffersize, "/%i\r\n", errID);
  a->socket->send (buffer, strlen (buffer), 0);

  gnulib::snprintf (buffer, buffersize, "/LEN %u\r\n", (u_int) dataLength);
  a->socket->send (buffer, strlen (buffer), 0);

  /* Send the end of the header.  */
  a->socket->send ("\r\n", 2, 0);

  /* Flush the content of the file if any.  */
  if (dataLength)
    {
      int dataToSend = dataLength;
      size_t nbr;
      for ( ; ; )
        {
          outFile->read (buffer, min (dataToSend, buffersize), &nbr);

          dataToSend -= nbr;
          a->socket->send (buffer, nbr, 0);

          if (dataToSend == 0)
            break;
        }
    }

  return 0;
}

/*!
  Show the currect active connections.
 */
int  ControlProtocol::showConnections (ConnectionPtr a,File *out, char *buffer,
                                      int bufferSize, ControlHeader &header)
{
  ControlProtocolVisitorArg args;
  args.command = ControlProtocolVisitorArg::SHOW_CONNECTIONS;
  args.connection = a;
  args.out = out;
  args.buffer = buffer;
  args.bufferSize = bufferSize;
  args.header = &header;

  return Server::getInstance ()->getConnectionsScheduler ()->accept (this, &args);
}

/*!
  Kill a connection by its ID.
 */
int ControlProtocol::killConnection (ConnectionPtr a, u_long id, File *out,
                                    char *buffer, int bufferSize,
                                     ControlHeader &header)
{
  if (id == 0)
    return -1;

  ControlProtocolVisitorArg args;
  args.command = ControlProtocolVisitorArg::KILL_CONNECTION;
  args.connection = a;
  args.id = id;
  args.out = out;
  args.buffer = buffer;
  args.bufferSize = bufferSize;
  args.header = &header;

  Server::getInstance ()->getConnectionsScheduler ()->accept (this, &args);

  return 0;
}

/*!
  Visitor.
 */
int ControlProtocol::visitConnection (ConnectionPtr con, void *argP)
{
  int ret = 0;
  size_t nbw;
  ControlProtocolVisitorArg* arg = static_cast<ControlProtocolVisitorArg*>(argP);

  if (arg->command == ControlProtocolVisitorArg::SHOW_CONNECTIONS)
  {
    gnulib::snprintf (arg->buffer, arg->bufferSize, "%i - %s - %i - %s - %i - %s - %s\r\n",
              static_cast<int>(con->getID ()),  con->getIpAddr (),
              static_cast<int>(con->getPort ()),
              con->getLocalIpAddr (),  static_cast<int>(con->getLocalPort ()),
              con->getLogin (), con->getPassword ());

    ret = arg->out->writeToFile (arg->buffer, strlen (arg->buffer), &nbw);

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
  Return the requested file to the client.
 */
int ControlProtocol::getFile (ConnectionPtr a, char *fn, File *in, File *out,
                              char *buffer, int bufferSize,
                              ControlHeader &header)
{
  const char *filename = 0;
  File localfile;
  int ret = 0;

  /* # of bytes read.  */
  size_t nbr = 0;

  /* # of bytes written.  */
  size_t nbw = 0;

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
    ret = localfile.read (buffer, bufferSize, &nbr);
    if (ret)
      {
        a->host->warningsLogWrite (_("Control: internal error"));
        return CONTROL_INTERNAL;
      }

    /* Break the loop when we can't read no more data.  */
    if (!nbr)
      break;

    ret = out->writeToFile (buffer, nbr, &nbw);

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
  Save the file on the local FS.
 */
int ControlProtocol::putFile (ConnectionPtr a, char *fn, File *in,
                              File *out, char *buffer, int bufferSize,
                              ControlHeader &header)
{
  const char *filename = 0;
  File localfile;
  int ret = 0;
  /*! # of bytes read.  */
  size_t nbr = 0;
  /*! # of bytes written.  */
  size_t nbw = 0;
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
      ret = in->read (buffer, bufferSize, &nbr);

      if (ret)
        {
          a->host->warningsLogWrite (_("Control: internal error"));
          return CONTROL_INTERNAL;
        }

      /* Break the loop when we can't read no more data.  */
      if (!nbr)
        break;

      ret = localfile.writeToFile (buffer, nbr, &nbw);

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
  Return the current MyServer version.
 */
int ControlProtocol::getVersion (ConnectionPtr a, File *out, char *buffer,
                                 int bufferSize, ControlHeader &header)
{
  size_t nbw;
  myserver_strlcpy (buffer, MYSERVER_VERSION, bufferSize);
  return out->writeToFile (buffer, strlen (buffer), &nbw);
}
