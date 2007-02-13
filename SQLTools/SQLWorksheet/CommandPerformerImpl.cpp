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

// 22.11.2004 bug fix, all compilation errors are ignored for Oracle7
// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 09.01.2005 (ak) R1092514: SQL*Plus CONNECT/DISCONNECT commands - small improvements
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables
// 28.09.2006 (ak) bXXXXXXX: poor fetch performance because prefetch rows disabled

#include "stdafx.h"
#include "SQLTools.h"
#include "SQLWorksheetDoc.h"
#include "CommandPerformerImpl.h"
#include <OCI8/BCursor.h>
#include <OCI8/ACursor.h>
#include <OCI8/Statement.h>
#include "COMMON/AppGlobal.h"
#include <COMMON/StrHelpers.h>
#include "InputDlg.h"

    static
    void make_title (const string& src, int titleLength, string& dest)
    {
        dest.clear();
        bool prevSpace = true;
        string::const_iterator it = src.begin();
        for (int i = 0; i < titleLength - int(sizeof("...")-1) && it != src.end(); i++, ++it) {
            if (!isspace(*it)) {
                dest += *it;
                prevSpace = false;
            } else {
                if (!prevSpace) dest += ' ';
                prevSpace = true;
            }
        }
        if (it != src.end())
            dest += "...";
    }

CommandPerformerImpl::CommandPerformerImpl (CPLSWorksheetDoc& doc)
    : m_doc(doc),
    m_abortCtrl(*GetAbortThread(), &doc.GetConnect()),
    m_cursor(new OciAutoCursor(doc.GetConnect(), 1000)),
    m_statementCount(0),
    m_Errors(false),
    m_LastStatementIsSelect(false),
	m_LastStatementIsBind(false)
{
}

std::auto_ptr<OciAutoCursor> CommandPerformerImpl::GetCursorOwnership ()
{
    std::auto_ptr<OciAutoCursor> cursor(new OciAutoCursor(m_doc.GetConnect(), 100));
    std::swap(m_cursor, cursor);
    return cursor;
}


/** @brief Handles execution of an SQL command or PL/SQL block.
 *
 * @arg commandParser Parser from which this was called.
 * @arg sql SQL command.
 */
