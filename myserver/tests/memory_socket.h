/*
  MyServer
  Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#include <include/base/socket/socket.h>
#include <string.h>
#include <include/base/mem_buff/mem_buff.h>


class MemorySocket : public Socket
{
public:
  MemorySocket (){};
  ~MemorySocket (){};


  virtual Handle getHandle () {return (Handle) -1;}

  virtual int connect (MYSERVER_SOCKADDR*, int) {return 0;}
  virtual int close (){return 0;}
  virtual int shutdown (int){return 0;}
  virtual int recv (char*, int, int, u_long){return 0;}
  virtual int recv (char*, int, int) {return 0;}
  virtual u_long bytesToRead () {return 0;}

  virtual int dataAvailable (int sec = 0, int usec = 500){return 0;}

  int read (char* buffer, size_t len, size_t *nbr)
  {
    return len;
  }

  int write (const char* buffer, size_t len, size_t *nbw)
  {
    interBuff << buffer;
    return len;
  }

  int rawSend (const char* buffer, int len, int flags)
  {
    interBuff << buffer;
    return len;
  }

  int getLength()
  {
    return interBuff.getLength();
  }

  operator const char *()
  {
    return interBuff.getBuffer ();
  }

private:
  MemBuf interBuff;
};
