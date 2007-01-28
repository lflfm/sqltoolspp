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
LangString DESC_SecCopyProgramFiles	${CURLANG} "Az SQLTools.exe �s egy�b sz�ks�ges komponensek m�sol�sa az alkalmaz�s k�nyvt�r�ba."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Az SQLTools men�csoport l�trehoz�sa a Windows men�ben."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "SQLTools parancsikon l�trehoz�sa az Asztalon."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "SQLTools parancsikon l�trehoz�sa a Gyorsind�t�s eszk�zt�rban."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (k�telez�)"
LangString DESC_SectionMenuName		${CURLANG} "Men� csoport"
LangString DESC_SectionDesktopIconName	${CURLANG} "Ikon az Asztalon"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Ikon a Gyorsind�t�s eszk�zt�rban"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Friss�t�si opci�k"
LangString DESC_UpgradeText		${CURLANG} "A telep�t� m�r tal�lt egy install�lt SQLTools p�ld�nyt a kiv�lasztott k�nyvt�rban. $\nJel�lje meg a friss�t�si opci�kat."
LangString DESC_WithoutUpgradeText	${CURLANG} "A telep�t� nem tal�lt install�lt SQLTools p�ld�nyt a kiv�lasztott k�nyvt�rban. $\nA friss�t�si opci�k nem el�rhet�k."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Biztons?0gi m�solat k�sz�t�se minden adatf�jlr�l (a r�gi biztons�gi ment�sek automatikusan fel�l�r�dnak)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Be�ll�t�sok lecser�l�se"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Nyelvi definici�k lecser�l�se (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Template-ek lecser�l�se"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Billenty�-lek�pez�sek lecser�l�se (custom.keymap kiv�tel�vel)"
