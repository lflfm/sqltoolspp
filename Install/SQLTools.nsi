;
; SQLTools NSIS 2.14 script installer
;

;--------------------------------
;Include Modern UI
	!include "MUI.nsh"


;--------------------------------
;General
	;TODO: ADD CHECK PREVENTING INSTALL ALPHA INTO BETA/STABLE FOLDER !!!
	Name "SQLTools_pp v${SQLTOOLSVER} Beta"
	OutFile SQLTools_pp_${SQLTOOLSVER}.exe

	!define DEST_NAME	"SQLTools_pp"
	!define SRCDIR 		"..\SQLTools"
	!define EXEDIR  	"..\_output_\${SQLTOOLS_BUILDDIR}"
	!define TEMP 		$R0


;--------------------------------
;Configuration
	SetCompressor lzma

	;--------------------------------
	;Modern UI Configuration

	!define MUI_CUSTOMPAGECOMMANDS
	!define MUI_LICENSEPAGE
	!define MUI_COMPONENTSPAGE
	!define MUI_COMPONENTSPAGE_SMALLDESC
	!define MUI_DIRECTORYPAGE
	!define MUI_ABORTWARNING
	!define MUI_UNINSTALLER
	!define MUI_UNCONFIRMPAGE
	!define MUI_FINISHPAGE_RUN "$INSTDIR\SQLTools.exe"

	;Pages
	!insertmacro MUI_PAGE_LICENSE $(MUILicense)
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	Page custom UpgradeOptionPage
	!insertmacro MUI_PAGE_INSTFILES
	!insertmacro MUI_PAGE_FINISH 
	;Uninstall pages
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES 
	!insertmacro MUI_UNPAGE_FINISH

	;Languages
	!insertmacro MUI_LANGUAGE "English" # first language is the default language
	!include "Languages\sqlt_English.nsh"
	!insertmacro MUI_LANGUAGE "Polish"
	!include "Languages\sqlt_Polish.nsh"
	!insertmacro MUI_LANGUAGE "French"
	!include "Languages\sqlt_French.nsh"
	!insertmacro MUI_LANGUAGE "Hungarian"
	!include "Languages\sqlt_Hungarian.nsh"
	!insertmacro MUI_LANGUAGE "Spanish"
	!include "Languages\sqlt_Spanish.nsh"
	!insertmacro MUI_LANGUAGE "PortugueseBR"  # Portuguese (Brasil)
	!include "Languages\sqlt_Portuguese_BR.nsh"

	;General
	Icon "${SRCDIR}\Res\InstallSQLTools14.ico"
	UninstallIcon "${SRCDIR}\Res\Uninstall.ico"

	;Folder-selection page
	InstallDir "$PROGRAMFILES\${DEST_NAME}"
	InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}" ""

	;InstType "Normal (recommended for fresh installation and upgrade)"
	;InstType "Full upgrade (with replacing all user settings)"

;--------------------------------
;Reserve Files
;Things that need to be extracted on first (keep these lines before any File command!)
;Only useful for BZIP2 compression
	ReserveFile "UpgradeOptionPage.ini"
	ReserveFile "UpgradeOptionPage-empty.ini"
	!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS


