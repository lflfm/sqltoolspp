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

// 25.12.2004 (ak) added abortDlgShowDelay to override a default settings value

#include "stdafx.h"
#include "resource.h"
#include "SQLTools.h"
#include "AbortThread.h"
#include <OCI8/Connect.h>
#pragma warning ( disable : 4355 )

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UM_START_BATCH_PROCESS WM_USER
#define UM_END_BATCH_PROCESS   WM_USER+1

/////////////////////////////////////////////////////////////////////////////
// CAbortThread

IMPLEMENT_DYNCREATE(CAbortThread, CWinThread)

CAbortThread::CAbortThread()
: m_state(esSleeping),
  m_AbortDialog(this)
{
    //m_bAutoDelete = FALSE;
}

void CAbortThread::StartBatchProcess (OciConnect* connect, unsigned abortDlgShowDelay)
{
    _ASSERTE(connect);

    if (connect) 
    {
        CSingleLock lk(&m_mutex, TRUE);
        m_pConnect = connect;
        m_state = esRunning;
        m_event.ResetEvent();
        m_AbortDialog.SetConnectStr(theApp.GetDisplayConnectionString());
        m_AbortDialog.SetActionStr("");
        PostThreadMessage(UM_START_BATCH_PROCESS, static_cast<WPARAM>(abortDlgShowDelay), 0);
    }
}

void CAbortThread::SetActionText (LPCSTR str)
{
    //CSingleLock lk(&m_mutex, TRUE);
    m_AbortDialog.SetActionStr(str);
}

void CAbortThread::EndBatchProcess ()
{
    CSingleLock lk(&m_mutex, TRUE);
    m_pConnect = NULL;
    m_state = esSleeping;
    PostThreadMessage(UM_END_BATCH_PROCESS, 0, 0);
    // wakeup if it's on delay
    m_event.SetEvent();
}

void CAbortThread::Abort ()
{
    CSingleLock lk(&m_mutex, TRUE);
    m_state = esAbort;
    
    if (m_pConnect)
        // if there is a process then it has call EndBatchProcess
        m_pConnect->Break(true);
    else
        // if failure and process already finished
        EndBatchProcess();
}

BOOL CAbortThread::InitInstance()
{
    HWND hDlg = ::CreateDialog(AfxGetApp()->m_hInstance, 
                  MAKEINTRESOURCE(IDD_ABORT_QUERY), NULL, NULL);
    m_AbortDialog.SubclassWindow(hDlg);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CAbortThread, CWinThread)
	//{{AFX_MSG_MAP(CAbortThread)
	//}}AFX_MSG_MAP
    ON_THREAD_MESSAGE(UM_START_BATCH_PROCESS, OnStartBatchProcess)
    ON_THREAD_MESSAGE(UM_END_BATCH_PROCESS,   OnEndBatchProcess)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbortThread message handlers

void CAbortThread::OnStartBatchProcess (WPARAM wparam, LPARAM)
{
    // some delay
    CSingleLock wait(&m_event, FALSE);
    
    unsigned abortDlgShowDelay = static_cast<unsigned>(wparam);
    
    if (!abortDlgShowDelay)
        abortDlgShowDelay = GetSQLToolsSettings().GetCancelQueryDelay();

    wait.Lock(abortDlgShowDelay * 1000);

    if (m_state == esRunning)
    {
        CSingleLock lk(&m_mutex, TRUE);
        m_AbortDialog.Show();
    }
}

void CAbortThread::OnEndBatchProcess (WPARAM, LPARAM)
{
    if (m_state != esRunning) 
    {
        CSingleLock lk(&m_mutex, TRUE);
        m_AbortDialog.Hide();
    }
}
