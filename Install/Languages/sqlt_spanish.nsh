;Language specific include file for SqlTools
;Translated by Oswaldo Enriquez
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_SPANISH}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\gpl_spanish.txt"

;Section names and comments
LangString DESC_SecCopyProgramFiles	${CURLANG} "Copiar SQLTools.exe y los otros componentes necesarios en el repertorio de la aplicacion."
LangString DESC_SecCreateMenuGroup	${CURLANG} "Crear un archivo SQLTools en el menu Inicio de Windows."
LangString DESC_SecCreateDesktopIcon	${CURLANG} "Crear un acceso directo SQLTools en el escritorio."
LangString DESC_SecQuickLaunchIcon	${CURLANG} "Crear un acceso directo SQLTools en la barra de lanzamiento rapido."
;
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (necesario)"
LangString DESC_SectionMenuName		${CURLANG} "Menu Inicio"
LangString DESC_SectionDesktopIconName	${CURLANG} "Icono en el escritorio"
LangString DESC_SectionQuickLaunchName	${CURLANG} "Icono en la barra de lanzamiento rapido"

;Upgrade dialog
LangString DESC_UpgradeTitle		${CURLANG} "Opciones de actualizacion"
LangString DESC_UpgradeText		${CURLANG} "Una instalacion de SQLTools existe en el repertorio seleccionado . $\nEscoger las opciones de actualizacion."
LangString DESC_WithoutUpgradeText	${CURLANG} "SQLTools no esta instalado en el repertorio seleccionado. $\nLas opciones de actualizacion no estan disponibles."

LangString DESC_UpgradeCheckboxBackup	${CURLANG} "Crear una copia de respaldo de todos los archivos de datos (Las copias de respaldo anteriores seran reemplazadas sin confirmacion)"
LangString DESC_UpgradeCheckboxSettings	${CURLANG} "Remplazar las opciones"
LangString DESC_UpgradeCheckboxLanguage	${CURLANG} "Remplazar las definiciones de lenguaje (PL/SQL,SQR,C++,...)"
LangString DESC_UpgradeCheckboxTempl	${CURLANG} "Remplazar los modelos"
LangString DESC_UpgradeCheckboxKeymap	${CURLANG} "Remplazar los accesos directos del teclado (excepto custom.keymap)"
