/*
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

// 30.09.2002 bug fix, wrong error positioning on EXEC (raised in pl/sql procedure)
// 01.10.2002 bug fix, wrong error positioning on execution of selected text
//           if selection does not starts at start of line
// 06.10.2002 changes, OnOutputOpen has been moved to GridView
// 09.03.2003 bug fix, error position fails sometimes
// 10.03.2003 bug fix, find object fails on object with schema
// 25.03.2003 bug refix, find object fails on object with schema
// 03.06.2003 bug fix, "Open In Editor" does not set document type
// 05.06.2003 improvement, output/error postions is based on internal line IDs and more stable on editing text above
// 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db
//            if "save files when switching takss" is activated.
// 29.06.2003 bug fix, "Open In Editor" fails on comma/tab delimited formats
// 07.12.2003 bug fix, CreareAs property is ignored for ddl scripts (always in Unix format)
// 27.06.2004 bug fix, Autocommit does not work
// 22.11.2004 bug fix, all compilation errors are ignored after "alter session set current schema"
// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 09.01.2005 (ak) R1092514: SQL*Plus CONNECT/DISCONNECT commands - small improvements
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 07.02.2005 (ak) R1105003: Bind variables
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

#include "stdafx.h"
#include <sstream>
#include <iomanip>
#include "COMMON/ExceptionHelper.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "SQLTools.h"
#include "DbBrowser/ObjectTreeBuilder.h"
#include "SQLWorksheetDoc.h"
#include "SQLToolsSettings.h"
#include "OutputView.h"
#include "StatView.h"
#include "HistoryView.h"
#include "2PaneSplitter.h"
#include "ExplainPlanView.h"
#include "BindGridView.h"

#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include <SQLWorksheet/CommandParser.h>
#include "MainFrm.h"
#include "Booklet.h"
#include "DbBrowser/DbSourceWnd.h"
#include <OpenGrid/OCIGridView.h>
#include <OpenGrid/OCISource.h>
#include <OCI8/BCursor.h>
#include <OCI8/ACursor.h>
#include <OCI8/Statement.h>
#include "AbortThread/AbortThread.h"
#include "CommandPerformerImpl.h"

#include "XPlanView.h"
#include "COMMON/TempFilesManager.h"
#include "COMMON\StrHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc

IMPLEMENT_DYNCREATE(CPLSWorksheetDoc, COEDocument)

CPLSWorksheetDoc::CPLSWorksheetDoc()
: m_connect(GetApp()->GetConnect())
{
    m_LoadedFromDB = false;
    m_pEditor   = 0;
    m_pBooklet  = 0;
    m_pXPlan    = 0;

    for (int i = 0; i < sizeof(m_BookletFamily) / sizeof(m_BookletFamily[0]); i++)
        m_BookletFamily[i] = 0;
}

void CPLSWorksheetDoc::Serialize (CArchive& ar)
{
    if (ar.IsStoring())
    {
        _CHECK_AND_THROW_(0, "CPLSWorksheetDoc::Serialize: method does not support writing into CArchive");
    }
    else
    {
        CFile* pFile = ar.GetFile();
        ASSERT(pFile->GetPosition() == 0);
        DWORD dwFileSize = (DWORD)pFile->GetLength();

        LPVOID vmdata = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT, PAGE_READWRITE);
        try
        {
            _CHECK_AND_THROW_(ar.Read((char*)vmdata, dwFileSize) == dwFileSize, "CPLSWorksheetDoc::Serialize: reading error from CArchive");
            SetText(vmdata, dwFileSize);
        }
        catch (...)
        {
            if (vmdata != GetVMData())
                VirtualFree(vmdata, 0, MEM_RELEASE);
        }
    }
}

BOOL CPLSWorksheetDoc::OnIdle (LONG lCount)
{
    if (lCount == 0 && m_pBooklet)
        m_pBooklet->OnIdleUpdateCmdUI();

    return COEDocument::OnIdle(lCount);
}

void CPLSWorksheetDoc::Init ()
{
    m_pEditor   = (COEditorView*)m_viewList.GetHead();
    m_pBooklet  = (CBooklet*)m_viewList.GetTail();
    m_pOutput   = new COutputView;
    m_pDbGrid   = new OciGridView;
    m_pStatGrid = new CStatView;
    m_pExplainPlan = new CExplainPlanView;
	m_pBindGrid = new BindGridView;

	m_pXPlan = new cXPlanEdit(*this);

	m_pXPlan->setOldView(m_pExplainPlan);

    if (!m_pHistory)
        m_pHistory  = new CHistoryView;
}

CPLSWorksheetDoc::~CPLSWorksheetDoc()
{
	if (m_pXPlan)
		delete m_pXPlan;
}

// 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db
//            if "save files when switching takss" is activated.
void CPLSWorksheetDoc::GetNewPathName (CString& newName) const
{
    COEDocument::GetNewPathName(newName);

    if (newName.IsEmpty())
        newName = m_newPathName;
}

void CPLSWorksheetDoc::ActivateEditor ()
{
    m_pEditor->GetParentFrame()->SetActiveView(m_pEditor);
    m_pEditor->SetFocus();
}

void CPLSWorksheetDoc::ActivateTab (CView* view)
{
    if (!m_pBooklet->IsTabPinned())
        m_pBooklet->ActivateTab(view);
}


BEGIN_MESSAGE_MAP(CPLSWorksheetDoc, COEDocument)
    //{{AFX_MSG_MAP(CPLSWorksheetDoc)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_SQL_EXECUTE, OnSqlExecute)
    ON_COMMAND(ID_SQL_EXECUTE_HALT_ON_ERRORS, OnSqlExecuteHaltOnErrors)
    ON_COMMAND(ID_SQL_EXECUTE_CURRENT, OnSqlExecuteCurrent)
    ON_COMMAND(ID_SQL_EXECUTE_CURRENT_AND_STEP, OnSqlExecuteCurrentAndStep)
    ON_COMMAND(ID_SQL_EXECUTE_EXTERNAL, OnSqlExecuteExternal)
	ON_COMMAND(ID_SQL_LOAD, OnSqlLoad)
    ON_COMMAND(ID_SQL_DESCRIBE, OnSqlDescribe)
    ON_COMMAND(ID_SQL_EXPLAIN_PLAN, OnSqlExplainPlan)

    // ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE, OnUpdate_SqlGroup)
    // ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_CURRENT, OnUpdate_SqlGroup)
    // ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_CURRENT_AND_STEP, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_LOAD, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_DESCRIBE, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXPLAIN_PLAN, OnUpdate_SqlGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_EXECUTE_EXTERNAL, OnUpdate_SqlGroup)

    ON_COMMAND(ID_SQL_CURR_ERROR, OnSqlCurrError)
    ON_COMMAND(ID_SQL_NEXT_ERROR, OnSqlNextError)
    ON_COMMAND(ID_SQL_PREV_ERROR, OnSqlPrevError)
    ON_UPDATE_COMMAND_UI(ID_SQL_CURR_ERROR, OnUpdate_ErrorGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_NEXT_ERROR, OnUpdate_ErrorGroup)
    ON_UPDATE_COMMAND_UI(ID_SQL_PREV_ERROR, OnUpdate_ErrorGroup)

    ON_COMMAND(ID_SQL_HISTORY_GET, OnSqlHistoryGet)
    ON_COMMAND(ID_SQL_HISTORY_STEPFORWARD_AND_GET, OnSqlHistoryStepforwardAndGet)
    ON_COMMAND(ID_SQL_HISTORY_GET_AND_STEPBACK, OnSqlHistoryGetAndStepback)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_GET, OnUpdate_SqlHistory)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_STEPFORWARD_AND_GET, OnUpdate_SqlHistory)
    ON_UPDATE_COMMAND_UI(ID_SQL_HISTORY_GET_AND_STEPBACK, OnUpdate_SqlHistory)

    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_OciGridIndicator)

    // Standard Mail Support
    ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
    ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)
    ON_COMMAND(ID_SQL_HELP, OnSqlHelp)

    ON_COMMAND(ID_WINDOW_NEW, OnWindowNew)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_NEW, OnUpdate_WindowNew)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc commands

void CPLSWorksheetDoc::OnUpdate_SqlGroup (CCmdUI* pCmdUI)
{
    ASSERT(ID_SQL_EXECUTE_CURRENT_AND_STEP - ID_SQL_EXPLAIN_PLAN == 5);

    if (pCmdUI->m_nID == ID_SQL_EXECUTE_EXTERNAL)
    {
        pCmdUI->Enable(TRUE);
        return;
    }

    if (m_connect.IsOpen())
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);
}

void CPLSWorksheetDoc::OnSqlExecute()
{
    DoSqlExecute(ALL);
}

void CPLSWorksheetDoc::OnSqlExecuteHaltOnErrors()
{
    DoSqlExecute(ALL, true);
}

void CPLSWorksheetDoc::OnSqlExecuteExternal()
{
    if (GetSQLToolsSettings().GetExternalToolCommand().empty() || (GetSQLToolsSettings().GetExternalToolCommand() == ""))
    {
        MessageBeep((UINT)-1);
        AfxMessageBox("You need to define the external tool command in the settings.", MB_OK|MB_ICONSTOP);
        return;
    }

    bool bWriteToTempFile;
    string sFilename;
    OpenEditor::Square sel;
    m_pEditor->GetSelection(sel);
    sel.normalize();
    int nlines = m_pEditor->GetLineCount();

    bWriteToTempFile = false;

    if (sel.is_empty())
    {
        if (IsModified())
        {
            switch (AfxMessageBox("Do you want to save the file before executing external tool?", MB_YESNOCANCEL|MB_ICONQUESTION))
            {
            case IDCANCEL:
                return;
            case IDYES:
                if (DoFileSave())
                {
                    sFilename = GetPathName();
                }
                else
                    return;

                break;

            case IDNO:
                bWriteToTempFile = true;
                sel.start.line = 0;
                sel.end.line = nlines - 1;
                sel.start.column = 0;
                sel.end.column = INT_MAX;
            }
        }
        else
        {
            sFilename = GetPathName();
            if (sFilename.empty() || (sFilename == ""))
            {
                MessageBeep((UINT)-1);
                AfxMessageBox("Running an unmodified new document in an external tool is not useful.", MB_OK|MB_ICONSTOP);
                return;
            }
        }
    }
    else
    {
        bWriteToTempFile = true;
    }

    if (bWriteToTempFile)
    {
        sel.start.column = m_pEditor->PosToInx(sel.start.line, sel.start.column);
        sel.end.column   = m_pEditor->PosToInx(sel.end.line, sel.end.column);

        sFilename = TempFilesManager::CreateFile("SQL");

        if (!sFilename.empty())
        {
            HANDLE hFile = ::CreateFile(sFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                MessageBeep((UINT)-1);
                AfxMessageBox("Cannot open temporary file for external tool.", MB_OK|MB_ICONSTOP);
                return;
            }
            DWORD written;

            int line = sel.start.line;
            int offset = sel.start.column;
            int len;
            const char* str;
            const char lf[] = "\r\n";

            for (; line < nlines && line <= sel.end.line; line++)
            {
                m_pEditor->GetLine(line, str, len);

                if (line == sel.end.line)
                    len = min(len, sel.end.column);

                if (len > 0)
                    WriteFile(hFile, str + offset, len - offset, &written, 0);

                WriteFile(hFile, lf, sizeof(lf) - 1, &written, 0);
                
				offset = 0;
            }

            CloseHandle(hFile);
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Cannot generate temporary file for external tool.", MB_OK|MB_ICONSTOP);
            return;
        }
    }

    Common::Substitutor subst;
    subst.AddPair("<CMD>", GetSQLToolsSettings().GetExternalToolCommand().c_str());
    subst.AddPair("<USER>", m_connect.GetUID());
    subst.AddPair("<PASSWORD>", m_connect.GetPassword());
    string sys_privs;
    switch (m_connect.GetMode())
    {
        case OCI8::ecmSysDba:  sys_privs = " AS SYSDBA"; break;
        case OCI8::ecmSysOper: sys_privs = " AS SYSOPER"; break;
    }
    subst.AddPair("<SYS_PRIVS>", sys_privs);
    subst.AddPair("<CONNECT_STRING>", m_connect.GetAlias());
    subst.AddPair("<FILENAME>", string(string("\"") + sFilename + "\"").c_str());
    subst << "\"<CMD>\" " << GetSQLToolsSettings().GetExternalToolParameters().c_str();

    string sParameter = subst.GetResult();

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    if( !CreateProcess( NULL, // module name
        (LPSTR) sParameter.c_str(),// Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((string("Spawning external tool failed. Command line used:\n") + sParameter).c_str(), MB_OK|MB_ICONSTOP);
        return;
    }
}

void CPLSWorksheetDoc::OnSqlExecuteCurrent()
{
    DoSqlExecute(CURRENT);
}

void CPLSWorksheetDoc::OnSqlExecuteCurrentAndStep()
{
    DoSqlExecute(CURRENT|STEP);
}

void CPLSWorksheetDoc::OnSqlExplainPlan()
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        try
        {
            OpenEditor::Square sel;
            m_pEditor->GetSelection(sel);
            sel.normalize();
            int nlines = m_pEditor->GetLineCount();

            if (sel.is_empty()) // make default selection
            {
                sel.start.column = 0;
                // search from the current line to top for blank line
                sel.start.line = m_pEditor->GetPosition().line;

                if (GetSQLToolsSettings().GetEmptyLineDelim() || 
                    GetSQLToolsSettings().GetWhitespaceLineDelim())
                {
				    const char *linePtr;
				    int len;

				    while (sel.start.line > 0)
				    {
					    m_pEditor->GetLine(sel.start.line - 1, linePtr, len);
					    if (GetSQLToolsSettings().GetEmptyLineDelim())
					        if (len == 0)
						        break;

					    if (GetSQLToolsSettings().GetWhitespaceLineDelim())
						    if ((len > 0) && IsBlankLine(linePtr, len))
							    break;

                        sel.start.line--;
				    }
                }
                // to the bottom
                sel.end.column = INT_MAX;

                // search from the current line to bottom for blank line
                sel.end.line = m_pEditor->GetPosition().line;

                if (GetSQLToolsSettings().GetEmptyLineDelim() || 
                    GetSQLToolsSettings().GetWhitespaceLineDelim())
                {
				    const char *linePtr;
				    int len;

				    while (sel.end.line < nlines - 1)
				    {
					    m_pEditor->GetLine(sel.end.line + 1, linePtr, len);
					    if (GetSQLToolsSettings().GetEmptyLineDelim())
					        if (len == 0)
						        break;

					    if (GetSQLToolsSettings().GetWhitespaceLineDelim())
						    if ((len > 0) && IsBlankLine(linePtr, len))
							    break;
    					
					    sel.end.line++;
				    }
                }
            }
            // convert positions to indexes (because of tabs)
            sel.start.column = m_pEditor->PosToInx(sel.start.line, sel.start.column);
            sel.end.column   = m_pEditor->PosToInx(sel.end.line, sel.end.column);


            SqlPlanPerformerImpl performer(*this);
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution());
            commandParser.SetSqlOnly(true);
            commandParser.Start(sel.start.line, sel.start.column);

            int line = sel.start.line;
            int offset = sel.start.column;
            for (; line < nlines && line <= sel.end.line; line++)
            {
                int len;
                const char* str;
				// dummy variable for bugfix
				// Prevent null pointer assertion failure in case of blank line returned by editor
				const char dummy = '\0';

				m_pEditor->GetLine(line, str, len);

                if (line == sel.end.line) len = min(len, sel.end.column);

                if (len > 0)
					commandParser.PutLine(str + offset, len - offset);
				else
					// Prevent null pointer assertion failure
					commandParser.PutLine(&dummy, len);

				offset = 0;

                if (performer.IsCompleted())
                    break;
            }
            if (!performer.IsCompleted())
                commandParser.PutEOF();
        }
        // the most of oci exeptions well be proceesed in ExecuteSql method,
        // but if it's a fatal problem (like connection lost), we receive it here
        catch (const OciException& x)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x, MB_OK|MB_ICONHAND);
        }
        // currently we break script execution on substitution exception,
        // but probably we should ask a user
        catch (const CommandParserException& x)
        {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x.what(), MB_OK|MB_ICONHAND);
        }
    }
    // any other exceptions are fatal, a bug report is calling here
    _DEFAULT_HANDLER_
}

void CPLSWorksheetDoc::PrintExecTime (std::ostream& out, double seconds)
{
    int n_hours = int(seconds)/3600;
    int n_minutes = (int(seconds) - n_hours*3600)/60;
    int n_seconds = int(seconds) - n_hours*3600 - n_minutes*60;
    double n_fraction = seconds - n_hours*3600 - n_minutes*60 - n_seconds;
    char s_fraction[20];

    sprintf_s(s_fraction, sizeof(s_fraction), "%.3f", n_fraction);

    if (n_hours > 0)
        out << setw(2) << setfill('0') << n_hours 
            << ':' << setw(2) << setfill('0') << n_minutes
            << ':' << setw(2) << setfill('0') << n_seconds
            << &s_fraction[1]
            << " (" << seconds << " sec.)";
    else if (n_minutes > 0)
        out << setw(2) << setfill('0') << n_minutes
            << ':' << setw(2) << setfill('0') << n_seconds
            << &s_fraction[1]
            << " (" << seconds << " sec.)";
    else if (n_seconds > 0)
        out << seconds << " sec.";
    else
        out << int(n_fraction * 1000) << " ms";
}

bool CPLSWorksheetDoc::IsBlankLine(const char *linePtr, const int len)
{
	for (int i = 0; i < len; i++)
	{
		if ((*(linePtr + i) != ' ') && (*(linePtr + i) != '\t'))
			return false;
	}

	return true;
}

void CPLSWorksheetDoc::DoSqlExecute (int mode, bool bHaltOnErrors)
{
    try { EXCEPTION_FRAME;
        if (! CheckFileSaveBeforeExecute())
            return;

        CWaitCursor wait;

        m_pOutput->Clear();
        m_pStatGrid->BeginMeasure(m_connect);
        clock_t startTime = clock(), endTime = 0;

        int line = -1;

        try
        {
            OpenEditor::Square sel;
            m_pEditor->GetSelection(sel);
            sel.normalize();
            int nlines = m_pEditor->GetLineCount();

            if (sel.is_empty()) // make default selection
            {
                sel.start.column = 0;

                if (mode == ALL) // from the top
                    sel.start.line = 0;
                else             // search from the current line to top for blank line
				{
                    sel.start.line = m_pEditor->GetPosition().line;

                    if (GetSQLToolsSettings().GetEmptyLineDelim() || 
                        GetSQLToolsSettings().GetWhitespaceLineDelim())
                    {
					    const char *linePtr;
					    int len;

					    while (sel.start.line > 0)
					    {
						    m_pEditor->GetLine(sel.start.line - 1, linePtr, len);
    					    if (GetSQLToolsSettings().GetEmptyLineDelim())
						        if (len == 0)
							        break;

						    if (GetSQLToolsSettings().GetWhitespaceLineDelim())
							    if ((len > 0) && IsBlankLine(linePtr, len))
								    break;
    						
						    sel.start.line--;
					    }
                    }
				}

                // to the bottom
                sel.end.column = INT_MAX;

                if (mode == ALL) // from the top
					sel.end.line   = INT_MAX;
                else             // search from the current line to bottom for blank line
				{
                    sel.end.line = m_pEditor->GetPosition().line;

                    if (GetSQLToolsSettings().GetEmptyLineDelim() || 
                        GetSQLToolsSettings().GetWhitespaceLineDelim())
                    {
					    const char *linePtr;
					    int len;

					    while (sel.end.line < nlines - 1)
					    {
						    m_pEditor->GetLine(sel.end.line + 1, linePtr, len);
    					    if (GetSQLToolsSettings().GetEmptyLineDelim())
						        if (len == 0)
							        break;

						    if (GetSQLToolsSettings().GetWhitespaceLineDelim())
							    if ((len > 0) && IsBlankLine(linePtr, len))
								    break;
    						
						    sel.end.line++;
					    }
                    }
				}
            }
            // convert positions to indexes (because of tabs)
            sel.start.column = m_pEditor->PosToInx(sel.start.line, sel.start.column);
            sel.end.column   = m_pEditor->PosToInx(sel.end.line, sel.end.column);


            CommandPerformerImpl performer(*this);
            CommandParser commandParser(performer, GetSQLToolsSettings().GetScanForSubstitution());
            commandParser.Start(sel.start.line, sel.start.column);

            line = sel.start.line;
            int offset = sel.start.column;
            for (; line < nlines && line <= sel.end.line; line++)
            {
                int len;
                const char* str;

				// dummy variable for bugfix
				// Prevent null pointer assertion failure in case of blank line returned by editor
				const char dummy = '\0';

                m_pEditor->GetLine(line, str, len);

                if (line == sel.end.line)
                    len = min(len, sel.end.column);

                if (len > 0)
					commandParser.PutLine(str + offset, len - offset);
				else
					// Prevent null pointer assertion failure
					commandParser.PutLine(&dummy, len);
                
				offset = 0;

                if ((performer.GetStatementCount() && mode != ALL) || 
                    ((GetSQLToolsSettings().GetHaltOnErrors() || bHaltOnErrors) && performer.HasErrors()))
                    break;
            }
            commandParser.PutEOF();

            bool switchToEditor = m_pEditor->GetParentFrame()->GetActiveView() == m_pEditor ? true : false;

            if (!performer.HasErrors() && performer.IsLastStatementSelect())
				ShowSelectResult(performer.GetCursorOwnership(), performer.GetLastExecTime());

			if (!performer.HasErrors() && performer.IsLastStatementBind())
                ActivateTab(m_pBindGrid);

            if (performer.HasErrors() || !(performer.IsLastStatementSelect() || performer.IsLastStatementBind()))
                ShowOutput(performer.HasErrors());

            if (switchToEditor)
                m_pEditor->GetParentFrame()->SetActiveView(m_pEditor);

            // reset modified flag for script which was made by DDL reverse eng
            // add an option to disable this behavior
            if (mode == 0 && m_LoadedFromDB) // execute
                SetModifiedFlag(FALSE);

            if (!performer.HasErrors() && mode & STEP) // step by step
                MakeStep(line);

            endTime = clock();
        }
        // the most of oci exeptions well be proceesed in ExecuteSql method,
        // but if it's a fatal problem (like connection lost), we receive it here
        catch (const OciException& x)
        {
            endTime = clock();
            PutError(x.what());
            ShowOutput(true);
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x, MB_OK|MB_ICONHAND);
        }
        // currently we break script execution on substitution exception,
        // but probably we should ask a user
        catch (const Common::AppException& x)
        {
            endTime = clock();
            PutError(x.what(), line);
            ShowOutput(true);
            MessageBeep(MB_ICONHAND);
            AfxMessageBox(x.what(), MB_OK|MB_ICONHAND);
        }

        ostringstream out;
        out << "Total execution time ";
        PrintExecTime(out, double(endTime - startTime)/ CLOCKS_PER_SEC);
        PutMessage(out.str());
        m_pStatGrid->EndMeasure(m_connect);
        m_pStatGrid->ShowStatistics();
    }
    // any other exceptions are fatal, a bug report is calling here
    _DEFAULT_HANDLER_
}


void CPLSWorksheetDoc::MakeStep (int curLine)
{
    int line = curLine+1;
    int nlines = m_pEditor->GetLineCount();

    for (; line < nlines; line++)
    {
        int len;
        const char* str;
        m_pEditor->GetLine(line, str, len);

        bool empty = true;

        for (int i = 0; empty && i < len; i++)
            if (!isspace(str[i]))
                empty = false;

        if (!empty) break;
    }

    GoTo(line);
}

void CPLSWorksheetDoc::ShowSelectResult (std::auto_ptr<OciAutoCursor>& cursor, double lastExecTime)
{
    ActivateTab(m_pDbGrid);
    clock_t startTime = clock();
    m_pDbGrid->SetCursor(cursor);

    ostringstream out;
	out << "Statement executed in ";
	PrintExecTime(out, lastExecTime);
    out << ", " << m_pDbGrid->GetRecordCount()
        << " first records fetched in ";
    PrintExecTime(out, (double(clock() - startTime)/ CLOCKS_PER_SEC));

	Global::SetStatusText(out.str());

    ostringstream out_message;
    out_message << " " << m_pDbGrid->GetRecordCount()
        << " first records fetched in ";
    PrintExecTime(out_message, (double(clock() - startTime)/ CLOCKS_PER_SEC));
    PutMessage(out_message.str());
}


void CPLSWorksheetDoc::ShowOutput (bool error)
{
    ActivateTab(m_pOutput);

    if (error)
    {
        MessageBeep(MB_ICONHAND);
        m_pOutput->FirstError(true);
        OnSqlCurrError();
    }
    else
        m_pOutput->FirstError();
}


    const int err_line = 0;
    const int err_pos  = 1;
    const int err_text = 2;

    LPSCSTR err_sttm =
        "SELECT line, position, text FROM sys.all_errors"
            " WHERE name = :name AND type = :type"
            " AND owner = :owner";

    LPSCSTR def_schema_err_sttm =
        "SELECT line, position, text FROM sys.all_errors"
            " WHERE name = :name AND type = :type"
            " AND owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA')";

    LPSCSTR legacy_def_schema_err_sttm =
        "SELECT line, position, text FROM user_errors"
            " WHERE name = :name AND type = :type";

// 22.11.2004 bug fix, all compilation errors are ignored after "alter session set current schema"
int CPLSWorksheetDoc::LoadErrors (const char* owner, const char* name, const char* type, int line)
{
    ASSERT(m_pOutput);

    bool def_schema_err = !owner || !owner[0] || !strlen(owner);

    OciCursor err_curs(m_connect,
        def_schema_err
            ? ((m_connect.GetVersion() < OCI8::esvServer81X) ? legacy_def_schema_err_sttm : def_schema_err_sttm)
            : err_sttm);

    if (!def_schema_err) err_curs.Bind(":owner", owner);
    err_curs.Bind(":name", name);
    err_curs.Bind(":type", type);

    err_curs.Execute();

    int errors = 0;
    for (; err_curs.Fetch(); errors++)
        PutError(err_curs.ToString(err_text).c_str(),
            line + err_curs.ToInt(err_line) - 1, err_curs.ToInt(err_pos) - 1);

    return errors;
}

void CPLSWorksheetDoc::OnSqlCurrError ()
{
    OpenEditor::LineId lid;
    OpenEditor::Position pos;
    if (m_pOutput->GetCurrError(pos.line, pos.column, lid))
    {
        if (lid.Valid()
        && (!(pos.line < m_pEditor->GetLineCount()) || lid != m_pEditor->GetLineId(pos.line)))
        {
            if (!m_pEditor->FindById(lid, pos.line))
            {
                MessageBeep(MB_ICONHAND);
                Global::SetStatusText("Cannot find the error/message line. The original line has been replaced or deleted.", TRUE);
            }
        }

        if (pos.line < m_pEditor->GetLineCount()) // 09.03.2003 bug fix, error position fails sometimes
            pos.column = m_pEditor->InxToPos(pos.line, pos.column);

        m_pEditor->MoveToAndCenter(pos);

        ActivateEditor();
    }
}

void CPLSWorksheetDoc::OnSqlNextError ()
{
    if (m_pOutput->NextError(true))
        OnSqlCurrError();
}

void CPLSWorksheetDoc::OnSqlPrevError ()
{
    if (m_pOutput->PrevError(true))
        OnSqlCurrError();
}


void CPLSWorksheetDoc::FetchDbmsOutputLines ()
{
    ASSERT(m_pOutput);

    try
    {
        CWaitCursor wait;

        const int cnArraySize = 100;

        OciNumberVar linesV(m_connect);
        linesV.Assign(cnArraySize);
        int nLineSize = 255;
        // Version 10 and above support line size of 32767
        if ((m_connect.GetVersion() >= OCI8::esvServer10X) && (m_connect.GetClientVersion() >= OCI8::ecvClient10X))
            nLineSize = 32767;

        OciStringArray output(nLineSize, cnArraySize);

        OciStatement cursor(m_connect);
        cursor.Prepare("BEGIN dbms_output.get_lines(:lines,:numlines); END;");
        cursor.Bind(":lines", output);
        cursor.Bind(":numlines", linesV);

        string buffer;
        cursor.Execute(1, true/*guaranteedSafe*/);
        int lines = linesV.ToInt();

        while (lines > 0)
        {
            for (int i(0); i < lines; i++)
            {
                output.GetString(i, buffer);
                PutDbmsMessage(buffer.c_str());
            }

            cursor.Execute(1);
            lines = linesV.ToInt();
        }
    }
    catch (const OciException& x)
    {
        if (x != 1480)
            throw;
    }
}

