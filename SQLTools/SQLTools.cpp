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

#include "stdafx.h"
#include "SQLTools.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/ExceptionHelper.h"
#include "FileWatch/FileWatch.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"
#include "Dlg/AboutDlg.h"
#include "OpenEditor/OEGeneralPage.h"
#include "CommandLineparser.h"
#include "VersionInfo.rh"
#include <COMMON/OsInfo.h>
#include <COMMON/ExceptionHelper.h>
#include "COMMON/NVector.h"
#include "COMMON/GUICommandDictionary.h"

/*
    30.03.2003 improvement, command line support, try sqltools /h to get help
    02.06.2003 bug fix, unknown exception if Oracle client is not installed or included in PATH
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp
const UINT CSQLToolsApp::m_msgCommandLineProcess = WM_USER+1;

SQLToolsSettings CSQLToolsApp::m_settings;

IMPLEMENT_DYNCREATE(CSQLToolsApp, CWinApp)

BEGIN_MESSAGE_MAP(CSQLToolsApp, CWinApp)
  //{{AFX_MSG_MAP(CSQLToolsApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_EDIT_PERMANENT_SETTINGS, OnEditPermanetSettings)
	ON_COMMAND(ID_APP_SETTINGS, OnAppSettings)
	ON_COMMAND(ID_SQL_CONNECT, OnSqlConnect)
	ON_COMMAND(ID_SQL_COMMIT, OnSqlCommit)
	ON_COMMAND(ID_SQL_ROLLBACK, OnSqlRollback)
	ON_COMMAND(ID_SQL_DISCONNECT, OnSqlDisconnect)
	ON_COMMAND(ID_SQL_DBMS_OUTPUT, OnSqlDbmsOutput)
	ON_COMMAND(ID_SQL_SUBSTITUTION_VARIABLES, OnSqlSubstitutionVariables)
    ON_UPDATE_COMMAND_UI(ID_SQL_SUBSTITUTION_VARIABLES, OnUpdate_SqlSubstitutionVariables)
	ON_COMMAND(ID_SQL_HELP, OnSqlHelp)
	ON_COMMAND(ID_SQL_SESSION_STATISTICS, OnSqlSessionStatistics)
	ON_COMMAND(ID_SQL_DBMS_XPLAN_DISPLAY_CURSOR, OnSqlDbmsXplanDisplayCursor)
	ON_COMMAND(ID_SQL_EXTRACT_SCHEMA, OnSqlExtractSchema)
	ON_COMMAND(ID_SQL_TABLE_TRANSFORMER, OnSqlTableTransformer)
	ON_COMMAND(ID_SQLTOOLS_ON_THE_WEB, OnSQLToolsOnTheWeb)
	//}}AFX_MSG_MAP
    ON_UPDATE_COMMAND_UI_RANGE(ID_SQL_DISCONNECT, ID_SQL_TABLE_TRANSFORMER, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_SQL_DBMS_XPLAN_DISPLAY_CURSOR, OnUpdate_SqlGroup)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// File based document commands
    ON_COMMAND(ID_FILE_CLOSE_ALL, OnFileCloseAll)
    ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdate_FileSaveAll)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
    ON_COMMAND_RANGE(ID_APP_DBLKEYACCEL_FIRST, ID_APP_DBLKEYACCEL_LAST, OnDblKeyAccel)

	ON_UPDATE_COMMAND_UI(ID_INDICATOR_POS,        OnUpdate_EditIndicators)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCROLL_POS, OnUpdate_EditIndicators)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_EditIndicators)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR,        OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_BLOCK_TYPE, OnUpdate_EditIndicators)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_LINES, OnUpdate_EditIndicators)
    ON_THREAD_MESSAGE(CFileWatch::m_msgFileWatchNotify, OnFileWatchNotify)
    ON_THREAD_MESSAGE(m_msgCommandLineProcess, OnCommandLineProcess)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp construction

    CAbortThread* CSQLToolsApp::m_pAbortThread = NULL;
    CString CSQLToolsApp::m_displayNotConnected = "Not connected";

CSQLToolsApp::CSQLToolsApp()
{
    m_hMutex = NULL;
    m_dblKeyAccelInx = -1;
    m_accelTable = NULL;
    m_pDocManager = new CDocManagerExt;

#if _MFC_VER > 0x0600
	EnableHtmlHelp();
#endif

    COEGeneralPage::m_keymapLayoutList = "Custom;Default;Default(MS);";

    m_displayConnectionString = m_displayNotConnected;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSQLToolsApp object

CSQLToolsApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp initialization

    static void AppInfo (std::string& name, std::string& info, std::string& filePrefix)
    {
        name = SQLTOOLS_VERSION;
        info = string("OCI.DLL: ") + getDllVersionProperty("OCI.DLL", "FileVersion");
        if (!theApp.m_serverVerion.IsEmpty())
        {
            info += "\nConnected to: ";
            info += (LPCSTR)theApp.m_serverVerion;
        }
        filePrefix = "SQLTools";
    }

BOOL CSQLToolsApp::InitInstance()
{
#if 0
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
                  |_CRTDBG_DELAY_FREE_MEM_DF
                  |_CRTDBG_CHECK_ALWAYS_DF
                  |_CRTDBG_CHECK_CRT_DF
                  |_CRTDBG_LEAK_CHECK_DF);
#endif

    try { EXCEPTION_FRAME;

        Common::SetAppInfoFn(AppInfo);
        SEException::InstallSETranslator();
        set_terminate(Common::terminate);

	    if (!AfxOleInit()) { AfxMessageBox("OLE Initialization failed!"); return FALSE; }

        m_connect = std::auto_ptr<OciConnect>(new OciConnect());
        SetHandler<CSQLToolsApp, OciConnect>(m_connect->m_evAfterOpen,   *this, &CSQLToolsApp::EvAfterOpenConnect);
        SetHandler<CSQLToolsApp, OciConnect>(m_connect->m_evBeforeClose, *this, &CSQLToolsApp::EvBeforeCloseConnect);
        SetHandler<CSQLToolsApp, OciConnect>(m_connect->m_evAfterClose,  *this, &CSQLToolsApp::EvAfterCloseConnect);

        m_settings.AddSubscriber(this);
	    
        SetRegistryKey("KochWare");
	    LoadStdProfileSettings(10);
        LoadSettings();
        COEDocument::LoadSettingsManager(); // Load editor settings

        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());

        if (!m_commandLineParser.Parse())
            return FALSE;

        if (!AllowThisInstance())
		    return FALSE;

        m_commandLineParser.SetStartingDefaults();

	    m_pPLSDocTemplate = new CMultiDocTemplateExt(
		    IDR_SQLTYPE,
		    RUNTIME_CLASS(CPLSWorksheetDoc),
		    RUNTIME_CLASS(CMDIChildFrame),
		    RUNTIME_CLASS(COEditorView));
	    AddDocTemplate(m_pPLSDocTemplate);

        m_orgMainWndTitle.LoadString(IDR_MAINFRAME);
	    // create main MDI Frame window
		InitGUICommand();
		string buffer;
		Global::GetSettingsPath(buffer);
		GUICommandDictionary::BuildAcceleratorTable(
			(buffer + '\\' + COEDocument::GetSettingsManager().GetGlobalSettings()->GetKeymapLayout() + ".keymap").c_str(),
			m_accelTable);

	    CMDIMainFrame* pMainFrame = new CMDIMainFrame;
        pMainFrame->m_bSaveMainWinPosition
            = COEDocument::GetSettingsManager().GetGlobalSettings()->GetSaveMainWinPosition();
        CMDIChildFrame::m_MaximizeFirstDocument
            = COEDocument::GetSettingsManager().GetGlobalSettings()->GetMaximizeFirstDocument();

	    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		    return FALSE;
        
        pMainFrame->GetConnectionBar().SetConnectionDescription(m_displayConnectionString, RGB(0,0,0));

        m_pMainWnd = pMainFrame;
        m_pMainWnd->DragAcceptFiles();     // Enable drag/drop open
        EnableShellOpen();                 // Enable DDE Execute open
        UpdateAccelAndMenu();
        ((CMDIMainFrame*)m_pMainWnd)->SetCloseFileOnTabDblClick(
            COEDocument::GetSettingsManager().GetGlobalSettings()->GetDoubleClickCloseTab() ? TRUE : FALSE);

	    //RegisterShellFileTypes(TRUE);    // NOT implemented yet in CDocManagerExt

        PostThreadMessage(m_msgCommandLineProcess, 0, 0);

	    m_pMainWnd->ShowWindow(m_nCmdShow); // The main window has been
	    m_pMainWnd->UpdateWindow();         // initialized, so show and update it.

        //// 02.06.2003 bug fix, unknown exception if Oracle client is not installed or included in PATH
        //char ociDllPath[MAX_PATH+1];
        //_CHECK_AND_THROW_(PathFindOnPath(strcpy(ociDllPath, "oci.dll"), NULL),
        //    "Cannon find OCI.DLL, an essential part of Oracle NET 8i/9i .\n"
        //    "Check Oracle client installation and system/user PATH variable.\n"
        //    );

        m_pAbortThread =
            (CAbortThread*)AfxBeginThread(RUNTIME_CLASS(CAbortThread),
                                        THREAD_PRIORITY_ABOVE_NORMAL/*THREAD_PRIORITY_LOWEST*/);

        if (!m_pAbortThread)
		    AfxMessageBox("Background Thread Initialization failed!");

