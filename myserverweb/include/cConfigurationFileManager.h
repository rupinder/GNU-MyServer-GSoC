#pragma once
#include "..\StdAfx.h"

class cConfigurationFileManager
{
	DWORD buffersize;
	char *buffer;
	char data[MAX_PATH];
	FILE *file;
public:
	void open(char*);
	char *getValue(char*);
	void close();
};
