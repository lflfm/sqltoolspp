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

#pragma once

#include <afxcview.h>
#include <string>

class CExplainPlanView : public CTreeView
{
public:
	CExplainPlanView();
	virtual ~CExplainPlanView();

    void StartHtmlTree (const std::string& sqlStatement);
    void AppendHtmlItem (int parentId, int itemId, const std::string& text);
    void EndHtmlTree ();

protected:
    CImageList m_Images;
    std::string m_sqlStatement;
    std::string m_htmlTree, m_rootItem;

	DECLARE_MESSAGE_MAP()
public:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnOpenWithIE ();
    afx_msg void OnUpdate_OpenWithIE (CCmdUI *pCmdUI);
};