void CPLSWorksheetDoc::AdjustErrorPosToLastLine (int& line, int& col) const
{
    if (col == 0 && line == m_pEditor->GetLineCount())
        col = m_pEditor->GetLineLength(--line);
}

void CPLSWorksheetDoc::OnUpdate_ErrorGroup (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_pOutput->IsEmpty());
}

#define TAB_OUT_ARR_SIZE (sizeof m_TableOutput / sizeof m_TableOutput[0])

CPLSWorksheetDoc::CLoader::CLoader ()
: m_Output(true, 32*1024),
  m_bAsOne(false),
  m_bPackAsWhole(false),
  m_nCounter(0)
{
    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
        m_TableOutput[i].Realloc(16*1024);
}

CPLSWorksheetDoc::CLoader::~CLoader ()
{
    try { EXCEPTION_FRAME;

        Flush(true);
    }
    _DESTRUCTOR_HANDLER_;
}

void CPLSWorksheetDoc::CLoader::Clear ()
{
    m_strOwner.Empty();
    m_strName.Empty();
    m_strType.Empty();
    m_Output.Erase();
    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
            m_TableOutput[i].Erase();
    m_nCounter = 0;
    m_bPackAsWhole = false;
}

    using namespace OraMetaDict;

    void put_split_line (TextOutput& out, int len = 60)
    {
        out.Put("PROMPT ");
        for (int i(4); i < len; i++) out.Put("=");
        out.NewLine();
    }

    void put_group_header (TextOutput& out, const char* title, int /*len*/ = 80)
    {
        put_split_line(out);
        out.Put("PROMPT "); out.PutLine(title);
        put_split_line(out);
    }

    void put_obj_header
        (
            TextOutput& out, const DbObject& object,
            const SQLToolsSettings& settings,
            const char* prefix = 0, int /*len*/ = 60
        )
    {
        out.Put("PROMPT ");
        if (prefix) out.Put(prefix);
        out.Put(object.GetTypeStr());
        out.Put(" ");
        out.PutOwnerAndName(object.GetOwner(), object.GetName(), settings.m_bShemaName);
        out.NewLine();
    }

