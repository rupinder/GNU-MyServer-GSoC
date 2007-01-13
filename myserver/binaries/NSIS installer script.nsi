;NSIS Installation script for MyServer
;
; Additional files needed for the installer can be downloaded at:
; http://people.myserverproject.net/~codingmaster/
;

!include "MUI.nsh"

SetCompressor /SOLID lzma

Name "MyServer"
OutFile "MyServer-win32-0.8.4.exe"

; Versioning Information
VIProductVersion "0.8.4.2"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" 		"MyServer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" 		"www.myserverproject.net"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" 		"MyServer Project"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" 		"MyServer Project"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" 	"MyServer webserver"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" 		"0.8.4.2"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" 		"0.8.4"
VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName" 		"Gathering Clouds"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" 	""
VIAddVersionKey /LANG=${LANG_ENGLISH} "OriginalFilename"	"MyServer-win32-0.8.4.exe"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "PrivateBuild" 		"2"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "SpecialBuild" 		"2"

; Setting the Style
;
; MUI Settings / Icons
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
 
; MUI Settings / Header
!define MUI_HEADERIMAGE ""
!define MUI_HEADERIMAGE_RIGHT ""
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-r.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall-r.bmp"
 
; MUI Settings / Wizard
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "uninstall.bmp"


!define MUI_ABORTWARNING



Var STARTMENU_FOLDER

!define SF_RO         16
  
InstallDir "$PROGRAMFILES\MyServer"

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\MyServer" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "STARTMENUFOLDER"

InstallDirRegKey HKLM "Software\MyServer" ""


; Install
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE $(MUILicense)
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_STARTMENU MyServer $STARTMENU_FOLDER
!insertmacro MUI_PAGE_FINISH

; Uninstall
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
  


; Available Languages
!insertmacro MUI_LANGUAGE "English"         ;DEFAULT LANGUAGE
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Catalan"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Kurdish"



; Licenses for the available languages
LicenseLangString MUILicense ${LANG_ENGLISH} "license.txt"
LicenseLangString MUILicense ${LANG_FRENCH} "license.txt"
LicenseLangString MUILicense ${LANG_GERMAN} "license.txt"
LicenseLangString MUILicense ${LANG_SPANISH} "license.txt"
LicenseLangString MUILicense ${LANG_SIMPCHINESE} "license.txt"
LicenseLangString MUILicense ${LANG_TRADCHINESE} "license.txt"
LicenseLangString MUILicense ${LANG_JAPANESE} "license.txt"
LicenseLangString MUILicense ${LANG_KOREAN} "license.txt"
LicenseLangString MUILicense ${LANG_ITALIAN} "license.txt"
LicenseLangString MUILicense ${LANG_DUTCH} "license.txt"
LicenseLangString MUILicense ${LANG_DANISH} "license.txt"
LicenseLangString MUILicense ${LANG_SWEDISH} "license.txt"
LicenseLangString MUILicense ${LANG_NORWEGIAN} "license.txt"
LicenseLangString MUILicense ${LANG_FINNISH} "license.txt"
LicenseLangString MUILicense ${LANG_GREEK} "license.txt"
LicenseLangString MUILicense ${LANG_RUSSIAN} "license.txt"
LicenseLangString MUILicense ${LANG_PORTUGUESE} "license.txt"
LicenseLangString MUILicense ${LANG_PORTUGUESEBR} "license.txt"
LicenseLangString MUILicense ${LANG_POLISH} "license.txt"
LicenseLangString MUILicense ${LANG_UKRAINIAN} "license.txt"
LicenseLangString MUILicense ${LANG_CZECH} "license.txt"
LicenseLangString MUILicense ${LANG_SLOVAK} "license.txt"
LicenseLangString MUILicense ${LANG_CROATIAN} "license.txt"
LicenseLangString MUILicense ${LANG_BULGARIAN} "license.txt"
LicenseLangString MUILicense ${LANG_HUNGARIAN} "license.txt"
LicenseLangString MUILicense ${LANG_THAI} "license.txt"
LicenseLangString MUILicense ${LANG_ROMANIAN} "license.txt"
LicenseLangString MUILicense ${LANG_LATVIAN} "license.txt"
LicenseLangString MUILicense ${LANG_MACEDONIAN} "license.txt"
LicenseLangString MUILicense ${LANG_ESTONIAN} "license.txt"
LicenseLangString MUILicense ${LANG_TURKISH} "license.txt"
LicenseLangString MUILicense ${LANG_LITHUANIAN} "license.txt"
LicenseLangString MUILicense ${LANG_CATALAN} "license.txt"
LicenseLangString MUILicense ${LANG_SLOVENIAN} "license.txt"
LicenseLangString MUILicense ${LANG_SERBIAN} "license.txt"
LicenseLangString MUILicense ${LANG_SERBIANLATIN} "license.txt"
LicenseLangString MUILicense ${LANG_ARABIC} "license.txt"
LicenseLangString MUILicense ${LANG_FARSI} "license.txt"
LicenseLangString MUILicense ${LANG_HEBREW} "license.txt"
LicenseLangString MUILicense ${LANG_INDONESIAN} "license.txt"
LicenseLangString MUILicense ${LANG_MONGOLIAN} "license.txt"
LicenseLangString MUILicense ${LANG_LUXEMBOURGISH} "license.txt"
LicenseLangString MUILicense ${LANG_ALBANIAN} "license.txt"
LicenseLangString MUILicense ${LANG_BRETON} "license.txt"
LicenseLangString MUILicense ${LANG_BELARUSIAN} "license.txt"
LicenseLangString MUILicense ${LANG_ICELANDIC} "license.txt"
LicenseLangString MUILicense ${LANG_MALAY} "license.txt"
LicenseLangString MUILicense ${LANG_BOSNIAN} "license.txt"
LicenseLangString MUILicense ${LANG_KURDISH} "license.txt"





