;Language specific include file for SqlTools
;Translated by Alexandre Tarquino
;

!ifdef CURLANG
  !undef CURLANG
!endif
!define CURLANG ${LANG_PORTUGUESEBR}

;License dialog
LicenseLangString MUILicense ${CURLANG} "Languages\gpl_portuguese_br.txt"

;Section names and comments  
LangString DESC_SecCopyProgramFiles      ${CURLANG} "Copiar SQLTools.exe e outros componentes necess�rios para o diret�rio da aplica�ao."  
LangString DESC_SecCreateMenuGroup      ${CURLANG} "Criar o grupo SQLTools no menu do Windows."  
LangString DESC_SecCreateDesktopIcon      ${CURLANG} "Criar atalho do SQLTools na �rea de Trabalho."  
LangString DESC_SecQuickLaunchIcon      ${CURLANG} "Criar acesso r�pido do SQLTools."  
;  
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (necess�rio)"  
LangString DESC_SectionMenuName            ${CURLANG} "Grupo de Menu"  
LangString DESC_SectionDesktopIconName      ${CURLANG} "�cone da �rea de Trabalho"  
LangString DESC_SectionQuickLaunchName      ${CURLANG} "�cone de Acesso R�pido"  
 
;Upgrade dialog  
LangString DESC_UpgradeTitle            ${CURLANG} "Op�oes de Atualiza�ao"  
LangString DESC_UpgradeText            ${CURLANG} "Uma instala�ao do SQLTools j� existe no diret�rio selecionado. $\nSelecione op�oes de atualiza�ao."  
LangString DESC_WithoutUpgradeText      ${CURLANG} "SQLTools nao est� instalado no diret�rio selecionado. $\nOp�oes de atualiza�ao nao estao dispon�veis."  
 
LangString DESC_UpgradeCheckboxBackup      ${CURLANG} "Criar c�pia de todos os arquivos de dados (as c�pias anteriores serao substitu�das sem confirma�ao)"  
LangString DESC_UpgradeCheckboxSettings      ${CURLANG} "Substituir Op�oes"  
LangString DESC_UpgradeCheckboxLanguage      ${CURLANG} "Substituir defini�oes de linguagem (PL/SQL,SQR,C++,...)"  
LangString DESC_UpgradeCheckboxTempl      ${CURLANG} "Substituir modelos"  
LangString DESC_UpgradeCheckboxKeymap      ${CURLANG} "Substituir acessos diretos do teclado (exceto custom.keymap)"  