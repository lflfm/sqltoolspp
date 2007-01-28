;Language specific include file for SqlTools
;Translated by Tamas Gati
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_HUNGARIAN}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\gpl_hungarian.txt"

;Section names and comments
LangString DESC_SecCopyProgramFiles	${CURLANG} "Az SQLTools.exe és egyéb szükséges komponensek másolása az alkalmazás könyvtárába."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Az SQLTools menücsoport létrehozása a Windows menüben."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "SQLTools parancsikon létrehozása az Asztalon."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "SQLTools parancsikon létrehozása a Gyorsindítás eszköztárban."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (kötelezô)"
LangString DESC_SectionMenuName		${CURLANG} "Menü csoport"
LangString DESC_SectionDesktopIconName	${CURLANG} "Ikon az Asztalon"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Ikon a Gyorsindítás eszköztárban"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Frissítési opciók"
LangString DESC_UpgradeText		${CURLANG} "A telepítô már talált egy installált SQLTools példányt a kiválasztott könyvtárban. $\nJelölje meg a frissítési opciókat."
LangString DESC_WithoutUpgradeText	${CURLANG} "A telepítô nem talált installált SQLTools példányt a kiválasztott könyvtárban. $\nA frissítési opciók nem elérhetök."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Biztons?0gi másolat készítése minden adatfájlról (a régi biztonsági mentések automatikusan felülíródnak)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Beállítások lecserélése"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Nyelvi definiciók lecserélése (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Template-ek lecserélése"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Billentyû-leképezések lecserélése (custom.keymap kivételével)"