#define OBJ_GROUP_HEIGHT  2
#define OBJ_HEADER_HEIGHT 1

void CPLSWorksheetDoc::CLoader::Put (const DbObject& object,
                                     const SQLToolsSettings& settings, BOOL putTitle)
{
    m_strOwner = object.GetOwner();
    m_strName  = object.GetName();
    m_strType  = object.GetTypeStr();
    m_strExt   = object.GetDefExt();

    m_Output.SetLowerDBName(settings.m_bLowerNames);

    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
        m_TableOutput[i].SetLowerDBName(settings.m_bLowerNames);

    bool splittedOutput = m_bAsOne && settings.m_bGroupByDDL && !strcmp(m_strType,"TABLE");

    if (!splittedOutput && !object.IsLight())
    {
        if (m_nCounter)
        {
            m_Output.NewLine();
            m_Output.NewLine();
        }
        if (m_bAsOne && putTitle
        && !GetSQLToolsSettings().GetNoPrompts())
            put_obj_header(m_Output, object, settings);
    }

    bool bSpecWithBody = settings.m_bSpecWithBody
                         && (!strcmp(m_strType,"PACKAGE") || !strcmp(m_strType,"TYPE"));
    bool bBodyWithSpec = !bSpecWithBody && settings.m_bBodyWithSpec
                         && (!strcmp(m_strType,"PACKAGE BODY") || !strcmp(m_strType,"TYPE BODY"));

    if (m_bAsOne && settings.m_bGroupByDDL && !strcmp(m_strType,"TABLE"))
    {
        const Table& table = static_cast<const Table&>(object);

        if (putTitle && !settings.GetNoPrompts())
            put_obj_header(m_TableOutput[0], object, settings);

        if (settings.m_bTableDefinition)
            table.WriteDefinition(m_TableOutput[0], settings);

        if (settings.m_bComments)
            table.WriteComments(m_TableOutput[0], settings);

        m_TableOutput[0].NewLine();

        TextOutputInMEM out(true, 2*1024);
        out.SetLike(m_TableOutput[0]);

        if (settings.m_bIndexes)
            table.WriteIndexes(out, settings);

        if (out.GetLength() > 0)
        {
            if (putTitle && !settings.GetNoPrompts())
                put_obj_header(m_TableOutput[1], object, settings);

                m_TableOutput[1] += out;
                m_TableOutput[1].NewLine();
                out.Erase();
        }

        if (object.UseDbms_MetaData())
        {
            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'E');

            if (out.GetLength() > 0)
            {
                if (putTitle && !settings.GetNoPrompts())
                    put_obj_header(m_TableOutput[2], object, settings);

                m_TableOutput[2] += out;
                m_TableOutput[2].NewLine();
                out.Erase();
            }

            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'R');

            if (out.GetLength() > 0)
            {
                if (putTitle && !settings.GetNoPrompts())
                    put_obj_header(m_TableOutput[3], object, settings);

                m_TableOutput[3] += out;
                m_TableOutput[3].NewLine();
                out.Erase();
            }
        }
        else
        {
            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'C');

            if (out.GetLength() > 0)
            {
                if (putTitle && !settings.GetNoPrompts())
                    put_obj_header(m_TableOutput[2], object, settings);

                m_TableOutput[2] += out;
                m_TableOutput[2].NewLine();
                out.Erase();
            }

            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'P');

            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'U');

            if (out.GetLength() > 0)
            {
                if (putTitle && !settings.GetNoPrompts())
                    put_obj_header(m_TableOutput[3], object, settings);

                m_TableOutput[3] += out;
                m_TableOutput[3].NewLine();
                out.Erase();
            }

            if (settings.m_bConstraints)
                table.WriteConstraints(out, settings, 'R');

            if (out.GetLength() > 0)
            {
                if (putTitle && !settings.GetNoPrompts())
                    put_obj_header(m_TableOutput[4], object, settings);

                m_TableOutput[4] += out;
                m_TableOutput[4].NewLine();
                out.Erase();
            }
        }

        if (settings.m_bTriggers)
            table.WriteTriggers(out, settings);

        if (out.GetLength() > 0)
        {
            if (putTitle && !settings.GetNoPrompts())
                put_obj_header(m_TableOutput[5], object, settings);

            m_TableOutput[5] += out;
            m_TableOutput[5].NewLine();
            out.Erase();
        }

        if (settings.m_bGrants)
            table.WriteGrants(out, settings);

        if (out.GetLength() > 0)
        {
            if (putTitle && !settings.GetNoPrompts())
                put_obj_header(m_TableOutput[6], table, settings, "GRANTS FOR ");

            m_TableOutput[6] += out;
            m_TableOutput[6].NewLine();
            out.Erase();
        }
    }
    else if (!(bSpecWithBody || bBodyWithSpec))
    {
        m_nOffset[0] = object.Write(m_Output, settings);
        object.WriteGrants(m_Output, settings);
    }
    else
    {
        m_bPackAsWhole = true;
        const DbObject* pPackage = 0;
        const DbObject* pPackageBody = 0;

        if (bSpecWithBody)
        {
            pPackage = &object;

            try
            {
                if (!strcmp(m_strType,"PACKAGE"))
                    pPackageBody = &object.GetDictionary().LookupPackageBody(m_strOwner, m_strName);
                else
                    pPackageBody = &object.GetDictionary().LookupTypeBody(m_strOwner, m_strName);
            }
            catch (const XNotFound&) {}

        }
        else
        {
            try
            {
                if (!strcmp(m_strType,"PACKAGE BODY"))
                    pPackage = &object.GetDictionary().LookupPackage(m_strOwner, m_strName);
                else
                    pPackage = &object.GetDictionary().LookupType(m_strOwner, m_strName);
            }
            catch (const XNotFound&) {}

            pPackageBody = &object;
        }

        if (!m_bAsOne) m_Output.EnableLineCounter(true);

        if (pPackage)
            m_nOffset[0] = pPackage->Write(m_Output, settings);
        else
        {
            m_nOffset[0] = -1;
            m_Output.Put("PROMPT Attention: specification of ");
            m_Output.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            m_Output.Put(" not found!");
            m_Output.NewLine();
            m_Output.NewLine();
        }

        int nPackSpecLines = m_Output.GetLineCounter();
        if (!m_bAsOne) m_Output.EnableLineCounter(false);

        if (pPackageBody)
            m_nOffset[1] = pPackageBody->Write(m_Output, settings) + nPackSpecLines;
        else if (bSpecWithBody && !strcmp(m_strType,"PACKAGE BODY"))
        {
            m_nOffset[1] = -1;
            m_Output.Put("PROMPT Attention: body of ");
            m_Output.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            m_Output.Put(" not found!");
            m_Output.NewLine();
            m_Output.NewLine();
        }
        else
            m_nOffset[1] = -1;

        if (pPackage)
            pPackage->WriteGrants(m_Output, settings);
    }

    m_nCounter++;

    Flush();
}

