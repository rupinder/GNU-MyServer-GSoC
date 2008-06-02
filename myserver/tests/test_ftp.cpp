/*
 MyServer
 Copyright (C) 2008 The MyServer Team
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

#include "../include/ftp.h"
#include "../include/mem_buff.h"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class TestFtp : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( TestFtp );
	CPPUNIT_TEST( testEscapeTelnet );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {}
	void tearDown() {}
	void testEscapeTelnet()
	{
		MemBuf inBuf, outBuf;
		inBuf << "\377\366RE\377\374TTR abc\377\377def\015\377\376\012\012";

		Ftp ftp;
		ftp.EscapeTelnet(inBuf, outBuf);
		char szOut[128], szExpected[128];
		memset(szOut, 0, 128);
		strncpy(szOut, outBuf.getBuffer(), outBuf.getLength());
		memset(szExpected, 0, 128);
		strcpy(szExpected, "RETR abc\377def\015\012");

		CPPUNIT_ASSERT( strcmp(szOut, szExpected) == 0);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( TestFtp );
