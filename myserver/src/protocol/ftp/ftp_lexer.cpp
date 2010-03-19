/* We need this hack to ensure myserver.h is include before any other file.  */
#include "myserver.h"

#undef fprintf
#undef malloc
#undef realloc
#undef fwrite

#define fprintf gnulib::fprintf
#define fwrite gnulib::fwrite
#define malloc gnulib::malloc
#define realloc gnulib::realloc

#include "ftp_lexer.out.cpp"

#undef fprintf
#undef fwrite
#undef malloc
#undef realloc

/* Shutup a warning.  */
void
ftp_lexer_dummy ()
{
  yyunput (0, NULL, NULL);
}