void CPLSWorksheetDoc::CLoader::Flush (bool bForce)
{
    if (m_nCounter
    && (bForce || !m_bAsOne))
    {
        CDocTemplate* pDocTemplate = CPLSWorksheetDoc::GetApp()->GetPLSDocTemplate();
        ASSERT(pDocTemplate);

        if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
        {
            ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);
            ASSERT(pDoc->m_pEditor);

            // 07.12.2003 bug fix, CreareAs property is ignored for ddl scripts (always in Unix format)
            pDoc->GetSettings().SetFileSaveAs(pDoc->GetSettings().GetFileCreateAs());

            std::vector<int> bookmarks;

            if (m_bAsOne && !m_strType.CompareNoCase("TABLE"))
            {
                LPSCSTR cszTitles[TAB_OUT_ARR_SIZE] =
                {
                    "TABLE DEFINITIONS",
                    "INDEXES",
                    "CHECK CONSTRAINTS",
                    "PRIMARY AND UNIQUE CONSTRAINTS",
                    "FOREIGN CONSTRAINTS",
                    "TRIGGERS",
                    "GRANTS",
                };

                LPSCSTR cszTitlesDBMS_MetaData[TAB_OUT_ARR_SIZE] =
                {
                    "TABLE DEFINITIONS",
                    "INDEXES",
                    "CONSTRAINTS",
                    "REFERENTIAL CONSTRAINTS",
                    "<DUMMY>",
                    "TRIGGERS",
                    "GRANTS",
                };

                bool bUseDBMS_MetaData = DbObject::UseDbms_MetaData();

                // bool bUseDBMS_MetaData = GetSQLToolsSettings().GetUseDbmsMetaData();

                // if (bUseDBMS_MetaData && 
                //    ((CSQLToolsApp*)AfxGetApp())->GetConnect().GetVersion() < OCI8::esvServer9X)
                //    bUseDBMS_MetaData = false;

                if (m_TableOutput[0].GetPosition())
                {
                    for (int i(0); i < TAB_OUT_ARR_SIZE; i++)
                    {
                        if (bUseDBMS_MetaData && (string(cszTitlesDBMS_MetaData[i]) == "<DUMMY>"))
                            continue;

                        if (m_TableOutput[i].GetPosition())
                        {
                            if (!GetSQLToolsSettings().GetNoPrompts())
                            {
                                put_group_header(m_Output, 
                                                 bUseDBMS_MetaData ? cszTitlesDBMS_MetaData[i] : cszTitles[i]);
                                m_Output.NewLine();
                            }

                            int lines = m_Output.CountLines();
                            bookmarks.push_back(lines + OBJ_HEADER_HEIGHT);

                            m_Output += m_TableOutput[i];
                            m_Output.NewLine();
                        }
                    }
                }
            }

            pDoc->SetText(m_Output, m_Output.GetLength());
            pDoc->SetModifiedFlag(FALSE);
            pDoc->m_LoadedFromDB = true;

            pDoc->m_pOutput->Clear();
            if (m_nCounter == 1 && !m_bAsOne)
            {
                // 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db
                //            if "save files when switching takss" is activated.
                //pDoc->m_strPathName = m_strName + '.' + m_strExt;
                pDoc->m_newPathName  = m_strName + '.' + m_strExt;

                CString title(m_strOwner);
                if (!m_strOwner.IsEmpty()) title += '.';
                title += pDoc->m_newPathName;
                pDoc->SetTitle(title);

                pDoc->m_pOutput->Clear();
                if (!m_bPackAsWhole)
                    pDoc->LoadErrors(m_strOwner, m_strName, m_strType, m_nOffset[0]);
                else
                {
                    if (m_nOffset[0] != -1)
                    {
                        pDoc->m_pEditor->SetQueueBookmark(m_nOffset[0]);
                        pDoc->LoadErrors(m_strOwner, m_strName, "PACKAGE", m_nOffset[0]);
                    }

                    if (m_nOffset[1] != -1)
                    {
                        pDoc->m_pEditor->SetQueueBookmark(m_nOffset[1]);
                        pDoc->LoadErrors(m_strOwner, m_strName, "PACKAGE BODY", m_nOffset[1]);
                    }
                }

                pDoc->ActivateTab(pDoc->m_pOutput);
                pDoc->ActivateEditor();
            }
            else
            {
                std::vector<int>::const_iterator it(bookmarks.begin()), end(bookmarks.end());
                for (; it != end; it++)
                    pDoc->m_pEditor->SetQueueBookmark(*it);
            }

        }

        Clear();
    }
}


