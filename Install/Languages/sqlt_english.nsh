;Language specific include file for SqlTools
;Created by Aleksey Kochetov & Tomasz Drzewiecki
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_ENGLISH}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\gpl_english.txt"

;Section names and comments
LangString DESC_SecCopyProgramFiles	${CURLANG} "Copy SQLTools.exe and other required components to the application folder."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Create SQLTools menu group in Windows menu."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Create SQLTools shortcut on your desktop."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Create SQLTools quick launch shortcut."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (required)"
LangString DESC_SectionMenuName		${CURLANG} "Menu Group"
LangString DESC_SectionDesktopIconName	${CURLANG} "Desktop Icon"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Quick Launch Icon"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Upgrade options"
LangString DESC_UpgradeText		${CURLANG} "Setup determinated that SQLTools has been already installed in the selected folder. $\nChoose upgrade options."
LangString DESC_WithoutUpgradeText	${CURLANG} "Setup determinated that SQLTools has not been installed in the selected folder. $\nUpgrade options are not available."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Create backup for all data files (old backup files will be replaced w/o confirmation)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Replace settings"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Replace language definitions (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Replace templates"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Replace keymap layouts (except custom.keymap)"
