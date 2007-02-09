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

// 03.01.2005 (ak) b1095462 Empty bug report on disconnect if connection was already lost.

#include "stdafx.h"
#include "SQLTools.h"
#include "MainFrm.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/ExceptionHelper.h"
#include "SQLToolsSettings.h"
#include "Dlg/ConnectDlg.h"
#include "SQLWorksheet/StatView.h"
#include "DbBrowser/DbSourceWnd.h"
#include "Tools/ExtractSchemaDlg.h"
#include "Tools/TableTransformer.h"
#include <OCI8/Statement.h>
#include <OCI8/ACursor.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

  LPSCSTR cszStatNotAvailable =
      "V$SESSION, V$STATNAME or V$SESSTAT is not accessible!"
      "\nStatistics collection is disabled. If it is necessary,"
      "\nask the DBA to grant the privileges required to access these views.";

bool CSQLToolsApp::EvAfterOpenConnect (OciConnect&)
{
    m_serverVerion = m_connect->GetVersionStr();

    COLORREF color = RGB(0, 0, 0);
    switch (m_connect->GetSafety())
    {
    case OCI8::esWarnings: color = RGB(0, 0, 255); break;
    case OCI8::esReadOnly: color = RGB(255, 0, 0); break;
    }

    m_displayConnectionString = m_connect->GetDisplayString(true).c_str();

    CString mainWndTitle = m_displayConnectionString;
    mainWndTitle.MakeUpper();
    mainWndTitle += " - ";
    mainWndTitle += m_orgMainWndTitle;

    AfxGetMainWnd()->SetWindowText(mainWndTitle);
    ((CFrameWnd*)AfxGetMainWnd())->SetTitle(mainWndTitle);

    free((void*)m_pszAppName);
    m_pszAppName = strdup(mainWndTitle);

    m_displayConnectionString += ' ';
    m_displayConnectionString += '[';
    m_displayConnectionString += m_serverVerion;
    m_displayConnectionString += ']';

    ((CMDIMainFrame*)m_pMainWnd)->GetConnectionBar().SetConnectionDescription(m_displayConnectionString, color);

    CDbSourceWnd::EvOnConnectAll();

    if (GetSQLToolsSettings().GetSessionStatistics())
    {
        try
        {
            CStatView::OpenAll(*m_connect);
        }
        catch (const OciException& x)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox((x == 942) ? cszStatNotAvailable : x.what());

            GetSQLToolsSettingsForUpdate().SetSessionStatistics(false);
            return false;
        }
    }

    return true;
}

