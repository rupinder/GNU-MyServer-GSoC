/*
*myServer
*Copyright (C) 2002 Giuseppe Scrivano
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef WINCGI_H
#define WINCGI_H

#include "../include/Response_RequestStructs.h"
#include "../include/MIME_manager.h"
#include "../include/security.h"
extern const char *versionOfSoftware;
#include "../include/http.h"

int sendWINCGI(httpThreadContext*,LPCONNECTION s,char* filename);

#endif
