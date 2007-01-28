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
#ifndef __SQLTOOLS_H__
#define __SQLTOOLS_H__

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <memory>
#include "resource.h"               // main symbols
#include "COMMON\DocManagerExt.h"
#include <OCI8/Connect.h>
#include "AbortThread/AbortThread.h"
#include "SQLToolsSettings.h"
#include "CommandLineparser.h"

class CSQLToolsApp : public CWinApp, SettingsSubscriber
{
	DECLARE_DYNCREATE(CSQLToolsApp)

    std::auto_ptr<OciConnect> m_connect;
    CString m_serverVerion; // cache it for exception handling
    CString m_displayConnectionString;
    CString m_orgMainWndTitle;
    static CString m_displayNotConnected;

    CMultiDocTemplateExt* m_pPLSDocTemplate;
    CCommandLineParser m_commandLineParser;

    HANDLE m_hMutex;
    HACCEL m_accelTable;
    int m_dblKeyAccelInx;

    void InitGUICommand ();
    void UpdateAccelAndMenu ();
    BOOL AllowThisInstance ();
    BOOL HandleAnotherInstanceRequest (COPYDATASTRUCT* pCopyDataStruct);

    friend CAbortThread* GetAbortThread ();
    static CAbortThread* m_pAbortThread;

    void DoConnect (const char* user, const char* pwd, const char* alias, int mode, int safety);
    void DoConnect (const char* user, const char* pwd, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid, int mode, int safety);

    bool EvAfterOpenConnect   (OciConnect&);
    bool EvBeforeCloseConnect (OciConnect&);
    bool EvAfterCloseConnect  (OciConnect&);

    void DoFileSaveAll (bool silent, bool skipNew);

    static SQLToolsSettings m_settings;
    friend const SQLToolsSettings& GetSQLToolsSettings ();
    friend SQLToolsSettings& GetSQLToolsSettingsForUpdate ();
    friend bool ShowDDLPreferences (SQLToolsSettings&, bool bLocal = false);

    void LoadSettings ();
    void SaveSettings ();

    // overrided SettingsSubscriber method
    virtual void OnSettingsChanged ();

public:
    static const UINT m_msgCommandLineProcess;

    CSQLToolsApp ();

    OciConnect& GetConnect ()               { return *m_connect; };
    CMultiDocTemplate* GetPLSDocTemplate()  { return m_pPLSDocTemplate; }
    LPCSTR GetDisplayConnectionString ()    { return m_displayConnectionString; }

    void OnActivateApp (BOOL bActive);

    //{{AFX_VIRTUAL(CSQLToolsApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

    //{{AFX_MSG(CSQLToolsApp)
	afx_msg void OnAppAbout();
	afx_msg void OnEditPermanetSettings();
	afx_msg void OnAppSettings();
	afx_msg void OnSqlConnect();
	afx_msg void OnSqlCommit();
	afx_msg void OnSqlRollback();
	afx_msg void OnSqlDisconnect();
	afx_msg void OnSqlDbmsOutput();
    afx_msg void OnSqlSubstitutionVariables();
	afx_msg void OnSqlHelp();
	afx_msg void OnSqlSessionStatistics();
	afx_msg void OnSqlExtractSchema();
	afx_msg void OnSqlTableTransformer();
	afx_msg void OnSQLToolsOnTheWeb();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdate_SqlGroup(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_SqlSubstitutionVariables(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_EditIndicators (CCmdUI* pCmdUI);
    // double keys accelerator 
    afx_msg void OnDblKeyAccel (UINT nID);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnFileWatchNotify (WPARAM, LPARAM);
    // override file save/close handlers
    afx_msg void OnFileCloseAll();
    afx_msg void OnFileSaveAll();
    afx_msg void OnUpdate_FileSaveAll(CCmdUI *pCmdUI);
    // background processing support
    virtual BOOL OnIdle(LONG lCount);
    afx_msg void OnCommandLineProcess (WPARAM, LPARAM);
};

inline
const SQLToolsSettings& GetSQLToolsSettings () { return CSQLToolsApp::m_settings; }

inline
SQLToolsSettings& GetSQLToolsSettingsForUpdate () { return CSQLToolsApp::m_settings; }

inline
CAbortThread* GetAbortThread () { return CSQLToolsApp::m_pAbortThread; }

extern CSQLToolsApp theApp;

//{{AFX_INSERT_LOCATION}}

#endif//__SQLTOOLS_H__
