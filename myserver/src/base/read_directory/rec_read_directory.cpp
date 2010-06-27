/*
  MyServer
  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
  Free Software Foundation, Inc.
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
#include <include/base/read_directory/rec_read_directory.h>

#include <string>

using namespace std;

/*!
  Generate a file tree given a path string.
  \param path The path to the root
 */
FTS* RecReadDirectory::fileTreeGenerate (const char* path)
{
  char *argv[2] = {(char *) path, NULL};

  fileTree = fts_open (argv, FTS_LOGICAL, NULL);

  return fileTree;
}

/*!
  Get the next member in the file tree.
 */
FTSENT* RecReadDirectory::nextMember ()
{
  fileTreeIter = fts_read (fileTree);
  return fileTreeIter;
}

/*!
  Clear the file tree.
 */
void RecReadDirectory::clearTree ()
{
  fileTree = NULL;
  fileTreeIter = NULL;
}

/*!
  Get info flag for particular member of the tree.
 */
short RecReadDirectory::getInfo ()
{
  return fileTreeIter->fts_level;
}

/*!
  Get path for particular member of the tree.
 */
char* RecReadDirectory::getPath ()
{
  return fileTreeIter->fts_path;
}
