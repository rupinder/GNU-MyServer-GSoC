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

#include <include/base/read_directory/rec_read_directory.h>
#include <fts_.h>

using namespace std;

/*!
  Generate a file tree given a path string.
  \param path The path to the root
 */
int RecReadDirectory::fileTreeGenerate (const char* path)
{
  char *argv[2] = {(char *) path, NULL};

  fileTree = fts_open (argv, (FTS_LOGICAL | FTS_NOCHDIR), NULL);

  return fileTree ? 0 : -1;
}

/*!
  Get stat(2) information.
 */
struct stat *RecReadDirectory::getStat ()
{
  return ((FTSENT *) fileTreeIter)->fts_statp;
}


/*!
  Get the next member in the file tree.
 */
bool RecReadDirectory::nextMember ()
{
  fileTreeIter = fts_read ((FTS *) fileTree);
  return fileTreeIter ? true : false;
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
  return ((FTSENT *) fileTreeIter)->fts_info;
}

/*!
  Get the current recursion level.
 */
short RecReadDirectory::getLevel ()
{
  return ((FTSENT *) fileTreeIter)->fts_level;
}

/*!
  Skip descent of the current directory.
*/
void RecReadDirectory::skip ()
{
  /*FIXME: check for errors.  */
  fts_set ((FTS *) fileTree, (FTSENT *) fileTreeIter, FTS_SKIP);
}

/*!
  Get path for particular member of the tree.
 */
char* RecReadDirectory::getPath ()
{
  return ((FTSENT *) fileTreeIter)->fts_path;
}
