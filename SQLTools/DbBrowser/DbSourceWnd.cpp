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

/*
    13.03.2003 bug fix, if you are not connected and display the Object List the close button is dimmed.
    26.10.2003 bug fix, multiple occurences of PUBLIC in "Schema" list ("Object List" window)
    09.11.2003 bug fix, Cancel window does not work properly on Load DDL
    21.01.2004 bug fix, "Object List" error handling chanded to avoid paranoic bug reporting
    10.02.2004 bug fix, "Object List" a small issue with cancellation of ddl loading
    22.03.2004 bug fix, CreateAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
    17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
    31.01.2006 R#1108078 - Dockable 'Objects list' (tMk).
    04.10.2006 B#XXXXXXX - cannot get DDL for db link if its name is longer than 30 chars
*/
#pragma message("\tTODO: Cancel window for all command!\n")

#include "stdafx.h"
#include "COMMON/ExceptionHelper.h"
#include "COMMON/AppGlobal.h"
#include "SQLTools.h"
#include "DbSourceWnd.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"
#include "MetaDict\MetaDictionary.h"
#include "MetaDict\Loader.h"
#include "Dlg/ConfirmationDlg.h"
#include "TableTruncateOptionsDlg.h"
#include "DeleteTableDataDlg.h"
#include "SQLWorksheet/2PaneSplitter.h"
#include "OCI8/BCursor.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OraMetaDict;

#define IDC_TAB_LIST      111
#define IDC_STATIC_SCHEMA 112
#define IDC_STATIC_FILTER 113

const int NAME_MAX_SIZE = 128+1; // resized to 128+1 because db link name length can be up to 128 chars

CPtrList CDbSourceWnd::m_thisDialogs;

/////////////////////////////////////////////////////////////////////////////
// CDbSourceWnd

CDbSourceWnd::CDbSourceWnd (OciConnect& connect)
: m_connect(connect),
  m_nItems(-1),
  m_nSelItems(-1)
{
    //{{AFX_DATA_INIT(CDbSourceWnd)
    //}}AFX_DATA_INIT
    m_AllSchemas = TRUE;
    m_bValid     = AfxGetApp()->GetProfileInt("Code", "Valid",  TRUE);
    m_bInvalid = AfxGetApp()->GetProfileInt("Code", "Invalid",TRUE);
    m_nViewAs  = AfxGetApp()->GetProfileInt("Code", "ViewAs", 1);

    m_bShowTabTitles = AfxGetApp()->GetProfileInt("Code", "ShowTabTitles",  TRUE);

    for (int i(0); i < (sizeof m_wndTabLists/ sizeof m_wndTabLists[0]); i++)
        m_wndTabLists[i] = 0;
}

CDbSourceWnd::~CDbSourceWnd ()
{
    try { EXCEPTION_FRAME;

        for (int i(0); i < (sizeof m_wndTabLists/ sizeof m_wndTabLists[0]); i++)
                delete m_wndTabLists[i];
    } 
    _DESTRUCTOR_HANDLER_
}

CDbObjListCtrl* CDbSourceWnd::GetCurSel () const
{
    CDbObjListCtrl* wndListCtrl = 0;
    
    int nTab = m_wndTab.GetCurSel();
    
    if (nTab != -1)
        wndListCtrl = m_wndTabLists[nTab];

    _ASSERTE(wndListCtrl);

    return wndListCtrl;
}

BOOL CDbSourceWnd::IsVisible () const
{
    const CWnd* pWnd = GetParent();
    if (!pWnd) pWnd = this;
    return pWnd->IsWindowVisible();
}

    #define AfxDeferRegisterClass(fClass) AfxEndDeferRegisterClass(fClass)
    BOOL AFXAPI AfxEndDeferRegisterClass(LONG/*SHORT*/ fToRegister);
    #define AFX_WNDCOMMCTLS_REG    0x00010 // means all original Win95
    #define AFX_WNDCOMMCTLSNEW_REG 0x3C000 // INTERNET|COOL|USEREX|DATE

BOOL CDbSourceWnd::Create (CWnd* pParentWnd)
{
    LPCTSTR lpszTemplateName = MAKEINTRESOURCE(IDD);
    _ASSERTE(pParentWnd != NULL);
    _ASSERTE(lpszTemplateName != NULL);

    // initialize common controls
    VERIFY(AfxDeferRegisterClass(AFX_WNDCOMMCTLS_REG));
    AfxDeferRegisterClass(AFX_WNDCOMMCTLSNEW_REG);

    // call PreCreateWindow to get prefered extended style
    CREATESTRUCT cs;
    memset(&cs, 0, sizeof(CREATESTRUCT));
    cs.style = WS_CHILD | WS_VISIBLE;//AFX_WS_DEFAULT_VIEW;
    if (!PreCreateWindow(cs))
        return FALSE;

    // create a modeless dialog
    if (!CreateDlg(lpszTemplateName, pParentWnd))
        return FALSE;

    // we use the style from the template - but make sure that
    //  the WS_BORDER bit is correct
    // the WS_BORDER bit will be whatever is in dwRequestedStyle
    ModifyStyle(WS_BORDER|WS_CAPTION, cs.style & (WS_BORDER|WS_CAPTION));
    ModifyStyleEx(WS_EX_CLIENTEDGE, cs.dwExStyle & WS_EX_CLIENTEDGE);

    SetDlgCtrlID(AFX_IDW_PANE_FIRST);

    // initialize controls etc
    if (!ExecuteDlgInit(lpszTemplateName))
        return FALSE;

    // make visible if requested

    ShowWindow(SW_NORMAL);

    CRect rect;
    GetParent()->GetClientRect(rect);
    SetWindowPos(0, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE|SWP_NOOWNERZORDER);

    EnableToolTips();

    return TRUE;
}