//try { char* ptr = 0; ptr[0] = 1; }
//catch (const std::exception& x) { DEFAULT_HANDLER(x); }
//try { nvector<int> v("local_v"); v.at(100); }
//catch (const std::exception& x) { DEFAULT_HANDLER(x); }
    }
    catch (CException* x)           { DEFAULT_HANDLER(x);   return FALSE; }
    catch (const std::exception& x) { DEFAULT_HANDLER(x);   return FALSE; }
    catch (...)                     { DEFAULT_HANDLER_ALL;  return FALSE; }

    if (m_hMutex)
        ReleaseMutex(m_hMutex);

    return TRUE;
}

int CSQLToolsApp::ExitInstance()
{
    if (m_pMainWnd)
	    DestroyAcceleratorTable(((CMDIMainFrame*)m_pMainWnd)->m_hAccelTable); // there is a better way

   if (m_hMutex)
       CloseHandle(m_hMutex);

    try { EXCEPTION_FRAME;
    
        if (m_connect.get())
	        m_connect->Close();
    }
    _DEFAULT_HANDLER_

    // I've used as an example a source code from:
    // http://hepg.sdu.edu.cn/zhangxueyao/root/html/TWin32Application_8cxx-source.html
    // http://bugs.php.net/bug.php?id=35612
    if (m_pAbortThread) {
        m_pAbortThread->PostThreadMessage(WM_QUIT, 0, 0);
        if(WaitForSingleObject(*m_pAbortThread, 1000) == WAIT_FAILED) {
            TerminateThread(m_pAbortThread, -1);// extremity
        }
    }

    m_settings.RemoveSubscriber(this);
    int retVal = CWinApp::ExitInstance();
    SEException::UninstallSETranslator();

	return retVal;
}

