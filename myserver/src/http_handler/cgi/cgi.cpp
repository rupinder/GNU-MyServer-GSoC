/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010 Free
  Software Foundation, Inc.
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
#include <include/http_handler/cgi/cgi.h>
#include <include/protocol/http/http_headers.h>
#include <include/protocol/http/http.h>
#include <include/protocol/http/http_errors.h>
#include <include/server/server.h>
#include <include/base/base64/mime_utils.h>
#include <include/base/file/file.h>
#include <include/base/file/files_utility.h>
#include <include/base/socket/socket.h>
#include <include/base/utility.h>
#include <include/base/mem_buff/mem_buff.h>
#include <include/filter/filters_chain.h>
#include <include/protocol/http/env/env.h>
#include <include/base/pipe/pipe.h>
#include <include/protocol/http/http_data_handler.h>

#include <string>
#include <sstream>

#ifdef WIN32
# include <direct.h>
#endif
#include <string.h>


using namespace std;


/*!
  Run the standard CGI and send the result to the client.
  \param td The HTTP thread context.
  \param scriptpath The script path.
  \param cgipath The CGI handler path as specified by the MIME type.
  \param execute Specify if the script has to be executed.
  \param onlyHeader Specify if send only the HTTP header.
 */
int Cgi::send (HttpThreadContext* td, const char* scriptpath,
               const char *cgipath, bool execute, bool onlyHeader)
{
   /*
    Use this flag to check if the CGI executable is
    nph (Non Parsed Header).
   */
  bool nph = false;
  ostringstream cmdLine;

  FiltersChain chain;
  Process cgiProc;

  StartProcInfo spi;
  string moreArg;
  string tmpCgiPath;
  string tmpScriptPath;

  /*
    Standard CGI uses STDOUT to output the result and the STDIN
    to get other params like in a POST request.
  */
  Pipe stdOutFile;
  File stdInFile;
  int len = strlen (cgipath);
  int i;

  try
    {
      if (! (td->permissions & MYSERVER_PERMISSION_EXECUTE))
        return td->http->sendAuth ();

      td->scriptPath.assign (scriptpath);

      if (!FilesUtility::nodeExists (scriptpath))
        return td->http->raiseHTTPError (404);

      int subString = cgipath[0] == '"';
      /* Do not modify the text between " and ".  */
      for (i = 1; i < len; i++)
        {
          if (!subString && cgipath[i] == ' ')
            break;
          if (cgipath[i] == '"' && cgipath[i - 1] != '\\')
            subString = !subString;
        }

      /*
        Save the cgi path and the possible arguments.
        the (x < len) case is when additional arguments are specified.
        If the cgipath is enclosed between " and " do not consider them
        when splitting directory and file name.
      */
      if (i < len)
        {
          string tmpString (cgipath);
          int begin = tmpString[0] == '"' ? 1 : 0;
          int end   = tmpString[i] == '"' ? i : i - 1;
          tmpCgiPath.assign (tmpString.substr (begin, end - 1));
          moreArg.assign (tmpString.substr (i, len - 1));
        }
      else
        {
          int begin = (cgipath[0] == '"') ? 1 : 0;
          int end   = (cgipath[len] == '"') ? len-1 : len;
          tmpCgiPath.assign (&cgipath[begin], end-begin);
          moreArg.assign ("");
        }
      FilesUtility::splitPath (tmpCgiPath, td->cgiRoot, td->cgiFile);

      tmpScriptPath.assign (scriptpath);
      FilesUtility::splitPath (tmpScriptPath, td->scriptDir, td->scriptFile);

      chain.setStream (td->connection->socket);

      if (execute)
        {
          const char *args = 0;
          if (td->request.uriOpts.length ())
            args = td->request.uriOpts.c_str ();
          else if (td->pathInfo.length ())
            args = &td->pathInfo[1];

          if (cgipath && strlen (cgipath))
            cmdLine << tmpCgiPath << " " << moreArg << " "
                    << td->scriptFile <<  (args ? args : "" ) ;
          else
            cmdLine << tmpScriptPath << moreArg << " " << (args ? args : "" );

          nph = td->scriptFile.length () > 4 && td->scriptFile[0] == 'n'
            && td->scriptFile[1] == 'p' && td->scriptFile[2] == 'h'
            && td->scriptFile[3] == '-' ;

          if (cgipath && strlen (cgipath))
            {
              spi.cmd.assign (tmpCgiPath);
              spi.arg.append (" ");
              spi.arg.assign (moreArg);
              spi.arg.append (" ");
              spi.arg.append (td->scriptFile);
              if (args)
                {
                  spi.arg.append (" ");
                  spi.arg.append (args);
                }
            }
          else
            {
              spi.cmd.assign (scriptpath);
              spi.arg.assign (moreArg);
              if (args)
                {
                  spi.arg.append (" ");
                  spi.arg.append (args);
                }
            }
        }
      else
        {
          if (! FilesUtility::nodeExists (tmpCgiPath.c_str ()))
            {
              if (tmpCgiPath.length () > 0)
                td->connection->host->warningsLogWrite
                         (_("Cgi: cannot find the %s file")),
                         tmpCgiPath.c_str ();
              else
                td->connection->host->warningsLogWrite
                           (_("Cgi: Executable file not specified"));

              td->scriptPath.assign ("");
              td->scriptFile.assign ("");
              td->scriptDir.assign ("");
              chain.clearAllFilters ();
              return td->http->raiseHTTPError (500);
            }

          spi.arg.assign (moreArg);
          spi.arg.append (" ");
          spi.arg.append (td->scriptFile);

          cmdLine << "\"" << td->cgiRoot << "/" << td->cgiFile << "\" "
                  << moreArg << " " << td->scriptFile;

          spi.cmd.assign (td->cgiRoot);
          spi.cmd.append ("/");
          spi.cmd.append (td->cgiFile);

          if (td->cgiFile.length () > 4 && td->cgiFile[0] == 'n'
              && td->cgiFile[1] == 'p' && td->cgiFile[2] == 'h'
              && td->cgiFile[3] == '-' )
            nph = true;
          else
            nph = false;
        }

      /*
        Open the stdout file for the new CGI process.
      */
      stdOutFile.create ();

      /* Open the stdin file for the new CGI process.  */
      stdInFile.openFile (td->inputDataPath,
                          File::READ | File::FILE_OPEN_ALWAYS);

      /*
        Build the environment string used by the CGI process.
        Use the td->auxiliaryBuffer to build the environment string.
      */
      (td->auxiliaryBuffer->getBuffer ())[0] = '\0';
      Env::buildEnvironmentString (td, td->auxiliaryBuffer->getBuffer ());

      spi.cmdLine = cmdLine.str ();
      spi.cwd.assign (td->scriptDir);

      spi.gid = atoi (td->securityToken.getData ("cgi.gid", MYSERVER_VHOST_CONF |
                                                 MYSERVER_MIME_CONF |
                                                 MYSERVER_SECURITY_CONF |
                                                 MYSERVER_SERVER_CONF, "0"));
      spi.uid = atoi (td->securityToken.getData ("cgi.uid", MYSERVER_VHOST_CONF |
                                                 MYSERVER_MIME_CONF |
                                                 MYSERVER_SECURITY_CONF |
                                                 MYSERVER_SERVER_CONF, "0"));
      spi.chroot.assign (td->securityToken.getData ("cgi.chroot", MYSERVER_VHOST_CONF |
                                                    MYSERVER_MIME_CONF |
                                                    MYSERVER_SECURITY_CONF |
                                                    MYSERVER_SERVER_CONF, ""));

      spi.stdError = (FileHandle) stdOutFile.getWriteHandle ();
      spi.stdIn = (FileHandle) stdInFile.getHandle ();
      spi.stdOut = (FileHandle) stdOutFile.getWriteHandle ();
      spi.envString = td->auxiliaryBuffer->getBuffer ();

      if (spi.stdError == (FileHandle) -1 ||
          spi.stdIn == (FileHandle) -1 ||
          spi.stdOut == (FileHandle) -1)
        {
          td->connection->host->warningsLogWrite (_("Cgi: internal error"));
          stdOutFile.close ();
          chain.clearAllFilters ();
          return td->http->raiseHTTPError (500);
        }

      /* Execute the CGI process. */
      if (Process::getForkServer ()->isInitialized ())
        {
          int pid;
          int port;
          int flags = ForkServer::FLAG_USE_IN | ForkServer::FLAG_USE_OUT
                                              | ForkServer::FLAG_USE_ERR;
          Process::getForkServer ()->executeProcess (&spi, flags, &pid, &port);
          cgiProc.setPid (pid);
        }
      else
        cgiProc.exec (&spi);

      /* Close the write stream of the pipe on the server.  */
      stdOutFile.closeWrite ();

      sendData (td, stdOutFile, chain, cgiProc, onlyHeader, nph);

      stdOutFile.close ();
      stdInFile.close ();
      cgiProc.terminateProcess ();
      chain.clearAllFilters ();

      cgiProc.terminateProcess ();

      /* Delete the file only if it was created by the CGI module.  */
      if (td->inputData.getHandle () >= 0)
        FilesUtility::deleteFile (td->inputDataPath.c_str ());
    }
  catch (exception & e)
    {
      td->connection->host->warningsLogWrite (_E ("Cgi: internal error"), &e);
      stdOutFile.close ();
      chain.clearAllFilters ();
      return td->http->raiseHTTPError (500);
    }

  return HttpDataHandler::RET_OK;
}

