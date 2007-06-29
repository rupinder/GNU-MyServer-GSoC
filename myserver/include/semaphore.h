/*
MyServer
Copyright (C) 2006, 2007 The MyServer Team
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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "../stdafx.h"

#ifdef HAVE_PTHREAD
#include <semaphore.h>
typedef sem_t SemaphoreHandle;
#else
typedef HANDLE SemaphoreHandle;
#endif

class Semaphore
{
private:
	int initialized;
	SemaphoreHandle semaphore;
public:
	Semaphore(int n);
	~Semaphore();
	int init(int n);
	int destroy();
	int lock(u_long id = 0);
	int unlock(u_long id = 0);
};
#endif
