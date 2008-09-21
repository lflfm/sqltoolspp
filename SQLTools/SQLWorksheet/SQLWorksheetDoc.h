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

// 06.10.2002 changes, OnOutputOpen has been moved to GridView
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables

#pragma once
#ifndef __PLSWORKSHEETDOC_H__
#define __PLSWORKSHEETDOC_H__

#include <stack>
#include "SQLTools.h"
#include "MainFrm.h"
#include "COMMON\DocManagerExt.h"
#include "MetaDict\TextOutput.h"
#include <oci.h>
#include "BindVariableTypes.h"

#include "OpenEditor\OEView.h"
#include "OpenEditor\OEDocument.h"

    using std::string;
    using std::vector;

    class OciGridView;
    class COutputView;
    class CStatView;
    class CHistoryView;
    class CommandParser;
    class CMDIMainFrame;
    class CMDIChildFrame;
    class CTextParser;
    class C2PaneSplitter;
    class CExplainPlanView;
	class BindGridView;

	class cXPlanEdit;

    class SQLToolsSettings;

    namespace OG2 {
        class GridView;
    }
    namespace OraMetaDict {
        class DbObject;
        class Dictionary;
    }

    using namespace OG2;

/** @brief Manages a single worksheet window, and all associated grids.
 */
class CPLSWorksheetDoc : public COEDocument
{
    friend class CBooklet;

    OciConnect& m_connect;
    bool m_LoadedFromDB;

    union
    {
        struct
        {
            COEditorView* m_pEditor;
            CBooklet*     m_pBooklet;
			// cXPlanView* m_pXPlan;
			cXPlanEdit* m_pXPlan;
			// CEdit* m_pXPlan;
            union
            {
                struct
                {
                    OciGridView*  m_pDbGrid;
                    CStatView*    m_pStatGrid;
                    CExplainPlanView* m_pExplainPlan;
                    COutputView*  m_pOutput;
                    CHistoryView* m_pHistory;
					BindGridView* m_pBindGrid;
                };
                CView* m_BookletFamily[6]; /**< Collection of all grids.*/
            };
        };
        //CView* m_ViewFamily[4];
    };

protected:
    CString m_newPathName;
    virtual void GetNewPathName (CString& newName) const;
    void ActivateEditor ();
    void ActivateTab (CView*);
	
	bool IsBlankLine(const char *linePtr, const int len);

	CPLSWorksheetDoc();
	DECLARE_DYNCREATE(CPLSWorksheetDoc)

// Operations
public:
    static CSQLToolsApp*  GetApp ()       { return (CSQLToolsApp*)AfxGetApp(); }
    static CMDIMainFrame* GetMainFrame () { return (CMDIMainFrame*)GetApp()->m_pMainWnd; }
    C2PaneSplitter* GetSplitter ();
    OciConnect& GetConnect () const       { return m_connect; }


    void Init ();

    enum EExecutionMode { ALL = 0, CURRENT = 0x1, STEP = 0x2, FROM_CURSOR = 0x4 };
    static void PrintExecTime (std::ostream& out, double seconds);
    void DoSqlExecute (int mode, bool bHaltOnErrors = false);

    void MakeStep (int curLine);
    void ShowSelectResult (std::auto_ptr<OciAutoCursor>& cursor, double lastExecTime);
    void ShowOutput (bool error);
    int  LoadErrors (const char* owner, const char* name, const char* type, int line);
    void FetchDbmsOutputLines ();
    void AdjustErrorPosToLastLine (int& line, int& col) const;

    void DoSqlExplainPlan (const string&);
	void DoSqlDbmsXPlanDisplayCursor();
    static BOOL CheckFileSaveBeforeExecute();

    void GoTo (int);

    void PutError (const string& str, int line =-1, int pos =-1);
    void PutMessage (const string& str, int line =-1, int pos =-1);
    void PutDbmsMessage (const string& str, int line =-1, int pos =-1);
    void AddStatementToHistory (time_t startTime, const string& connectDesc, const string& sqlSttm);
	void PutBind (const string& name, EBindVariableTypes type, unsigned short size);
	void DoBinds (OCI8::AutoCursor& cursor, const vector<string>& bindVars);
	void RefreshBinds ();

    class CLoader
    {
        OraMetaDict::TextOutputInMEM m_Output,
                                     m_TableOutput[7];

        bool m_bAsOne, m_bPackAsWhole;
        int  m_nCounter, m_nOffset[2];
        CString m_strOwner, m_strName,
                m_strType, m_strExt;
    public:
        CLoader ();
        ~CLoader ();
        bool IsAsOne () const       { return m_bAsOne; }
        void SetAsOne (bool bAsOne) { m_bAsOne = bAsOne; }
        void Put (const OraMetaDict::DbObject&, const SQLToolsSettings&, BOOL putTitle);
        void Flush (const SQLToolsSettings & settings, bool bForce = false);
        void Clear ();
    };

    friend class CPLSWorksheetDoc::CLoader;

	//{{AFX_VIRTUAL(CPLSWorksheetDoc)
	protected:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	//}}AFX_VIRTUAL
    virtual BOOL OnIdle (LONG);

public:
	virtual ~CPLSWorksheetDoc();

protected:
	//{{AFX_MSG(CPLSWorksheetDoc)
	afx_msg void OnSqlExecute();
    afx_msg void OnSqlExecuteHaltOnErrors();
	afx_msg void OnSqlExecuteFromCursor();
    afx_msg void OnSqlExecuteFromCursorHaltOnErrors();
	afx_msg void OnSqlExecuteBelow();
	afx_msg void OnSqlExecuteCurrent();
	afx_msg void OnSqlExecuteCurrentAndStep();
    afx_msg void OnSqlExecuteExternal();
	afx_msg void OnSqlDescribe();
	afx_msg void OnSqlExplainPlan();
	afx_msg void OnSqlLoad();
	//}}AFX_MSG
	afx_msg void OnUpdate_SqlGroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdate_ErrorGroup(CCmdUI* pCmdUI);
	afx_msg void OnSqlCurrError();
	afx_msg void OnSqlNextError();
	afx_msg void OnSqlPrevError();
    afx_msg void OnSqlHistoryGet();
    afx_msg void OnSqlHistoryStepforwardAndGet();
    afx_msg void OnSqlHistoryGetAndStepback();
    afx_msg void OnUpdate_SqlHistory (CCmdUI *pCmdUI);
    afx_msg void OnUpdate_OciGridIndicator (CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnSqlHelp();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void OnCloseDocument();
    afx_msg void OnWindowNew();
    afx_msg void OnUpdate_WindowNew (CCmdUI *pCmdUI);
};

//{{AFX_INSERT_LOCATION}}

#endif//__PLSWORKSHEETDOC_H__
