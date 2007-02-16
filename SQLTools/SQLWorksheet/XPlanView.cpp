// XPlanView.cpp : implementation file
//

#include "stdafx.h"
#include "SQLTools.h"
#include "XPlanView.h"


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
}

cXPlanEdit::~cXPlanEdit()
{
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

// cXPlanView message handlers

void cXPlanEdit::OnEditCopy()
{
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
    try { EXCEPTION_FRAME;
	m_doc.DoSqlDbmsXPlanDisplayCursor();
    } 
    _DEFAULT_HANDLER_
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

	return retval;
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
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
}

void cXPlanEdit::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CEdit::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	int nStart, nEnd;
	CEdit::GetSel(nStart, nEnd);

	pPopupMenu->EnableMenuItem(ID_EDIT_COPY,                 MF_BYCOMMAND|((nEnd > nStart) ? MF_ENABLED : MF_GRAYED));
	pPopupMenu->EnableMenuItem(ID_NP_SWITCH_OLD_VIEW,        MF_BYCOMMAND|((! m_IsDisplayCursor) ? MF_ENABLED : MF_GRAYED));
	pPopupMenu->EnableMenuItem(ID_NP_REFRESH,                MF_BYCOMMAND|((m_IsDisplayCursor) ? MF_ENABLED : MF_GRAYED));
}

