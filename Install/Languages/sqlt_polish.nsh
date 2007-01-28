;Language specific include file for SqlTools
;Created by Tomasz Drzewiecki
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_POLISH}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\gpl_polish.txt"

;Section names and comments
LangString DESC_SecCopyProgramFiles	${CURLANG} "Kopiuj SQLTools.exe i inne wymagane komponenty do folderu aplikacji."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Utw�rz grup� ikonek SQLTools w menu Windows."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Utw�rz skr�t do SQLTools na pulpicie."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Utw�rz ikonk� szybkiego uruchamiania dla SQLTools."

LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (wymagane)"
LangString DESC_SectionMenuName		${CURLANG} "Grupa menu"
LangString DESC_SectionDesktopIconName	${CURLANG} "Ikona na pulpicie"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Ikona szybkiego uruchamiania"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Opcje uaktualnienia"
LangString DESC_UpgradeText		${CURLANG} "Instalator wykry�, �e SQLTools jest zainstalowany w wybranym folderze. $\nWybierz opcje aktualizacji."
LangString DESC_WithoutUpgradeText	${CURLANG} "Instalator wykry�, �e SQLTools nie jest zainstalowany w wybranym folderze.$\nOpcje aktualizacji nie s� dost�pne."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Utw�rz kopie bezpiecze�stwa wszystkich plik�w danych (stara kopa bezpiecze�stwa b�dzie nadpisana bez potwierdzenia)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Zast�p ustawienia"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Zast�p definicje j�zyk�w (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Zast�p wzorce"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Zast�p mapy ustawie� klawiatury (opr�cz custom.keymap)"
