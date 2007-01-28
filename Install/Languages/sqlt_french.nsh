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
LangString DESC_SecCopyProgramFiles	${CURLANG} "Copier SQLTools.exe et les autres composants nécessaires dans le répertoire de l'application."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Créer le groupe de menu SQLTools dans le menu Windows."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Créer un raccourci SQLTools sur le bureau."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Créer un raccourci de démarrage rapide SQLTools."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (requis)"
LangString DESC_SectionMenuName		${CURLANG} "Groupe de menu"
LangString DESC_SectionDesktopIconName	${CURLANG} "Icône de bureau"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Icône de démarrage rapide"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Options de mise à jour"
LangString DESC_UpgradeText		${CURLANG} "SQLTools est déjà installé dans le répertoire sélectionné. $\nChoisir les options de mise à jour."
LangString DESC_WithoutUpgradeText	${CURLANG} "SQLTools n'est pas installé dans le répertoire sélectionné. $\nLes options de mise à jour ne sont pas disponibles."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Créer une copie de sauvegarde pour tous les fichiers de données (les anciens fichiers de sauvegarde seront remplacés sans confirmation)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Remplacer les options"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Remplacer les définitions de langage (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Remplacer les modèles"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Remplacer la liste des touches de raccourci (excepté custom.keymap)"