bool CSQLToolsApp::EvBeforeCloseConnect (OciConnect&)
{
    STACK_OVERFLOW_GUARD(3);

    try { EXCEPTION_FRAME;

        if (!GetSQLToolsSettings().GetAutocommit())
        {
            switch (GetSQLToolsSettings().GetCommitOnDisconnect())
            {
            case 0: m_connect->Rollback(); return true; // always rollback
            case 1: m_connect->Commit();   return true; // always commit
            }

            enum ETransaction { etUnknown, etNo, etYes };
            ETransaction transaction = etUnknown;

            if (m_connect->GetVersion() >= OCI8::esvServer80X)
            {
                try
                {
                    OciStatement cursor(*m_connect);
                    OciStringVar transaction_id(100);
                    cursor.Prepare("BEGIN :transaction_id := dbms_transaction.local_transaction_id; END;");
                    cursor.Bind(":transaction_id", transaction_id);
	                cursor.Execute(1, true);
                    transaction = transaction_id.IsNull() ?  etNo : etYes;
                }
                catch (const OciException& x)
                {
                    if (!m_connect->IsOpen())
                    {
                        MessageBeep(MB_ICONSTOP);
                        AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                        return true;
                    }
                    else // most likely connection was lost but it was not detected by some reason
                    {
                        try
                        {
                            OciAutoCursor cursor(*m_connect);
                            cursor.Prepare("SELECT * FROM sys.dual");
	                        cursor.Execute();
	                        cursor.Fetch();
                        }
                        catch (const OciException& x)
                        {
                            if (!m_connect->IsOpen())
                            {
                                MessageBeep(MB_ICONSTOP);
                                AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                                return true;
                            }
                        }
                    }
                }
            }

            if (transaction != etNo)
            {
                if (AfxMessageBox(
                        (transaction == etUnknown)
                            ? "Auto commit is disabled and the current session may have an open transaction."
                            "\nDo yo want to perform commit before closing this session?"
                            : "SQLTools detected that the current session has an open transaction."
                            "\nDo yo want to perform commit before closing this session?",
                        MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                    m_connect->Commit();
                else
                    m_connect->Rollback();
            }
        }
    }
    _DEFAULT_HANDLER_

    return true;
}

bool CSQLToolsApp::EvAfterCloseConnect (OciConnect&)
{
    STACK_OVERFLOW_GUARD(3);

    m_serverVerion.Empty();

    m_displayConnectionString = m_displayNotConnected;

    if (m_pMainWnd)
        ((CMDIMainFrame*)m_pMainWnd)->GetConnectionBar()
            .SetConnectionDescription(m_displayConnectionString, RGB(0, 0, 0));

    CDbSourceWnd::EvOnDisconnectAll();

    if (GetSQLToolsSettings().GetSessionStatistics())
    {
        try { EXCEPTION_FRAME;

            CStatView::CloseAll();
        }
        _DEFAULT_HANDLER_
    }

    return true;
}


void CSQLToolsApp::DoConnect (const char* user, const char* pwd, const char* alias, int mode_, int safety_)
{
	m_connect->Close();

    OCI8::EConnectionMode mode = OCI8::ecmDefault;
    switch (mode_)
    {
    case 1: mode = OCI8::ecmSysDba; break;
    case 2: mode = OCI8::ecmSysOper; break;
    }

    COLORREF color = RGB(0, 0, 0);
    OCI8::ESafety safety = OCI8::esNone;
    switch (safety_)
    {
    case 1: safety = OCI8::esWarnings; color = RGB(0, 0, 255); break;
    case 2: safety = OCI8::esReadOnly; color = RGB(255, 0, 0); break;
    }

    m_connect->Open(user, pwd, alias, mode, safety);
}

void CSQLToolsApp::DoConnect (const char* user, const char* pwd, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid, int mode_, int safety_)
{
	m_connect->Close();

    OCI8::EConnectionMode mode = OCI8::ecmDefault;
    switch (mode_)
    {
    case 1: mode = OCI8::ecmSysDba; break;
    case 2: mode = OCI8::ecmSysOper; break;
    }

    OCI8::ESafety safety = OCI8::esNone;
    switch (safety_)
    {
    case 1: safety = OCI8::esWarnings; break;
    case 2: safety = OCI8::esReadOnly; break;
    }

    m_connect->Open(user, pwd, host, port, sid, serviceInsteadOfSid, mode, safety);
}

void CSQLToolsApp::OnSqlConnect()
{
    CConnectDlg connectDlg(m_pMainWnd);

    while (connectDlg.DoModal() == IDOK)
    {
        try
		{
            if (!connectDlg.m_directConnection)
                DoConnect(connectDlg.m_user, connectDlg.m_password, connectDlg.m_tnsAlias,
                    connectDlg.m_connectionMode, connectDlg.m_safety);
            else
                DoConnect(connectDlg.m_user, connectDlg.m_password,
                    connectDlg.m_host, connectDlg.m_tcpPort, connectDlg.m_sid, connectDlg.m_serviceInsteadOfSid,
                    connectDlg.m_connectionMode, connectDlg.m_safety);

            break;
        }
        catch (const OciException& x)
        {
            AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
	        m_connect->Close(true);
        }
    }
}

void CSQLToolsApp::OnSqlCommit()
{
    try { EXCEPTION_FRAME;

	    m_connect->Commit();
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::OnSqlRollback()
{
    try { EXCEPTION_FRAME;

	    m_connect->Rollback();
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::OnSqlDisconnect()
{
    try { EXCEPTION_FRAME;

        try
        {
            m_connect->Close();
        }
        catch (const OciException& x)
        {
            DEFAULT_HANDLER(x);
            m_connect->Close(true);
        }

        free((void*)m_pszAppName);
        m_pszAppName = strdup(m_orgMainWndTitle);
        AfxGetMainWnd()->SetWindowText(m_orgMainWndTitle);
        ((CFrameWnd*)AfxGetMainWnd())->SetTitle(m_orgMainWndTitle);
    }
    _DEFAULT_HANDLER_
}

void CSQLToolsApp::OnSqlDbmsOutput()
{
    //m_connect.EnableOutput(!m_connect.IsOutputEnabled());
    GetSQLToolsSettingsForUpdate().SetOutputEnable(!m_connect->IsOutputEnabled());
}

void CSQLToolsApp::OnSqlSubstitutionVariables()
{
    GetSQLToolsSettingsForUpdate().SetScanForSubstitution(!GetSQLToolsSettings().GetScanForSubstitution());
}

void CSQLToolsApp::OnSqlDbmsXplanDisplayCursor()
{
    GetSQLToolsSettingsForUpdate().SetDbmsXplanDisplayCursor(!GetSQLToolsSettings().GetDbmsXplanDisplayCursor());
}

void CSQLToolsApp::OnUpdate_SqlSubstitutionVariables (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(GetSQLToolsSettings().GetScanForSubstitution() ?  1 : 0);
}

void CSQLToolsApp::OnUpdate_SqlGroup (CCmdUI* pCmdUI)
{
    ASSERT(ID_SQL_SESSION_STATISTICS - ID_SQL_DISCONNECT == 5);

    if (m_connect->IsOpen())
    {
        switch (pCmdUI->m_nID)
        {
        case ID_SQL_DBMS_OUTPUT:
            if (m_connect->IsOutputEnabled())
                pCmdUI->SetCheck(1);
            else
                pCmdUI->SetCheck(0);
            break;
        case ID_SQL_SESSION_STATISTICS:
            if (GetSQLToolsSettings().GetSessionStatistics())
                pCmdUI->SetCheck(1);
            else
                pCmdUI->SetCheck(0);
            break;
        case ID_SQL_DBMS_XPLAN_DISPLAY_CURSOR:
            if (GetSQLToolsSettings().GetDbmsXplanDisplayCursor())
                pCmdUI->SetCheck(1);
            else
                pCmdUI->SetCheck(0);
            break;
        }
        pCmdUI->Enable(TRUE);
    }
    else
        pCmdUI->Enable(FALSE);
}

void CSQLToolsApp::OnSqlSessionStatistics()
{
    try { EXCEPTION_FRAME;

        GetSQLToolsSettingsForUpdate().SetSessionStatistics(!GetSQLToolsSettings().GetSessionStatistics());

        if (GetSQLToolsSettings().GetSessionStatistics())
            CStatView::OpenAll(*m_connect);
        else
            CStatView::CloseAll();
    }
    catch (const OciException& x)
    {
        if (x == 942)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(cszStatNotAvailable);
        }
        else
            DEFAULT_OCI_HANDLER(x);
    }
    _COMMON_DEFAULT_HANDLER_
}

void CSQLToolsApp::OnSqlExtractSchema()
{
    CExtractSchemaDlg(*m_connect, m_pMainWnd).DoModal();
}

void CSQLToolsApp::OnSqlTableTransformer()
{
    CTableTransformer(*m_connect, m_pMainWnd).DoModal();
}

