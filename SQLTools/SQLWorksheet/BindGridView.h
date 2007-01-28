/* 
    This code contributed by Ken Clubok

	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2005 Aleksey Kochetov

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

// 13.03.2005 (ak) R1105003: Bind variables

#ifndef __BINDSTRGRIDVIEW_H__
#define __BINDSTRGRIDVIEW_H__

#include <OpenGrid/GridView.h>
#include <COMMON/QuickArray.h>
#include "BindSource.h"

    class CSQLToolsApp;
	class OCI8::Statement;

/** @brief Manages tab for Bind grid.
 */
class BindGridView : public OG2::GridView
{
	DECLARE_DYNCREATE(BindGridView)
    BindGridSource* m_pBindSource;

public:
	BindGridView();
	virtual ~BindGridView();

// Operations for SOURCE
    void Clear ();
    void Refresh ();
	void BindGridView::PutLine (const BindMapKey& name, EBindVariableTypes, ub2 size=1);
	void DoBinds(OCI8::AutoCursor& cursor, const vector<string>& bindVars);
/*
bool BindGridView::SetValue (const char* name, const char* value);
	bool BindGridView::GetValue (const char* name, char* value);

    int  GetCurrentRecord () const;
    int  GetRecordCount () const;
	int Find (const char* name) const;

/*
protected:
	DECLARE_MESSAGE_MAP()

    afx_msg void OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnChangeColumnFit (UINT);
    afx_msg void OnUpdate_OciGridColsToHeaders(CCmdUI *pCmdUI);
    afx_msg void OnUpdate_OciGridDataFit(CCmdUI *pCmdUI);
    afx_msg void OnRotate ();   
*/
/*
public:
    afx_msg void OnUpdate_OciGridIndicator (CCmdUI* pCmdUI);
*/
};

#endif//__BINDSTRGRIDVIEW_H__