void CDbSourceWnd::DoDataExchange (CDataExchange* pDX)
{
    CWnd::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DS_SCHEMA, m_wndSchemaList);
    DDX_Control(pDX, IDC_DS_FILTER, m_wndFilterList);
    //{{AFX_DATA_MAP(CDbSourceWnd)
    DDX_Control(pDX, IDC_DS_TAB, m_wndTab);
    DDX_Check(pDX, IDC_DS_VALID, m_bValid);
    DDX_Check(pDX, IDC_DS_INVALID, m_bInvalid);
    DDX_CBString(pDX, IDC_DS_SCHEMA, m_strSchema);
    DDX_CBString(pDX, IDC_DS_FILTER, m_strFilter);
    DDX_Radio(pDX, IDC_DS_AS_LIST, m_nViewAs);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDbSourceWnd, CWnd)
    //{{AFX_MSG_MAP(CDbSourceWnd)
    ON_WM_SIZE()
    ON_CBN_SETFOCUS(IDC_DS_SCHEMA, OnSetFocusSchema)
    ON_CBN_SELCHANGE(IDC_DS_SCHEMA, OnDataChanged)
    ON_BN_CLICKED(IDC_DS_INVALID, OnDataChanged2)
    ON_BN_CLICKED(IDC_DS_REFRESH, OnRefresh)
    ON_NOTIFY(NM_DBLCLK, IDC_TAB_LIST, OnDblClikList)
    ON_COMMAND(IDC_DS_LOAD, OnLoad)
    ON_COMMAND(IDC_DS_LOAD_ALL_IN_ONE, OnLoadAsOne)
    ON_COMMAND(IDC_DS_COMPILE, OnCompile)
    ON_COMMAND(IDC_DS_DELETE, OnDelete)
    ON_COMMAND(IDC_DS_DISABLE, OnDisable)
    ON_COMMAND(IDC_DS_ENABLE, OnEnable)
    ON_BN_CLICKED(IDC_DS_AS_LIST, OnShowAsList)
    ON_BN_CLICKED(IDC_DS_AS_REPORT, OnShowAsReport)
    ON_WM_DESTROY()
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
    ON_WM_INITMENUPOPUP()
    ON_COMMAND(ID_DS_SHOW_OPTIONS, OnShowOptions)
    ON_NOTIFY(TCN_SELCHANGE, IDC_DS_TAB, OnSelChangeTab)
    ON_NOTIFY(NM_RCLICK, IDC_DS_TAB, OnRClickOnTab)
    ON_CBN_SELCHANGE(IDC_DS_FILTER, OnDataChanged)
    ON_CBN_EDITCHANGE(IDC_DS_FILTER, OnDataChanged)
    ON_BN_CLICKED(IDC_DS_VALID, OnDataChanged2)
    ON_COMMAND(IDC_DS_CANCEL, OnCancel)
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
    ON_MESSAGE_VOID(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
    ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
    ON_COMMAND_RANGE(IDC_DS_OTHER, IDC_DS_OPTIONS, OnMenuButtons)
    ON_COMMAND(ID_DS_QUERY, OnQuery)
    ON_COMMAND(ID_DS_DELETE, OnDataDelete)
    ON_COMMAND(ID_DS_TRUNCATE, OnTruncate)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDbSourceWnd message handlers


    static const CDbObjListCtrl::CData* types[] = 
    {
        &CDbObjListCtrl::sm_FunctionData,
        &CDbObjListCtrl::sm_ProcedureData,
        &CDbObjListCtrl::sm_PackageData,
        &CDbObjListCtrl::sm_PackageBodyData,
        &CDbObjListCtrl::sm_TriggerData,
        &CDbObjListCtrl::sm_ViewData,
        &CDbObjListCtrl::sm_TableData,
        &CDbObjListCtrl::sm_ChkConstraintData,
        &CDbObjListCtrl::sm_PkConstraintData,
        &CDbObjListCtrl::sm_UkConstraintData,
        &CDbObjListCtrl::sm_FkConstraintData,
        &CDbObjListCtrl::sm_IndexData,
        &CDbObjListCtrl::sm_SequenceData,
        &CDbObjListCtrl::sm_SynonymData,
        &CDbObjListCtrl::sm_GranteeData,
        &CDbObjListCtrl::sm_ClusterData,
        &CDbObjListCtrl::sm_DbLinkData,
//      &CDbObjListCtrl::sm_SnapshotData,
//      &CDbObjListCtrl::sm_SnapshotLogData,
        &CDbObjListCtrl::sm_TypeData,
        &CDbObjListCtrl::sm_TypeBodyData,
    };

LRESULT CDbSourceWnd::OnInitDialog (WPARAM, LPARAM)
{
    _ASSERTE(IDC_DS_OPTIONS - IDC_DS_OTHER == 1);

    CWinApp* pApp = AfxGetApp();

    m_thisDialogs.AddTail(this);

    _ASSERTE((sizeof types/ sizeof types[0]) == (sizeof m_wndTabLists/ sizeof m_wndTabLists[0]));

    UpdateData(FALSE);

    if (m_Images.m_hImageList == NULL)
        m_Images.Create(IDB_IMAGELIST3, 16, 64, RGB(0,255,0));

    m_wndTab.SetImageList(&m_Images);

    for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
    {
        m_wndTabLists[i] = new CDbObjListCtrl(m_connect, *types[i]);
        m_wndTabLists[i]->Create(LVS_REPORT|WS_CHILD|WS_BORDER|WS_GROUP|WS_TABSTOP|LVS_SHOWSELALWAYS,
                                                         CRect(0,0,0,0), this, IDC_TAB_LIST);
        m_wndTabLists[i]->ModifyStyleEx(0, WS_EX_CLIENTEDGE|WS_EX_CONTROLPARENT, 0);
        m_wndTabLists[i]->SetOwner(this);

        m_wndTab.InsertItem(m_wndTab.GetItemCount(), m_bShowTabTitles ? types[i]->m_szTitle : "", types[i]->m_nImageId);
    }

    if (!m_menuOther.m_hMenu)
        VERIFY(m_menuOther.LoadMenu(IDR_LS_OTHER_POPUP));
    if (!m_menuOptions.m_hMenu)
        VERIFY(m_menuOptions.LoadMenu(IDR_LS_OPTIONS_POPUP));

    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_AS_REPORT));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_AS_LIST));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_VALID), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_VALID));
    ::SendMessage(::GetDlgItem(*this, IDC_DS_INVALID), BM_SETIMAGE, IMAGE_ICON, (LPARAM)pApp->LoadIcon(IDI_INVALID));


    if (m_connect.IsOpen()) EvOnConnect();
    else                       EvOnDisconnect();

    int nTab = AfxGetApp()->GetProfileInt("Code", "nTab", 0);
    _ASSERTE(nTab < sizeof m_wndTabLists/sizeof m_wndTabLists[0]);
    m_wndTab.SetCurSel(nTab);
    m_wndTabLists[nTab]->ShowWindow(SW_SHOW);

    if (m_nViewAs == 0)
        m_wndTabLists[nTab]->ModifyStyle(LVS_REPORT, LVS_LIST, 0);
    else
        m_wndTabLists[nTab]->ModifyStyle(LVS_LIST, LVS_REPORT, 0);

	// create 2 static controls (we must move them in OnSize)
	HFONT hfont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    if (!hfont) 
        hfont = (HFONT)::GetStockObject(SYSTEM_FONT);

	m_StaticSchema.Create("Schema: ", WS_CHILD|WS_VISIBLE|WS_GROUP|SS_RIGHT, 
		CRect(50,4,50+25*2,4+8*2), this, IDC_STATIC_SCHEMA);
    m_StaticSchema.SendMessage(WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

	m_StaticFilter.Create("Filter: ", WS_CHILD|WS_VISIBLE|WS_GROUP|SS_RIGHT, 
		CRect(250,4,250+25*2,4+8*2), this, IDC_STATIC_FILTER);
    m_StaticFilter.SendMessage(WM_SETFONT, (WPARAM)hfont, (LPARAM)TRUE);

    return TRUE;
}

