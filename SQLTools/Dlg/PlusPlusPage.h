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

#ifndef _PLUSPLUSPAGE_H_
#define _PLUSPLUSPAGE_H_
#pragma once

    
    class SQLToolsSettings;

class CPlusPlusPage : public CPropertyPage
{
    SQLToolsSettings& m_Settings;

public:
	CPlusPlusPage (SQLToolsSettings&);   // standard constructor

	//{{AFX_DATA(CPlusPlusPage)
	enum { IDD = IDD_PROP_PLUSPLUS };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CPlusPlusPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CPlusPlusPage)
	//}}AFX_MSG
	//}}AFX_MSG
	afx_msg void OnSelectDir();
	DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnBnClicked_EnhancedVisuals();
};

//{{AFX_INSERT_LOCATION}}
#endif//_PLUSPLUSPAGE_H_
