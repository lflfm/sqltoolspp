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

#ifndef __ABORTDIALOG_H__
#define __ABORTDIALOG_H__
#pragma once

#include <afxmt.h>
#include "resource.h"

    class CAbortThread;

class CAbortDialog : public CDialog
{
    CAbortThread* m_pAbortThread;
    bool m_movedAfterActivation;
    
    CMutex m_mutex;
    CString m_connectStr, m_actionStr;

    using CDialog::ShowWindow;

public:
    CAbortDialog (CAbortThread* pAbortThread);

    void Show ();
    void Hide ();

    void SetConnectStr (LPCSTR str);
    void SetActionStr (LPCSTR str);

	//{{AFX_DATA(CAbortDialog)
	enum { IDD = IDD_ABORT_QUERY };
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CAbortDialog)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
    virtual void OnCancel ();

protected:
    //{{AFX_MSG(CAbortDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnActivateApp (BOOL bActive, DWORD dwThreadID);
    afx_msg LRESULT OnConnectStrChanged (WPARAM, LPARAM);
    afx_msg LRESULT OnActionStrChanged (WPARAM, LPARAM);
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}

#endif//__ABORTDIALOG_H__