void CDbSourceWnd::OnDestroy()
{
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        AfxGetApp()->WriteProfileInt("Code", "nTab",    nTab);

    AfxGetApp()->WriteProfileInt("Code", "Valid",   m_bValid);
    AfxGetApp()->WriteProfileInt("Code", "Invalid", m_bInvalid);
    AfxGetApp()->WriteProfileInt("Code", "ViewAs",  m_nViewAs);

    m_thisDialogs.RemoveAt(m_thisDialogs.Find(this));

    CWnd::OnDestroy();
}

void CDbSourceWnd::EvOnConnect ()
{
    CWnd* wndChild = GetWindow(GW_CHILD);
    while (wndChild) 
    {
        switch (wndChild->GetDlgCtrlID()) 
        {
        default:
            wndChild->EnableWindow(TRUE);
        case IDC_DS_CANCEL:
        case IDC_DS_OPTIONS:
        case IDC_DS_FILTER:;
        }
        wndChild = wndChild->GetWindow(GW_HWNDNEXT);
    }

    UpdateSchemaList();
    DirtyObjectsLists();
    if (IsVisible())
        OnRefresh();
}

void CDbSourceWnd::EvOnDisconnect ()
{
    for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
        m_wndTabLists[i]->DeleteAllItems();

    CWnd* wndChild = GetWindow(GW_CHILD);
    while (wndChild) 
    {
        // 13.03.2003 buf fix, if you are not connected and display the Object List the close button is dimmed.
        switch (wndChild->GetDlgCtrlID()) 
        {
        default:
            wndChild->EnableWindow(FALSE);
        case IDC_DS_CANCEL:
        case IDC_DS_OPTIONS:
        case IDC_DS_FILTER:;
        }
        wndChild = wndChild->GetWindow(GW_HWNDNEXT);
    }
}

void CDbSourceWnd::EvOnConnectAll ()
{
    POSITION pos = m_thisDialogs.GetHeadPosition();
    while (pos != NULL) 
    {
        CDbSourceWnd* pDlg = (CDbSourceWnd*)m_thisDialogs.GetNext(pos);
        pDlg->EvOnConnect();
    }
}

void CDbSourceWnd::EvOnDisconnectAll ()
{
    POSITION pos = m_thisDialogs.GetHeadPosition();
    while (pos != NULL) 
    {
        CDbSourceWnd* pDlg = (CDbSourceWnd*)m_thisDialogs.GetNext(pos);
        pDlg->EvOnDisconnect();
    }
}

/*! \fn void CDbSourceWnd::OnSize (UINT nType, int cx, int cy)
 *
 * \brief Handling resize event of the 'Object List'. Moving top, bottom, right components and tabs accordingly to the new window size.
 *
 * \param nType	Specifies the type of resizing requested (SIZE_MAXIMIZED, SIZE_MINIMIZED, ...).
 * \param cx New width of the window.
 * \param cy New height of the window.
 */ 
void CDbSourceWnd::OnSize (UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0 // only if visible
    && nType != SIZE_MAXHIDE
    && nType != SIZE_MINIMIZED) 
    {
		// tables with IDs of components
        static WORD nRightChildren[]  = 
        { 
            IDC_DS_FILTER, IDC_DS_STATUS
        };
        static WORD nTopChildren[]  = 
        { 
            IDC_DS_AS_LIST, IDC_DS_AS_REPORT, IDC_STATIC_SCHEMA, IDC_DS_SCHEMA, 
			IDC_DS_VALID, IDC_DS_INVALID, IDC_STATIC_FILTER, IDC_DS_FILTER
        };
		// warning: IDC_DS_STATUS must be the last position in nBottomChildren[]!
        static WORD nBottomChildren[] = 
        {
            IDC_DS_LOAD, IDC_DS_COMPILE, IDC_DS_DELETE,
            IDC_DS_REFRESH, IDC_DS_OTHER, IDC_DS_OPTIONS,
            IDC_DS_CANCEL, IDC_DS_STATUS
        };
		int nRightChildrenCount = sizeof nRightChildren / sizeof nRightChildren[0];
		int nTopChildrenCount = sizeof nTopChildren / sizeof nTopChildren[0];
		int nBottomChildrenCount = sizeof nBottomChildren / sizeof nBottomChildren[0];
		const int TOP_BREAK_WIDTH = 400;// magic border :) (anybody has better idea?)
		const int MIN_STATUS_WIDTH = 60;

        CRect rect;
		CWnd* pWnd = 0;
        int i, nH, nW, nMaxH, nLineWidth, nBottomLines, nTopLines, nCurrentLine;

		// resize IDC_DS_SCHEMA
		pWnd = GetDlgItem(IDC_DS_SCHEMA);
		if (pWnd) 
		{
			pWnd->GetWindowRect(rect);
			ScreenToClient(&rect);
			if (cx <= TOP_BREAK_WIDTH) {// fill remaining space (like Right component)
				pWnd->SetWindowPos(0, 0, 0,	cx - rect.left,	rect.Height(), SWP_NOMOVE|SWP_NOZORDER);
			} else {// fill only 1/4 of space
				pWnd->SetWindowPos(0, 0, 0,	cx * 1/4, rect.Height(), SWP_NOMOVE|SWP_NOZORDER);
			}
		}
		// set positions of top components 
		nTopLines = 1, nLineWidth = 0;
		for	(i = 0;	i <	nTopChildrenCount; i++)
		{
			if (cx <= TOP_BREAK_WIDTH && nTopChildren[i] ==	IDC_DS_VALID) {	// we have two lines
				nTopLines = 2;
				nLineWidth = 0;
			}
			pWnd = GetDlgItem(nTopChildren[i]);
			if (pWnd) 
			{
				pWnd->GetWindowRect(rect);
				nW = rect.Width()+1;
				nH = rect.Height();
				if(nTopChildren[i] == IDC_STATIC_SCHEMA || nTopChildren[i] == IDC_STATIC_FILTER) {
					pWnd->SetWindowPos(0, nLineWidth, (nH+6) * nTopLines - nH,
						0, 0, SWP_NOZORDER|SWP_NOSIZE);
				} else {
					pWnd->SetWindowPos(0, nLineWidth, (nH+2) * nTopLines - nH,
						0, 0, SWP_NOZORDER|SWP_NOSIZE);
				}
				nLineWidth += nW;
			}
		}

		// how many lines is needed by bottom components
		nMaxH = 0, nLineWidth = 0, nBottomLines = 1;
		for (i = 0; i < nBottomChildrenCount; i++)
		{
			pWnd = GetDlgItem(nBottomChildren[i]);
            if (pWnd) 
            {
                pWnd->GetWindowRect(rect);
				if (nBottomChildren[i] == IDC_DS_STATUS) {// exception for this component!
					nW = MIN_STATUS_WIDTH+1;// min width of STATUS
				} else {
					nW = rect.Width()+1;
				}
				nLineWidth += nW;
				if (nLineWidth >= cx) {
					nBottomLines++;
					nLineWidth = nW;
				}
			}
		}

		// set positions of bottom components 
		nCurrentLine = nBottomLines;
        for (i = 0, nLineWidth = 0; i < nBottomChildrenCount; i++) 
        {
            pWnd = GetDlgItem(nBottomChildren[i]);
            if (pWnd) 
            {
                pWnd->GetWindowRect(rect);
                ScreenToClient(&rect);
                nH = rect.Height();
                if (nMaxH < nH) nMaxH = nH;
                // draw min. 1 button per line
				if (nBottomChildren[i] == IDC_DS_STATUS) {// exception for this component!
					nW = MIN_STATUS_WIDTH+1;
					nH += 2;// move 2 pixels higher
				} else {
					nW = rect.Width()+1;
				}
				if (nLineWidth > 0 && nLineWidth + nW >= cx) {// is the new line?
					nCurrentLine--;
					nLineWidth = 0;
				}
				pWnd->SetWindowPos(0, nLineWidth, cy - (nH+2) * nCurrentLine ,
                                   0, 0, SWP_NOZORDER|SWP_NOSIZE);
				nLineWidth += nW;
			}
        }

		// set 'right' components positions (after 'top' and 'bottom'!) - widen it
        for (i=0; i < nRightChildrenCount; i++) 
        {
            pWnd = GetDlgItem(nRightChildren[i]);
            if (pWnd) 
            {
                pWnd->GetWindowRect(rect);
                ScreenToClient(&rect);
                pWnd->SetWindowPos(0, 0, 0, cx - rect.left, rect.Height(), SWP_NOMOVE|SWP_NOZORDER);
				//pWnd->Invalidate();// to fix some 'strange' cases (parts of buttons over this components)
            }
        }

		// set tab position
        pWnd = GetDlgItem(IDC_DS_LOAD);
        if (pWnd) 
        {
            pWnd->GetWindowRect(rect); // get the first button (LOAD) size and position
            ScreenToClient(&rect);
            /*CRect rcTab;
            m_wndTab.GetWindowRect(rcTab); // get tab size
            ScreenToClient(&rcTab);*/
			
			// set proper position
            m_wndTab.SetWindowPos(&CWnd::wndBottom, 0, nTopLines * rect.Height() + 4,
                                  cx, rect.top - nTopLines * rect.Height() - 6,
                                  SWP_NOZORDER);
			// set all tabs positions
            m_wndTab.GetWindowRect(&rect);
            m_wndTab.AdjustRect(FALSE, &rect);
            ScreenToClient(rect);
            rect.InflateRect(1, 0);
            for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
                m_wndTabLists[i]->SetWindowPos(0, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);
        }
    }
	Invalidate();// to fix some 'strange' cases (parts of buttons over components, etc.)
}

