/**
 * Include represents the commandlist that needs to be supported for the
 * FTP implementation
 */

#ifndef FTPCOMMAND_H
#define FTPCOMMAND_H

#include "../stdafx.h"
#define unsupportedresponse  " Command not supported at this point"

class ftpcommand
{ 
  enum 
   {
     ERR_ABORT,ERR_UKNOWN,COM_APPE,COM_BYE,COM_CDUP,COM_CWD,COM_DELE,COM_HELP,
     COM_LIST,COM_MKD ,COM_NLST,COM_NOOP,COM_PASS,COM_PASV,COM_PORT,COM_PWD, 
     COM_QUIT,COM_RETR,COM_SIZE,COM_STOR,COM_SYST,COM_TYPE,COM_USER,COM_XCWD,
     COM_XMKD,COM_XPWD,COM_XRMD,COM_REST,COM_RMD ,COM_RNFR,COM_RNTO
   };

   private :     
   static BOOL checkerr(char * buff);
   static unsigned int getCommandToken(char * buff);
   static unsigned int checkErrorToken(char * buff);
   
   /**
    * structural definition of each error
    */
   struct errstruct
   {
     int token;
     char * error;
     char * response;
   };
   
   /**
    * structural definition of each command
    */
   struct comstruct
   {
     int token;
     char * command;
     BOOL hasparams;
     BOOL hasresponse;
     char * initialresponse;
   };
   
   void filler();

 public:      
   /**
    * structure will be used to decipher incoming buffer requests
    */
   struct commanddef
   {
     int commandTok;
     BOOL reqparams;
     BOOL errorcommand;
     char *  paramlist; // string representing rest of paramaters
     char * errorresponse; // empty if not error
   };
   /**
    * one ftp command will be created for each received buffer, making it 
    * easier to process information
    */

   ftpcommand();
   ~ftpcommand();
   void start(char * onreceive);
   char * getResponse();  
}; // END OF CLASS
#endif