;--------------------------------
;Installer Sections
Section $(DESC_SectionProgramFilesName) SecCopyProgramFiles
	SectionIn 1 2 RO
	SetOutPath "$INSTDIR"

	IfFileExists "$INSTDIR\Data\*.dat" 0 CreateDataFiles

	; delete obsolete stuff
	Delete "$INSTDIR\Data\context.keymap"
	Delete "$INSTDIR\Data\editplus2.keymap"
	Delete "$INSTDIR\Data\textpad.keymap"
	Delete "$INSTDIR\Data\ultraedit.keymap"
	Delete "$INSTDIR\Licence.txt"
	Delete "$INSTDIR\License.txt"
	Delete "$INSTDIR\sesstat.dat"
	Delete "$INSTDIR\Bugs.txt"

	Delete "$SMPROGRAMS\${DEST_NAME}\SQLTools Home Page.lnk"
    	Delete "$SMPROGRAMS\${DEST_NAME}\Bugs.lnk"

	; backup old files
	!insertmacro MUI_INSTALLOPTIONS_READ ${TEMP} "UpgradeOptionPage.ini" "Field 1" "State"
	StrCmp ${TEMP} "1" "" +2
		CreateDirectory "$INSTDIR\Data\Backup"
	CopyFiles /FILESONLY "$INSTDIR\Data\*.*" "$INSTDIR\Data\Backup"

	SetOutPath "$INSTDIR\Data"

	; add new files to existing
	IfFileExists "$INSTDIR\Data\sesstat.dat" +2
		File "${SRCDIR}\Settings\sesstat.dat"

	IfFileExists "$INSTDIR\Data\sqltools.dat" +2
		File "${SRCDIR}\Settings\sqltools.dat"

	IfFileExists "$INSTDIR\Data\default(ms).keymap" +2
		File "${SRCDIR}\Settings\default(ms).keymap"

	; read upgrade option and process
	!insertmacro MUI_INSTALLOPTIONS_READ ${TEMP} "UpgradeOptionPage.ini" "Field 2" "State"
	StrCmp ${TEMP} "1" "" SkipReplaceSettings
		File "${SRCDIR}\Settings\settings.dat"
		File "${SRCDIR}\Settings\sqltools.dat"
		File "${SRCDIR}\Settings\sesstat.dat"
		File "${SRCDIR}\Settings\schemaddl.dat"

	SkipReplaceSettings:
		!insertmacro MUI_INSTALLOPTIONS_READ ${TEMP} "UpgradeOptionPage.ini" "Field 3" "State"
		StrCmp ${TEMP} "1" "" +2
			File "${SRCDIR}\Settings\languages.dat"

		!insertmacro MUI_INSTALLOPTIONS_READ ${TEMP} "UpgradeOptionPage.ini" "Field 4" "State"
		StrCmp ${TEMP} "1" "" +2
			File "${SRCDIR}\Settings\templates.dat"

		!insertmacro MUI_INSTALLOPTIONS_READ ${TEMP} "UpgradeOptionPage.ini" "Field 5" "State"
		StrCmp ${TEMP} "1" "" +2
			File "${SRCDIR}\Settings\default.keymap"
		Goto CreateAppFiles

	CreateDataFiles:
		CreateDirectory "$INSTDIR\Data"
		SetOutPath "$INSTDIR\Data"
		File "${SRCDIR}\Settings\custom.keymap"
		File "${SRCDIR}\Settings\default.keymap"
		File "${SRCDIR}\Settings\default(ms).keymap"
		File "${SRCDIR}\Settings\languages.dat"
		File "${SRCDIR}\Settings\settings.dat"
		File "${SRCDIR}\Settings\sqltools.dat"
		File "${SRCDIR}\Settings\templates.dat"
		File "${SRCDIR}\Settings\sesstat.dat"
		File "${SRCDIR}\Settings\schemaddl.dat"

	CreateAppFiles:
		SetOutPath "$INSTDIR"
		File "..\SQLTools\HelpSQL\SQLqkref.chm"
		File "${EXEDIR}\SQLTools.chm"
		File "${EXEDIR}\SQLTools.exe"
		File "Tools\Grep.exe"
		File "${SRCDIR}\ReadME.TXT"
		; File "${SRCDIR}\LICENSE.TXT"
		File "${SRCDIR}\History.TXT"

		WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}" "" "$INSTDIR"
		WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}" "DisplayName" "${DEST_NAME} (remove only)"
		WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}" "UninstallString" '"$INSTDIR\uninst.exe"'
		WriteUninstaller "$INSTDIR\uninst.exe"

		CreateDirectory $INSTDIR\Samples
		SetOutPath $INSTDIR\Samples

		File "PlSqlCatalog\substitution.sql"
		File "PlSqlCatalog\drop_all_objects.sql"

	;CreateHtmlFiles:
		CreateDirectory "$INSTDIR\Html"
		CreateDirectory "$INSTDIR\Html\xtree"
		SetOutPath "$INSTDIR\Html\xtree"
		File "${SRCDIR}\Html\xtree\explain_plan.html-template"
		File "${SRCDIR}\Html\xtree\explain_plan.css"
		File "${SRCDIR}\Html\xtree\xtree.js"
		CreateDirectory "$INSTDIR\Html\xtree\images"
		SetOutPath "$INSTDIR\Html\xtree\images"
		File "${SRCDIR}\Html\xtree\images\*.*"

	;Language specific files
		SetOutPath $INSTDIR
		StrCmp $LANGUAGE ${LANG_ENGLISH} 0 +2
			File "/oname=Licence.txt" "Languages\gpl_english.txt"
		StrCmp $LANGUAGE ${LANG_POLISH} 0 +2
			File "/oname=Licence.txt" "Languages\gpl_polish.txt"
		StrCmp $LANGUAGE ${LANG_FRENCH} 0 +2
			File "/oname=Licence.txt" "Languages\CeCILL_french.txt"
		StrCmp $LANGUAGE ${LANG_HUNGARIAN} 0 +2
			File "/oname=Licence.txt" "Languages\gpl_hungarian.txt"
		StrCmp $LANGUAGE ${LANG_SPANISH} 0 +2
			File "/oname=Licence.txt" "Languages\gpl_spanish.txt"
		StrCmp $LANGUAGE ${LANG_PORTUGUESEBR} 0 +2
			File "/oname=Licence.txt" "Languages\gpl_portuguese_br.txt"