void CDbSourceWnd::OnSelChangeTab (NMHDR*, LRESULT* pResult)
{
    int nTab = m_wndTab.GetCurSel();
    _ASSERTE(nTab < sizeof m_wndTabLists/sizeof m_wndTabLists[0]);

    if (nTab != -1) {
        for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
            m_wndTabLists[i]->ShowWindow(i == nTab ? SW_SHOW : SW_HIDE);

        if (m_nViewAs == 0)
            m_wndTabLists[nTab]->ModifyStyle(LVS_REPORT, LVS_LIST, 0);
        else
            m_wndTabLists[nTab]->ModifyStyle(LVS_LIST, LVS_REPORT, 0);

        if (m_connect.IsOpen())
            m_wndTabLists[nTab]->Refresh(m_strSchema, m_strFilter,
                                         m_bValid ? true : false,
                                         m_bInvalid ? true : false);
		// B#1445114 - Dockable objects list - refreshing problem.
		// Invalidate and set to refresh a whole window. Without this we have some strange
		// cases, when window wasn't refreshed after changing tab. 
		// It is caused by added docking possibility.
		Invalidate();
    }
    *pResult = 0;
}

void CDbSourceWnd::UpdateSchemaList ()
{
    m_wndSchemaList.ResetContent();
    
    try { EXCEPTION_FRAME;

        OciCursor cursor(m_connect, "select user from dual", 1);
        cursor.Execute();
        cursor.Fetch();
        m_strSchema = cursor.ToString(0).c_str();
        m_wndSchemaList.SetCurSel(m_wndSchemaList.AddString(m_strSchema));
    } 
    _DEFAULT_HANDLER_
}

    LPSCSTR cszSchemasSttm =
        "select username from all_users a_u"
        " where username <> user"
        " and (exists (select null from all_source a_s where a_s.owner = a_u.username)"
        " or exists (select null from all_triggers a_t where a_t.owner = a_u.username))";

    LPSCSTR cszAllSchemasSttm =
        "select username from all_users a_u"
        " where username <> user";

void CDbSourceWnd::OnSetFocusSchema ()
{
    if (m_wndSchemaList.GetCount() <= 1) 
    {
        CWaitCursor wait;

        try { EXCEPTION_FRAME;

            OciCursor cursor(m_connect, m_AllSchemas ? cszAllSchemasSttm : cszSchemasSttm);
            cursor.Execute();
            while (cursor.Fetch())
                m_wndSchemaList.AddString(cursor.ToString(0).c_str());
        } 
        _DEFAULT_HANDLER_

#pragma message("reimplement PUBLIC support!")
        m_wndSchemaList.AddString("PUBLIC");
    }
}

void CDbSourceWnd::DirtyObjectsLists ()
{
    for (int i(0); i < sizeof m_wndTabLists/sizeof m_wndTabLists[0]; i++)
        m_wndTabLists[i]->Dirty();
}

void CDbSourceWnd::OnDataChanged ()
{
    DirtyObjectsLists();
    UpdateData();

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->Refresh(m_strSchema, m_strFilter,
                                     m_bValid ? true : false,
                                     m_bInvalid ? true : false);
}

void CDbSourceWnd::OnDataChanged2 ()
{
    UpdateData();

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->RefreshList(m_bValid ? true : false,
                                                                         m_bInvalid ? true : false);
}

void CDbSourceWnd::OnRefresh ()
{
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->Refresh(m_strSchema, m_strFilter,
                                     m_bValid ? true : false,
                                     m_bInvalid ? true : false,
                                     true);
}

void CDbSourceWnd::OnShowAsList ()
{
    m_nViewAs = 0;
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->ModifyStyle(LVS_REPORT, LVS_LIST, 0);

    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETCHECK, BST_UNCHECKED, 0L);
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETCHECK, BST_CHECKED, 0L);
}

