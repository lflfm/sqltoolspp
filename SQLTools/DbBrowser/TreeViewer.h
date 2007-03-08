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

#ifndef __TREEVIEWER_H__
#define __TREEVIEWER_H__
#pragma once

#include "TreeCtrlEx.h"

class CTreeViewer : public CTreeCtrlEx
{
    CImageList m_Images;
public:
    CTreeViewer() {};
	DECLARE_DYNCREATE(CTreeViewer)

// Operations
public:
    CSQLToolsApp* GetApp () const            { return (CSQLToolsApp*)AfxGetApp(); }
    BOOL Create (CWnd* pFrameWnd);
    using CTreeCtrlEx::Create;
    void LoadAndSetImageList (UINT nResId);
	const string GetItemStrippedText(HTREEITEM hItem, const bool b_force_alt = false);
    const string GetSelectedItemsAsText(const bool b_force_alt = false);

	//{{AFX_VIRTUAL(CTreeViewer)
	protected:
    virtual void OnDraw(CDC*) {};      // overridden to draw this view
	//}}AFX_VIRTUAL

protected:
//	virtual ~CTreeViewer();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	//{{AFX_MSG(CTreeViewer)
	afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
#endif//__TREEVIEWER_H__
