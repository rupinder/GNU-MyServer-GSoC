# Microsoft Developer Studio Project File - Name="myserver" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=myserver - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "myserver.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "myserver.mak" CFG="myserver - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "myserver - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "myserver - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "myserver - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "binaries"
# PROP BASE Intermediate_Dir "tmp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "binaries"
# PROP Intermediate_Dir "tmp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /Zi /W4 /Oi /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /GF /Gm /YX /Fp"$(IntDir)/$(TargetName).pch" /GZ /c /GX 
# ADD CPP /nologo /MTd /Zi /W4 /Oi /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /GF /Gm /YX /Fp"$(IntDir)/$(TargetName).pch" /GZ /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /out:"binaries\myserver.exe" /incremental:no /debug /pdb:"binaries\myserver.pdb" /pdbtype:sept /subsystem:console /heap:0,0 /largeaddressaware /delay:unload /machine:ix86 /FORCE:MULTIPLE /NODEFAULTLIB:LIBCMT
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /out:"binaries\myserver.exe" /incremental:no /debug /pdb:"binaries\myserver.pdb" /pdbtype:sept /subsystem:console /heap:0,0 /largeaddressaware /delay:unload /machine:ix86 /FORCE:MULTIPLE /NODEFAULTLIB:LIBCMT

!ELSEIF  "$(CFG)" == "myserver - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "binaries"
# PROP BASE Intermediate_Dir "tmp"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "binaries"
# PROP Intermediate_Dir "tmp"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W0 /O2 /Og /Ob2 /Oi /Ot /Oy /GT /G6 /GA /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /GF /Gm /Gy /Yu"stdafx.h" /c /GX 
# ADD CPP /nologo /MT /W0 /O2 /Og /Ob2 /Oi /Ot /Oy /GT /G6 /GA /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /GF /Gm /Gy /Yu"stdafx.h" /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /out:"binaries\myserver.exe" /incremental:no /debug /pdbtype:sept /subsystem:console /heap:0,0 /largeaddressaware /opt:ref /opt:icf /machine:ix86 /FORCE:MULTIPLE /NODEFAULTLIB:LIBCMT
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /out:"binaries\myserver.exe" /incremental:no /debug /pdbtype:sept /subsystem:console /heap:0,0 /largeaddressaware /opt:ref /opt:icf /machine:ix86 /FORCE:MULTIPLE /NODEFAULTLIB:LIBCMT

!ENDIF

# Begin Target

# Name "myserver - Win32 Debug"
# Name "myserver - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm"
# Begin Source File

SOURCE=source\AMMimeUtils.cpp
# End Source File
# Begin Source File

SOURCE=source\MIME_manager.cpp
# End Source File
# Begin Source File

SOURCE=source\cXMLParser.cpp
# End Source File
# Begin Source File

SOURCE=source\clientsThread.cpp
# End Source File
# Begin Source File

SOURCE=source\cserver.cpp
# End Source File
# Begin Source File

SOURCE=source\filemanager.cpp
# End Source File
# Begin Source File

SOURCE=source\http.cpp
# End Source File
# Begin Source File

SOURCE=source\httpmsg.cpp
# End Source File
# Begin Source File

SOURCE=source\myserver.cpp
# End Source File
# Begin Source File

SOURCE=source\security.cpp
# End Source File
# Begin Source File

SOURCE=source\sockets.cpp
# End Source File
# Begin Source File

SOURCE=stdafx.cpp
# End Source File
# Begin Source File

SOURCE=source\utility.cpp

!IF  "$(CFG)" == "myserver - Win32 Debug"

# ADD CPP /nologo /TP /GZ /GX 
!ELSEIF  "$(CFG)" == "myserver - Win32 Release"

# ADD CPP /nologo /Yu"stdafx.h" /TC /GX 
!ENDIF

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc"
# Begin Source File

SOURCE=include\AMMimeUtils.h
# End Source File
# Begin Source File

SOURCE=include\HTTPmsg.h
# End Source File
# Begin Source File

SOURCE=include\MIME_manager.h
# End Source File
# Begin Source File

SOURCE=include\Response_RequestStructs.h
# End Source File
# Begin Source File

SOURCE=include\cXMLParser.h
# End Source File
# Begin Source File

SOURCE=include\clientsThread.h
# End Source File
# Begin Source File

SOURCE=include\connectionstruct.h
# End Source File
# Begin Source File

SOURCE=include\cserver.h
# End Source File
# Begin Source File

SOURCE=include\filemanager.h
# End Source File
# Begin Source File

SOURCE=include\http.h
# End Source File
# Begin Source File

SOURCE=include\security.h
# End Source File
# Begin Source File

SOURCE=include\sockets.h
# End Source File
# Begin Source File

SOURCE=stdafx.h
# End Source File
# Begin Source File

SOURCE=include\utility.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