/*
  Read data from the CGI process and send it back to the client.
 */
int Cgi::sendData (HttpThreadContext* td, Pipe &stdOutFile, FiltersChain& chain,
                   Process &cgiProc, bool onlyHeader, bool nph)
{
  u_long nbw = 0;
  u_long nbw2 = 0;
  u_long nBytesRead = 0;
  u_long procStartTime;
  bool useChunks = false;
  bool keepalive = false;
  int ret = 0;

  /* Reset the auxiliaryBuffer length counter. */
  td->auxiliaryBuffer->setLength (0);

  procStartTime = getTicks ();

  checkDataChunks (td, &keepalive, &useChunks);

  if (sendHeader (td, stdOutFile, chain, cgiProc, onlyHeader, nph,
                  procStartTime, keepalive, useChunks, &ret))
    return ret;

  if (!nph && onlyHeader)
    return HttpDataHandler::RET_OK;

  /* Create the output filters chain.  */
  if (td->mime)
    {
      FiltersFactory *ff = Server::getInstance ()->getFiltersFactory ();
      ff->chain (&chain, td->mime->filters, td->connection->socket, &nbw, 1);
    }

  if (td->response.getStatusType () == HttpResponseHeader::SUCCESSFUL)
    {
      /* Send the rest of the data until we can read from the pipe.  */
      for (;;)
        {
          nBytesRead = 0;
          int aliveProcess = 0;
          u_long ticks = getTicks () - procStartTime;
          u_long timeout = td->http->getTimeout ();
          if (timeout <= ticks
              || stdOutFile.waitForData ((timeout - ticks) / 1000,
                                         (timeout - ticks) % 1000) == 0)
            {
              td->connection->host->warningsLogWrite (_("Cgi: process %i timeout"),
                                                      cgiProc.getPid ());
              break;
            }

          aliveProcess = !stdOutFile.pipeTerminated ();

          /* Read data from the process standard output file.  */
          stdOutFile.read (td->auxiliaryBuffer->getBuffer (),
                           td->auxiliaryBuffer->getRealLength (),
                           &nBytesRead);

          if (!aliveProcess && !nBytesRead)
            break;

          if (nBytesRead)
            HttpDataHandler::appendDataToHTTPChannel (td,
                                             td->auxiliaryBuffer->getBuffer (),
                                                      nBytesRead,
                                                      &(td->outputData),
                                                      &chain,
                                                      td->appendOutputs,
                                                      useChunks);

          nbw += nBytesRead;
        }

      /* Send the last null chunk if needed.  */
      if (useChunks && chain.getStream ()->write ("0\r\n\r\n", 5, &nbw2))
        return HttpDataHandler::RET_FAILURE;
    }

  /* Update the Content-length field for logging activity.  */
  td->sentData += nbw;

  return HttpDataHandler::RET_OK;
}

