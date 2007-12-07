// XPlanView.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "XPlanView.h"
#include "COMMON/GUICommandDictionary.h"
#include "OpenGrid/GridView.h"
// #include "OpenEditor/OEView.h"

// cXPlanView

IMPLEMENT_DYNCREATE(cXPlanView, CEditView)

cXPlanView::cXPlanView()
{

}

cXPlanView::~cXPlanView()
{
}

BEGIN_MESSAGE_MAP(cXPlanView, CEditView)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_CONTROL_REFLECT(EN_CHANGE, OnEditChange)
END_MESSAGE_MAP()


// cXPlanView diagnostics

#ifdef _DEBUG
void cXPlanView::AssertValid() const
{
	CEditView::AssertValid();
}

#ifndef _WIN32_WCE
void cXPlanView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif
#endif //_DEBUG


// cXPlanView message handlers

void cXPlanView::OnLButtonDblClk(UINT , CPoint )
{
	// TODO: Add your message handler code here and/or call default
	ShowWindow(SW_HIDE);

	m_OldView->ShowWindow(SW_SHOW);
	m_OldView->SetFocus();

	// CEditView::OnLButtonDblClk(nFlags, point);
}

int  cXPlanView::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
	int retval = CEditView::OnCreate(lpCreateStruct);
	
	VisualAttribute attr;

	m_Font.CreateFont(
          -attr.PointToPixel(9), 0, 0, 0,
          FW_NORMAL,
          0,
          0,
          0, ANSI_CHARSET,//DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
          "Courier New"
        );

	// m_Font.CreatePointFont(10, "Courier New");

	SetFont(&m_Font);

	return retval;
}

void cXPlanView::OnEditChange()
{
	ASSERT_VALID(this);
	// GetDocument()->SetModifiedFlag();
	// ASSERT_VALID(this);

	// return FALSE;   // continue routing
}

/*
cXPlanEdit::cXPlanEdit()
{

}
*/

cXPlanEdit::cXPlanEdit(CPLSWorksheetDoc& doc) : m_doc(doc)
{
	m_IsDisplayCursor = false;
	m_accelTable = 0;
}

cXPlanEdit::~cXPlanEdit()
{
	if (m_accelTable)
		DestroyAcceleratorTable(m_accelTable);
}

BEGIN_MESSAGE_MAP(cXPlanEdit, CEdit)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_NP_SWITCH_OLD_VIEW, OnSwitchOldView)
	ON_COMMAND(ID_NP_REFRESH, OnRefreshPlan)
	ON_WM_CONTEXTMENU()
    ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)

	// ON_CONTROL_REFLECT(STN_DBLCLK, &cXPlanEdit::OnStnDblclick)
END_MESSAGE_MAP()


// cXPlanEdit diagnostics

/*
#ifdef _DEBUG
void cXPlanEdit::AssertValid() const
{
	cXPlanEdit::AssertValid();
}

#ifndef _WIN32_WCE
void cXPlanEdit::Dump(CDumpContext& dc) const
{
	CEdit::Dump(dc);
}
#endif
#endif //_DEBUG
*/

// cXPlanEdit message handlers

void cXPlanEdit::OnEditCopy()
{
	int nStart, nEnd;
	CEdit::GetSel(nStart, nEnd);

	if (nStart >= nEnd)
		OnEditSelectAll();

	CEdit::Copy();
}

void cXPlanEdit::OnEditSelectAll()
{
	CEdit::SetSel(0, -1, TRUE);
}

void cXPlanEdit::OnSwitchOldView()
{
	ShowWindow(SW_HIDE);

	m_OldView->ShowWindow(SW_SHOW);
	m_OldView->SetFocus();
}

void cXPlanEdit::OnRefreshPlan()
{
	if (m_IsDisplayCursor)
	{
		try { EXCEPTION_FRAME;
		m_doc.DoSqlDbmsXPlanDisplayCursor();
		} 
		_DEFAULT_HANDLER_
    } 
}

void cXPlanEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CEdit::OnLButtonDblClk(nFlags, point);
}

int  cXPlanEdit::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
    int retval = CEdit::OnCreate(lpCreateStruct);
		
    VisualAttribute attr;

	m_Font.CreateFont(
          -attr.PointToPixel(9), 0, 0, 0,
          FW_NORMAL,
          0,
          0,
          0, ANSI_CHARSET,//DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
          "Courier New"
        );

	// m_Font.CreatePointFont(10, "Courier New");

	SetFont(&m_Font);

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_NEWPLAN_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

	m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(pPopup->m_hMenu);

	((CMDIMainFrame *)AfxGetMainWnd())->SetXPlanEdit(this);

	return retval;
}

BOOL cXPlanEdit::PreTranslateMessage(MSG* pMsg)
{
	if (m_accelTable)
		if (TranslateAccelerator(m_hWnd, m_accelTable, pMsg) == 0)
			return CEdit::PreTranslateMessage(pMsg);
		else
			return true;
	else
		return CEdit::PreTranslateMessage(pMsg);
}

