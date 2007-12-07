/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/
#pragma warning ( disable : 4430 )

#include "stdafx.h"
#include "SQLTools.h"
#include "MainFrm.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/ExceptionHelper.h"
#include "COMMON/VisualAttributesPage.h"
#include "COMMON/PropertySheetMem.h"
#include "Dlg/PropServerPage.h"
#include "Dlg/PropGridPage.h"
#include "Dlg/PropGridOutputPage.h"
#include "Dlg/DBSCommonPage.h"
#include "Dlg/DBSTablePage.h"
#include "Dlg/PropHistoryPage.h"
#include "Dlg/PlusPlusPage.h"
#include "OpenEditor/OEDocument.h"

/*
    31.03.2003 bug fix, redundant sqltools settings saving
*/
    
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    static bool g_isLoading = false;
    static bool g_isBackupDone = false;

void CSQLToolsApp::OnSettingsChanged ()
{
    try { EXCEPTION_FRAME;
    
        const SQLToolsSettings& settings = GetSQLToolsSettings();
        m_connect->EnableOutput(settings.GetOutputEnable(), settings.GetOutputSize());
        m_connect->SetNlsDateFormat(settings.GetDateFormat());
        m_connect->AlterSessionNlsParams();
    }
    _DEFAULT_HANDLER_

    if (!g_isLoading)
        SaveSettings();
}

void CSQLToolsApp::LoadSettings ()
{
    std::string path;
    Global::GetSettingsPath(path);
    SQLToolsSettingsReader(OpenEditor::FileInStream((path + "\\sqltools.dat").c_str())) >> m_settings;

    // 31.03.2003 bug fix, redundant sqltools settings saving
    g_isLoading = true;
    try 
    { 
        OnSettingsChanged(); 
    }
    catch (...) 
    { 
        g_isLoading = false; throw; 
    }
    g_isLoading = false;
}

void CSQLToolsApp::SaveSettings ()
{
    std::string path;
    Global::GetSettingsPath(path);

    if (!g_isBackupDone)
    {
        _CHECK_AND_THROW_(
                CopyFile((path + "\\sqltools.dat").c_str(), 
                    (path + "\\sqltools.dat.old").c_str(), FALSE) != 0,
                "Settings backup file creation error!"
            );
        g_isBackupDone = true;
    }


    SQLToolsSettingsWriter(OpenEditor::FileOutStream((path + "\\sqltools.dat").c_str())) << m_settings;
}


struct SettingsDialogCallback : COEDocument::SettingsDialogCallback
{
    SQLToolsSettings    m_settings;
    CPropServerPage     m_serverPage;
    CDBSCommonPage      m_commonPage;
    CDBSTablePage       m_tablePage;
    CPropGridPage1      m_gridPage1;
    CPropGridPage2      m_gridPage2;
    CPropGridOutputPage m_gridOutputPage;
    CPropHistoryPage    m_histPage;
    CVisualAttributesPage m_vaPage;
    CPlusPlusPage       m_plusPlusPage;
    bool                m_ok;

    SettingsDialogCallback (const SQLToolsSettings& settings)
        : m_settings(settings),
        m_serverPage(m_settings),
        m_commonPage(m_settings),
        m_tablePage(m_settings),
        m_gridPage1(m_settings),
        m_gridPage2(m_settings),
        m_gridOutputPage(m_settings),
        m_histPage(m_settings),
        m_plusPlusPage(m_settings),
        m_ok(false)
    {
        std::vector<VisualAttributesSet*> vasets;
        vasets.push_back(&m_settings.GetQueryVASet());
        vasets.push_back(&m_settings.GetStatVASet());
        vasets.push_back(&m_settings.GetHistoryVASet());
        vasets.push_back(&m_settings.GetOutputVASet());
        m_vaPage.Init(vasets);
        m_vaPage.m_psp.pszTitle = "SQLTools::Font and Color";
        m_vaPage.m_psp.dwFlags |= PSP_USETITLE;
    }

    virtual void BeforePageAdding (Common::CPropertySheetMem& sheet)
    {
        sheet.AddPage(&m_serverPage);
        sheet.AddPage(&m_commonPage);
        sheet.AddPage(&m_tablePage);
        sheet.AddPage(&m_gridPage1);
        sheet.AddPage(&m_gridPage2);
        sheet.AddPage(&m_gridOutputPage);
        sheet.AddPage(&m_histPage);
        sheet.AddPage(&m_vaPage);
        sheet.AddPage(&m_plusPlusPage);
    }

    virtual void AfterPageAdding  (Common::CPropertySheetMem& /*sheet*/)
    {
    }

    virtual void OnOk ()
    {
        m_ok = true;
    }

    virtual void OnCancel () 
    {
    }
};

void CSQLToolsApp::OnAppSettings ()
{
    SettingsDialogCallback callback(GetSQLToolsSettings());
    COEDocument::ShowSettingsDialog(&callback);
    
    if (callback.m_ok)
    {
        GetSQLToolsSettingsForUpdate() = callback.m_settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();

        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());
        UpdateAccelAndMenu();
        ((CMDIMainFrame*)m_pMainWnd)->SetCloseFileOnTabDblClick(
            COEDocument::GetSettingsManager().GetGlobalSettings()->GetDoubleClickCloseTab() ? TRUE : FALSE);

    }
}


bool ShowDDLPreferences (SQLToolsSettings& settings, bool bLocal)
{
    BOOL showOnShiftOnly = AfxGetApp()->GetProfileInt("showOnShiftOnly", "DDLPreferences",  FALSE);

    static UINT gStartPage = 0;
    Common::CPropertySheetMem sheet(bLocal ? "Local DDL Preferences" : "Global DDL Preferences", gStartPage);
    sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    if (!bLocal || !showOnShiftOnly || HIBYTE(GetKeyState(VK_SHIFT))) 
    {
        CDBSCommonPage  commonPage(settings);
        //commonPage.m_psp.pszTitle = "Common DDL";
        //commonPage.m_psp.dwFlags |= PSP_USETITLE;

        CDBSTablePage   tablePage(settings);
        //tablePage.m_psp.pszTitle = "Table Specific DDL";
        //tablePage.m_psp.dwFlags |= PSP_USETITLE;

        commonPage.m_bShowShowOnShiftOnly = TRUE;
        commonPage.m_bShowOnShiftOnly = showOnShiftOnly;

        sheet.AddPage(&commonPage);
        sheet.AddPage(&tablePage);
        
        if (sheet.DoModal() == IDOK) 
        {
            if (!bLocal)
            {
                GetSQLToolsSettingsForUpdate() = settings;
                GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
            }
            else
                AfxGetApp()->WriteProfileInt("showOnShiftOnly", "DDLPreferences",  commonPage.m_bShowOnShiftOnly);

            return true;
        } 
        else
            return false;
    } 
    else
        return true;
}
