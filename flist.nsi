; Script based on the Facebookchat, Skype4Pidgin and Off-the-Record Messaging NSI files

SetCompress off

; todo: SetBrandingImage
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "fchat-pidgin"
#!define PRODUCT_VERSION "0.4.0"
!define PRODUCT_PUBLISHER "Nelwill, Sabhak"
!define PRODUCT_WEB_SITE "http://github.com/fcwill/fchat-pidgin"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page misused to display README
!define MUI_PAGE_HEADER_TEXT "Readme"
!define MUI_PAGE_HEADER_SUBTEXT "Read below for more information about the plugin."
!define MUI_LICENSEPAGE_TEXT_TOP "Press Page Down to see the rest of the readme."
!define MUI_LICENSEPAGE_TEXT_BOTTOM "Thanks for reading!"
!define MUI_LICENSEPAGE_BUTTON "&Next >"
!insertmacro MUI_PAGE_LICENSE "README.md"

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run Pidgin"
!define MUI_FINISHPAGE_RUN_FUNCTION "RunPidgin"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
;!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}-setup.exe"

Var "PidginDir"

ShowInstDetails show
ShowUnInstDetails show
; 
Section "MainSection" SEC01
    ;Check for pidgin installation
    Call GetPidginInstPath
    
    SetOverwrite try
    
    SetOutPath "$PidginDir\pixmaps\pidgin"
    File "/oname=protocols\16\flist.png" "icons/flist16.png"
    File "/oname=protocols\22\flist.png" "icons/flist22.png"
    File "/oname=protocols\48\flist.png" "icons/flist48.png"

    SetOutPath "$PidginDir\"
    File "libjson-glib-1.0.dll"

    SetOverwrite try
    copy:
        ClearErrors
        Delete "$PidginDir\plugins\libflist.dll"
        IfErrors dllbusy
        SetOutPath "$PidginDir\plugins"
        File "libflist.dll"
        Goto after_copy
    dllbusy:
        MessageBox MB_RETRYCANCEL "libflist.dll is busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
        Goto copy
    cancel:
        Abort "Installation of flist-pidgin aborted"
    after_copy:
    
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
    IfFileExists "$0\pidgin.exe" cont
    ReadRegStr $0 HKCU "Software\pidgin" ""
    IfFileExists "$0\pidgin.exe" cont
        MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
        Abort "Failed to find Pidgin installation. Please install Pidgin first."
  cont:
    StrCpy $PidginDir $0
FunctionEnd

Function RunPidgin
    ExecShell "" "$PidginDir\pidgin.exe"
FunctionEnd
