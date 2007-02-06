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
#include "COMMON/AppGlobal.h"
#include "COMMON/ExceptionHelper.h"
#include "COMMON/GUICommandDictionary.h"
#include "SQLTools.h"
#include "MainFrm.h"
#include "Tools/Grep/GrepDlg.h"

/*
    22.03.2003 bug fix, checking for changes works even the program is inactive - checking only on activation now
    23.03.2003 bug fix, "Find in Files" does not show the output automatically
    30.03.2003 bug fix, updated a control bars initial state
    30.03.2003 improvement, command line support, try sqltools /h to get help
    16.11.2003 bug fix, shortcut F1 has been disabled until sqltools help can be worked out 
    20.4.2005 bug fix, (ak) changing style after toolbar creation is a workaround for toolbar background artifacts
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    LPCSTR CMDIMainFrame::m_cszClassName = "Kochware.SQLTools.MainFrame";
    static const char lpszProfileName[] = "Workspace_1.4.1\\";

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame

IMPLEMENT_DYNAMIC(CMDIMainFrame, CWorkbookMDIFrame)

BEGIN_MESSAGE_MAP(CMDIMainFrame, CWorkbookMDIFrame)
	//{{AFX_MSG_MAP(CMDIMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_SQL_OBJ_VIEWER, OnSqlObjViewer)
	ON_UPDATE_COMMAND_UI(ID_SQL_OBJ_VIEWER, OnUpdate_SqlObjViewer)
//	ON_COMMAND(ID_SQL_PLAN_VIEWER, OnSqlPlanViewer)
//	ON_UPDATE_COMMAND_UI(ID_SQL_PLAN_VIEWER, OnUpdate_SqlPlanViewer)
	ON_COMMAND(ID_SQL_DB_SOURCE, OnSqlDbSource)
	ON_UPDATE_COMMAND_UI(ID_SQL_DB_SOURCE, OnUpdate_SqlDbSource)
	ON_COMMAND(ID_FILE_FIND_IN_FILE, OnFileGrep)
	ON_COMMAND(ID_FILE_SHOW_GREP_OUTPUT, OnFileShowGrepOutput)
	ON_UPDATE_COMMAND_UI(ID_FILE_SHOW_GREP_OUTPUT, OnUpdate_FileShowGrepOutput)
	ON_UPDATE_COMMAND_UI(ID_FILE_FIND_IN_FILE, OnUpdate_FileGrep)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_VIEW_FILE_TOOLBAR, OnViewFileToolbar)
	ON_COMMAND(ID_VIEW_SQL_TOOLBAR, OnViewSqlToolbar)
	ON_COMMAND(ID_VIEW_CONNECT_TOOLBAR, OnViewConnectToolbar)
	ON_COMMAND(IDC_DS_COPY, OnObjectListCopy)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FILE_TOOLBAR, OnUpdate_FileToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SQL_TOOLBAR, OnUpdate_SqlToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONNECT_TOOLBAR, OnUpdate_ConnectToolbar)
	ON_NOTIFY(TCN_SELCHANGE, IDC_WORKBOOK_TAB, OnChangeWorkbookTab)
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CWorkbookMDIFrame::OnHelpFinder)
// 16.11.2003 bug fix, shortcut F1 has been disabled until sqltools help can be worked out 
//	ON_COMMAND(ID_HELP, CWorkbookMDIFrame::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CWorkbookMDIFrame::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CWorkbookMDIFrame::OnHelpFinder)
    ON_WM_COPYDATA()
    ON_WM_ACTIVATEAPP()   // 22.03.2003 bug fix, checking for changes works even the program is inactive - checking only on activation now
//    ON_WM_SIZE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_OCIGRID,
    ID_INDICATOR_FILE_TYPE,
    ID_INDICATOR_BLOCK_TYPE,
//    ID_INDICATOR_FILE_LINES,
    ID_INDICATOR_POS,
//    ID_INDICATOR_SCROLL_POS,
    ID_INDICATOR_OVR,
};

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame construction/destruction

CMDIMainFrame::CMDIMainFrame()
: m_wndDbSourceWnd(((CSQLToolsApp*)AfxGetApp())->GetConnect())
{
    m_cszProfileName = lpszProfileName;
}

CMDIMainFrame::~CMDIMainFrame()
{
}

BOOL CMDIMainFrame::PreCreateWindow (CREATESTRUCT& cs)
{
    WNDCLASS wndClass;
    BOOL bRes = CWorkbookMDIFrame::PreCreateWindow(cs);
    HINSTANCE hInstance = AfxGetInstanceHandle();

    // see if the class already exists
    if (!::GetClassInfo(hInstance, m_cszClassName, &wndClass))
    {
        // get default stuff
        ::GetClassInfo(hInstance, cs.lpszClass, &wndClass);
        wndClass.lpszClassName = m_cszClassName;
        wndClass.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        // register a new class
        if (!AfxRegisterClass(&wndClass))
            AfxThrowResourceException();
    }
    cs.lpszClass = m_cszClassName;

    return bRes;
}

const int IDC_MF_DBSOURCE_BAR  = (AFX_IDW_CONTROLBAR_LAST - 10);
const int IDC_MF_DBTREE_BAR    = (AFX_IDW_CONTROLBAR_LAST - 11);
const int IDC_MF_DBPLAN_BAR    = (AFX_IDW_CONTROLBAR_LAST - 12);
const int IDC_MF_GREP_BAR      = (AFX_IDW_CONTROLBAR_LAST - 13);

int CMDIMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWorkbookMDIFrame::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndFileToolBar.CreateEx(this, 0,
                                  //WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY ,
                                  WS_CHILD | WS_VISIBLE | CBRS_GRIPPER | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
                                  CRect(1, 1, 1, 1),
                                  AFX_IDW_CONTROLBAR_FIRST - 1)
    || !m_wndFileToolBar.LoadToolBar(IDT_FILE))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }
    // 20.4.2005 bug fix, (ak) changing style after toolbar creation is a workaround for toolbar background artifacts
    m_wndFileToolBar.ModifyStyle(0, TBSTYLE_FLAT);
    m_wndFileToolBar.EnableDocking(CBRS_ALIGN_ANY);

    if (!m_wndSqlToolBar.CreateEx(this, 0,
                                  //WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY ,
                                  WS_CHILD | WS_VISIBLE | CBRS_GRIPPER | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
                                  CRect(1, 1, 1, 1),
                                  AFX_IDW_CONTROLBAR_FIRST - 2)
    || !m_wndSqlToolBar.LoadToolBar(IDT_SQL))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }
    m_wndSqlToolBar.ModifyStyle(0, TBSTYLE_FLAT);
    m_wndSqlToolBar.EnableDocking(CBRS_ALIGN_ANY);

    if (!m_wndConnectionBar.Create(this, AFX_IDW_CONTROLBAR_FIRST - 3))
    {
		TRACE0("Failed to create tool bar\n");
        return -1;
    }

    if (!m_wndStatusBar.Create(this)
    || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;
	}

	EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndFileToolBar);
    DockControlBar(&m_wndSqlToolBar);
    DockControlBar(&m_wndConnectionBar);

    if (!m_wndObjectViewerFrame.Create("Quick Viewer", this, CSize(200, 300), TRUE, IDC_MF_DBTREE_BAR)
    || !m_wndObjectViewer.Create(&m_wndObjectViewerFrame))
    {
		TRACE("Failed to create Quick Viewer\n");
		return -1;
    }
    m_wndObjectViewer.LoadAndSetImageList(IDB_SQL_GENERAL_IMAGELIST);
    //m_wndObjectViewer.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndObjectViewerFrame, FALSE, FALSE);
    m_wndObjectViewerFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndObjectViewerFrame.SetSCBStyle(m_wndObjectViewerFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndObjectViewerFrame.SetBarStyle(m_wndObjectViewerFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndObjectViewerFrame.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndObjectViewerFrame, AFX_IDW_DOCKBAR_RIGHT);

    if (!m_wndPlanFrame.Create("Explain Plan", this, CSize(600, 200), TRUE, IDC_MF_DBPLAN_BAR)
    || !m_wndPlanViewer.Create(&m_wndPlanFrame))
    {
		TRACE("Failed to create Explain Plan Viewer\n");
		return -1;
    }
    m_wndPlanViewer.LoadAndSetImageList(IDB_SQL_GENERAL_IMAGELIST);
    m_wndPlanViewer.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndPlanFrame, FALSE, FALSE);
    m_wndPlanFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndPlanFrame.SetSCBStyle(m_wndPlanFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndPlanFrame.SetBarStyle(m_wndPlanFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndPlanFrame.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndPlanFrame, AFX_IDW_DOCKBAR_BOTTOM);

    if (!m_wndDbSourceFrame.Create("Objects List", this, CSize(680, 400), TRUE, IDC_MF_DBSOURCE_BAR)
    || !m_wndDbSourceWnd.Create(&m_wndDbSourceFrame))
    {
		TRACE("Failed to create DB Objects Viewer\n");
		return -1;
    }
    ShowControlBar(&m_wndDbSourceFrame, FALSE, FALSE);
    m_wndDbSourceFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndDbSourceFrame.SetSCBStyle(m_wndDbSourceFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndDbSourceFrame.SetBarStyle(m_wndDbSourceFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndDbSourceFrame.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	DockControlBar(&m_wndDbSourceFrame, AFX_IDW_DOCKBAR_LEFT);

    if (!m_wndGrepFrame.Create("Find in Files", this, CSize(600, 200), TRUE, IDC_MF_GREP_BAR)
    || !m_wndGrepViewer.Create(&m_wndGrepFrame)) {
		TRACE("Failed to create Grep Viewer\n");
		return -1;
    }
    m_wndGrepViewer.ModifyStyleEx(0, WS_EX_CLIENTEDGE);

    ShowControlBar(&m_wndGrepFrame, FALSE, FALSE);
    m_wndGrepFrame.ModifyStyle(0, WS_CLIPCHILDREN, 0);
	m_wndGrepFrame.SetSCBStyle(m_wndGrepFrame.GetSCBStyle()|SCBS_SIZECHILD);
    m_wndGrepFrame.SetBarStyle(m_wndGrepFrame.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC);
	m_wndGrepFrame.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndGrepFrame, AFX_IDW_DOCKBAR_BOTTOM);

    if (CWorkbookMDIFrame::DoCreate(FALSE) == -1)
    {
        TRACE0("CMDIMainFrame::OnCreate: Failed to create CWorkbookMDIFrame\n");
        return -1;
    }

    //DockControlBarLeftOf(&m_wndFilePanelBar, &m_wndObjectViewerFrame);
    ShowControlBar(&m_wndFilePanelBar, FALSE, FALSE);

    CWorkbookMDIFrame::LoadBarState();

    // 30.03.2003 bug fix, updated a control bars initial state
    ShowControlBar(&m_wndDbSourceFrame, m_wndDbSourceFrame.IsVisible(), FALSE);
    ShowControlBar(&m_wndObjectViewerFrame    , m_wndObjectViewerFrame.IsVisible()    , FALSE);
    ShowControlBar(&m_wndPlanFrame    , m_wndPlanFrame.IsVisible()    , FALSE);
    ShowControlBar(&m_wndGrepFrame    , m_wndGrepFrame.IsVisible()    , FALSE);

    Global::SetStatusHwnd(m_wndStatusBar.m_hWnd);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMDIMainFrame message handlers

void CMDIMainFrame::OnSqlObjViewer_Public()
{
    OnSqlObjViewer();
}

void CMDIMainFrame::OnSqlObjViewer()
{
    ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsVisible(), FALSE);
}

void CMDIMainFrame::OnUpdate_SqlObjViewer(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndObjectViewerFrame.IsWindowVisible());
}

void CMDIMainFrame::OnSqlPlanViewer()
{
    ShowControlBar(&m_wndPlanFrame, !m_wndPlanFrame.IsVisible(), FALSE);
}

void CMDIMainFrame::OnUpdate_SqlPlanViewer(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPlanFrame.IsWindowVisible());
}

void CMDIMainFrame::OnSqlDbSource()
{
    ShowControlBar(&m_wndDbSourceFrame, !m_wndDbSourceFrame.IsVisible(), FALSE);
}

void CMDIMainFrame::OnUpdate_SqlDbSource(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndDbSourceFrame.IsWindowVisible());
}

void CMDIMainFrame::OnFileGrep ()
{
    if (m_wndGrepViewer.IsRunning()
    && AfxMessageBox("Abort already running grep process?", MB_ICONQUESTION|MB_YESNO) == IDYES)
        m_wndGrepViewer.AbortProcess();

    if (!m_wndGrepViewer.IsRunning()
    && CGrepDlg(this, &m_wndGrepViewer).DoModal() == IDOK)
       ShowControlBar(&m_wndGrepFrame, TRUE, FALSE); // 23.03.2003 bug fix, "Find in Files" does not show the output automatically
}

void CMDIMainFrame::OnUpdate_FileGrep (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndGrepViewer.QuickIsRunning());
}

void CMDIMainFrame::OnFileShowGrepOutput()
{
    ShowControlBar(&m_wndGrepFrame, !m_wndGrepFrame.IsVisible(), FALSE);
}

void CMDIMainFrame::OnUpdate_FileShowGrepOutput(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndGrepFrame.IsWindowVisible());
}

void CMDIMainFrame::OnViewFileToolbar ()
{
    ShowControlBar(&m_wndFileToolBar, !m_wndFileToolBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnViewSqlToolbar ()
{
    ShowControlBar(&m_wndSqlToolBar, !m_wndSqlToolBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnViewConnectToolbar ()
{
    ShowControlBar(&m_wndConnectionBar, !m_wndConnectionBar.IsVisible(), FALSE);
}

void CMDIMainFrame::OnObjectListCopy ()
{
	m_wndDbSourceWnd.OnCopy_Public();
}

void CMDIMainFrame::OnUpdate_FileToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndFileToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_SqlToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndSqlToolBar.IsVisible());
}

void CMDIMainFrame::OnUpdate_ConnectToolbar (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndConnectionBar.IsVisible());
}

#define MAX_TIT_LEN 48
    static void append_striped_string (CString& outStr, const char* inStr)
    {
        for (int i(0); i < MAX_TIT_LEN && inStr && *inStr && *inStr != '\n'; i++, inStr++) {
            if (*inStr == '\t') outStr += ' ';
            else                outStr += *inStr;
        }
        if (i == MAX_TIT_LEN || (inStr && inStr[0] == '\n' && inStr[1] != 0))
            outStr += "...";
    }

CObjectViewerWnd& CMDIMainFrame::ShowTreeViewer ()
{
    ShowControlBar(&m_wndObjectViewerFrame, TRUE, FALSE);
    return m_wndObjectViewer;
}

CTreeCtrl& CMDIMainFrame::ShowPlanViewer (const char* str)
{
    CString title("Explain Plan for \"");
    append_striped_string(title, str);
    title += '\"';
    m_wndPlanFrame.SetWindowText(title);
    ShowControlBar(&m_wndPlanFrame, TRUE, FALSE);
    return m_wndPlanViewer;
}

BOOL CMDIMainFrame::OnCopyData (CWnd*, COPYDATASTRUCT* pCopyDataStruct)
{
    // 30.03.2003 improvement, command line support, try sqltools /h to get help
	if (AfxGetApp()->IsKindOf(RUNTIME_CLASS(CSQLToolsApp))
    && ((CSQLToolsApp*)AfxGetApp())->HandleAnotherInstanceRequest(pCopyDataStruct))
    {
        if (IsIconic()) OpenIcon();
        return TRUE;
    }
    return FALSE;
}

#if _MFC_VER > 0x0600

void CMDIMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
    CWorkbookMDIFrame::OnActivateApp(bActive, dwThreadID);

	if (AfxGetApp()->IsKindOf(RUNTIME_CLASS(CSQLToolsApp)))
        ((CSQLToolsApp*)AfxGetApp())->OnActivateApp(bActive);
}

#else

void CMDIMainFrame::OnActivateApp(BOOL bActive, HTASK hTask)
{
    CWorkbookMDIFrame::OnActivateApp(bActive, hTask);

	if (AfxGetApp()->IsKindOf(RUNTIME_CLASS(CSQLToolsApp)))
        ((CSQLToolsApp*)AfxGetApp())->OnActivateApp(bActive);
}

#endif


//void CMDIMainFrame::OnSize (UINT nType, int cx, int cy)
//{
//    CWorkbookMDIFrame::OnSize(nType, cx, cy);
//
//    if (nType == SIZE_MINIMIZED)
//    {
//        m_orgTitle = GetTitle();
//        CString title(m_orgTitle);
//        title += '\n';
//        title += m_wndConnectionBar.GetText();
//        title += '\n';
//        SetTitle(title);
//        OnUpdateFrameTitle(FALSE);
//    }
//    else if (!m_orgTitle.IsEmpty())
//    {
//        SetTitle(m_orgTitle);
//        OnUpdateFrameTitle(FALSE);
//        m_orgTitle.Empty();
//    }
//}
