/*
MyServer
Copyright (C) 2007 The MyServer Team
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


#include "../include/dynamic_executor.h"
#include "../include/xml_parser.h"
#include "../include/server.h"
#include "../include/lfind.h"
#include "../include/file.h"

#include <string>

typedef int (*executePROC)(char*, u_long);
typedef int (*executeFromFilePROC)(char*);


/*!
 *Default constructor.
 */
DynamicExecutor::DynamicExecutor() : Plugin()
{

}

/*!
 *Destroy the object.
 */
DynamicExecutor::~DynamicExecutor()
{
  hinstLib.close();
}

/*!
 *Execute the code.
 *\param buffer Buffer with the code.
 *\param length Buffer length.
 */
int DynamicExecutor::execute(char* buffer, u_long length)
{
  executePROC execute = (executePROC) hinstLib.getProc("execute");
  if(execute)
    return execute(buffer, length);
  else
    return -1;
}

/*!
 *Execute the code from a file.
 *\param fileName Execute the code from a file.
 */
int DynamicExecutor::executeFromFile(char* fileName)
{
  executeFromFilePROC execFile = 
		(executeFromFilePROC)hinstLib.getProc("executeFromFile");

  if(execFile)
    return execFile(fileName);
  else
    return -1;
}

/*!
 *Load the file in memory and try to execute the buffer.
 */
int DynamicExecutor::loadFileAndExecute(char* fileName)
{
	File file;
	char *buffer = 0;
	u_long size = 0;
	u_long nbr = 0;
	int ret = 0;
	if(file.openFile(fileName, File::MYSERVER_OPEN_IFEXISTS))
		return -1;

	size = file.getFileSize();

	if(size == (u_long)-1)
	{
		ret = -1;
	}

	if(!ret)
		buffer = new char[size];

	if(buffer == 0)
		ret = -1;

	if(!ret)
		if(file.read(buffer, size, &nbr))
			ret = -1;
 
	if(!ret)
		ret = execute(buffer, size);

	if(buffer)
		delete []buffer;

	file.closeFile();
	
	return ret;
}
