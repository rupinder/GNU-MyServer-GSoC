/* We need this hack to ensure myserver.h is include before any other file.  */
#include "myserver.h"
#include "ftp_lexer.out.cpp"

/* Shutup a warning.  */
void
ftp_lexer_dummy ()
{
  yyunput (0, NULL, NULL);
}