/*! \fn void CPLSWorksheetDoc::OnSqlDescribe()
 *
 * \brief Find text under cursor, set it in the 'Object Viewer' and get description of it.
 */
void CPLSWorksheetDoc::OnSqlDescribe()
{
    try { EXCEPTION_FRAME;

		std::string buff;
		OpenEditor::Square sel;

		// 25.03.2003 bug refix, find object fails on object with schema
		string delims = GetSettings().GetDelimiters();
		string::size_type pos = delims.find('.');
		if (pos != string::npos) delims.erase(pos, 1);

		// CObjectViewerWnd& viewer = GetMainFrame()->ShowTreeViewer();
        CObjectViewerWnd& viewer = ((CMDIMainFrame *)AfxGetMainWnd())->ShowTreeViewer();

		if (m_pEditor->GetBlockOrWordUnderCursor(buff,  sel, true/*only one line*/, delims.c_str()))
			viewer.ShowObject(buff);

		// R#1078900 - 'Find object' (F12) and focus of editor cursor
		if (COEDocument::GetSettingsManager().GetGlobalSettings()->GetFindObjectMoveCursor() == true)
			viewer.SetFocus();
	}
	_DEFAULT_HANDLER_
}


#include "MetaDict\MetaDictionary.h"
#include "MetaDict\Loader.h"

