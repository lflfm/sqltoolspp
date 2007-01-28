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
LangString DESC_SecCopyProgramFiles      ${CURLANG} "Copiar SQLTools.exe e outros componentes necessários para o diretório da aplicaçao."  
LangString DESC_SecCreateMenuGroup      ${CURLANG} "Criar o grupo SQLTools no menu do Windows."  
LangString DESC_SecCreateDesktopIcon      ${CURLANG} "Criar atalho do SQLTools na Área de Trabalho."  
LangString DESC_SecQuickLaunchIcon      ${CURLANG} "Criar acesso rápido do SQLTools."  
;  
LangString DESC_SectionProgramFilesName ${CURLANG} "SQLTools (necessário)"  
LangString DESC_SectionMenuName            ${CURLANG} "Grupo de Menu"  
LangString DESC_SectionDesktopIconName      ${CURLANG} "Ícone da Área de Trabalho"  
LangString DESC_SectionQuickLaunchName      ${CURLANG} "Ícone de Acesso Rápido"  
 
;Upgrade dialog  
LangString DESC_UpgradeTitle            ${CURLANG} "Opçoes de Atualizaçao"  
LangString DESC_UpgradeText            ${CURLANG} "Uma instalaçao do SQLTools já existe no diretório selecionado. $\nSelecione opçoes de atualizaçao."  
LangString DESC_WithoutUpgradeText      ${CURLANG} "SQLTools nao está instalado no diretório selecionado. $\nOpçoes de atualizaçao nao estao disponíveis."  
 
LangString DESC_UpgradeCheckboxBackup      ${CURLANG} "Criar cópia de todos os arquivos de dados (as cópias anteriores serao substituídas sem confirmaçao)"  
LangString DESC_UpgradeCheckboxSettings      ${CURLANG} "Substituir Opçoes"  
LangString DESC_UpgradeCheckboxLanguage      ${CURLANG} "Substituir definiçoes de linguagem (PL/SQL,SQR,C++,...)"  
LangString DESC_UpgradeCheckboxTempl      ${CURLANG} "Substituir modelos"  
LangString DESC_UpgradeCheckboxKeymap      ${CURLANG} "Substituir acessos diretos do teclado (exceto custom.keymap)"  