void CDbSourceWnd::OnShowAsReport ()
{
    m_nViewAs = 1;
    for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
        m_wndTabLists[i]->ModifyStyle(LVS_LIST, LVS_REPORT, 0);

    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_REPORT), BM_SETCHECK, BST_CHECKED, 0L);
    ::SendMessage(::GetDlgItem(*this, IDC_DS_AS_LIST), BM_SETCHECK, BST_UNCHECKED, 0L);
}

void CDbSourceWnd::OnSelectAll ()
{
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1)
        m_wndTabLists[nTab]->SendMessage(WM_COMMAND, ID_EDIT_SELECT_ALL);
}

void CDbSourceWnd::OnIdleUpdateCmdUI ()
{
    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1) 
    {
        int nItems    = m_wndTabLists[nTab]->GetItemCount();
        int nSelItems = m_wndTabLists[nTab]->GetSelectedCount();

        if (m_nItems != nItems
        || m_nSelItems != nSelItems) 
        {
            char szBuffer[80];
            m_nItems    = nItems;
            m_nSelItems = nSelItems;
            sprintf(szBuffer, "%d, %d"/*"Total:%d Selected:%d"*/, nItems, nSelItems);
            HWND hWnd = ::GetDlgItem(m_hWnd, IDC_DS_STATUS);
            ::SetWindowText(hWnd, szBuffer);
        }

        if (m_connect.IsOpen()) 
        {
            HWND btnCompile = ::GetDlgItem(*this, IDC_DS_COMPILE);
            ::EnableWindow(btnCompile, m_wndTabLists[nTab]->m_Data.m_bCanCompile);

            HWND btnDelete = ::GetDlgItem(*this, IDC_DS_DELETE);
            ::EnableWindow(btnDelete, m_wndTabLists[nTab]->m_Data.m_bCanDrop);

            if (m_wndTabLists[nTab]->IsDirty())
                m_wndTabLists[nTab]->Refresh(m_strSchema, m_strFilter, m_bValid ? true : false, m_bInvalid ? true : false);
        }
    }
}

void CDbSourceWnd::OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    int nTab = m_wndTab.GetCurSel();
    if (nTab != -1 && !bSysMenu)
        m_wndTabLists[nTab]->OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}

void CDbSourceWnd::OnDblClikList (NMHDR*, LRESULT* pResult)
{
    OnLoad();
    *pResult = 0;
}

void CDbSourceWnd::Do (bool (CDbSourceWnd::*pmfnDo)(CDoContext&),
        void* param, void* param2, void* param3, CAbortController* pAbortCtrl)
{
    CWaitCursor wait;

    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
    {
        UpdateData();

        CDoContext doContext;
        doContext.m_szOwner  = m_strSchema;
        doContext.m_szType   = wndListCtrl->m_Data.m_szType;
        doContext.m_pvParam  = param;
        doContext.m_pvParam2 = param2;
        doContext.m_pvParam3 = param3;
        doContext.m_pAbortCtrl = pAbortCtrl;

        LV_ITEM lvi;
        memset(&lvi, 0, sizeof lvi);
        lvi.mask = LVIF_TEXT|LVIF_PARAM;
        char szBuff[NAME_MAX_SIZE]; 
        lvi.pszText = szBuff;
        lvi.cchTextMax = sizeof szBuff;

        int index = -1;
        while ((index = wndListCtrl->GetNextItem(index, LVNI_SELECTED))!=-1) 
        {
            lvi.iItem      = index;
            lvi.iSubItem   = 0;
            lvi.pszText    = szBuff;
            lvi.cchTextMax = sizeof szBuff;
            if (!wndListCtrl->GetItem(&lvi)) {
                AfxMessageBox("Unknown error!");
                AfxThrowUserException();
            }
            doContext.m_nRow   = lvi.lParam;
            doContext.m_nItem  = lvi.iItem;
            doContext.m_szName = lvi.pszText;

#pragma message("reimplement it ASAP!")
            char szBuff2[NAME_MAX_SIZE];
            if (!stricmp(doContext.m_szType, "CONSTRAINT"))
            {
                lvi.iItem  = index;
                lvi.iSubItem     = 1;
                lvi.pszText      = szBuff2;
                lvi.cchTextMax = sizeof szBuff2;
                if (!wndListCtrl->GetItem(&lvi)) {
                    AfxMessageBox("Unknown error!");
                    AfxThrowUserException();
                }
                doContext.m_szTable = lvi.pszText;
            }

            try { EXCEPTION_FRAME;

                if (!(this->*pmfnDo)(doContext))
                    break;
            }
            catch (const OciException& x)
            {
                if (pAbortCtrl) pAbortCtrl->EndBatchProcess();
                MessageBeep((UINT)-1);
                AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                return;
            }
            catch (const OraMetaDict::XNotFound& x)
            {
                if (pAbortCtrl) pAbortCtrl->EndBatchProcess();
                MessageBeep((UINT)-1);
                AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                return;
            }
            catch(CUserException*) // B#1373859 - Unserved exception after cancelling Drop
            {
                if (pAbortCtrl) pAbortCtrl->EndBatchProcess();
                MessageBeep((UINT)-1);
                return;
            }
            catch (const std::exception& x)
            {
                if (pAbortCtrl) pAbortCtrl->EndBatchProcess();
                DEFAULT_HANDLER(x);
                return;
            }
            catch (...)
            {
                if (pAbortCtrl) pAbortCtrl->EndBatchProcess();
                DEFAULT_HANDLER_ALL;
                return;
            }
        }
    }
}

void CDbSourceWnd::OnLoad (bool bAsOne)
{
    SQLToolsSettings settings = GetSQLToolsSettings();
    CPLSWorksheetDoc::CLoader Loader;
    Loader.SetAsOne(bAsOne);

    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if ( ShowDDLPreferences(settings, true)) 
    {

        try { EXCEPTION_FRAME;

            Dictionary dictionary;
            dictionary.SetRewriteSharedDuplicate(true);

            CAbortController abortCtrl(*GetAbortThread(), &m_connect);

            if (settings.m_bPreloadDictionary
            && settings.m_bPreloadStartPercent
            && wndListCtrl->GetItemCount() > 7
            && wndListCtrl->GetSelectedCount() > 1
            && static_cast<int>(wndListCtrl->GetSelectedCount())
                >= (wndListCtrl->GetItemCount()*settings.m_bPreloadStartPercent)/100) 
            {
                Global::SetStatusText("Building meta dictionary...", TRUE);
                abortCtrl.SetActionText("Building meta dictionary...");

#pragma warning ( disable : 4800 )
                OraMetaDict::Loader loader(m_connect, dictionary);
                loader.Init();
                loader.LoadObjects(m_strSchema, "%", wndListCtrl->m_Data.m_szType, 
                                    settings, settings.m_bSpecWithBody, settings.m_bBodyWithSpec,
                                    true/*bUseLike*/);
#pragma warning ( default : 4800 )
            }

            Do(&CDbSourceWnd::DoLoad, &Loader, &settings, &dictionary, &abortCtrl);
        } 
        _DEFAULT_HANDLER_
    }
}