void CPLSWorksheetDoc::OnSqlLoad ()
{
    try { EXCEPTION_FRAME;

        std::string buff;
        OpenEditor::Square sel;

        // 10.03.2003 bug fix, find object fails on object with schema
        string delims = GetSettings().GetDelimiters();
        string::size_type pos = delims.find('.');
        if (pos != string::npos) delims.erase(pos, 1);

        if (m_pEditor->GetBlockOrWordUnderCursor(buff,  sel, true/*only one line*/, delims.c_str()))
        {
            std::string owner, name, type;
            // make local copy of sqltools settings
            SQLToolsSettings settings = GetSQLToolsSettings();

            if (FindObject(m_connect, buff.c_str(), owner, name, type))
            {
                if (ShowDDLPreferences(settings, true/*bLocal*/))
                {
                    Dictionary dict;
                    {
                        CAbortController abortCtrl(*GetAbortThread(), &m_connect);
                        CString strBuff;
                        strBuff.Format("Loading %s...", name.c_str());
                        abortCtrl.SetActionText(strBuff);

                        OraMetaDict::Loader loader(m_connect, dict);
                        loader.Init();
                        loader.LoadObjects(
                            owner.c_str(), name.c_str(), type.c_str(),
                            settings,
                            settings.m_bSpecWithBody,
                            settings.m_bBodyWithSpec,
                            false/*bUseLike*/
                            );
                    }
                    {
                        CPLSWorksheetDoc::CLoader loader;
                        const DbObject* pObject = &dict.LookupObject(owner.c_str(), name.c_str(), type.c_str());
                        loader.Put(*pObject, settings, TRUE);
                    }
                }
            }
            else
            {
                AfxMessageBox((string("Object <") + buff + "> not found!").c_str(), MB_OK|MB_ICONHAND);
            }
        }

    }
    _DEFAULT_HANDLER_
}