void CommandPerformerImpl::DoExecuteSql (CommandParser& commandParser, const string& sql, const vector<string>& bindVars)
{
    CheckConnect();

	const SQLToolsSettings& settings = GetSQLToolsSettings();

    string title;
    make_title(sql, 48, title);
    Global::SetStatusText(title, TRUE);
    m_abortCtrl.SetActionText(title.c_str());

    OciConnect& connect = m_doc.GetConnect();
    m_statementCount++;
	m_LastStatementIsBind = false;

    bool isOutputEnabled = connect.IsOutputEnabled();
    time_t startTime = time((time_t*)0);

    if (commandParser.IsTextSubstituted())
        m_doc.PutMessage("NEW:\n" + sql, commandParser.GetBaseLine());

    try
    {
        if(m_cursor->IsOpen()) m_cursor->Close();

        m_cursor->Prepare(sql.c_str());

        m_LastStatementIsSelect = (m_cursor->GetType() == OCI8::StmtSelect);

        if (m_LastStatementIsSelect)
        {
            m_cursor->SetNumberFormat      (settings.GetGridNlsNumberFormat());
            m_cursor->SetDateFormat        (settings.GetGridNlsDateFormat());
            m_cursor->SetTimeFormat        (settings.GetGridNlsTimeFormat());
            m_cursor->SetTimestampFormat   (settings.GetGridNlsTimestampFormat());
            m_cursor->SetTimestampTzFormat (settings.GetGridNlsTimestampTzFormat());
            m_cursor->SetStringNull        (settings.GetGridNullRepresentation());
            m_cursor->SetSkipLobs          (settings.GetGridSkipLobs());
            m_cursor->SetStringLimit       (settings.GetGridMaxLobLength());
            m_cursor->SetBlobHexRowLength  (settings.GetGridBlobHexRowLength());
        }

        m_cursor->EnableAutocommit(settings.GetAutocommit());

        if (!bindVars.empty())
		    m_doc.DoBinds(*m_cursor, bindVars);

        if (isOutputEnabled)
            connect.ResetOutput();

        clock_t startClock = clock();
        m_cursor->Execute();
        double execTime = double(clock() - startClock)/ CLOCKS_PER_SEC;
		if (GetSQLToolsSettings().GetDbmsXplanDisplayCursor())
			connect.RetrieveCurrentSqlInfo();

        ostringstream message;
        message << m_cursor->GetSQLFunctionDescription();

        switch (m_cursor->GetType())
        {
        case OCI8::StmtInsert:
        case OCI8::StmtUpdate:
        case OCI8::StmtDelete:
            message << " - " << m_cursor->GetRowCount() << " row(s)";
        }
        message << ", executed in ";
        CPLSWorksheetDoc::PrintExecTime(message, execTime);

		m_LastExecTime = execTime;

        m_doc.PutMessage(message.str(), commandParser.GetBaseLine());

        // TODO: recognize changed variables, highlight them and switch to the "bins" page
        if (!bindVars.empty())
		    m_doc.RefreshBinds();

		if (GetSQLToolsSettings().GetDbmsXplanDisplayCursor())
			m_doc.DoSqlDbmsXPlanDisplayCursor();
        // TODO: implement isDML
        if (isOutputEnabled /*&& !m_cursor->IsDML()*/) // even select can write dbms output
            m_doc.FetchDbmsOutputLines();

        // 22.11.2004 bug fix, all compilation errors are ignored for Oracle7
        // OCI8 does not report properly about sql type in case Oracle7 server
        // so ask parser about sql is parsed
        //if (m_cursor->PossibleCompilationErrors())
        if (commandParser.MayHaveCompilationErrors())
        {
            string name, owner, type;
            commandParser.GetCodeName(owner, name, type);
            m_Errors |= m_doc.LoadErrors(owner.c_str(), name.c_str(), type.c_str(), commandParser.GetErrorBaseLine()) > 0;
        }

        m_doc.AddStatementToHistory(startTime, connect.GetDisplayString(), sql);
    }
    catch (const OciException& x)
    {
        if (x == 0 || !connect.IsOpen())
            throw;

        m_Errors = true;
        m_LastStatementIsSelect = (m_cursor->GetType() == OCI8::StmtSelect) ? true : false;

        // TODO: implement isDML
        if (isOutputEnabled /*&& !m_cursor->IsDML()*/) // even select can write dbms output
            m_doc.FetchDbmsOutputLines();

        int line, col;
        commandParser.GetSelectErrorPos(m_cursor->GetParseErrorOffset(), line, col);

        // ORA-00921: unexpected end of SQL command
        // move error position on last existing line
        // because we need line id for stable error position
        if (x == 921)
            m_doc.AdjustErrorPosToLastLine(line, col);

        m_doc.PutError(x.what(), line, col);

        if (!GetSQLToolsSettings().GetHistoryValidOnly())
            m_doc.AddStatementToHistory(startTime, connect.GetDisplayString(), sql);

        if (!connect.IsOpen()) throw;
        // TODO: error filter here!!!
    }

    Global::SetStatusText(NULL,TRUE);
    commandParser.Restart();
}

void CommandPerformerImpl::DoMessage (CommandParser& commandParser, const string& text)
{
    m_statementCount++;
    m_doc.PutMessage(text.c_str(), commandParser.GetBaseLine());
    commandParser.Restart();
}

void CommandPerformerImpl::DoNothing (CommandParser& commandParser)
{
    m_statementCount++;
    commandParser.Restart();
}

bool CommandPerformerImpl::InputValue (CommandParser&, const string& prompt, string& value)
{
    m_abortCtrl.EndBatchProcess();

    CInputDlg dlg;
    dlg.m_prompt = prompt;
    bool retVal = dlg.DoModal() == IDOK ? true : false;
    value = dlg.m_value;

    m_abortCtrl.StartBatchProcess(&m_doc.GetConnect());

    return retVal;
}

void CommandPerformerImpl::GoTo (CommandParser&, int line)
{
    m_doc.GoTo(line);
}

/** @brief Handles CONNECT command.
 *
 * If a connect string is given, we use that.  Otherwise, display the connect dialog box.
 *
 * @arg commandParser Parser from which this was called.
 * @arg connectStr Connect string.
 */
