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

#ifndef __ABORTTHREAD_H__
#define __ABORTTHREAD_H__
#pragma once

#include "AbortDialog.h"
#include <afxmt.h>

class CAbortThread : public CWinThread
{
    friend CAbortDialog;

    DECLARE_DYNCREATE(CAbortThread)

protected:
	CAbortThread ();

// Attributes
    enum State { esSleeping, esRunning, esAbort, esShutdown };
    
    State  m_state;
    CEvent m_event;
    CMutex m_mutex;

    CAbortDialog m_AbortDialog;
    OciConnect*  m_pConnect;

// Operations
public:
    // main thread operations
    void StartBatchProcess (OciConnect* connect, unsigned abortDlgShowDelay);
    void SetActionText (LPCSTR);
    void EndBatchProcess ();
    bool IsAbort () const { return m_state == esAbort; }

    // abort thread operations
    void Abort ();

	//{{AFX_VIRTUAL(CAbortThread)
public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CAbortThread)
	//}}AFX_MSG
    afx_msg void OnStartBatchProcess (WPARAM, LPARAM);
    afx_msg void OnEndBatchProcess   (WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

class CAbortController 
{
public:
    CAbortController (CAbortThread& thread, OciConnect* connect, unsigned abortDlgShowDelay = 0) 
    : m_batchProcess(false),
    m_AbortThread(thread)
    {
        StartBatchProcess(connect, abortDlgShowDelay);
    }

    ~CAbortController ()
    {
        try {
            EndBatchProcess ();
        } _DESTRUCTOR_HANDLER_
    }

    void SetActionText (LPCSTR str)
    {
        m_AbortThread.SetActionText(str);
    }

    void StartBatchProcess (OciConnect* connect, unsigned abortDlgShowDelay = 0)
    {
        if (!m_batchProcess)
        {
            m_AbortThread.StartBatchProcess(connect, abortDlgShowDelay);
            m_batchProcess = true;
        }
    }

    void EndBatchProcess ()
    {
        if (m_batchProcess)
        {
            m_AbortThread.EndBatchProcess ();
            m_batchProcess = false;
        }
    }

    bool IsAbort () const { return m_AbortThread.IsAbort(); }

private:
    bool m_batchProcess;
    CAbortThread& m_AbortThread;

    CAbortController (const CAbortController&);
    CAbortController& operator = (const CAbortController&);
};



//{{AFX_INSERT_LOCATION}}

#endif//__ABORTTHREAD_H__