/*
void cXPlanEdit::OnStnDblclick()
{
	// TODO: Add your control notification handler code here
	ShowWindow(SW_HIDE);

	m_OldView->ShowWindow(SW_SHOW);
	m_OldView->SetFocus();
}
*/

void cXPlanEdit::OnContextMenu (CWnd* , CPoint pos)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_NEWPLAN_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
}

void cXPlanEdit::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CEdit::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	// pPopupMenu->EnableMenuItem(ID_EDIT_COPY,                 MF_BYCOMMAND|((nEnd > nStart) ? MF_ENABLED : MF_GRAYED));
	pPopupMenu->EnableMenuItem(ID_NP_SWITCH_OLD_VIEW,        MF_BYCOMMAND|((! m_IsDisplayCursor) ? MF_ENABLED : MF_GRAYED));
	pPopupMenu->EnableMenuItem(ID_NP_REFRESH,                MF_BYCOMMAND|((m_IsDisplayCursor) ? MF_ENABLED : MF_GRAYED));
}

// CPopupFrameWnd

IMPLEMENT_DYNCREATE(CPopupFrameWnd, CFrameWnd)

void CPopupFrameWnd::SetGridPopupWordWrap(bool bWordWrap)
{
    CString theText;
    CRect theRect = CFrameWnd::rectDefault;

    if (m_EditBox)
    {
        m_EditBox->GetWindowText(theText);
        m_EditBox->GetWindowRect(theRect);
        ScreenToClient(theRect);

        delete m_EditBox;
    }

    m_EditBox = new CGridPopupEdit(bWordWrap);

    DWORD nStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY|ES_NOHIDESEL;

    if (! bWordWrap)
        nStyle |= WS_HSCROLL|ES_AUTOHSCROLL;

    if (!m_EditBox->Create(nStyle,
		    theRect, this, IDC_STATIC)) 
    {
	    MessageBox("The edit control was not created");
	}

    m_EditBox->SetWindowText(theText);
    m_EditBox->SetFocus();
}

CPopupFrameWnd::CPopupFrameWnd()
{
    m_EditBox = 0;
}

CPopupFrameWnd::~CPopupFrameWnd()
{
    if (m_EditBox)
        delete m_EditBox;
}

BEGIN_MESSAGE_MAP(CPopupFrameWnd, CFrameWnd)
	ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()


// CPopupFrameWnd diagnostics

#ifdef _DEBUG
void CPopupFrameWnd::AssertValid() const
{
	CFrameWnd::AssertValid();
}

#ifndef _WIN32_WCE
void CPopupFrameWnd::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif
#endif //_DEBUG


// CPopupFrameWnd message handlers

void CPopupFrameWnd::OnSetFocus(CWnd *)
{
    m_EditBox->SetFocus();
}

int  CPopupFrameWnd::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
	int retval = CFrameWnd::OnCreate(lpCreateStruct);

    /*
    VisualAttribute attr;

	m_Font.CreateFont(
          -attr.PointToPixel(9), 0, 0, 0,
          FW_NORMAL,
          0,
          0,
          0, ANSI_CHARSET,//DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
          "Courier New"
        );
    */

    m_EditBox = new CGridPopupEdit;

    if (!m_EditBox->Create(WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_READONLY|ES_NOHIDESEL,
		    CFrameWnd::rectDefault, this, IDC_STATIC)) 
    {
	    MessageBox("The edit control was not created");
	    return -1;
	}
    // m_EditBox.SetFont(&m_Font);
    m_EditBox->SetWindowText("This is the edit control;\r\nobviously it was created");

	return retval;
}

void CPopupFrameWnd::OnSize(UINT nType, int cx, int cy)
{
    if (nType==SIZE_RESTORED || nType==SIZE_MAXIMIZED) 
    {
        m_EditBox->MoveWindow(0, 0, cx, cy);
    }
}

void CPopupFrameWnd::OnDestroy()
{
    ((GridView *)GetParent())->SetPopupToNull();
}

CGridPopupEdit::CGridPopupEdit(bool bWordWrap)
{
	m_accelTable = 0;
    m_bWordWrap = bWordWrap;
}

CGridPopupEdit::CGridPopupEdit()
{
	m_accelTable = 0;
    m_bWordWrap = true;
}

CGridPopupEdit::~CGridPopupEdit()
{
	if (m_accelTable)
		DestroyAcceleratorTable(m_accelTable);
}

BEGIN_MESSAGE_MAP(CGridPopupEdit, CEdit)
	ON_WM_CREATE()
	ON_COMMAND(ID_GRIDPOPUP_WORDWRAP, OnGridPopupWordWrap)
	ON_WM_CONTEXTMENU()
    ON_WM_INITMENUPOPUP()
    ON_WM_KEYDOWN()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_COMMAND(ID_GRIDPOPUP_CLOSE, OnGridPopupClose)

	// ON_CONTROL_REFLECT(STN_DBLCLK, &CGridPopupEdit::OnStnDblclick)