SectionEnd

Section $(DESC_SectionMenuName) SecCreateMenuGroup
	SectionIn 1 2
    SetOutPath "$SMPROGRAMS\${DEST_NAME}"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\SQLTools.lnk" \
                   "$INSTDIR\SQLTools.exe"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\SQLTools Help.lnk" \
                   "$INSTDIR\SQLTools.chm"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\SQL Quick Reference.lnk" \
                   "$INSTDIR\SQLqkref.chm"
    WriteINIStr "$SMPROGRAMS\${DEST_NAME}\SQLTools Home Page.url" \
                "InternetShortcut" "URL" "http://www.sqltools.net"
    WriteINIStr "$SMPROGRAMS\${DEST_NAME}\SQLTools on the SourceForge.url" \
                "InternetShortcut" "URL" "http://www.sourceforge.net/projects/sqlt"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\ReadME.lnk" \
                   "$INSTDIR\ReadME.TXT"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\Licence.lnk" \
                   "$INSTDIR\Licence.TXT"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\History.lnk" \
                   "$INSTDIR\History.txt"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\Uninstall SQLTools.lnk" \
                   "$INSTDIR\Uninst.exe"
    SetOutPath $INSTDIR
SectionEnd

Section $(DESC_SectionDesktopIconName) SecCreateDesktopIcon
	SectionIn 1 2
    CreateShortCut "$DESKTOP\${DEST_NAME}.lnk" "$INSTDIR\SQLTools.exe"
SectionEnd

Section $(DESC_SectionQuickLaunchName) SecQuickLaunchIcon
	SectionIn 1 2
    CreateShortCut "$QUICKLAUNCH\${DEST_NAME}.lnk" "$INSTDIR\SQLTools.exe"
SectionEnd


;--------------------------------
;MUI macros
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCopyProgramFiles}  $(DESC_SecCopyProgramFiles)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCreateMenuGroup}   $(DESC_SecCreateMenuGroup)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCreateDesktopIcon} $(DESC_SecCreateDesktopIcon)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunchIcon}   $(DESC_SecQuickLaunchIcon)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


;--------------------------------
;Functions
Function .onInit
  ;Prevent multiple instances of SQLTools intaller
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "SQLTmutex") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "The SQLTools installer is already running."
  Abort
  ;Extract InstallOptions INI Files
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "UpgradeOptionPage.ini"
  ;Choose language
  !insertmacro MUI_LANGDLL_DISPLAY 
FunctionEnd