/*!
  Send the HTTP header.
  \return nonzero if the reply is already complete.
 */
int Cgi::sendHeader (HttpThreadContext *td, Pipe &stdOutFile, FiltersChain &chain,
                     Process &cgiProc, bool onlyHeader, bool nph,
                     u_long procStartTime, bool keepalive, bool useChunks,
                     int *ret)
{
  u_long headerSize = 0;
  bool headerCompleted = false;
  u_long headerOffset = 0;
  u_long nBytesRead;

  /* Parse initial chunks of data looking for the HTTP header.  */
  while (!headerCompleted && !nph)
    {
      u_long timeout = td->http->getTimeout ();
      u_long ticks = getTicks () - procStartTime;
      bool term;

      nBytesRead = 0;

      /* Do not try to read using a small buffer as this has some
         bad influence on the performances.  */
      if (td->auxiliaryBuffer->getRealLength () - headerOffset - 1 < 512)
        break;

      term = stdOutFile.pipeTerminated ();
      if (!term
          && stdOutFile.waitForData ((timeout - ticks) / 1000,
                                     (timeout - ticks) % 1000) == 0)
        {
          td->connection->host->warningsLogWrite (_("Cgi: process %i timeout"),
                                                  cgiProc.getPid ());
          break;
        }

      if (stdOutFile.read (td->auxiliaryBuffer->getBuffer () + headerOffset,
                           td->auxiliaryBuffer->getRealLength () - headerOffset - 1,
                           &nBytesRead))
        {
          *ret = td->http->raiseHTTPError (500);
          return 1;
        }

      if (nBytesRead == 0 && term)
        {
          headerCompleted = true;
          headerSize = 0;
          break;
        }

      headerOffset += nBytesRead;

      if (headerOffset == 0)
        {
          *ret = td->http->raiseHTTPError (500);
          return 1;
        }

      for (u_long i = std::max (0UL, (headerOffset - nBytesRead - 10));
           i < headerOffset; i++)
        {
          char *buff = td->auxiliaryBuffer->getBuffer ();
          if ((buff[i] == '\r') && (buff[i+1] == '\n')
              && (buff[i+2] == '\r') && (buff[i+3] == '\n'))
            {
              headerSize = i + 4;
              headerCompleted = true;
              break;
            }
          else if ((buff[i] == '\n') && (buff[i+1] == '\n'))
            {
              headerSize = i + 2;
              headerCompleted = true;
              break;
            }
        }
    }

  /* Send the header.  */
  if (!nph)
    {
      if (keepalive)
        td->response.setValue ("Connection", "keep-alive");

      /* Send the header.  */
      if (headerSize)
        HttpHeaders::buildHTTPResponseHeaderStruct (td->auxiliaryBuffer->getBuffer (),
                                                    &td->response,
                                                    &(td->nBytesToRead));
      /*
       *If we have not to append the output send data
       *directly to the client.
       */
      if (!td->appendOutputs)
        {
          string* location = td->response.getValue ("Location", NULL);

          /* If it is present "Location: foo" in the header then send a redirect
            to `foo'.  */
          if (location && location->length ())
            {
              *ret = td->http->sendHTTPRedirect (location->c_str ());
              return 1;
            }

          HttpHeaders::sendHeader (td->response, *chain.getStream (),
                                   *td->buffer, td);
        }
    }

  if (headerOffset - headerSize)
    {
      /* Flush the buffer.  Data from the header parsing can be present.  */
      HttpDataHandler::appendDataToHTTPChannel (td,
                               td->auxiliaryBuffer->getBuffer () + headerSize,
                                                    headerOffset - headerSize,
                                                    &(td->outputData),
                                                    &chain,
                                                    td->appendOutputs,
                                                useChunks);

      td->sentData += headerOffset - headerSize;
    }

  return 0;
}