void CDbSourceWnd::OnLoadAsOne ()
{
    OnLoad(true);
}

void CDbSourceWnd::OnLoad ()
{
    OnLoad(false);
}

bool CDbSourceWnd::DoLoad (CDoContext& doContext)
{
    CPLSWorksheetDoc::CLoader* pLoader = (CPLSWorksheetDoc::CLoader*)doContext.m_pvParam;

    //try { EXCEPTION_FRAME;
    {
        _ASSERTE(doContext.m_pvParam);

        CString strBuff;
        strBuff.Format("Loading %s...", doContext.m_szName);
        Global::SetStatusText(strBuff, TRUE);
        doContext.m_pAbortCtrl->SetActionText(strBuff);

        const DbObject* pObject = 0;
        Dictionary* pDict = (Dictionary*)doContext.m_pvParam3;
        const SQLToolsSettings* pSettings = (SQLToolsSettings*)doContext.m_pvParam2;

        try {
            m_connect.CHECK_INTERRUPT();
            pObject = &pDict->LookupObject(doContext.m_szOwner, doContext.m_szName, doContext.m_szType);
        } 
        catch (const XNotFound&) 
        {
#pragma warning ( disable : 4800 )
            OraMetaDict::Loader loader(m_connect, *pDict);
            loader.Init();
            loader.LoadObjects(doContext.m_szOwner,
                               doContext.m_szName,
                               doContext.m_szType,
                               *pSettings, pSettings->m_bSpecWithBody, pSettings->m_bBodyWithSpec,
                               false/*bUseLike*/);
#pragma warning ( default : 4800 )
            pObject = &pDict->LookupObject(doContext.m_szOwner,
                                           doContext.m_szName,
                                           doContext.m_szType);
        }

        pLoader->Put(*pObject, *pSettings, TRUE);

        Global::SetStatusText("", TRUE);
    } 
    //catch (...)
    //{
    //    return false;
    //}
    //catch (const OciException& x) 
    //{
    //    // 09.11.2003 bug fix, Cancel window does not work properly on Load DDL
    //    doContext.m_pAbortCtrl->EndBatchProcess();
    //    pLoader->Clear();
    //    DEFAULT_OCI_HANDLER(x);
    //    return false;
    //} 
    //catch (CUserException*) 
    //{
    //    pLoader->Clear();
    //    return false;
    //}

    return true;
}

void CDbSourceWnd::OnDelete ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (wndListCtrl->m_Data.m_bCanDrop) 
        {
            CDWordArray dwArray;
            try 
            {
                Do(&CDbSourceWnd::DoDelete, &dwArray);
            }
            catch (CUserException*) {}

            for (int i = dwArray.GetSize()-1; i >= 0; i--)
                wndListCtrl->DeleteItem(dwArray[i]);
        }
}


/*! \fn bool CDbSourceWnd::DoDelete (CDoContext& doContext)
 *
 * \brief Procedure for DROPPING objects in 'Object List' window.
 *
 * \return	Returns true or throws exception (bug?).
 */ 
bool CDbSourceWnd::DoDelete (CDoContext& doContext)
{
    _ASSERTE(doContext.m_pvParam 
             && (!doContext.m_pvParam2 || doContext.m_pvParam2 == (void*)-1));

    bool isTable = !stricmp(doContext.m_szType, "TABLE");
    bool del = false;

    // ask user for confirmation
	if (!doContext.m_pvParam2)
    {
        CConfirmationDlg dlg(this);
        dlg.m_strText.Format("Are you sure you want to drop \"%s\" %s?", doContext.m_szName,
                                                 isTable ? "\r\ncascade constraints" : "");

        switch (dlg.DoModal()) 
        {
        case ID_CNF_ALL:
                doContext.m_pvParam2 = (void*)-1;
        case IDYES:
                del = true;
        case IDNO:
                break;
        case IDCANCEL:;
                AfxThrowUserException();
        }
    }

    if (del || doContext.m_pvParam2 == (void*)-1)
    {
        CString cmd;

		// DROP CONSTRAINT
		if (!stricmp(doContext.m_szType, "CONSTRAINT")) {
            cmd.Format("ALTER TABLE  \"%s\".\"%s\" DROP CONSTRAINT \"%s\"",
                           doContext.m_szOwner, doContext.m_szTable, doContext.m_szName);
		
		// B#1221764 - DROP DATABASE LINK error. Cutting SCHEMA name.
		} else if(!stricmp(doContext.m_szType, "DATABASE LINK")) {
			cmd.Format("DROP DATABASE LINK \"%s\"", doContext.m_szName);
		
		// ANOTHER DROPS
		} else {
            cmd.Format("DROP %s \"%s\".\"%s\"%s",
                        doContext.m_szType, doContext.m_szOwner, doContext.m_szName,
                        isTable ? " CASCADE CONSTRAINTS" : "");
		}

        OciStatement sttm(m_connect, cmd);
        sttm.Execute(1);
        ((CDWordArray*)doContext.m_pvParam)->Add(doContext.m_nItem);
    }

    return true;
}

void CDbSourceWnd::OnCompile ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (wndListCtrl->m_Data.m_bCanCompile)
    {
        if (!stricmp(wndListCtrl->m_Data.m_szType, "PACKAGE BODY"))
            Do(&CDbSourceWnd::DoCompileBody, wndListCtrl, "Compiling %s ...", "PACKAGE");
        else if (!stricmp(wndListCtrl->m_Data.m_szType, "TYPE BODY"))
            Do(&CDbSourceWnd::DoCompileBody, wndListCtrl, "Compiling %s ...", "TYPE");
        else
            Do(&CDbSourceWnd::DoAlter, wndListCtrl, "COMPILE", "Compiling %s ...");
    }
}

void CDbSourceWnd::OnDisable ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (wndListCtrl->m_Data.m_bCanDisable)
            Do(&CDbSourceWnd::DoAlter, wndListCtrl, "DISABLE", "Disabling %s ...");
}

void CDbSourceWnd::OnEnable ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (wndListCtrl->m_Data.m_bCanDisable)
            Do(&CDbSourceWnd::DoAlter, wndListCtrl, "ENABLE", "Enabling %s ...");
}