Function UpgradeOptionPage
	;Set language strings
	!insertmacro MUI_INSTALLOPTIONS_WRITE "UpgradeOptionPage.ini" "Field 1" text $(DESC_UpgradeCheckboxBackup)
	!insertmacro MUI_INSTALLOPTIONS_WRITE "UpgradeOptionPage.ini" "Field 2" text $(DESC_UpgradeCheckboxSettings)
	!insertmacro MUI_INSTALLOPTIONS_WRITE "UpgradeOptionPage.ini" "Field 3" text $(DESC_UpgradeCheckboxLanguage)
	!insertmacro MUI_INSTALLOPTIONS_WRITE "UpgradeOptionPage.ini" "Field 4" text $(DESC_UpgradeCheckboxTempl)
	!insertmacro MUI_INSTALLOPTIONS_WRITE "UpgradeOptionPage.ini" "Field 5" text $(DESC_UpgradeCheckboxKeymap)

	;Skip this page if it's fresh install
	IfFileExists "$INSTDIR\Data\*.dat" Found
		!insertmacro MUI_HEADER_TEXT $(DESC_UpgradeTitle) $(DESC_WithoutUpgradeText)
		!insertmacro MUI_INSTALLOPTIONS_DISPLAY "UpgradeOptionPage-empty.ini"
		Goto Done
	Found:
		!insertmacro MUI_HEADER_TEXT $(DESC_UpgradeTitle) $(DESC_UpgradeText)
		!insertmacro MUI_INSTALLOPTIONS_DISPLAY "UpgradeOptionPage.ini"
	Done:
FunctionEnd


;--------------------------------
;Uninstaller
Section Uninstall
	Delete "$INSTDIR\sqlnet.log"
	Delete "$INSTDIR\uninst.exe"
	Delete "$INSTDIR\sqlqkref.chm"
	Delete "$INSTDIR\sqltools.chm"
	Delete "$INSTDIR\SQLTools.exe"
	Delete "$INSTDIR\grep.exe"
	Delete "$INSTDIR\ReadMe.txt"
	Delete "$INSTDIR\Licence.txt"
	Delete "$INSTDIR\License.txt"
	Delete "$INSTDIR\History.txt"
	Delete "$INSTDIR\Bugs.txt"

	Delete "$INSTDIR\Data\Backup\*.*"
	RMDir "$INSTDIR\Data\Backup"

	Delete "$INSTDIR\Data\custom.keymap"
	Delete "$INSTDIR\Data\default.keymap"
	Delete "$INSTDIR\Data\default(ms).keymap"
	Delete "$INSTDIR\Data\languages.dat"
	Delete "$INSTDIR\Data\languages.dat.old"
	Delete "$INSTDIR\Data\settings.dat"
	Delete "$INSTDIR\Data\settings.dat.old"
	Delete "$INSTDIR\Data\sqltools.dat"
	Delete "$INSTDIR\Data\sqltools.dat.old"
	Delete "$INSTDIR\Data\templates.dat"
	Delete "$INSTDIR\Data\templates.dat.old"
	Delete "$INSTDIR\Data\sesstat.dat"
	Delete "$INSTDIR\Data\sesstat.dat.old"
	Delete "$INSTDIR\Data\schemaddl.dat"
	Delete "$INSTDIR\Data\schemaddl.dat.old"
	RMDir "$INSTDIR\Data"

	Delete "$INSTDIR\Samples\substitution.sql"
	Delete "$INSTDIR\Samples\drop_all_objects.sql"
	RMDir  "$INSTDIR\Samples"

	Delete "$INSTDIR\Html\xtree\images\*.*"
	RMDir  "$INSTDIR\Html\xtree\images"
	Delete "$INSTDIR\Html\xtree\*.*"
	RMDir  "$INSTDIR\Html\xtree"
	RMDir  "$INSTDIR\Html"
	
	Delete "$SMPROGRAMS\${DEST_NAME}\*.lnk"
	Delete "$SMPROGRAMS\${DEST_NAME}\*.url"
	RMDir  "$SMPROGRAMS\${DEST_NAME}"
	Delete "$DESKTOP\${DEST_NAME}.lnk"
	Delete "$QUICKLAUNCH\${DEST_NAME}.lnk"

	;DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}"
	RMDir "$INSTDIR"
SectionEnd ; end of uninstall section

;--------------------------------
;Uninstaller Functions
Function un.onInit
  !insertmacro MUI_UNGETLANGUAGE
FunctionEnd