; reserves the files: speeds up installer
!insertmacro MUI_RESERVEFILE_LANGDLL
  



Section "MyServer core" SecCore

  SetOutPath $INSTDIR
  WriteUninstaller "uninstall.exe"
  File "myserver.exe"
  File "libxml2.dll"
  File "libiconv-2.dll"
  File "iconv.dll"
  File "libcharset-1.dll"
  File "libpng13.dll"
  File "zlib1.dll"
  File "libssl32.dll"
  File "libintl3.dll"
  File "rx.dll"
  File "libeay32.dll"
  File "MIMEtypes.xml.default"
  File "myserver.xml.default"
  File "virtualhosts.xml.default"
  File "readme.txt"
  File "license.txt"
  CreateDirectory "$INSTDIR\logs"
  SetOutPath $INSTDIR\web
  File "web\*.html"
  File "web\*.png"
  SetOutPath $INSTDIR\system
  File "system\security"
  File "system\*.css"
  SetOutPath $INSTDIR\system\errors
  File "system\errors\*.html"
  SetOutPath $INSTDIR\system\icons
  SetOutPath $INSTDIR\system\icons\codes
  File "system\icons\codes\*.png"
  SetOutPath $INSTDIR\languages
  File "languages\English.xml"
  SetOutPath $INSTDIR\languages\configure
  File "languages\configure\english.xml"
   
  !insertmacro MUI_STARTMENU_WRITE_BEGIN MyServer

  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Run Console.lnk" "$INSTDIR\myserver.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Remove MyServer.lnk" "$INSTDIR\uninstall.exe"
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\Virtual hosts.lnk" "$INSTDIR\virtualhosts.xml"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\MyServer configuration.lnk" "$INSTDIR\myserver.xml"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Configuration files\MIME types.lnk" "$INSTDIR\MIMEtypes.xml"
  
  !insertmacro MUI_STARTMENU_WRITE_END
  
  CreateDirectory "$INSTDIR\certificates"
  SetOutPath $INSTDIR\certificates
  File "certificates\*.txt"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyServer" \
		   "DisplayName" "MyServer webserver"


  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyServer" \
                   "UninstallString" "$INSTDIR\uninstall.exe"

SectionEnd


Section "Web examples" SecWebEx
  SetOutPath "$INSTDIR\web\cgi-bin"
  File "web\cgi-bin\*.html"
  File "web\cgi-bin\*.mscgi"
; COPY SOURCE CODE
;  SetOutPath $INSTDIR\web\cgi-src\math_sum
;  File "web\cgi-src\math_sum\*.*"
;  SetOutPath "$INSTDIR\web\cgi-src\post"
;  File "web\cgi-src\post\*.*"
;  SetOutPath "$INSTDIR\web\cgi-src\counter"
;  File "web\cgi-src\counter\*.*"
  SetOutPath "$INSTDIR\web\downloads"
  File "web\downloads\*.php"
  File "web\downloads\*.txt"
  File "web\downloads\*.sh"
SectionEnd

Section "MyServer center" SecControl
  DetailPrint "Control Center Application"
  SetOutPath $INSTDIR
  File "Myserver Configure.exe"
  File "myserver.ico"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Control Center.lnk" "$INSTDIR\Myserver Configure.exe"
SectionEnd

Section "MSCGI library" SecMSCGI
  SetOutPath $INSTDIR\cgi-lib
  File "cgi-lib\cgi-lib.dll"
  File "cgi-lib\cgi_manager.h"
;  File "cgi-lib\CGI-LIB.a"
  File "cgi-lib\license.txt"
SectionEnd


Section "Documentation" SecDocumentation
  SetOutPath "$INSTDIR\web\documentation"
  File "..\documentation\english\*.htm"

  SetOutPath "$INSTDIR\web\documentation\images"
  File "..\Documentation\english\images\*.png"
  File "..\Documentation\english\images\*.jpg"

  SetOutPath "$INSTDIR\web\documentation\style"
  File "..\documentation\english\style\*.css"

  SetOutPath "$INSTDIR\web\documentation\texts"
  File "..\documentation\english\texts\*.htm"

SectionEnd

Section "Install other languages" SecLanguages
  SetOutPath $INSTDIR\languages
  File "languages\*.xml"
  SetOutPath $INSTDIR\languages\configure
  File "languages\configure\*.xml"
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



Function .onInit



; Fade in the splash image
InitPluginsDir

File /oname=$PLUGINSDIR\splash.bmp "splash.bmp"
        advsplash::show 1000 600 400 -1 $PLUGINSDIR\splash

        Pop $0


       Delete $PLUGINSDIR\splash.bmp


!insertmacro MUI_LANGDLL_DISPLAY



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

DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MyServer"


SectionEnd




; Uninstaller Functions

;Function un.onInit
;  select language during uninstall
;  !insertmacro MUI_UNGETLANGUAGE
  
;FunctionEnd