bool CDbSourceWnd::DoAlter (CDoContext& doContext)
{
    _ASSERTE(doContext.m_pvParam && doContext.m_pvParam2 && doContext.m_pvParam3);

    CString strBuff;
    strBuff.Format((char*)doContext.m_pvParam3, doContext.m_szName);
    Global::SetStatusText(strBuff, TRUE);

    if (!stricmp(doContext.m_szType, "CONSTRAINT"))
        strBuff.Format("ALTER TABLE  \"%s\".\"%s\" %s CONSTRAINT \"%s\"",
                       doContext.m_szOwner, doContext.m_szTable,
                       (char*)doContext.m_pvParam2, doContext.m_szName);
    else
        strBuff.Format("ALTER %s \"%s\".\"%s\" %s",
                       doContext.m_szType, doContext.m_szOwner,
                       doContext.m_szName, (char*)doContext.m_pvParam2);

    m_connect.ExecuteStatement(strBuff);
    ((CDbObjListCtrl*)doContext.m_pvParam)->RefreshRow(doContext.m_nItem, doContext.m_nRow,
                                                       doContext.m_szOwner, doContext.m_szName);
    Global::SetStatusText("", TRUE);

    return true;
}

bool CDbSourceWnd::DoCompileBody (CDoContext& doContext)
{
    _ASSERTE(doContext.m_pvParam && doContext.m_pvParam2 && doContext.m_pvParam3);

    CString strBuff;
    strBuff.Format((char*)doContext.m_pvParam2, doContext.m_szName);
    Global::SetStatusText(strBuff, TRUE);


    strBuff.Format("ALTER %s \"%s\".\"%s\" COMPILE BODY",
                   (const char*)doContext.m_pvParam3,
                   doContext.m_szOwner, doContext.m_szName);
    m_connect.ExecuteStatement(strBuff);
    ((CDbObjListCtrl*)doContext.m_pvParam)->RefreshRow(doContext.m_nItem, doContext.m_nRow, 
                                                       doContext.m_szOwner, doContext.m_szName);
    Global::SetStatusText("", TRUE);

    return true;
}

void CDbSourceWnd::OnShowOptions()
{
    SQLToolsSettings settings = GetSQLToolsSettings();
    ShowDDLPreferences(settings, false);
}

LRESULT CDbSourceWnd::OnHelpHitTest(WPARAM, LPARAM lParam)
{
    ASSERT_VALID(this);

    int nID = OnToolHitTest((DWORD)lParam, NULL);
    if (nID != -1)
        return HID_BASE_COMMAND+nID;

    return HID_BASE_CONTROL + IDD;
}

// See CControlBar::WindowProc
LRESULT CDbSourceWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    
    try { EXCEPTION_FRAME;

        if (nMsg == WM_NOTIFY) 
        {
            // send these messages to the owner if not handled
            if (OnWndMsg(nMsg, wParam, lParam, &lResult))
                return lResult;
            else
            {
                // try owner next
                lResult = GetOwner()->SendMessage(nMsg, wParam, lParam);

                // special case for TTN_NEEDTEXTA and TTN_NEEDTEXTW
                if(nMsg == WM_NOTIFY)
                {
                    NMHDR* pNMHDR = (NMHDR*)lParam;
                    if (pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
                        TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
                        if ((pNMHDR->code == TTN_NEEDTEXTA && (!pTTTA->lpszText || !*pTTTA->lpszText)) ||
                            (pNMHDR->code == TTN_NEEDTEXTW && (!pTTTW->lpszText || !*pTTTW->lpszText)))
                        {
                            // not handled by owner, so let bar itself handle it
                            lResult = CWnd::WindowProc(nMsg, wParam, lParam);
                        }
                    }
                }
                return lResult;
            }
        }
        // otherwise, just handle in default way
        lResult = CWnd::WindowProc(nMsg, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return lResult;
}

void CDbSourceWnd::OnMenuButtons (UINT nID)
{
    HWND hButton = ::GetDlgItem(m_hWnd, nID);
    
    _ASSERTE(hButton);
    
    if (hButton
    && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED) 
    {
        ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

        CRect rect;
        ::GetWindowRect(hButton, &rect);

        CMenu* pPopup = (nID == IDC_DS_OTHER) ? m_menuOther.GetSubMenu(0) : m_menuOptions.GetSubMenu(0);
        _ASSERTE(pPopup != NULL);
        TPMPARAMS param;
        param.cbSize = sizeof param;
        param.rcExclude.left     = 0;
        param.rcExclude.top      = rect.top;
        param.rcExclude.right  = USHRT_MAX;
        param.rcExclude.bottom = rect.bottom;

        if (::TrackPopupMenuEx(*pPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               rect.left, rect.bottom + 1, *this, &param)) 
        {
            MSG msg;
            ::PeekMessage(&msg, hButton, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
        }

        ::PostMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0L);
    }
}

void CDbSourceWnd::OnCancel ()
{
    AfxGetApp()->GetMainWnd()->SendMessage(WM_COMMAND, ID_SQL_DB_SOURCE);
}

void CDbSourceWnd::OnRClickOnTab (NMHDR*, LRESULT* pResult)
{
    m_bShowTabTitles = !m_bShowTabTitles;
    AfxGetApp()->WriteProfileInt("Code", "ShowTabTitles",  m_bShowTabTitles);


    TCITEM item;
    item.mask = TCIF_TEXT;
    for (int i(0); i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++) 
    {
        item.pszText = const_cast<char*>(m_bShowTabTitles ? types[i]->m_szTitle : "");
        m_wndTab.SetItem(i, &item);
    }

    CRect rect;
    m_wndTab.GetWindowRect(&rect);
    m_wndTab.AdjustRect(FALSE, &rect);
    ScreenToClient(rect);
    rect.InflateRect(1, 0);

    for (i = 0; i < sizeof m_wndTabLists/ sizeof m_wndTabLists[0]; i++)
        m_wndTabLists[i]->SetWindowPos(0, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);

    *pResult = 0;
}

void CDbSourceWnd::OnSetFocus (CWnd* /*pOldWnd*/) 
{
    int nTab = m_wndTab.GetCurSel();

    if (nTab != -1 
    && (sizeof m_wndTabLists/ sizeof m_wndTabLists[0])
    && m_wndTabLists[nTab] 
    && m_wndTabLists[nTab]->m_hWnd)
        m_wndTabLists[nTab]->SetFocus();
}


bool CDbSourceWnd::DoQuery (CDoContext& doContext)
{
    CMemFile of(256);
    TextOutputInMFCFile out(of, true);

    out.PutIndent();
    out.Put("SELECT * FROM ");
    if (stricmp(m_connect.GetUID(), doContext.m_szOwner))
    {
        out.SafeWriteDBName(doContext.m_szOwner);
        out.Put(".");
    }
    out.SafeWriteDBName(doContext.m_szName);
    out.Put(";");
    out.NewLine();

    POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
    if (CDocTemplate* pDocTemplate = AfxGetApp()->GetNextDocTemplate(pos))
    {
        if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
        {
            ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);

            of.SeekToBegin();
            ((CDocument*)pDoc)->Serialize(CArchive(&of, CArchive::load));
            pDoc->SetTitle(CString(doContext.m_szName) + ".sql");
            pDoc->SetClassSetting("PL/SQL");
            // 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
            pDoc->SetSaveAsToDefault();

	        pDoc->GetSplitter()->SetDefaultHight(0, 50);
            pDoc->SetModifiedFlag(FALSE);
            pDoc->DoSqlExecute(0);

            return true;
        }
    }

    return false;
}

