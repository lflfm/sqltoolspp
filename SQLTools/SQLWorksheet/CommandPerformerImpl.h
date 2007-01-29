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

// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#pragma once
#include "CommandParser.h"
#include "AbortThread/AbortThread.h"

class CPLSWorksheetDoc;

/** @brief Executes script commands
 */
class CommandPerformerImpl : public CommandPerformer
{
public:
    CommandPerformerImpl (CPLSWorksheetDoc& doc);

    virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>& bindVariables);
    virtual void DoMessage    (CommandParser&, const string& msg);
    virtual void DoNothing    (CommandParser&);
    virtual bool InputValue   (CommandParser&, const string& prompt, string& value);
    virtual void GoTo         (CommandParser&, int line);
	virtual void DoConnect    (CommandParser&, const string& connectStr);
	virtual void DoDisconnect (CommandParser&);
	virtual void CheckConnect ();
	virtual void AddBind	  (CommandParser&, const string& var, EBindVariableTypes type, ub2 size);


    int  GetStatementCount () const     { return m_statementCount; }
    bool HasErrors () const             { return m_Errors; }
    bool IsLastStatementSelect () const { return m_LastStatementIsSelect; }
	bool IsLastStatementBind () const	{ return m_LastStatementIsBind; }
    double GetLastExecTime () const     { return m_LastExecTime; }

    std::auto_ptr<OciAutoCursor> GetCursorOwnership ();
private:
    CPLSWorksheetDoc& m_doc;
    CAbortController m_abortCtrl;
    std::auto_ptr<OciAutoCursor> m_cursor;

    int m_statementCount;
    bool m_Errors, m_LastStatementIsSelect, m_LastStatementIsBind;
	double m_LastExecTime;

    // prohibited
    CommandPerformerImpl (const CommandPerformerImpl&);
    void operator = (const CommandPerformerImpl&);
};

/** @brief Executes explain plan.
 */
class SqlPlanPerformerImpl : public CommandPerformer
{
public:
    SqlPlanPerformerImpl (CPLSWorksheetDoc& doc);

    bool IsCompleted () const           { return m_completed; }

    virtual void DoExecuteSql (CommandParser&, const string& sql, const vector<string>& bindVariables);
    virtual void DoMessage    (CommandParser&, const string& msg);
    virtual void DoNothing    (CommandParser&);
    virtual bool InputValue   (CommandParser&, const string& prompt, string& value);
    virtual void GoTo         (CommandParser&, int line);
	virtual void DoConnect    (CommandParser&, const string& connectStr);
	virtual void DoDisconnect (CommandParser&);
	virtual void AddBind	  (CommandParser&, const string& var, EBindVariableTypes type, ub2 size);


private:
    bool m_completed;
    CPLSWorksheetDoc& m_doc;
    CAbortController m_abortCtrl;
    // prohibited
    SqlPlanPerformerImpl (const SqlPlanPerformerImpl&);
    void operator = (const SqlPlanPerformerImpl&);
};

