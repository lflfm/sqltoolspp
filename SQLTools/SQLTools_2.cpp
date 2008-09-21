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
#include "SQLTools.h"
#include "COMMON/AppGlobal.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/GUICommandDictionary.h"
#include "OpenEditor/OEDocument.h"
#include "MainFrm.h"
#include "CommandLineParser.h"
#include <sstream>

/*
    30.03.2003 improvement, command line support, try sqltools /h to get help
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Common;

    const int SQLTOOLS_COPYDATA_ID = 2;

void CSQLToolsApp::InitGUICommand ()
{
#if (ID_APP_DBLKEYACCEL_FIRST != 0xDF00 || ID_APP_DBLKEYACCEL_LAST != 0xDFFF)
#pragma error("Check ID_APP_DBLKEYACCEL_FIRST or ID_APP_DBLKEYACCEL_LAST definition")
#endif

    if (!GUICommandDictionary::m_firstDblKeyAccelCommandId)
    {
    GUICommandDictionary::m_firstDblKeyAccelCommandId = ID_APP_DBLKEYACCEL_FIRST;

//File
    GUICommandDictionary::InsertCommand("File.New",                        ID_FILE_NEW);
    GUICommandDictionary::InsertCommand("File.Open",                       ID_FILE_OPEN);
    GUICommandDictionary::InsertCommand("File.Reload",                     ID_FILE_RELOAD);
    GUICommandDictionary::InsertCommand("File.Close",                      ID_FILE_CLOSE);
    GUICommandDictionary::InsertCommand("File.CloseAll",                   ID_FILE_CLOSE_ALL);
    GUICommandDictionary::InsertCommand("File.Save",                       ID_FILE_SAVE);
    GUICommandDictionary::InsertCommand("File.SaveAs",                     ID_FILE_SAVE_AS);
    GUICommandDictionary::InsertCommand("File.SaveAll",                    ID_FILE_SAVE_ALL);
    GUICommandDictionary::InsertCommand("File.ShowFileLocation",           ID_FILE_SYNC_LOCATION);
    GUICommandDictionary::InsertCommand("File.PageSetup",                  ID_EDIT_PRINT_PAGE_SETUP);
    GUICommandDictionary::InsertCommand("File.PrintSetup",                 ID_FILE_PRINT_SETUP);
    GUICommandDictionary::InsertCommand("File.PrintPreview",               ID_FILE_PRINT_PREVIEW);
    GUICommandDictionary::InsertCommand("File.Print",                      ID_FILE_PRINT);
    GUICommandDictionary::InsertCommand("File.Exit",                       ID_APP_EXIT);
//Edit
    GUICommandDictionary::InsertCommand("Edit.Undo",                       ID_EDIT_UNDO);
    GUICommandDictionary::InsertCommand("Edit.Undo",                       ID_EDIT_UNDO);
    GUICommandDictionary::InsertCommand("Edit.Redo",                       ID_EDIT_REDO);
    GUICommandDictionary::InsertCommand("Edit.Cut",                        ID_EDIT_CUT);
    GUICommandDictionary::InsertCommand("Edit.Cut",                        ID_EDIT_CUT);
    GUICommandDictionary::InsertCommand("Edit.Copy",                       ID_EDIT_COPY);
    GUICommandDictionary::InsertCommand("Edit.Copy",                       ID_EDIT_COPY);
    GUICommandDictionary::InsertCommand("Edit.Paste",                      ID_EDIT_PASTE);
    GUICommandDictionary::InsertCommand("Edit.Paste",                      ID_EDIT_PASTE);
    GUICommandDictionary::InsertCommand("Edit.CutAppend",                  ID_EDIT_CUT_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CutAppend",                  ID_EDIT_CUT_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CutBookmarkedLines",         ID_EDIT_CUT_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.CopyAppend",                 ID_EDIT_COPY_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CopyAppend",                 ID_EDIT_COPY_N_APPEND);
    GUICommandDictionary::InsertCommand("Edit.CopyBookmarkedLines",        ID_EDIT_COPY_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.Delete",                     ID_EDIT_DELETE);
    GUICommandDictionary::InsertCommand("Edit.DeletetoEndOfWord",          ID_EDIT_DELETE_WORD_TO_RIGHT);
    GUICommandDictionary::InsertCommand("Edit.DeleteWordBack",             ID_EDIT_DELETE_WORD_TO_LEFT);
    GUICommandDictionary::InsertCommand("Edit.DeleteLine",                 ID_EDIT_DELETE_LINE);
    GUICommandDictionary::InsertCommand("Edit.DeleteBookmarkedLines",      ID_EDIT_DELETE_BOOKMARKED);
    GUICommandDictionary::InsertCommand("Edit.SelectWord",                 ID_EDIT_SELECT_WORD);
    GUICommandDictionary::InsertCommand("Edit.SelectLine",                 ID_EDIT_SELECT_LINE);
    GUICommandDictionary::InsertCommand("Edit.SelectAll",                  ID_EDIT_SELECT_ALL);
    GUICommandDictionary::InsertCommand("Edit.ScrollUp",                   ID_EDIT_SCROLL_UP);
    GUICommandDictionary::InsertCommand("Edit.ScrollToCenter",             ID_EDIT_SCROLL_CENTER);
    GUICommandDictionary::InsertCommand("Edit.ScrollToCenter",             ID_EDIT_SCROLL_CENTER);
    GUICommandDictionary::InsertCommand("Edit.ScrollDown",                 ID_EDIT_SCROLL_DOWN);
//Search
    GUICommandDictionary::InsertCommand("Edit.Find",                       ID_EDIT_FIND);
    GUICommandDictionary::InsertCommand("Edit.Replace",                    ID_EDIT_REPLACE);
    GUICommandDictionary::InsertCommand("Edit.FindNext",                   ID_EDIT_FIND_NEXT);
    GUICommandDictionary::InsertCommand("Edit.FindPrevious",               ID_EDIT_FIND_PREVIOUS);
    GUICommandDictionary::InsertCommand("Edit.GoTo",                       ID_EDIT_GOTO);
    GUICommandDictionary::InsertCommand("Edit.JumpNext",                   ID_EDIT_NEXT);
    GUICommandDictionary::InsertCommand("Edit.JumpPrevious",               ID_EDIT_PREVIOUS);
    GUICommandDictionary::InsertCommand("Edit.ToggleBookmark",             ID_EDIT_BKM_TOGGLE);
    GUICommandDictionary::InsertCommand("Edit.NextBookmark",               ID_EDIT_BKM_NEXT);
    GUICommandDictionary::InsertCommand("Edit.PreviousBookmark",           ID_EDIT_BKM_PREV);
    GUICommandDictionary::InsertCommand("Edit.RemoveallBookmarks",         ID_EDIT_BKM_REMOVE_ALL);
//SetRandomBookmark
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.0",        ID_EDIT_BKM_SET_0);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.1",        ID_EDIT_BKM_SET_1);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.2",        ID_EDIT_BKM_SET_2);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.3",        ID_EDIT_BKM_SET_3);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.4",        ID_EDIT_BKM_SET_4);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.5",        ID_EDIT_BKM_SET_5);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.6",        ID_EDIT_BKM_SET_6);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.7",        ID_EDIT_BKM_SET_7);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.8",        ID_EDIT_BKM_SET_8);
    GUICommandDictionary::InsertCommand("Edit.SetRandomBookmark.9",        ID_EDIT_BKM_SET_9);
//GetRandomBookmark
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.0",        ID_EDIT_BKM_GET_0);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.1",        ID_EDIT_BKM_GET_1);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.2",        ID_EDIT_BKM_GET_2);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.3",        ID_EDIT_BKM_GET_3);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.4",        ID_EDIT_BKM_GET_4);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.5",        ID_EDIT_BKM_GET_5);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.6",        ID_EDIT_BKM_GET_6);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.7",        ID_EDIT_BKM_GET_7);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.8",        ID_EDIT_BKM_GET_8);
    GUICommandDictionary::InsertCommand("Edit.GetRandomBookmark.9",        ID_EDIT_BKM_GET_9);
    GUICommandDictionary::InsertCommand("Edit.FindMatch",                  ID_EDIT_FIND_MATCH);
    GUICommandDictionary::InsertCommand("Edit.FindMatchSelect",            ID_EDIT_FIND_MATCH_N_SELECT);
    GUICommandDictionary::InsertCommand("Edit.FindInFile",                 ID_FILE_FIND_IN_FILE);
//Format
    GUICommandDictionary::InsertCommand("Edit.Sort",                       ID_EDIT_SORT);
    GUICommandDictionary::InsertCommand("Edit.IndentSelection",            ID_EDIT_INDENT);
    GUICommandDictionary::InsertCommand("Edit.UndentSelection",            ID_EDIT_UNDENT);
    GUICommandDictionary::InsertCommand("Edit.UntabifySelection",          ID_EDIT_UNTABIFY);
    GUICommandDictionary::InsertCommand("Edit.TabifySelectionAll",         ID_EDIT_TABIFY);
    GUICommandDictionary::InsertCommand("Edit.TabifySelectionLeading",     ID_EDIT_TABIFY_LEADING);
    GUICommandDictionary::InsertCommand("Edit.NormalizeKeyword",           ID_EDIT_NORMALIZE_TEXT);
    GUICommandDictionary::InsertCommand("Edit.Lowercase",                  ID_EDIT_LOWER);
    GUICommandDictionary::InsertCommand("Edit.Uppercase",                  ID_EDIT_UPPER);
    GUICommandDictionary::InsertCommand("Edit.Capitalize",                 ID_EDIT_CAPITALIZE);
    GUICommandDictionary::InsertCommand("Edit.InvertCase",                 ID_EDIT_INVERT_CASE);
    GUICommandDictionary::InsertCommand("Edit.CommentLine",                ID_EDIT_COMMENT);
    GUICommandDictionary::InsertCommand("Edit.UncommentLine",              ID_EDIT_UNCOMMENT);
    GUICommandDictionary::InsertCommand("Edit.ExpandTemplate",             ID_EDIT_EXPAND_TEMPLATE);
    GUICommandDictionary::InsertCommand("Edit.DatetimeStamp",              ID_EDIT_DATETIME_STAMP);
    GUICommandDictionary::InsertCommand("Edit.OpenFileUnderCursor",        ID_EDIT_OPENFILEUNDERCURSOR);
//Text
    GUICommandDictionary::InsertCommand("Edit.StreamSelection",            ID_EDIT_STREAM_SEL);
    GUICommandDictionary::InsertCommand("Edit.ColumnSelection",            ID_EDIT_COLUMN_SEL);
    GUICommandDictionary::InsertCommand("Edit.ToggleSelectionMode",        ID_EDIT_TOGGLES_SEL);
    GUICommandDictionary::InsertCommand("Edit.VisibleSpaces",              ID_EDIT_VIEW_WHITE_SPACE);
    GUICommandDictionary::InsertCommand("Edit.LineNumbers",                ID_EDIT_VIEW_LINE_NUMBERS);
    GUICommandDictionary::InsertCommand("Edit.Ruler",                      ID_EDIT_VIEW_RULER);
    GUICommandDictionary::InsertCommand("Edit.FileSettings",               ID_EDIT_FILE_SETTINGS);
    GUICommandDictionary::InsertCommand("Edit.PermanentSettings",          ID_APP_SETTINGS); //ID_EDIT_PERMANENT_SETTINGS
    GUICommandDictionary::InsertCommand("Edit.ColumnMarkers",              ID_EDIT_VIEW_COLUMN_MARKERS);
    GUICommandDictionary::InsertCommand("Edit.IndentGuide",                ID_EDIT_VIEW_INDENT_GUIDE);
// Misc
    GUICommandDictionary::InsertCommand("Edit.CopyWithNewLines",           ID_EDIT_COPY_NEW_LINES);

//View
    GUICommandDictionary::InsertCommand("View.Toolbar",                    ID_VIEW_TOOLBAR);
    GUICommandDictionary::InsertCommand("View.StatusBar",                  ID_VIEW_STATUS_BAR);
    GUICommandDictionary::InsertCommand("View.FilePanel",                  ID_VIEW_FILE_PANEL);
    GUICommandDictionary::InsertCommand("View.Workbook",                   ID_VIEW_WORKBOOK);
    GUICommandDictionary::InsertCommand("View.NextPane",                   ID_CUSTOM_NEXT_PANE); // there is a big difference
    GUICommandDictionary::InsertCommand("View.PrevPane",                   ID_CUSTOM_PREV_PANE); // between ID_NEXT_PANE and ID_CUSTOM_NEXT_PANE
//Window
    GUICommandDictionary::InsertCommand("Window.NewWindow",                ID_WINDOW_NEW);
    GUICommandDictionary::InsertCommand("Window.Cascade",                  ID_WINDOW_CASCADE);
    GUICommandDictionary::InsertCommand("Window.TileHorizontally",         ID_WINDOW_TILE_HORZ);
    GUICommandDictionary::InsertCommand("Window.TileVertically",           ID_WINDOW_TILE_VERT);
    GUICommandDictionary::InsertCommand("Window.ArrangeIcons",             ID_WINDOW_ARRANGE);
    GUICommandDictionary::InsertCommand("Window.LastWindow",               ID_WINDOW_LAST);
//Help
    GUICommandDictionary::InsertCommand("Help.AboutOpenEditor",            ID_APP_ABOUT);


//SQLTools command set
//View
    GUICommandDictionary::InsertCommand("View.ObjectTree",                 ID_SQL_OBJ_VIEWER);
    GUICommandDictionary::InsertCommand("View.ExplainPlan",                ID_SQL_PLAN_VIEWER);
    GUICommandDictionary::InsertCommand("View.ObjectList",                 ID_SQL_DB_SOURCE);
    GUICommandDictionary::InsertCommand("View.FindInFiles",                ID_FILE_SHOW_GREP_OUTPUT);
//Session
    GUICommandDictionary::InsertCommand("Session.Connect",                 ID_SQL_CONNECT);
    GUICommandDictionary::InsertCommand("Session.Disconnect",              ID_SQL_DISCONNECT);
    GUICommandDictionary::InsertCommand("Session.Reconnect",               ID_SQL_RECONNECT);
    GUICommandDictionary::InsertCommand("Session.Commit",                  ID_SQL_COMMIT);
    GUICommandDictionary::InsertCommand("Session.Rollback",                ID_SQL_ROLLBACK);
    GUICommandDictionary::InsertCommand("Session.EnableDbmsOutput",        ID_SQL_DBMS_OUTPUT);
    GUICommandDictionary::InsertCommand("Session.EnableSessionStatistics", ID_SQL_SESSION_STATISTICS);
    GUICommandDictionary::InsertCommand("Session.DbmsXplanDisplayCursor",  ID_SQL_DBMS_XPLAN_DISPLAY_CURSOR);
//Script
    GUICommandDictionary::InsertCommand("Script.Execute",                  ID_SQL_EXECUTE);
    GUICommandDictionary::InsertCommand("Script.ExecuteHaltOnErrors",      ID_SQL_EXECUTE_HALT_ON_ERRORS);
    GUICommandDictionary::InsertCommand("Script.ExecuteFromCursor",        ID_SQL_EXECUTE_FROM_CURSOR);
    GUICommandDictionary::InsertCommand("Script.ExecuteFromCursorHaltOnErrors",ID_SQL_EXECUTE_FROM_CURSOR_HALT_ON_ERRORS);
    GUICommandDictionary::InsertCommand("Script.ExecuteBelow",             ID_SQL_EXECUTE_BELOW);
    GUICommandDictionary::InsertCommand("Script.ExecuteCurrent",           ID_SQL_EXECUTE_CURRENT);
    GUICommandDictionary::InsertCommand("Script.ExecuteCurrentAndStep",    ID_SQL_EXECUTE_CURRENT_AND_STEP);
    GUICommandDictionary::InsertCommand("Script.ExecuteExternal",          ID_SQL_EXECUTE_EXTERNAL);
    GUICommandDictionary::InsertCommand("Script.NextError",                ID_SQL_NEXT_ERROR);
    GUICommandDictionary::InsertCommand("Script.PreviousError",            ID_SQL_PREV_ERROR);
    GUICommandDictionary::InsertCommand("Script.GetHistoryAndStepBack",    ID_SQL_HISTORY_GET_AND_STEPBACK);
    GUICommandDictionary::InsertCommand("Script.StepForwardAndGetHistory", ID_SQL_HISTORY_STEPFORWARD_AND_GET);
    GUICommandDictionary::InsertCommand("Script.FindObject",               ID_SQL_DESCRIBE);
    GUICommandDictionary::InsertCommand("Script.FindObject",               ID_SQL_DESCRIBE);
    GUICommandDictionary::InsertCommand("Script.LoadDDL",                  ID_SQL_LOAD);
    GUICommandDictionary::InsertCommand("Script.LoadDDL",                  ID_SQL_LOAD);
    GUICommandDictionary::InsertCommand("Script.ExplainPlan",              ID_SQL_EXPLAIN_PLAN);
    GUICommandDictionary::InsertCommand("Script.SubstitutionVariables",    ID_SQL_SUBSTITUTION_VARIABLES);
//Tools
    GUICommandDictionary::InsertCommand("Tools.ExtractSchemaDDL",          ID_SQL_EXTRACT_SCHEMA);
    GUICommandDictionary::InsertCommand("Tools.TableTransformationHelper", ID_SQL_TABLE_TRANSFORMER);
    GUICommandDictionary::InsertCommand("Tools.SessionDdlGridSettings",    ID_APP_SETTINGS);
//Window
    GUICommandDictionary::InsertCommand("Window.NextPane",                 ID_CUSTOM_NEXT_PANE);
    GUICommandDictionary::InsertCommand("Window.PreviousPane",             ID_CUSTOM_PREV_PANE);
    GUICommandDictionary::InsertCommand("Window.NextTab",                  ID_CUSTOM_NEXT_TAB);
    GUICommandDictionary::InsertCommand("Window.PreviousTab",              ID_CUSTOM_PREV_TAB);
    GUICommandDictionary::InsertCommand("Window.ActivateSplitter",         ID_WINDOW_SPLIT);
    GUICommandDictionary::InsertCommand("Window.SplitterDown",             ID_CUSTOM_SPLITTER_DOWN);
    GUICommandDictionary::InsertCommand("Window.SplitterUp",               ID_CUSTOM_SPLITTER_UP);
//Help
    GUICommandDictionary::InsertCommand("Help.Help",                       ID_HELP);
    GUICommandDictionary::InsertCommand("Help.SqlHelp",                    ID_SQL_HELP);
    GUICommandDictionary::InsertCommand("Help.ContextHelp",                ID_CONTEXT_HELP);
//Object List Accelerators
	// GUICommandDictionary::InsertCommand("ObjectList.Copy",                 IDC_DS_COPY);
	GUICommandDictionary::InsertCommand("PlanView.Refresh",                ID_NP_REFRESH);
	GUICommandDictionary::InsertCommand("PopupEditor.Close",               ID_GRIDPOPUP_CLOSE);
    }
}

void CSQLToolsApp::UpdateAccelAndMenu ()
{
    InitGUICommand();

    string buffer;
    Global::GetSettingsPath(buffer);
    if(GUICommandDictionary::BuildAcceleratorTable(
        (buffer + '\\' + COEDocument::GetSettingsManager().GetGlobalSettings()->GetKeymapLayout() + ".keymap").c_str(),
        m_accelTable))
    {
		POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
		while (pos != NULL)
		{
		    CDocTemplate* pTemplate = m_pDocManager->GetNextDocTemplate(pos);
			if (pTemplate && pTemplate->IsKindOf(RUNTIME_CLASS(CMultiDocTemplate)))
            {
                ((CMultiDocTemplate*)pTemplate)->m_hAccelTable = m_accelTable;
                GUICommandDictionary::AddAccelDescriptionToMenu(((CMultiDocTemplate*)pTemplate)->m_hMenuShared);
            }
        }

        if (m_pMainWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
        {
            ((CFrameWnd*)m_pMainWnd)->m_hAccelTable = m_accelTable;
            GUICommandDictionary::AddAccelDescriptionToMenu(((CFrameWnd*)m_pMainWnd)->m_hMenuDefault);
        }

    }
}

void CSQLToolsApp::OnDblKeyAccel (UINT nID)
{
    string desc;
    m_dblKeyAccelInx = nID - ID_APP_DBLKEYACCEL_FIRST;
    if (GUICommandDictionary::GetDblKeyDescription(m_dblKeyAccelInx, desc))
    {
        desc +=  " was pressed. Waiting for second key of chord...";
        Global::SetStatusText(desc.c_str(), TRUE);
    }
}


BOOL CSQLToolsApp::PreTranslateMessage (MSG* pMsg)
{
    if (m_dblKeyAccelInx != -1)
    {
        Common::Command commandId;
        Common::VKey vkey = static_cast<Common::VKey>(pMsg->wParam);

        if (pMsg->message == WM_KEYDOWN
        && GUICommandDictionary::GetDblKeyAccelCmdId(m_dblKeyAccelInx, vkey, commandId))
        {
            pMsg->message = WM_NULL;
            m_dblKeyAccelInx = -1;
            Global::SetStatusText("", TRUE);
            if (m_pMainWnd) m_pMainWnd->SendMessage(WM_COMMAND, commandId);
			return FALSE;
        }
        else if((pMsg->message == WM_KEYDOWN)
            || (pMsg->message == WM_COMMAND)
            || (pMsg->message == WM_SYSCOMMAND)
            || (pMsg->message > WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST))
        {
            string desc;
            GUICommandDictionary::GetDblKeyDescription(m_dblKeyAccelInx, desc, vkey);
            desc +=  "   is not an acceleration sequence.";
            Global::SetStatusText(desc.c_str(), TRUE);
            pMsg->message = WM_NULL;
            m_dblKeyAccelInx = -1;
            MessageBeep((UINT)-1);
			return FALSE;
        }
    }

    return CWinApp::PreTranslateMessage(pMsg);
}


BOOL CSQLToolsApp::AllowThisInstance ()
{
    if (!m_commandLineParser.GetReuseOption())
    {
        if (m_commandLineParser.GetStartOption())
            return TRUE;

        if (COEDocument::GetSettingsManager().GetGlobalSettings()->GetAllowMultipleInstances())
            return TRUE;
    }

    const char* cszMutex = "Kochware.SQLTools";
    const char* cszError = "Cannot connect to another program instance. ";

    m_hMutex = CreateMutex(NULL, FALSE, cszMutex);
    if (m_hMutex == NULL)
    {
        AfxMessageBox(CString(cszError) + "CreateMutex error.", MB_OK|MB_ICONHAND);
        return TRUE;
    }

    DWORD dwWaitResult = WaitForSingleObject(m_hMutex, 3000L);
    if (dwWaitResult == WAIT_TIMEOUT)
    {
        AfxMessageBox(CString(cszError) + " WaitForMutex timeout.", MB_OK|MB_ICONHAND);
        return TRUE;
    }

    HWND hAnother = FindWindow(CMDIMainFrame::m_cszClassName, NULL);

    if (hAnother)
    {
        std::string data;
        m_commandLineParser.Pack(data);

        COPYDATASTRUCT cpdata;
        cpdata.dwData = SQLTOOLS_COPYDATA_ID;
        cpdata.lpData = (PVOID)data.c_str();
        cpdata.cbData = data.length();

        DWORD dwResult = TRUE;
        if (SendMessageTimeout(
            hAnother,                       // handle to window
            WM_COPYDATA,                    // message
            0,                              // first message parameter
            (LPARAM)&cpdata,                // second message parameter
            SMTO_ABORTIFHUNG|SMTO_BLOCK,    // send options
            1500,                           // time-out duration
            &dwResult                       // return value for synchronous call
            )
        && dwResult == TRUE)
        {
            ::SetForegroundWindow(hAnother);
            if (::IsIconic(hAnother)) OpenIcon(hAnother);
            return FALSE;
        }
        else
            AfxMessageBox(CString(cszError) + "Request ignored or timeout.", MB_OK|MB_ICONHAND);
    }

    return TRUE;
}

BOOL CSQLToolsApp::HandleAnotherInstanceRequest (COPYDATASTRUCT* pCopyDataStruct)
{
    try { EXCEPTION_FRAME;

        if (pCopyDataStruct->dwData == SQLTOOLS_COPYDATA_ID)
        {
            if (pCopyDataStruct->cbData)
            {
                m_commandLineParser.Unpack((LPCSTR)pCopyDataStruct->lpData, pCopyDataStruct->cbData);
                PostThreadMessage(m_msgCommandLineProcess, 0, 0);
            }
            return TRUE;
        }
    }
    _DEFAULT_HANDLER_

    return FALSE;
}


afx_msg void CSQLToolsApp::OnCommandLineProcess (WPARAM, LPARAM)
{
    try { EXCEPTION_FRAME;

        m_commandLineParser.Process();

        if (m_commandLineParser.GetConnectOption())
        {
            if (m_connect->IsOpen())
            {
                switch (AfxMessageBox("You have an open session." 
                "Would you like to do commit before changing connection?"
                "\nPress \"Yes\" to commit, \"No\" to rollback or \"Cancel\" to keep the current connection.", MB_YESNOCANCEL))
                {
                case IDYES:
                    m_connect->Commit();
                    break;
                case IDNO:
                    m_connect->Rollback();
                    break;
                case IDCANCEL:
                    return;
                }
            }

            std::string user, password, alias, port, sid, mode;

            if (m_commandLineParser.GetConnectionInfo(user, password, alias, port, sid, mode))
            {
                try { EXCEPTION_FRAME;

	                m_connect->Close();
                }
                _DEFAULT_HANDLER_

                try
                {
                    int mode_ = 0;

                    if (!stricmp(mode.c_str(), "SYSDBA"))       mode_ = 1;
                    else if (!stricmp(mode.c_str(), "SYSOPER")) mode_ = 2;

                    if (port.empty())
                        DoConnect(user.c_str(), password.c_str(), alias.c_str(), mode_, 0);
                    else
                        DoConnect(user.c_str(), password.c_str(), alias.c_str(), port.c_str(), sid.c_str(), false, mode_, 0);
                }
                catch (const OciException& x)
                {
                    AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                    OnSqlConnect();
                }
            }
            else
                OnSqlConnect();
        }
    }
    _DEFAULT_HANDLER_

}
