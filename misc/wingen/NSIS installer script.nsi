;NSIS Installation script for GNU MyServer

!include "MUI2.nsh"

SetCompressor /SOLID lzma

Name "MyServer"
OutFile "MyServer-win32-0.9.0.exe"

; Versioning Information
VIProductVersion "0.9.0.3"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName"         "GNU MyServer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments"            "www.gnu.org/software/myserver/"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName"         "Free Software Foundation Inc."
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright"      "GNU MyServer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription"     "GNU MyServer webserver"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion"         "0.9.0.1"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion"      "0.9.0.1"
VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName"        ""
;VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks"    ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "OriginalFilename"    "MyServer-win32-0.9-rc1.exe"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "PrivateBuild"       "1"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "SpecialBuild"       "1"

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

; !define SF_RO         16
  
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
!insertmacro MUI_LANGUAGE "Afrikaans"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Basque"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Welsh"



; Available Languages
LicenseLangString MUILicense ${LANG_ENGLISH} "COPYING"			;DEFAULT LANGUAGE
LicenseLangString MUILicense ${LANG_AFRIKAANS} "COPYING"
LicenseLangString MUILicense ${LANG_ALBANIAN} "COPYING"
LicenseLangString MUILicense ${LANG_ARABIC} "COPYING"
LicenseLangString MUILicense ${LANG_BASQUE} "COPYING"
LicenseLangString MUILicense ${LANG_BELARUSIAN} "COPYING"
LicenseLangString MUILicense ${LANG_BOSNIAN} "COPYING"
LicenseLangString MUILicense ${LANG_BRETON} "COPYING"
LicenseLangString MUILicense ${LANG_BULGARIAN} "COPYING"
LicenseLangString MUILicense ${LANG_CROATIAN} "COPYING"
LicenseLangString MUILicense ${LANG_CZECH} "COPYING"
LicenseLangString MUILicense ${LANG_DANISH} "COPYING"
LicenseLangString MUILicense ${LANG_DUTCH} "COPYING"
LicenseLangString MUILicense ${LANG_ESTONIAN} "COPYING"
LicenseLangString MUILicense ${LANG_FARSI} "COPYING"
LicenseLangString MUILicense ${LANG_FINNISH} "COPYING"
LicenseLangString MUILicense ${LANG_FRENCH} "COPYING"
LicenseLangString MUILicense ${LANG_GALICIAN} "COPYING"
LicenseLangString MUILicense ${LANG_GERMAN} "COPYING"
LicenseLangString MUILicense ${LANG_GREEK} "COPYING"
LicenseLangString MUILicense ${LANG_HEBREW} "COPYING"
LicenseLangString MUILicense ${LANG_HUNGARIAN} "COPYING"
LicenseLangString MUILicense ${LANG_ICELANDIC} "COPYING"
LicenseLangString MUILicense ${LANG_INDONESIAN} "COPYING"
LicenseLangString MUILicense ${LANG_IRISH} "COPYING"
LicenseLangString MUILicense ${LANG_ITALIAN} "COPYING"
LicenseLangString MUILicense ${LANG_JAPANESE} "COPYING"
LicenseLangString MUILicense ${LANG_KOREAN} "COPYING"
LicenseLangString MUILicense ${LANG_KURDISH} "COPYING"
LicenseLangString MUILicense ${LANG_LATVIAN} "COPYING"
LicenseLangString MUILicense ${LANG_LITHUANIAN} "COPYING"
LicenseLangString MUILicense ${LANG_LUXEMBOURGISH} "COPYING"
LicenseLangString MUILicense ${LANG_MACEDONIAN} "COPYING"
LicenseLangString MUILicense ${LANG_MALAY} "COPYING"
LicenseLangString MUILicense ${LANG_MONGOLIAN} "COPYING"
LicenseLangString MUILicense ${LANG_NORWEGIAN} "COPYING"
LicenseLangString MUILicense ${LANG_NORWEGIANNYNORSK} "COPYING"
LicenseLangString MUILicense ${LANG_POLISH} "COPYING"
LicenseLangString MUILicense ${LANG_PORTUGUESE} "COPYING"
LicenseLangString MUILicense ${LANG_PORTUGUESEBR} "COPYING"
LicenseLangString MUILicense ${LANG_ROMANIAN} "COPYING"
LicenseLangString MUILicense ${LANG_RUSSIAN} "COPYING"
LicenseLangString MUILicense ${LANG_SERBIAN} "COPYING"
LicenseLangString MUILicense ${LANG_SERBIANLATIN} "COPYING"
LicenseLangString MUILicense ${LANG_SIMPCHINESE} "COPYING"
LicenseLangString MUILicense ${LANG_SLOVAK} "COPYING"
LicenseLangString MUILicense ${LANG_SLOVENIAN} "COPYING"
LicenseLangString MUILicense ${LANG_SPANISH} "COPYING"
LicenseLangString MUILicense ${LANG_SWEDISH} "COPYING"
LicenseLangString MUILicense ${LANG_THAI} "COPYING"
LicenseLangString MUILicense ${LANG_TRADCHINESE} "COPYING"
LicenseLangString MUILicense ${LANG_TURKISH} "COPYING"
LicenseLangString MUILicense ${LANG_UKRAINIAN} "COPYING"
LicenseLangString MUILicense ${LANG_UZBEK} "COPYING"
LicenseLangString MUILicense ${LANG_WELSH} "COPYING"