void CDbSourceWnd::OnQuery ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if ((!strcmp(wndListCtrl->m_Data.m_szType, "TABLE") 
        || !strcmp(wndListCtrl->m_Data.m_szType, "VIEW")))
            Do(&CDbSourceWnd::DoQuery);
}

bool CDbSourceWnd::DoDataDelete (CDoContext& doContext)
{
    CString strBuff;
    strBuff.Format("Deleting from %s ...", doContext.m_szName);
    Global::SetStatusText(strBuff, TRUE);
    strBuff.Format("DELETE FROM  \"%s\".\"%s\"", doContext.m_szOwner, doContext.m_szName);
    m_connect.ExecuteStatement(strBuff);
    Global::SetStatusText("", TRUE);

    return true;
}

void CDbSourceWnd::OnDataDelete ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (!strcmp(wndListCtrl->m_Data.m_szType, "TABLE") 
        || !strcmp(wndListCtrl->m_Data.m_szType, "VIEW"))
    {
        CDeleteTableDataDlg dlg;
        if (dlg.DoModal() == IDOK)
        {
            Do(&CDbSourceWnd::DoDataDelete);
        }
    }
}

bool CDbSourceWnd::DoTruncate (CDoContext& doContext)
{
    CString strBuff;
    strBuff.Format("Truncating %s ...", doContext.m_szName);
    Global::SetStatusText(strBuff, TRUE);
    strBuff.Format("TRUNCATE TABLE  \"%s\".\"%s\"", doContext.m_szOwner, doContext.m_szName);
    m_connect.ExecuteStatement(strBuff);
    strBuff.Format("Table %s has been truncated.", doContext.m_szName);
    Global::SetStatusText(strBuff, TRUE);

    return true;
}

    
    struct DisabledFK { string owner, table, constraint; bool enabled; };

bool CDbSourceWnd::DoTruncatePre (CDoContext& doContext)
{
    try { EXCEPTION_FRAME;

        _ASSERTE(doContext.m_pvParam);
        vector<DisabledFK>& disabledFKs = *((vector<DisabledFK>*)doContext.m_pvParam);

        const int fk_owner      = 0;
        const int fk_table      = 1;
        const int fk_constraint = 2;

        OciCursor cursor(m_connect, 
            "SELECT owner, table_name, constraint_name FROM sys.all_constraints"
            " WHERE (r_owner, r_constraint_name) IN"
                "("
                " SELECT owner, constraint_name FROM sys.all_constraints"
                  " WHERE owner = :p_owner"
                    " AND table_name = :p_table"
                    " AND constraint_type IN ('P', 'U')"
                    " AND status = 'ENABLED'"
                ")"
                " AND constraint_type = 'R'"
                " AND status = 'ENABLED'"
            );

        cursor.Bind("p_owner", doContext.m_szOwner);
        cursor.Bind("p_table", doContext.m_szName);

        cursor.Execute();
        while (cursor.Fetch())
        {
            try { EXCEPTION_FRAME;

                DisabledFK fk;
                fk.enabled = true;
                cursor.GetString(fk_owner, fk.owner);
                cursor.GetString(fk_table, fk.table);
                cursor.GetString(fk_constraint, fk.constraint);
                disabledFKs.push_back(fk);
            } 
            _DEFAULT_HANDLER_
        }
        Global::SetStatusText("", TRUE);

    } 
    _DEFAULT_HANDLER_

    return true;
}

void CDbSourceWnd::OnTruncate ()
{
    if (CDbObjListCtrl* wndListCtrl = GetCurSel()) 
        if (!strcmp(wndListCtrl->m_Data.m_szType, "TABLE"))
    {
        CTableTruncateOptionsDlg dlg;
        if (dlg.DoModal() == IDOK)
        {
            vector<DisabledFK> disabledFKs;
            Do(&CDbSourceWnd::DoTruncatePre, &disabledFKs);

            if (dlg.m_checkDependencies)
            {
                bool ignore = false;
                vector<DisabledFK>::iterator it = disabledFKs.begin();
                for (; !ignore && it != disabledFKs.end(); ++it)
                {
                    try { EXCEPTION_FRAME;

                        CString strBuff;
                        strBuff.Format("SELECT 1 FROM \"%s\".\"%s\" WHERE ROWNUM < 2", it->owner.c_str(), it->table.c_str());
                        OciCursor cursor(m_connect, strBuff, 1);
                        cursor.Execute();
                        if (cursor.Fetch())
                        {
                            strBuff.Format("Table \"%s\".\"%s\" has one or more rows."
                                "\nProbably after truncation constraint \"%s\" will be invalid."
                                "\nDo you want stop? Press <Cancel> to ignore this check.", 
                                it->owner.c_str(), it->table.c_str(), it->constraint.c_str());
                            
                            switch (AfxMessageBox(strBuff, MB_YESNOCANCEL|MB_ICONERROR))
                            {
                            case IDYES:    return;
                            case IDCANCEL: ignore = true; break;
                            }
                        }
                    } 
                    _DEFAULT_HANDLER_
                }
            }
            
            if (dlg.m_disableFKs)
            {
                vector<DisabledFK>::iterator it = disabledFKs.begin();
                for (; it != disabledFKs.end(); ++it)
                {
                    try { EXCEPTION_FRAME;

                        CString strBuff;
                        strBuff.Format("ALTER TABLE \"%s\".\"%s\" DISABLE CONSTRAINT \"%s\"", 
                            it->owner.c_str(), it->table.c_str(), it->constraint.c_str());
                        Global::SetStatusText(strBuff, TRUE);
                        m_connect.ExecuteStatement(strBuff);
                        it->enabled = false;
                    } 
                    _DEFAULT_HANDLER_
                }
            }

            Do(&CDbSourceWnd::DoTruncate);
            
            if (dlg.m_disableFKs)
            {
                vector<DisabledFK>::iterator it = disabledFKs.begin();
                for (; it != disabledFKs.end(); ++it)
                {
                    try { EXCEPTION_FRAME;

                        if (!it->enabled)
                        {
                            CString strBuff;
                            strBuff.Format("ALTER TABLE \"%s\".\"%s\" ENABLE CONSTRAINT \"%s\"", 
                                it->owner.c_str(), it->table.c_str(), it->constraint.c_str());
                            Global::SetStatusText(strBuff, TRUE);
                            m_connect.ExecuteStatement(strBuff);
                        }
                    } 
                    _DEFAULT_HANDLER_
                }
                Global::SetStatusText("", TRUE);
            }
        }
    }
}
