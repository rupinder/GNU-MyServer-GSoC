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

#include <include/base/xml/xml_parser.h>
#include <include/base/utility.h>
#include <include/base/file/files_utility.h>
#include <include/base/exceptions/checked.h>

#include <string.h>


/**
 * Internal call back functions for saveMemBuf
 * \param context Context
 * \param buffer Buffer
 * \param len Length
 * \return Returns the length
 */
static int MemBufWriteCallback (void * context, const char * buffer, int len)
{
  ((MemBuf *)context)->addBuffer ((const void *)buffer, len);
  return len;
}


/**
 * Memory Buffer Close Callback
 * \param context Context
 * \return Returns 0
 */
static int MemBufCloseCallback (void *context)
{
  return 0;
}


/**
 * Initializes the libxml2 library
 * Calls xmlInitParser ()
 * @return Returns true
 */
bool XmlParser::startXML ()
{
  xmlInitParser ();
  return 1;
}


/**
 * Cleans up the libxml2 library.
 * @return Returns true
 */
bool XmlParser::cleanXML ()
{
  xmlCleanupParser ();
  return 1;
}


/**
 * Opens a files and stores it in memory.
 * \param filename The XML file to open.
 * \param useXpath Specify if XPath is enabled.
 * \return Returns 0 on success, non zero values on failure
 */
int XmlParser::open (const char* filename, bool useXpath)
{
  cur = NULL;
  this->useXpath = useXpath;

  if (!FilesUtility::nodeExists (filename))
    return -1;

  if (doc!= NULL)
    close ();

  doc = xmlParseFile (filename);

  if (doc == NULL)
    return -1;

  cur = xmlDocGetRootElement (doc);
  if (!cur)
  {
    close ();
    return -1;
  }

  mtime = FilesUtility::getLastModTime (filename);

  if (mtime == static_cast<time_t>(-1))
  {
    close ();
    return -1;
  }

  if (useXpath)
  {
    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL)
    {
      close ();
      return -1;
    }
  }
  return 0;
}


/**
 * Gets the last modification time of the file
 * @return Returns last modification time
 */
time_t XmlParser::getLastModTime ()
{
  return mtime;
}

/**
 * Read the XML data from a char array
 * \param memory The memory buffer to read from.
 * \param useXpath Specify if XPath is enabled.
 * \return Returns 0 on succes, non 0 on failure
 */
int XmlParser::openMemBuf (MemBuf & memory, bool useXpath)
{
  mtime = 0;
  cur = NULL;
  this->useXpath = useXpath;

  if (memory.getLength () == 0)
    return -1;

  if (!doc)
    {
      doc = xmlParseMemory ((const char * )memory.getBuffer (),
                            memory.getLength ());
    }
  else
    close ();

  if (!doc)
    return -1;

  cur = xmlDocGetRootElement (doc);

  if (!cur)
    {
      close ();
      return -1;
    }

  if (useXpath)
    {
      xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL)
      {
      close ();
      return -1;
      }
    }
  return 0;
}

/**
 * Constructor of the XmlParser class
 */
XmlParser::XmlParser ()
{
  doc = NULL;
  cur = NULL;
  xpathCtx = NULL;
  useXpath = false;
  prevCur = NULL;
  lastNode = NULL;
  mtime = 0;
}

/**
 * Destructor of the XmlParser class
 * Destroys the XmlParser object
 */
XmlParser::~XmlParser ()
{
  close ();
}

/**
 * Returns the XML document
 * @return Returns XML document
 */
xmlDocPtr XmlParser::getDoc ()
{
  return doc;
}

/**
 * Gets the value of the vName root child element.
 * \param vName vName of the root child elment
 * \return Returns the value of the vName
 */
const char *XmlParser::getValue (const char* vName)
{
  char *ret = NULL;
  xmlNodePtr lcur;
  cur = xmlDocGetRootElement (doc);

  if (!cur)
    return 0;

  lcur = cur->xmlChildrenNode;

  while (lcur)
  {
    if (!xmlStrcmp (lcur->name, (const xmlChar *)vName))
    {
      lastNode = lcur;
      return (char*)lcur->children->content;
    }

    lcur = lcur->next;
  }

  return ret;
}


/**
 * Sets the value of the vName root child element
 * \param vName
 * \param value
 * \return Returns 0 on success, non zero on failures
 */
int XmlParser::setValue (const char* vName, const char *value)
{
  xmlNodePtr lcur = cur->xmlChildrenNode;
  while (lcur)
  {
    if (!xmlStrcmp (lcur->name, (const xmlChar *)vName))
    {
      lastNode = lcur;

      if (lcur->children->content)
        xmlFree (lcur->children->content);

      lcur->children->content = (xmlChar *)
        checked::checkErrorNull (xmlStrdup ((const xmlChar *) value));

      return 0;
    }

    lcur = lcur->next;
  }

  return 1;
}


/**
 * Gets the attribute for the node field.
 * \param field Field
 * \param attr Attribute
 * \return
 */