C2PaneSplitter* CPLSWorksheetDoc::GetSplitter ()
{
    C2PaneSplitter* splitter = (C2PaneSplitter*)CView::GetParentSplitter(m_pEditor, FALSE);
    ASSERT_KINDOF(C2PaneSplitter, splitter);
    return splitter;
}

void CPLSWorksheetDoc::GoTo (int line)
{
    OpenEditor::Position pos;
    pos.column = 0;
    pos.line = line;
    m_pEditor->MoveToAndCenter(pos);
}


void CPLSWorksheetDoc::OnSqlHelp()
{
#if _MFC_VER <= 0x0600
#pragma message("There is no html help support for VC6!")
#else//_MFC_VER > 0x0600
    string helpPath;
    Global::GetHelpPath(helpPath);
    helpPath += "\\sqlqkref.chm";

    std::string key;
    OpenEditor::Square sel;
    if (m_pEditor->GetBlockOrWordUnderCursor(key,  sel, true/*only one line*/))
    {
        HH_AKLINK link;
        memset(&link, 0, sizeof link);
        link.cbStruct     = sizeof(HH_AKLINK) ;
        link.pszKeywords  = key.c_str();
        link.fIndexOnFail = TRUE ;

/*
    24.05.2002  SQLTools hangs on a context-sensitive help call
                there is more than one topic, which is found by keyword
*/
//        ::HtmlHelp(*AfxGetMainWnd(), (const char*)helpPath.c_str(), HH_KEYWORD_LOOKUP, (DWORD)&link);
        ::HtmlHelp(GetDesktopWindow(), (const char*)helpPath.c_str(), HH_KEYWORD_LOOKUP, (DWORD)&link);
    }
    else
        ::HtmlHelp(*AfxGetMainWnd(), (const char*)helpPath.c_str(), HH_DISPLAY_TOPIC, 0);
#endif
}


