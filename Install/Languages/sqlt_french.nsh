; What is CeCILL and why we use it in the French translation?
; http://www.cecill.info/index.en.html
;

;Language specific include file for SqlTools
;Translated by Michel Traisnel
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_FRENCH}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\CeCILL_french.txt"

;Section names and comments
LangString DESC_SecCopyProgramFiles	${CURLANG} "Copier SQLTools.exe et les autres composants n�cessaires dans le r�pertoire de l'application."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Cr�er le groupe de menu SQLTools dans le menu Windows."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Cr�er un raccourci SQLTools sur le bureau."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Cr�er un raccourci de d�marrage rapide SQLTools."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (requis)"
LangString DESC_SectionMenuName		${CURLANG} "Groupe de menu"
LangString DESC_SectionDesktopIconName	${CURLANG} "Ic�ne de bureau"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Ic�ne de d�marrage rapide"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Options de mise � jour"
LangString DESC_UpgradeText		${CURLANG} "SQLTools est d�j� install� dans le r�pertoire s�lectionn�. $\nChoisir les options de mise � jour."
LangString DESC_WithoutUpgradeText	${CURLANG} "SQLTools n'est pas install� dans le r�pertoire s�lectionn�. $\nLes options de mise � jour ne sont pas disponibles."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Cr�er une copie de sauvegarde pour tous les fichiers de donn�es (les anciens fichiers de sauvegarde seront remplac�s sans confirmation)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Remplacer les options"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Remplacer les d�finitions de langage (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Remplacer les mod�les"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Remplacer la liste des touches de raccourci (except� custom.keymap)"