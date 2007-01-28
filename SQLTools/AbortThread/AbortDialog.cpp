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

#include "stdafx.h"
#include "resource.h"
#include "SQLTools.h"
#include "AbortDialog.h"
#include "AbortThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UM_ACTION_STR_CHANGED   WM_USER
#define UM_CONNECT_STR_CHANGED  WM_USER+1

BEGIN_MESSAGE_MAP(CAbortDialog, CDialog)
	//{{AFX_MSG_MAP(CAbortDialog)
	//}}AFX_MSG_MAP
    ON_WM_ACTIVATEAPP()
    ON_MESSAGE(UM_ACTION_STR_CHANGED, OnActionStrChanged)
    ON_MESSAGE(UM_CONNECT_STR_CHANGED, OnConnectStrChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbortDialog message handlers

CAbortDialog::CAbortDialog (CAbortThread* pAbortThread)
    : CDialog(CAbortDialog::IDD, NULL),
    m_pAbortThread(pAbortThread), 
    m_movedAfterActivation(false)
{ 
}

void CAbortDialog::Show ()
{
    m_movedAfterActivation = false;
    bool topmost = GetSQLToolsSettings().GetTopmostCancelQuery();

    CenterWindow(AfxGetMainWnd());

    if (!topmost) 
        ModifyStyleEx(0, WS_EX_APPWINDOW);
    else
        ModifyStyleEx(WS_EX_APPWINDOW, 0);

    SetWindowPos(topmost ? &wndTopMost : &wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);

    SetForegroundWindow();
}

void CAbortDialog::Hide ()
{
    if (IsWindowVisible())
    {
        ShowWindow(SW_HIDE);
        AfxGetMainWnd()->SetForegroundWindow();
    }
}

void CAbortDialog::OnCancel () 
{
    m_pAbortThread->Abort();
}

BOOL CAbortDialog::PreCreateWindow(CREATESTRUCT& cs) 
{
    static bool s_bRegister = false;
    LPSCSTR s_szClassName = "AbortDialog";

    if (!s_bRegister) 
    {
	    WNDCLASS wndcls;
        if (GetClassInfo(NULL, cs.lpszClass, &wndcls)) 
        {
            wndcls.style         = ~CS_SAVEBITS & wndcls.style;
            wndcls.lpszClassName = s_szClassName;
            
            s_bRegister = AfxRegisterClass(&wndcls) ? true : false;
        }
    }
    if (s_bRegister)
        cs.lpszClass = s_szClassName;
  
	return CDialog::PreCreateWindow(cs);
}

void CAbortDialog::OnActivateApp (BOOL bActive, DWORD dwThreadID)
{
    CDialog::OnActivateApp(bActive, dwThreadID);

    if (!bActive && !m_movedAfterActivation
    && GetSQLToolsSettings().GetTopmostCancelQuery())
    {
        m_movedAfterActivation = true;

        CRect rc, desktopRc;
        GetWindowRect(rc);
        GetDesktopWindow()->GetWindowRect(desktopRc);
        SetWindowPos(&wndTopMost, desktopRc.right - rc.Width() - 10, desktopRc.bottom - rc.Height() - 10, 
            0, 0, SWP_NOSIZE);
    }
}

void CAbortDialog::SetConnectStr (LPCSTR str)         
{ 
    CSingleLock lk(&m_mutex, TRUE);
    m_connectStr = str;
    PostMessage(UM_CONNECT_STR_CHANGED);
}

void CAbortDialog::SetActionStr (LPCSTR str)         
{ 
    CSingleLock lk(&m_mutex, TRUE);
    m_actionStr = str;
    PostMessage(UM_ACTION_STR_CHANGED);
}

LRESULT CAbortDialog::OnActionStrChanged (WPARAM, LPARAM)
{
    CSingleLock lk(&m_mutex, TRUE);
    ::SetWindowText(::GetDlgItem(m_hWnd, IDC_ABORT_ACTION), m_actionStr);
    return 0;
}

LRESULT CAbortDialog::OnConnectStrChanged (WPARAM, LPARAM)
{
    CSingleLock lk(&m_mutex, TRUE);
    ::SetWindowText(::GetDlgItem(m_hWnd, IDC_ABORT_CONNECTION), m_connectStr);
    return 0;
}



LRESULT CAbortDialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
