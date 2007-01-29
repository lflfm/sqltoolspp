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

void cXPlanView::OnLButtonDblClk(UINT nFlags, CPoint point)
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

cXPlanEdit::cXPlanEdit()
{

}

cXPlanEdit::~cXPlanEdit()
{
}

BEGIN_MESSAGE_MAP(cXPlanEdit, CEdit)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
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

void cXPlanEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	ShowWindow(SW_HIDE);

	m_OldView->ShowWindow(SW_SHOW);
	m_OldView->SetFocus();

	// CEditView::OnLButtonDblClk(nFlags, point);
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