void CPLSWorksheetDoc::PutError (const string& str, int line, int pos)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutError(str.c_str(), line, pos, lineId);
}

void CPLSWorksheetDoc::PutMessage (const string& str, int line, int pos)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutMessage(str.c_str(), line, pos, lineId);
}

void CPLSWorksheetDoc::PutDbmsMessage (const string& str, int line, int pos)
{
    OpenEditor::LineId lineId;

    if (line != -1 && line < m_pEditor->GetLineCount())
        lineId = OpenEditor::LineId(m_pEditor->GetLineId(line));

    m_pOutput->PutDbmsMessage(str.c_str(), line, pos, lineId);
}

void CPLSWorksheetDoc::AddStatementToHistory (time_t startTime, const string& connectDesc, const string& sqlSttm)
{
    m_pHistory->AddStatement(startTime, connectDesc, sqlSttm);
}

void CPLSWorksheetDoc::PutBind (const string& name, EBindVariableTypes type, unsigned short size)
{
	m_pBindGrid->PutLine(name, type, size);
}

void CPLSWorksheetDoc::DoBinds (OCI8::AutoCursor& cursor, const vector<string>& bindVars)
{
	m_pBindGrid->DoBinds(cursor, bindVars);
}

void CPLSWorksheetDoc::RefreshBinds ()
{
	m_pBindGrid->Refresh();
}

void CPLSWorksheetDoc::OnSqlHistoryGet()
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();

    if ((settings.GetHistoryEnabled()))
    {
        std::string text;

        if (m_pHistory->GetHistoryEntry(text))
        {
            switch (settings.GetHistoryAction())
            {
            case SQLToolsSettings::HistoryAction::Copy:
                COEditorView::DoEditCopy(text);
                break;
            case SQLToolsSettings::HistoryAction::Paste:
                if (!m_pEditor->IsSelectionEmpty())
                    m_pEditor->DeleteBlock();
                m_pEditor->InsertBlock(text.c_str(), !settings.GetHistoryKeepSelection());
                break;
            case SQLToolsSettings::HistoryAction::ReplaceAll:
                m_pEditor->OnCmdMsg(ID_EDIT_SELECT_ALL, 0, 0, 0);
                m_pEditor->OnCmdMsg(ID_EDIT_DELETE, 0, 0, 0);
                m_pEditor->InsertBlock (text.c_str(), !settings.GetHistoryKeepSelection());
                break;
            }
        }

        ActivateEditor();
    }
    else
        MessageBeep((UINT)-1);

}

void CPLSWorksheetDoc::OnSqlHistoryStepforwardAndGet()
{
    if ((GetSQLToolsSettings().GetHistoryEnabled()))
    {
        m_pHistory->StepForward();
        OnSqlHistoryGet();
    }
}

void CPLSWorksheetDoc::OnSqlHistoryGetAndStepback()
{
    if ((GetSQLToolsSettings().GetHistoryEnabled()))
    {
        OnSqlHistoryGet();
        m_pHistory->StepBackward();
    }
}

void CPLSWorksheetDoc::OnUpdate_SqlHistory (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(GetSQLToolsSettings().GetHistoryEnabled() && !m_pHistory->IsEmpty());
}

BOOL CPLSWorksheetDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!COEDocument::OnOpenDocument(lpszPathName))
        return FALSE;

    if (!m_pHistory)
        m_pHistory  = new CHistoryView;

    _ASSERTE(m_pHistory);

    if (m_pHistory)
        try { EXCEPTION_FRAME;

            m_pHistory->Load(lpszPathName);
        }
        _DEFAULT_HANDLER_;

    return TRUE;
}

BOOL CPLSWorksheetDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    _ASSERTE(m_pHistory);

    if (m_pHistory)
        try { EXCEPTION_FRAME;

            m_pHistory->Save(lpszPathName);
        }
        _DEFAULT_HANDLER_;

    return COEDocument::OnSaveDocument(lpszPathName);
}

void CPLSWorksheetDoc::OnCloseDocument()
{

    _ASSERTE(m_pHistory);

    if (m_pHistory && !GetPathName().IsEmpty())
        try { EXCEPTION_FRAME;

            m_pHistory->Save(GetPathName());
        }
        _DEFAULT_HANDLER_;

	// TODO: Fix this strange issue when setting active view in OEView::SetFocus
	// ((CMDIMainFrame *)AfxGetMainWnd())->SetActiveView(NULL, FALSE);

    return COEDocument::OnCloseDocument();
}

void CPLSWorksheetDoc::OnUpdate_OciGridIndicator (CCmdUI* pCmdUI)
{
    if (m_pDbGrid)
        m_pDbGrid->OnUpdate_OciGridIndicator(pCmdUI);
}

void CPLSWorksheetDoc::OnWindowNew ()
{
    // this command is not supported
    // the default implementation crashes the application
}

void CPLSWorksheetDoc::OnUpdate_WindowNew (CCmdUI *pCmdUI)
{
    // see CPLSWorksheetDoc::OnWindowNew
    pCmdUI->Enable(FALSE);
}
