#pragma once

#include "ExplainPLanView.h"

// cXPlanView view

class cXPlanView : public CEditView
{
	DECLARE_DYNCREATE(cXPlanView)

public:
	cXPlanView();
	virtual ~cXPlanView();

	CExplainPlanView* m_OldView;

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

	void setOldView(CExplainPlanView* p_OldView) {m_OldView = p_OldView;}

private:
	CFont m_Font;

protected:
	// afx_msg BOOL OnEditChange();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditChange();
};

class cXPlanEdit : public CEdit
{

public:
	cXPlanEdit();
	virtual ~cXPlanEdit();

	CExplainPlanView* m_OldView;

public:
/*
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif
*/
	void setOldView(CExplainPlanView* p_OldView) {m_OldView = p_OldView;}

private:
	CFont m_Font;

protected:
	// afx_msg BOOL OnEditChange();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
	// afx_msg void OnStnDblclick();
};

