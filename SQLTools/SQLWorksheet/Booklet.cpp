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
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables

// TODO: add a context menu and an option to hide/show text labels

#include "stdafx.h"
#include "Booklet.h"
#include "SQLWorksheetDoc.h"
#include <OpenGrid/GridView.h>

#include "XPlanView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;

IMPLEMENT_DYNCREATE(CBooklet, CView)

    const int TAB_BUTTON_OFFSET = 2;
    const int TAB_BUTTON_NUMBER = 6;
        //= sizeof(CPLSWorksheetDoc::m_BookletFamily)/sizeof(CPLSWorksheetDoc::m_BookletFamily[0]);
    const int ALL_BUTTON_OFFSET = 8;

    const int   PIN_IMAGE_ID = 17;
    const int   PIN_UNIMAGE_ID = 18;
    const char* PIN_IMAGE_TEXT = "Disable auto switching panes";
    const char* PIN_UNIMAGE_TEXT = "Enable auto switching panes";

    struct 
    {
        int   iBitmap;   // zero-based index of button image
        int   idCommand; // command to be sent when button pressed
        BYTE  fsState;   // button state--see below
        BYTE  fsStyle;   // button style--see below
        const char* szString;  // label string
    } 
    g_buttons[] =
    {
        { PIN_IMAGE_ID, ID_BOOKLET_PIN, TBSTATE_ENABLED, TBSTYLE_CHECK, PIN_IMAGE_TEXT },
        { 0,  0, 0, TBSTYLE_SEP, 0 },
        { 0,  ID_BOOKLET_RESULT,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "Query     " },
        { 1,  ID_BOOKLET_STATISTICS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "Statistics" },
        { 2,  ID_BOOKLET_SQL_PLAN,   TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "Plan      " },
        { 3,  ID_BOOKLET_OUTPUT,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "Output    " },
        { 4,  ID_BOOKLET_HISTORY,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "History   " },
		{ 5,  ID_BOOKLET_BINDS,	     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP|BTNS_SHOWTEXT, "Binds" },
        { 0,  0, 0, TBSTYLE_SEP, 0 },
        { 6,  ID_GRID_ROTATE,  TBSTATE_ENABLED,    TBSTYLE_BUTTON,  "Rotate grid"   },
        
//        { 6,  ID_GRID_DATA_FIT,         TBSTATE_ENABLED,    TBSTYLE_BUTTON,  "Size Columns to Data"   },
//        { 7,  ID_GRID_HEADER_FIT,       TBSTATE_ENABLED,    TBSTYLE_BUTTON,  "Size Columns to Headers"   },
        { 7,  ID_OCIGRID_DATA_FIT,        TBSTATE_ENABLED,    TBSTYLE_BUTTON,  "Size Columns to Data"   },
        { 8,  ID_OCIGRID_COLS_TO_HEADERS, TBSTATE_ENABLED,    TBSTYLE_BUTTON,  "Size Columns to Headers"   },

        { 0,  0, 0, TBSTYLE_SEP, 0 },
        { 9,  ID_GRID_MOVE_HOME,        TBSTATE_ENABLED,    TBSTYLE_BUTTON, "The Home"      },
        { 10,  ID_GRID_MOVE_PGUP,        TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Page Up"       },
        { 11, ID_GRID_MOVE_UP,          TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Previous"      },
        { 12, ID_GRID_MOVE_DOWN,        TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Next"          },
        { 13, ID_GRID_MOVE_PGDOWN,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Page Down"     },
        { 14, ID_GRID_MOVE_END,         TBSTATE_ENABLED,    TBSTYLE_BUTTON, "The End"       },
        { 0,  0, 0, TBSTYLE_SEP, 0 },
        { 21, ID_GRID_CLEAR,            TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Clear rows from current grid"   },
        { 0,  0, 0, TBSTYLE_SEP, 0 },
//        { 14, ID_GRID_EXPORT,           TBSTATE_ENABLED,    BTNS_DROPDOWN,  "Export data"   },
        { 19, ID_GRID_OPEN_WITH_IE,     TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Open with default HTML viewer..."  },
        { 20, ID_GRID_OPEN_WITH_EXCEL,  TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Open with default CSV viewer..."   },
        { 0,  0, 0, TBSTYLE_SEP, 0 },
        { 16, ID_GRID_SETTINGS,         TBSTATE_ENABLED,    TBSTYLE_BUTTON, "Settings..."   },
    };

//TBSTYLE_DROPDOWN

/////////////////////////////////////////////////////////////////////////////
// CBooklet

CBooklet::CBooklet ()
    : m_pDocument(0), 
    m_nActiveItem(0),
    m_tabPinned(false)
{
}

CBooklet::~CBooklet()
{
}


/////////////////////////////////////////////////////////////////////////////
// Operations

bool CBooklet::SetDocument (CPLSWorksheetDoc* doc)
{
    _ASSERTE(doc);
    m_pDocument = doc;

    if (m_pDocument)
    {
        m_pDocument->Init();

        for (int i(0); i < TAB_BUTTON_NUMBER; i++)
        {
            if (!m_pDocument->m_BookletFamily[i]
            || !m_pDocument->m_BookletFamily[i]->CreateEx(0, NULL, NULL, WS_CHILD|WS_HSCROLL|WS_VSCROLL, CRect(0,0,0,0), this, 100))
                return false;

            m_pDocument->AddView(m_pDocument->m_BookletFamily[i]);
            m_Tabs.push_back(m_pDocument->m_BookletFamily[i]);
        }

		//SSNOTIFY CView: Create(NULL, NULL, WS_CHILD|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|ES_WANTRETURN, CRect(0,0,0,0), this, 111))
		if (! m_pDocument->m_pXPlan->Create(ES_READONLY|ES_MULTILINE|ES_WANTRETURN|WS_CHILD|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE, CRect(0,0,0,0), this, 111))
		{
            MessageBeep((UINT)-1);
            AfxMessageBox("Fatal error: cannot create a new XPlan window.");
            AfxThrowUserException( );
		}
		else
		{
			// m_pDocument->AddView(m_pDocument->m_pXPlan);
		}

		ActivateTab(0);
    }
    return true;
}

void CBooklet::ActivateTab (int nTab)
{
    _ASSERTE(nTab < static_cast<int>(m_Tabs.size()));

    if (nTab < static_cast<int>(m_Tabs.size()))
    {
		if (m_pDocument->m_pXPlan->IsWindowVisible())
			m_pDocument->m_pXPlan->ShowWindow(SW_HIDE);

		if (getActiveView())
            getActiveView()->ShowWindow(SW_HIDE);

        m_nActiveItem = nTab;

        _ASSERTE(nTab < sizeof(g_buttons)/sizeof(g_buttons[0]));
        m_toolbar.CheckButton(g_buttons[nTab + TAB_BUTTON_OFFSET].idCommand);

        if (getActiveView()) 
            getActiveView()->ShowWindow(SW_SHOW);
    }
}

void CBooklet::ActivateTab (CView* pWnd)
{
    vector<CView*>::const_iterator 
        it(m_Tabs.begin()), end(m_Tabs.end());

    for (int i(0); it != end; it++, i++) 
    {
        if (*it == pWnd) 
        {
            ActivateTab(i);
            break;
        }
    }
    _ASSERTE(it != end);
}

BOOL CBooklet::GetActiveTab (int& nTab) const
{
    _ASSERTE(m_Tabs.size() > 0);

    nTab = m_nActiveItem;

    return (m_Tabs.size() > 0) ? TRUE : FALSE;
}

BOOL CBooklet::GetActiveTab (CView*& pWnd) const
{
    _ASSERTE(m_Tabs.size() > 0);

    if (m_Tabs.size())
        pWnd = getActiveView();

    return (m_Tabs.size() > 0) ? TRUE : FALSE;
}


BEGIN_MESSAGE_MAP(CBooklet, CView)
	//{{AFX_MSG_MAP(CBooklet)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_BOOKLET_PIN,       OnPin)
    ON_COMMAND(ID_BOOKLET_RESULT,    OnQuery)
    ON_COMMAND(ID_BOOKLET_STATISTICS,OnStatistics)
    ON_COMMAND(ID_BOOKLET_SQL_PLAN,  OnPlan)
    ON_COMMAND(ID_BOOKLET_OUTPUT,    OnOutput)
    ON_COMMAND(ID_BOOKLET_HISTORY,   OnHistory)
	ON_COMMAND(ID_BOOKLET_BINDS,     OnBinds)
    ON_COMMAND_RANGE(ID_GRID_FIST_COMMAND, ID_GRID_LAST_COMMAND, OnGridCommand)
    ON_COMMAND_RANGE(ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, OnGridCommand)
    ON_WM_ERASEBKGND()
    ON_MESSAGE_VOID(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBooklet message handlers

int CBooklet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_toolbar.Create(WS_VISIBLE | WS_BORDER | WS_CHILD 
            | CCS_TOP | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS,
	        CRect(0,0,0,0),this, 111) == -1)
        return -1;

//    m_toolbar.ModifyStyleEx(0, WS_EX_WINDOWEDGE);
//    m_toolbar.SetButtonSize(CSize(18, 15));
    m_toolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
    m_toolbar.SetBitmapSize(CSize(16, 16));
    m_toolbar.SetIndent(4);

    VERIFY(m_toolbar.AddBitmap(17, IDT_BOOKLET) != -1);

    TBBUTTONINFO btnInfo;
    memset(&btnInfo, 0, sizeof(btnInfo));
    btnInfo.cbSize = sizeof(btnInfo);
    btnInfo.dwMask = TBIF_TEXT|TBIF_BYINDEX;

    for (int i = 0; i < sizeof(g_buttons)/sizeof(g_buttons[0]); i++)
    {
        TBBUTTON btn;
        btn.iBitmap    = g_buttons[i].iBitmap  ;
        btn.idCommand  = g_buttons[i].idCommand;
        btn.fsState    = g_buttons[i].fsState  ;
        btn.fsStyle    = g_buttons[i].fsStyle  ;
	    VERIFY(m_toolbar.AddButtons(1, &btn));

        if (g_buttons[i].szString)
        {
            btnInfo.pszText = const_cast<char*>(g_buttons[i].szString);
            m_toolbar.SetButtonInfo(i, &btnInfo);
        }
    }

	m_toolbar.AutoSize();

	return 0;
}

void CBooklet::OnSize (UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
    m_toolbar.AutoSize();
    
    if (cx > 0 && cy > 0)
    {
        CRect rc;
        m_toolbar.GetWindowRect(rc);

	    for (int i = 0, size = m_Tabs.size(); i < size; i++) 
        {
            if (m_Tabs[i])
                m_Tabs[i]->SetWindowPos(0, 0, rc.Height()-1, cx, cy - rc.Height()+1, SWP_NOZORDER);
	    }

		if (m_pDocument)
			m_pDocument->m_pXPlan->SetWindowPos(0, 0, rc.Height()-1, cx, cy - rc.Height()+1, SWP_NOZORDER);
    }
}

void CBooklet::OnPin ()
{
    m_tabPinned = !m_tabPinned;

    TBBUTTONINFO info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    info.dwMask = TBIF_IMAGE|TBIF_TEXT;
    info.iImage  = !m_tabPinned ? PIN_IMAGE_ID : PIN_UNIMAGE_ID;
    info.pszText = const_cast<char*>(!m_tabPinned ? PIN_IMAGE_TEXT : PIN_UNIMAGE_TEXT);
    m_toolbar.SetButtonInfo(ID_BOOKLET_PIN, &info);
}


void CBooklet::OnQuery ()
{
    ActivateTab(0);
    getActiveView()->SetFocus();
}

void CBooklet::OnStatistics ()
{
    ActivateTab(1);
    getActiveView()->SetFocus();
}

void CBooklet::OnPlan ()
{
    ActivateTab(2);
    getActiveView()->SetFocus();

	if (m_pDocument->m_pXPlan->GetWindowTextLength() > 0)
	{
		getActiveView()->ShowWindow(SW_HIDE);
		m_pDocument->m_pXPlan->ShowWindow(SW_SHOW);
		m_pDocument->m_pXPlan->SetFocus();
	}
}

void CBooklet::OnOutput ()
{
    ActivateTab(3);
    getActiveView()->SetFocus();
}

void CBooklet::OnHistory ()
{
    ActivateTab(4);
    getActiveView()->SetFocus();
}

void CBooklet::OnBinds ()
{
	ActivateTab(5);
	getActiveView()->SetFocus();
}

void CBooklet::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    m_pDocument = (CPLSWorksheetDoc*)GetDocument();

    if (m_pDocument)
    {
        m_pDocument->Init();

        for (int i(0); i < TAB_BUTTON_NUMBER; i++)
        {
            if (!m_pDocument->m_BookletFamily[i]
            || !m_pDocument->m_BookletFamily[i]->CreateEx(0, NULL, NULL, WS_CHILD|WS_HSCROLL|WS_VSCROLL, CRect(0,0,0,0), this, 100))
            {
                MessageBeep((UINT)-1);
                AfxMessageBox("Fatal error: cannoct create a child window.");
                AfxThrowUserException( );
            }

            m_pDocument->AddView(m_pDocument->m_BookletFamily[i]);
            m_Tabs.push_back(m_pDocument->m_BookletFamily[i]);
        }

		//SSNOTIFY CView: Create(NULL, NULL, WS_CHILD|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|ES_WANTRETURN, CRect(0,0,0,0), this, 111))
		if (! m_pDocument->m_pXPlan->Create(ES_READONLY|ES_MULTILINE|ES_WANTRETURN|WS_CHILD|WS_HSCROLL|WS_VSCROLL, CRect(0,0,0,0), this, 111))
		{
            MessageBeep((UINT)-1);
            AfxMessageBox("Fatal error: cannot create a new XPlan window.");
            AfxThrowUserException( );
		}
		else
		{
			// m_pDocument->AddView(m_pDocument->m_pXPlan);
		}

        ActivateTab(0);
    }
}

BOOL CBooklet::OnEraseBkgnd (CDC*)
{
    return FALSE;
}


void CBooklet::OnGridCommand (UINT nID)
{
    CView* view = getActiveView();
    view->GetParentFrame()->SetActiveView(view);
    view->OnCmdMsg(nID, 0, 0, 0);
}

class CToolbarCmdUI : public CCmdUI
{
    CToolBarCtrl& m_toolbar;
public:
    CToolbarCmdUI (CToolBarCtrl& toolbar) : m_toolbar(toolbar) {}

	virtual void Enable (BOOL bOn = TRUE)
    {
        m_toolbar.EnableButton(m_nID, bOn);
    }
	virtual void SetCheck (int nCheck = 1)   // 0, 1 or 2 (indeterminate)
    {
        // ignore 2 
        m_toolbar.CheckButton(m_nID, nCheck == 1);
    }
};

void CBooklet::OnIdleUpdateCmdUI ()
{
    CToolbarCmdUI state(m_toolbar);
    for (int i = ALL_BUTTON_OFFSET; i < sizeof(g_buttons)/sizeof(g_buttons[0]); i++)
        if (g_buttons[i].idCommand)
        {
            state.m_nID = g_buttons[i].idCommand;
            if (!state.DoUpdate(getActiveView(), FALSE))
                state.Enable(FALSE);
        }
}

LRESULT CBooklet::WindowProc (UINT message, WPARAM wParam, LPARAM lParam)
{
    try { return CView::WindowProc(message, wParam, lParam); }
    _DEFAULT_HANDLER_;
    return 0;
}
