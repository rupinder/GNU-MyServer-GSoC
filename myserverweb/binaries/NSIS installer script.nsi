;NSIS Installation script for myServer
SetCompressor bzip2
!include "MUI.nsh"
!define MUI_VERSION "0.6.2"
!define MUI_PRODUCT "MyServer"
!define MUI_COMPONENTSPAGE
!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_DIRECTORYPAGE
!define MUI_LICENSEPAGE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "MyServer"
!define MUI_FINISHPAGE
!define MUI_STARTMENUPAGE
!insertmacro MUI_RESERVEFILE_WELCOMEFINISHPAGE
!define MUI_ICON ${NSISDIR}\Contrib\Icons\setup.ico
!define SF_RO         16
XPStyle on


InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"
!insertmacro MUI_LANGUAGE "English"

  

InstallDirRegKey HKCU "Software\${MUI_PRODUCT}" ""


!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${MUI_PRODUCT}" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "STARTMENUFOLDER"

!define TEMP $R0




LicenseData "license.txt"
ShowInstDetails show


Section "MyServer core" SecCore
  SetOutPath $INSTDIR
  WriteUninstaller "remove.exe"
  File "myserver.exe"
  File "libxml2.dll"
  File "libpng12.dll"
  File "zlib1.dll"
  File "libeay32.dll"
  File "ssleay32.dll"
  File "MIMEtypes.xml.default"
  File "myserver.xml.default"
  File "virtualhosts.xml.default"
  File "readme.txt"
  CreateDirectory "$INSTDIR\logs"
  SetOutPath $INSTDIR\web
  File "web\*.*"
  SetOutPath $INSTDIR\system
  File "system\*.*"
  SetOutPath $INSTDIR\languages
  File "languages\English.xml"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN
    CreateDirectory "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Run Console.lnk" "$INSTDIR\myserver.exe"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Remove MyServer.lnk" "$INSTDIR\remove.exe"
    CreateDirectory "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Configuration files"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Configuration files\Virtual hosts.lnk" "$INSTDIR\virtualhosts.xml"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Configuration files\MyServer configuration.lnk" "$INSTDIR\myserver.xml"
    CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Configuration files\MIME types.lnk" "$INSTDIR\MIMEtypes.xml"
  !insertmacro MUI_STARTMENU_WRITE_END

   CreateDirectory "$INSTDIR\certificates"
SectionEnd


Section "Web examples" SecWebEx
  SetOutPath "$INSTDIR\web\cgi-bin"
  File "web\cgi-bin\*.*"
  SetOutPath $INSTDIR\web\cgi-src\math_sum
  File "web\cgi-src\math_sum\*.*"
  SetOutPath "$INSTDIR\web\cgi-src\post"
  File "web\cgi-src\post\*.*"
  SetOutPath "$INSTDIR\web\cgi-src\counter"
  File "web\cgi-src\counter\*.*"
  SetOutPath "$INSTDIR\web\downloads"
  File "web\downloads\*.*"
SectionEnd


Section "MyServer control center" SecControl
  DetailPrint "Control Center Application"
  SetOutPath $INSTDIR
  File "control.exe"
  File "myserver.ico"
  CreateShortCut "$SMPROGRAMS\${MUI_STARTMENUPAGE_VARIABLE}\Control Center.lnk" "$INSTDIR\control.exe"
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
  File "web\documentation\*.*"
  SetOutPath "$INSTDIR\web\documentation\images"
  File "web\documentation\images\*.*"
SectionEnd

Section "Install other languages" SecLanguages
  SetOutPath $INSTDIR\languages
  File "languages\*.*"
SectionEnd

Section "Install the service" SecService
	SetOutPath "$INSTDIR"
	ExecWait "$INSTDIR\control.exe REGISTER"
SectionEnd

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "Install the MyServer core application(this element is required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocumentation} "Install the MyServer documentation"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecControl} "Install the Control Center application(the installation of this element is highly recommended)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMSCGI} "Copy the MyServer MSCGI library(premature status yet)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecLanguages} "Copy all the languages files(by default only the english language is copied)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecWebEx} "Install some web examples"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecService} "Install MyServer like a service (loaded automatically on startup)"

!insertmacro MUI_FUNCTIONS_DESCRIPTION_END

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
  ExecWait "$INSTDIR\control.exe UNREGISTER" ;Remove the service if installed
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
  RMDir "$INSTDIR\web\cgi-bin"
  RMDir "$INSTDIR\web\cgi-src\post"
  RMDir "$INSTDIR\web\cgi-src\math_sum"
  RMDir "$INSTDIR\web\downloads"
  RMDir "$INSTDIR\web\cgi-src"
  RMDir "$INSTDIR\documentation"
  RMDir "$INSTDIR\images"
;	REMOVE ALL THE WEB AND CONFIGURATION FILES
;  Delete "$INSTDIR\web\*.*"
;  Delete "$INSTDIR\web\cgi-bin\*.*"
;  Delete "$INSTDIR\web\cgi-src\math_sum\*.*"
;  Delete "$INSTDIR\web\cgi-src\post\*.*"
;  Delete "$INSTDIR\web\cgi-src\*.*"
;  Delete "$INSTDIR\web\downloads\*.*"
;  Delete "$INSTDIR\system\*.*"
;  RMDir "$INSTDIR\web\"  
;  RMDir "$INSTDIR\system"
  RMDir "$INSTDIR\languages"
  RMDir "$INSTDIR\logs"
  RMDir "$INSTDIR"

  ReadRegStr ${TEMP} "${MUI_STARTMENUPAGE_REGISTRY_ROOT}" "${MUI_STARTMENUPAGE_REGISTRY_KEY}" "${MUI_STARTMENUPAGE_REGISTRY_VALUENAME}"

  Delete "$SMPROGRAMS\${TEMP}\*.lnk" 
  Delete "$SMPROGRAMS\${TEMP}\Configuration files\*.lnk" 
  RMDir "$SMPROGRAMS\${TEMP}\Configuration files"
  RMDir "$SMPROGRAMS\${TEMP}"
  DeleteRegKey /ifempty "${MUI_STARTMENUPAGE_REGISTRY_ROOT}" "Software\${MUI_PRODUCT}"

SectionEnd