END_MESSAGE_MAP()


// CGridPopupEdit diagnostics

/*
#ifdef _DEBUG
void CGridPopupEdit::AssertValid() const
{
	CGridPopupEdit::AssertValid();
}

#ifndef _WIN32_WCE
void CGridPopupEdit::Dump(CDumpContext& dc) const
{
	CEdit::Dump(dc);
}
#endif
#endif //_DEBUG
*/

// CGridPopupEdit message handlers

void CGridPopupEdit::OnGridPopupWordWrap()
{
    m_bWordWrap = ! m_bWordWrap;

    /* Stupid windows, changing these styles just does not work, you need to recreate the control...
    LONG nStyle = GetWindowLong(m_hWnd, GWL_STYLE);
    if (! m_bWordWrap)
        nStyle |= WS_HSCROLL|ES_AUTOHSCROLL;
    else
        nStyle &= ~(WS_HSCROLL|ES_AUTOHSCROLL);

    SetWindowLong(m_hWnd, GWL_STYLE, nStyle);

    LONG nStyle = WS_HSCROLL|ES_AUTOHSCROLL;

    ModifyStyle(m_bWordWrap ? nStyle : 0, m_bWordWrap ? 0 : nStyle, SWP_NOSIZE);

    Invalidate();
    */

    ((CPopupFrameWnd *)GetParent())->SetGridPopupWordWrap(m_bWordWrap);
}

void CGridPopupEdit::OnEditCopy()
{
	int nStart, nEnd;
	CEdit::GetSel(nStart, nEnd);

	if (nStart >= nEnd)
		OnEditSelectAll();

	CEdit::Copy();
}

void CGridPopupEdit::OnEditSelectAll()
{
	CEdit::SetSel(0, -1, TRUE);
}

int  CGridPopupEdit::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
    int retval = CEdit::OnCreate(lpCreateStruct);
		
    VisualAttribute attr;

    /*const VisualAttributesSet& set = ((COEditorView *) ((CMDIMainFrame *)AfxGetMainWnd())->GetActiveView())->GetEditorSettings().GetVisualAttributesSet();
    const VisualAttribute& textAttr = set.FindByName("Text");

    SetFont(textAttr.NewFont());
    */

    m_Font.CreateFont(
          -attr.PointToPixel(9), 0, 0, 0,
          FW_NORMAL,
          0,
          0,
          0, ANSI_CHARSET,//DEFAULT_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
          "Courier New"
        );

	SetFont(&m_Font);

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_GRIDPOPUP_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

	m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(pPopup->m_hMenu);

    return retval;
}

void CGridPopupEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    switch (nChar)
    {
    case VK_ESCAPE:
        OnGridPopupClose();
        return;
        break;
    }

    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CGridPopupEdit::PreTranslateMessage(MSG* pMsg)
{
	if (m_accelTable)
		if (TranslateAccelerator(m_hWnd, m_accelTable, pMsg) == 0)
			return CEdit::PreTranslateMessage(pMsg);
		else
			return true;
	else
		return CEdit::PreTranslateMessage(pMsg);
}

void CGridPopupEdit::OnContextMenu (CWnd* , CPoint pos)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_GRIDPOPUP_OPTIONS));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);

    const int MENU_ITEM_TEXT_SIZE   = 80;
    const int MENUITEMINFO_OLD_SIZE = 44;

    MENUITEMINFO mii;
    memset(&mii, 0, sizeof mii);
    mii.cbSize = MENUITEMINFO_OLD_SIZE;  // 07.04.2003 bug fix, no menu shortcut labels on Win95,... because of SDK incompatibility
    mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

    char buffer[MENU_ITEM_TEXT_SIZE];
    mii.dwTypeData = buffer;
    mii.cch = sizeof buffer;

    if (pPopup->GetMenuItemInfo(ID_GRIDPOPUP_CLOSE, &mii))
    {
        string menuText = buffer;

        if (menuText.find('\t') != string::npos)
            menuText += "; ";
        else
            menuText += "\t";

        menuText += "ESC";

        strcpy(buffer, menuText.c_str());

        mii.cch = menuText.size() + 1;

        pPopup->SetMenuItemInfo(ID_GRIDPOPUP_CLOSE, &mii);
    }

    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
}

void CGridPopupEdit::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CEdit::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    pPopupMenu->CheckMenuItem(ID_GRIDPOPUP_WORDWRAP, MF_BYCOMMAND|((m_bWordWrap) ? MF_CHECKED : MF_UNCHECKED));
}

void CGridPopupEdit::OnGridPopupClose()
{
    ((CPopupFrameWnd *)GetParent())->SendMessage(WM_CLOSE);
}

