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
LangString DESC_SecCreateMenuGroup	${CURLANG} "Utwórz grupê ikonek SQLTools w menu Windows."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Utwórz skrót do SQLTools na pulpicie."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Utwórz ikonkê szybkiego uruchamiania dla SQLTools."

LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (wymagane)"
LangString DESC_SectionMenuName		${CURLANG} "Grupa menu"
LangString DESC_SectionDesktopIconName	${CURLANG} "Ikona na pulpicie"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Ikona szybkiego uruchamiania"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Opcje uaktualnienia"
LangString DESC_UpgradeText		${CURLANG} "Instalator wykry³, ¿e SQLTools jest zainstalowany w wybranym folderze. $\nWybierz opcje aktualizacji."
LangString DESC_WithoutUpgradeText	${CURLANG} "Instalator wykry³, ¿e SQLTools nie jest zainstalowany w wybranym folderze.$\nOpcje aktualizacji nie s¹ dostêpne."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Utwórz kopie bezpieczeñstwa wszystkich plików danych (stara kopa bezpieczeñstwa bêdzie nadpisana bez potwierdzenia)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Zast¹p ustawienia"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Zast¹p definicje jêzyków (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Zast¹p wzorce"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Zast¹p mapy ustawieñ klawiatury (oprócz custom.keymap)"
