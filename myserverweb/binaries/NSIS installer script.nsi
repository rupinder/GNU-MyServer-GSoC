;NSIS Installation script for MyServer
!include "MUI.nsh"

SetCompressor lzma

Name "MyServer"
OutFile "setup.exe"

Var STARTMENU_FOLDER

!define SF_RO         16
  
InstallDir "$PROGRAMFILES\MyServer"

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\MyServer" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "STARTMENUFOLDER"

InstallDirRegKey HKLM "Software\MyServer" ""

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_STARTMENU MyServer $STARTMENU_FOLDER
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
  
  
!insertmacro MUI_LANGUAGE "English"
  
Section "MyServer core" SecCore
  SetOutPath $INSTDIR
  WriteUninstaller "remove.exe"
  File "myserver.exe"
  File "libxml2.dll"
  File "libiconv-2.dll"
  File "iconv.dll"
  File "libcharset-1.dll"
  File "libpng13.dll"
  File "zlib1.dll"
  File "libssl32.dll"
  File "libintl-2.dll"
  File "rx.dll"
  File "libeay32.dll"
  File "MIMEtypes.xml.default"
  File "myserver.xml.default"
  File "virtualhosts.xml.default"
  File "readme.txt"
  CreateDirectory "$INSTDIR\logs"
  SetOutPath $INSTDIR\web
  File "web\*.*"
  SetOutPath $INSTDIR\web\icons
  File "web\icons\*.*"
  SetOutPath $INSTDIR\system
  File "system\security"
  File "system\*.*"
  SetOutPath $INSTDIR\system\errors
  File "system\errors\*.*"
  SetOutPath $INSTDIR\system\icons
  File "system\icons\*.*"
  SetOutPath $INSTDIR\system\icons\codes
  File "system\icons\codes\*.*"
  SetOutPath $INSTDIR\languages
  File "languages\English.xml"
  SetOutPath $INSTDIR\languages\control
  File "languages\configure\english.xml"
   
  !insertmacro MUI_STARTMENU_WRITE_BEGIN MyServer

  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Run Console.lnk" "$INSTDIR\myserver.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Remove MyServer.lnk" "$INSTDIR\remove.exe"
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\Virtual hosts.lnk" "$INSTDIR\virtualhosts.xml"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\MyServer configuration.lnk" "$INSTDIR\myserver.xml"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\MIME types.lnk" "$INSTDIR\MIMEtypes.xml"
  
  !insertmacro MUI_STARTMENU_WRITE_END
  
  CreateDirectory "$INSTDIR\certificates"
SectionEnd


Section "Web examples" SecWebEx
  SetOutPath "$INSTDIR\web\cgi-bin"
  File "web\cgi-bin\*.*"
; COPY SOURCE CODE
;  SetOutPath $INSTDIR\web\cgi-src\math_sum
;  File "web\cgi-src\math_sum\*.*"
;  SetOutPath "$INSTDIR\web\cgi-src\post"
;  File "web\cgi-src\post\*.*"
;  SetOutPath "$INSTDIR\web\cgi-src\counter"
;  File "web\cgi-src\counter\*.*"
  SetOutPath "$INSTDIR\web\downloads"
  File "web\downloads\*.*"
SectionEnd

Section "MyServer center" SecControl
  DetailPrint "Control Center Application"
  SetOutPath $INSTDIR
  File "Myserver Configure.exe"
  File "myserver.ico"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Control Center.lnk" "$INSTDIR\Myserver Configure.exe"
SectionEnd

Section "MSCGI lib" SecMSCGI
  SetOutPath $INSTDIR\cgi-lib
  File "cgi-lib\cgi-lib.dll"
  File "cgi-lib\cgi_manager.h"
;  File "cgi-lib\CGI-LIB.a"
  File "cgi-lib\license.txt"
SectionEnd


Section "Documentation" SecDocumentation
  SetOutPath "$INSTDIR\web\documentation"
  File "..\Documentation\En\*.*"
  SetOutPath "$INSTDIR\web\documentation\images"
  File "..\Documentation\En\images\*.*"
SectionEnd

Section "Install other languages" SecLanguages
  SetOutPath $INSTDIR\languages
  File "languages\*.*"
  SetOutPath $INSTDIR\languages\configure
  File "languages\configure\*.*"
SectionEnd

Section "Install the service" SecService
 SetOutPath "$INSTDIR"
 ExecWait "$INSTDIR\myserver.exe REGISTER"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "Install the MyServer core application(this element is required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocumentation} "Install the MyServer documentation"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecControl} "Install the Control Center application(the installation of this element is highly recommended)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMSCGI} "Copy the MyServer MSCGI library(premature status yet)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecLanguages} "Copy all the languages files(by default only the english language is copied)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecWebEx} "Install some web examples"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecService} "Install MyServer like a service (loaded automatically on startup)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


OutFile setup.exe ;NSIS output
Function .onInit
  Push $0
  StrCpy $1 ${SecCore}
  SectionGetFlags ${SecCore} $0
  IntOp $0 $0 | ${SF_RO}
  SectionSetFlags ${SecCore} $0 ;Make the MyServer core installed in every case
  Pop $0
FunctionEnd

; MyServer Uninstaller

Section "Uninstall"
  ExecWait "$INSTDIR\myserver.exe UNREGISTER" ;Remove the service if installed
  Delete "$INSTDIR\cgi-lib\*.*"
  Delete "$INSTDIR\languages\*.*"
  Delete "$INSTDIR\logs\*.*"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\*.txt"  
  Delete "$INSTDIR\documentation\*.*"
  Delete "$INSTDIR\documentation\images\*.*"
  RMDir "$INSTDIR\cgi-lib"
  RMDir "$INSTDIR\documentation"
  RMDir "$INSTDIR\images"
; REMOVE ALL THE WEB AND CONFIGURATION FILES
;  Delete "$INSTDIR\web\*.*"
;  Delete "$INSTDIR\web\cgi-bin\*.*"
;  Delete "$INSTDIR\web\cgi-src\math_sum\*.*"
;  Delete "$INSTDIR\web\cgi-src\post\*.*"
;  Delete "$INSTDIR\web\cgi-src\*.*"
;  Delete "$INSTDIR\web\downloads\*.*"
;  Delete "$INSTDIR\system\*.*"
;  RMDir "$INSTDIR\web\"  
;  RMDir "$INSTDIR\system"
;  RMDir "$INSTDIR\web\cgi-bin"
;  RMDir "$INSTDIR\web\cgi-src\post"
;  RMDir "$INSTDIR\web\cgi-src\math_sum"
;  RMDir "$INSTDIR\web\downloads"
;  RMDir "$INSTDIR\web\cgi-src"
 
  RMDir "$INSTDIR\languages"
  RMDir "$INSTDIR\logs"
  RMDir "$INSTDIR"

  ReadRegStr $0 HKLM  "Software\MyServer"  "STARTMENUFOLDER"

  Delete "$SMPROGRAMS\$0\*.lnk" 
  Delete "$SMPROGRAMS\$0\Configuration files\*.lnk" 
  RMDir "$SMPROGRAMS\$0\Configuration files"
  RMDir "$SMPROGRAMS\$0"
  DeleteRegKey /ifempty HKLM  "Software\MyServer"  

SectionEnd