; reserves the files: speeds up installer
!insertmacro MUI_RESERVEFILE_LANGDLL

Section "MyServer core" SecCore

  SetOutPath $INSTDIR
  WriteUninstaller "uninstall.exe"
  File "myserver.exe"
  File "MIMEtypes.xml"
  File "myserver.xml"
  File "virtualhosts.xml"
  File "README"
  File "COPYING"
  CreateDirectory "$INSTDIR\logs"
  SetOutPath $INSTDIR\web
  File "web\*.html"
  File "web\*.png"
  SetOutPath $INSTDIR\system
  File "system\.security.xml"
  SetOutPath $INSTDIR\system\css
  File "system\css\*.css"
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
  File /nonfatal "web\cgi-bin\*.mscgi"
; COPY SOURCE CODE
;  SetOutPath $INSTDIR\web\cgi-src\math_sum
;  File "web\cgi-src\math_sum\*.*"
;  SetOutPath "$INSTDIR\web\cgi-src\post"
;  File "web\cgi-src\post\*.*"
  SetOutPath "$INSTDIR\web\downloads"
  File /nonfatal "web\downloads\*.php"
  File /nonfatal "web\downloads\*.txt"
  File /nonfatal "web\downloads\*.sh"
SectionEnd

Section "Documentation" SecDocumentation
  SetOutPath "$INSTDIR\web\documentation"
  File /nonfatal "..\documentation\myserver.html\*"
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

;This needs the NSIS Simple Firewall Plugin
;http://nsis.sourceforge.net/NSIS_Simple_Firewall_Plugin
Section "Modify the Windows firewall settings (HTTP)" SecFirewallHttp
SimpleFC::AddPort 80 web 6 0 2 "" 1
SectionEnd

Section "Modify the Windows firewall settings (FTP)" SecFirewallFtp
SimpleFC::AddPort 21 ftp 6 0 2 "" 1
SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "Install the MyServer core application(this element is required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocumentation} "Install the MyServer documentation"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecLanguages} "Copy all the languages files(by default only the english language is copied)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecWebEx} "Install some web examples"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecService} "Install MyServer like a service (loaded automatically on startup)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFirewallHttp} "Modify the Windows firewall settings to allow MyServer access from other hosts on HTTP"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecFirewallFtp} "Modify the Windows firewall settings to allow MyServer access from other hosts on FTP"
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
  Delete "$INSTDIR\languages\*.*"
  Delete "$INSTDIR\logs\*.*"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.ico"
  Delete "$INSTDIR\*.txt"  
  Delete "$INSTDIR\documentation\*.*"
  Delete "$INSTDIR\documentation\images\*.*"
  RMDir "$INSTDIR\documentation"
  RMDir "$INSTDIR\images"
;;;;;;;;;;; REMOVE ALL THE WEB AND CONFIGURATION FILES
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