void CommandPerformerImpl::DoConnect (CommandParser& commandParser, const string& connectStr)
{
	string user, password, alias, port, sid, mode;
    m_statementCount++;
	m_LastStatementIsSelect = false;
	m_LastStatementIsBind = false;

	m_abortCtrl.EndBatchProcess();

	if (CCommandLineParser::GetConnectionInfo(connectStr, user, password, alias, port, sid, mode))
	{
		try { EXCEPTION_FRAME;

	        m_doc.GetApp()->OnSqlDisconnect();
		}
		_DEFAULT_HANDLER_

		try
		{
			int mode_ = 0;

			if (!stricmp(mode.c_str(), "SYSDBA"))       mode_ = 1;
			else if (!stricmp(mode.c_str(), "SYSOPER")) mode_ = 2;

            if (!user.empty() && password.empty())
            {
                CPasswordDlg dlg;
                dlg.m_prompt = "Enter password for \"" + user + '@' + alias;
                if (!port.empty()) dlg.m_prompt += ':' + port + ':' + sid;
                dlg.m_prompt += "\":";

                GoTo(commandParser, commandParser.GetBaseLine());
                if (dlg.DoModal() != IDOK)
                {
                    ASSERT_EXCEPTION_FRAME;
                    throw Common::AppException("User canceled conection!");
                }

                password = dlg.m_value;
            }

			if (port.empty())
				m_doc.GetApp()->DoConnect(user.c_str(), password.c_str(), alias.c_str(), mode_, 0);
			else
				m_doc.GetApp()->DoConnect(user.c_str(), password.c_str(), alias.c_str(), port.c_str(), sid.c_str(), false, mode_, 0);
		}
		catch (const OciException& x)
		{
			AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
            GoTo(commandParser, commandParser.GetBaseLine());
			m_doc.GetApp()->OnSqlConnect();
		}
	}
	else
    {
        GoTo(commandParser, commandParser.GetBaseLine());
		m_doc.GetApp()->OnSqlConnect();
    }

    string message = (m_doc.GetApp()->GetConnect().IsOpen())
        ? string("Connected to \"") + m_doc.GetApp()->GetDisplayConnectionString() + '\"'
        : string("Connection failed");

    m_doc.PutMessage(message, commandParser.GetBaseLine());
    commandParser.Restart();
    m_abortCtrl.StartBatchProcess(&m_doc.GetConnect());
}

/** @brief Handles DISCONNECT command.
 *
 * @arg commandParser Parser from which this was called.
 */
void CommandPerformerImpl::DoDisconnect (CommandParser& commandParser)
{
	m_abortCtrl.EndBatchProcess(); // hide it because commit/rollback dialog might appear on disconnect
    m_statementCount++;
	m_LastStatementIsSelect = false;
	m_LastStatementIsBind = false;

    GoTo(commandParser, commandParser.GetBaseLine());
	m_doc.GetApp()->OnSqlDisconnect();
    m_doc.PutMessage("Disconnected", commandParser.GetBaseLine());
    commandParser.Restart();
    m_abortCtrl.StartBatchProcess(&m_doc.GetConnect());
}

/** @brief Verifies connection to database.
 *
 * Throws an exception if not connected.
 */
void CommandPerformerImpl::CheckConnect ()
{
	if (!m_doc.GetConnect().IsOpen())
    {
        ASSERT_EXCEPTION_FRAME;
		throw Common::AppException("SQL command: not connected to database!");
    }
}

/** @brief Adds a bind variable.
 *  See BindGridSource::AddBind().
 */
void CommandPerformerImpl::AddBind (CommandParser& commandParser, const string& name, EBindVariableTypes type, ub2 size)
{
	m_statementCount++;
	m_LastStatementIsSelect = false;
	m_LastStatementIsBind = true;

	m_doc.PutBind(name, type, size);
	commandParser.Restart();
}

////////////////////////////////////////////////////////////////////////////////
// CommandPerformerImpl
SqlPlanPerformerImpl::SqlPlanPerformerImpl (CPLSWorksheetDoc& doc)
    : m_completed(false),
    m_doc(doc),
    m_abortCtrl(*GetAbortThread(), &doc.GetConnect())
{
}

void SqlPlanPerformerImpl::DoExecuteSql (CommandParser&, const string& sql, const vector<string>&)
{
    m_completed = true;
    m_doc.DoSqlExplainPlan(sql);
}

void SqlPlanPerformerImpl::DoMessage (CommandParser&, const string&)
{
    // do nothing
}

void SqlPlanPerformerImpl::DoNothing (CommandParser&)
{
    // do nothing
}

bool SqlPlanPerformerImpl::InputValue (CommandParser&, const string& prompt, string& value)
{
    m_abortCtrl.EndBatchProcess();

    CInputDlg dlg;
    dlg.m_prompt = prompt;
    bool retVal = dlg.DoModal() == IDOK ? true : false;
    value = dlg.m_value;

    m_abortCtrl.StartBatchProcess(&m_doc.GetConnect());

    return retVal;
}

void SqlPlanPerformerImpl::GoTo (CommandParser&, int line)
{
    m_doc.GoTo(line);
}

void SqlPlanPerformerImpl::DoConnect (CommandParser&, const string& /*connectStr*/)
{
	// do nothing
}

void SqlPlanPerformerImpl::DoDisconnect (CommandParser&)
{
	// do nothing
}

void SqlPlanPerformerImpl::AddBind (CommandParser&, const string&, EBindVariableTypes, ub2)
{
	// do nothing
}