/////////////////////////////////////////////////////////////////////////////
// CSQLToolsApp commands
// App command to run the dialog

void CSQLToolsApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CSQLToolsApp::OnEditPermanetSettings()
{
    try { EXCEPTION_FRAME;

        COEDocument::ShowSettingsDialog();
        setlocale(LC_ALL, COEDocument::GetSettingsManager().GetGlobalSettings()->GetLocale().c_str());
        UpdateAccelAndMenu();
        ((CMDIMainFrame*)m_pMainWnd)->SetCloseFileOnTabDblClick(
            COEDocument::GetSettingsManager().GetGlobalSettings()->GetDoubleClickCloseTab() ? TRUE : FALSE);
    }
    _DEFAULT_HANDLER_;
}

void CSQLToolsApp::OnSqlHelp()
{
#if _MFC_VER <= 0x0600
#pragma message("There is no html help support for VC6!")
#else//_MFC_VER > 0x0600
    std::string path;
    AppGetPath(path);
    ::HtmlHelp(*AfxGetMainWnd(), (path + "\\sqlqkref.chm").c_str(), HH_DISPLAY_TOPIC, 0);
#endif
}

void CSQLToolsApp::OnSQLToolsOnTheWeb()
{
    HINSTANCE result = ShellExecute( NULL, "open", "http://www.sqltools-plusplus.org:7676", NULL, NULL, SW_SHOW);

    if((UINT)result <= HINSTANCE_ERROR)
    {
        MessageBeep((UINT)-1);
        AfxMessageBox("Cannot open a default browser.");
    }
}

void CSQLToolsApp::OnUpdate_EditIndicators (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CSQLToolsApp::OnFileWatchNotify (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;

        TRACE("CSQLToolsApp::OnFileWatchNotify\n");
        CFileWatch::NotifyClients();
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::OnActivateApp (BOOL bActive)
{
    if (!bActive && COEDocument::GetSettingsManager().GetGlobalSettings()->GetFileSaveOnSwith())
        DoFileSaveAll(/*silent*/true, /*skipNew*/true);

    if (bActive)
        CFileWatch::ResumeThread();
    else
        CFileWatch::SuspendThread();
}

void CSQLToolsApp::OnFileCloseAll ()
{
	if (m_pDocManager != NULL && m_pDocManager->IsKindOf(RUNTIME_CLASS(CDocManagerExt)))
		((CDocManagerExt*)m_pDocManager)->SaveAndCloseAllDocuments();
}

void CSQLToolsApp::DoFileSaveAll (bool silent, bool skipNew)
{
    bool _silent  = COEDocument::GetSaveModifiedSilent();
    bool _skipNew = COEDocument::GetSaveModifiedSkipNew();
    try
    {
        COEDocument::SetSaveModifiedSilent(silent);
        COEDocument::SetSaveModifiedSkipNew(skipNew);
        SaveAllModified();
        COEDocument::SetSaveModifiedSilent(_silent);
        COEDocument::SetSaveModifiedSkipNew(_skipNew);
    }
    catch (...)
    {
        COEDocument::SetSaveModifiedSilent(_silent);
        COEDocument::SetSaveModifiedSkipNew(_skipNew);
        throw;
    }
}

void CSQLToolsApp::OnFileSaveAll ()
{
    DoFileSaveAll(/*silent*/true, /*skipNew*/false);
}

void CSQLToolsApp::OnUpdate_FileSaveAll (CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pDocManager != NULL && m_pDocManager->GetOpenDocumentCount());
}


BOOL CSQLToolsApp::OnIdle (LONG lCount)
{
    BOOL more = FALSE;

    try { EXCEPTION_FRAME;

        more = CWinApp::OnIdle(lCount);

#pragma message ("\tOnIdle requires timerot processing (with time quotes)!!!")
//        TRACE("CSQLToolsApp::OnIdle more = %d\n", more);
//        if (m_pPLSDocTemplate && lCount >= 1)
//		    more |= m_pPLSDocTemplate->OnIdle(lCount);
    }
    catch (...) { /*it's silent*/ }

    return more;
}