const char *XmlParser::getAttr (const char* field, const char *attr)
{
  xmlNodePtr lcur = cur->xmlChildrenNode;

  while (lcur)
  {
    if (!xmlStrcmp (lcur->name, (const xmlChar *)field))
    {
      lastNode = lcur;
      xmlAttr *attrs =  lcur->properties;

      while (attrs)
      {
        if (!xmlStrcmp (attrs->name, (const xmlChar *)attr))
        {
          return (char*)attrs->children->content;
        }

        attrs = attrs->next;
      }
    }

    lcur = lcur->next;
  }

  return 0;
}

/**
 *Evaluate an XPath expression.
 *\param expr The xpath expression.
 *\return NULL on errors.
 *\return The XmlXPathResult containing the result.
 */
XmlXPathResult* XmlParser::evaluateXpath (const char* expr)
{
  xmlXPathObjectPtr xpathObj;

  if (!useXpath)
    return NULL;

  xpathObj = xmlXPathEvalExpression ((const xmlChar*)expr, xpathCtx);

  if (xpathObj == NULL)
    return NULL;

  return new XmlXPathResult (xpathObj);
}


/**
 * Frees the memory, use by the XmlParser class
 */
int XmlParser::close ()
{
  int ret = 1;

  if (doc)
    {
      xmlFreeDoc (doc);
      ret = 0;
    }

  if (useXpath && xpathCtx)
    xmlXPathFreeContext (xpathCtx);

  doc = NULL;
  cur = NULL;
  prevCur = NULL;
  lastNode = NULL;

  return ret;
}

/**
 * Saves the XML tree into a file
 * If no errors occur nbytes[optional] will contain
 * the amount of written bytes
 * \param filename Filename
 * \param nbytes Amount of bytes
 * \return Returns 0 on success, non 0 on failures
 */
int XmlParser::save (const char *filename,int *nbytes)
{
  int err = xmlSaveFile (filename,doc);

  if (nbytes)
    *nbytes = err;

  return err;
}


/**
 * Saves the XML tree into memory
 * If no errors occur nbytes[optional] will contain
 * the amount of written bytes
 * \param memory Memory Buffer
 * \param nbytes Amount of bytes
 * \return Returns 0 on success, non 0 on failures
 */
int XmlParser::saveMemBuf (MemBuf & memory,int *nbytes)
{
  /*! Initialize the callback struct. */
  xmlOutputBufferPtr callback;
  callback = xmlOutputBufferCreateIO (MemBufWriteCallback,
                                      MemBufCloseCallback,
                                      (void *)&memory,
                                      NULL);

  /*! Clear the buffer */
  memory.free ();

  /*! Let libxml2 fill the MemBuf class with our interal callbacks. */
  int err = xmlSaveFileTo (callback, doc, NULL);

  if (nbytes)
    *nbytes = err;

  return err;
}


/**
 * Starts a new XML tree for a new file
 * \param root roote elment entry
 */
void XmlParser::newfile (const char * root)
{
  if (doc != NULL)
    close ();

  doc = xmlNewDoc ((const xmlChar*)"1.0");
  cur = xmlNewDocNode (doc, NULL, (const xmlChar*)root, NULL);

  xmlDocSetRootElement (doc, cur);

  addLineFeed ();
  addLineFeed ();
}


/**
 * Adds a new child element entry
 * \param name Child name
 * \param value Value of the child
 */
void XmlParser::addChild (const char * name, const char * value)
{
  lastNode = xmlNewTextChild (cur, NULL, (const xmlChar*)name,
                              (const xmlChar*)value);

  addLineFeed ();
}


/**
 * Starts a new sub group
 * Only one level for now
 * \param name Name of the sub group
 */
void XmlParser::addGroup (const char * name)
{
  if (prevCur == NULL)
    {
      prevCur = cur;
      cur = xmlNewTextChild (cur, NULL, (const xmlChar*)name, NULL);
      lastNode = cur;
      addLineFeed ();
    }
}


/**
 * Ends the sub group, if any
 * Only one level for now
 */
void XmlParser::endGroup ()
{
  if (prevCur != NULL)
  {
    cur = prevCur;
    prevCur = NULL;
    addLineFeed ();
    addLineFeed ();
  }
}


/**
 * Sets an attribute, using the last node entry
 * \param name Name
 * \param value Value
 */
void XmlParser::setAttr (const char * name, const char * value)
{
  if (lastNode == NULL)
    return;

  xmlSetProp (lastNode, (const xmlChar*)name, (const xmlChar*)value);
}


/**
 * Adds a line feed to the XML data
 */
void XmlParser::addLineFeed ()
{
#ifdef WIN32
    xmlNodePtr endline = xmlNewDocText (doc, (const xmlChar *)"\r\n");
#else
    xmlNodePtr endline = xmlNewDocText (doc, (const xmlChar *)"\n");
#endif

  xmlAddChild (cur, endline);
}
