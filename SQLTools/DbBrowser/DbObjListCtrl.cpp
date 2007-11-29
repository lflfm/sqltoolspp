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
#include "DbObjListCtrl.h"
#include "DbSourceWnd.h"
#include <COMMON\SimpleDragDataSource.h> // for Drag & Drop
#include <COMMON\ExceptionHelper.h>
#include <OCI8/BCursor.h>

/*
    30.03.2003 optimization, removed images from Object Browser for Win95/Win98/WinMe because of resource cost
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OCI8;

/////////////////////////////////////////////////////////////////////////////
// CDbObjListCtrl

CDbObjListCtrl::CDbObjListCtrl (OciConnect& connect, const CData& data)
:  CManagedListCtrl(m_adapter), m_Data(data),
   m_bDirty(true),
   m_nStatusColumn(-1), m_nEnabledColumn(-1),
   m_nSortColumn(-1), m_nDirection(0),
   m_connect(connect), m_adapter(m_arrObjects, data)
{
    ASSERT(&data);
}

CDbObjListCtrl::~CDbObjListCtrl()
{
}


BEGIN_MESSAGE_MAP(CDbObjListCtrl, CManagedListCtrl)
    //{{AFX_MSG_MAP(CDbObjListCtrl)
    ON_WM_CREATE()
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnSelectAll)
    ON_WM_INITMENUPOPUP()
    ON_WM_LBUTTONDBLCLK()
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDbObjListCtrl message handlers

void CDbObjListCtrl::SetStyle(void)
{
    ModifyStyle(LVS_SINGLESEL, 0);

    // 30.03.2003 optimization, removed images from Object Browser for Win95/Win98/WinMe because of resource cost
    if (!(::GetVersion() & 0x80000000))
    {
        if (m_Images.m_hImageList == NULL)
            m_Images.Create(IDB_IMAGELIST3, 16, 64, RGB(0,255,0));

        SetImageList(&m_Images, LVSIL_SMALL);
    }

    if (m_Data.m_szStatusColumn || m_Data.m_szEnabledColumn)
    {
        if (m_StateImages.m_hImageList == NULL)
            m_StateImages.Create(IDB_IMAGELIST4, 16, 64, RGB(0,255,0));

        SetImageList(&m_StateImages, LVSIL_STATE);

        SetCallbackMask(LVIS_STATEIMAGEMASK);
    }
}

int CDbObjListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CManagedListCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetStyle();
    /*
    LV_COLUMN lvcolumn;
    for (int i = 0; i < m_Data.m_nColumns; i++)
    {
        lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvcolumn.fmt = m_Data.m_pColumns[i].m_dwAttr;
        lvcolumn.pszText = (char*)(const char*)m_Data.m_pColumns[i].m_szTitle;
        lvcolumn.iSubItem = i;
        lvcolumn.cx = m_Data.m_pColumns[i].m_szWidth;
        InsertColumn(i, &lvcolumn);
    }

    SetExtendedStyle(LVS_EX_FULLROWSELECT);
    */

    return 0;
}

    /*
    static DWORD get_item_state (BYTE state[2])
    {
        DWORD retVal = 0;
        if (state[0] && !state[1])
        {
            retVal = 2 << (8 + 4);
        }
        else if (!state[0] && state[1])
        {
            retVal = 3 << (8 + 4);
        }
        else if (state[0] && state[1])
        {
            retVal = 4 << (8 + 4);
        }
        return retVal;
    }
    */

void CDbObjListCtrl::ExecuteQuery (const CString& strSchema, const CString& /*strFilter*/)
{
    try { EXCEPTION_FRAME;
        if (m_Data.m_MinVer > m_connect.GetVersion()) return;

        CWaitCursor wait;
        CAbortController abortCtrl(*GetAbortThread(), &m_connect);
        abortCtrl.SetActionText("Loading object list...");

        bool is7X = m_connect.GetVersion() <= esvServer73X ? true : false;
        OciCursor curObjects(m_connect, (is7X && m_Data.m_szSqlStatement73) ? m_Data.m_szSqlStatement73 : m_Data.m_szSqlStatement, 200);
        curObjects.SetDateFormat("%y.%m.%d %H:%M");
        curObjects.Bind(":p_owner", strSchema);
        curObjects.Execute();

        m_mapColumns.SetSize(m_Data.m_nColumns);

        for (int j(0); j < m_Data.m_nColumns; j++)
            m_mapColumns[j] = (DWORD)-1;

        m_nStatusColumn  = -1;
        m_nEnabledColumn = -1;

        int nFiels = curObjects.GetFieldCount();
        for (int i(0); i < nFiels; i++)
        {
            if (m_Data.m_szStatusColumn
            && !stricmp(m_Data.m_szStatusColumn, curObjects.GetFieldName(i).c_str()))
                m_nStatusColumn  = i;

            if (m_Data.m_szEnabledColumn
            && !stricmp(m_Data.m_szEnabledColumn, curObjects.GetFieldName(i).c_str()))
                m_nEnabledColumn = i;

            for (int j(0); j < m_Data.m_nColumns; j++)
                if (!stricmp(m_Data.m_pColumns[j].m_szName, curObjects.GetFieldName(i).c_str()))
                    m_mapColumns[j] = i;
        }

#ifdef _DEBUG
        for (j = 0; j < m_Data.m_nColumns; j++)
            ASSERT(m_mapColumns[j] != -1);
#endif//_DEBUG

        string buffer;
        m_arrObjects.Clear();
        m_arrObjects.Format(m_Data.m_nColumns, 2);
        
        for (i = 0; curObjects.Fetch(); i++)
        {
            int row = m_arrObjects.AddRow();
            for (j = 0; j < m_Data.m_nColumns; j++)
            {
                if (m_mapColumns[j] != -1)
                {
                    curObjects.GetString(m_mapColumns[j], buffer);
                    m_arrObjects.SetCell(row, j, buffer.c_str());
                }
            }
            BYTE bFlags[2] =
            {
                m_nStatusColumn  != -1 && curObjects.ToString(m_nStatusColumn)  == "INVALID"  ? 1 : 0,
                m_nEnabledColumn != -1 && curObjects.ToString(m_nEnabledColumn) == "DISABLED" ? 1 : 0,
            };
            m_arrObjects.SetRowData(row, bFlags, sizeof bFlags);
        }
    }
    _DEFAULT_HANDLER_
}

void CDbObjListCtrl::RefreshRow (int nItem, int nRow, const char* szOwner, const char* szName)
{
    try { EXCEPTION_FRAME;

        bool is7X = m_connect.GetVersion() <= esvServer73X ? true : false;
        string buffer = (is7X && m_Data.m_szSqlStatement73) ? m_Data.m_szSqlStatement73 : m_Data.m_szSqlStatement;
        buffer += m_Data.m_szRefreshWhere;

        OciCursor curObjects(m_connect, buffer.c_str(), 1);
        curObjects.SetDateFormat("%y.%m.%d %H:%M");
        curObjects.Bind(":p_owner", szOwner);
        curObjects.Bind(":p_name", szName);
        curObjects.Execute();

        if (curObjects.Fetch())
        {
            for (int j = 0; j < m_Data.m_nColumns; j++)
            {
                if (m_mapColumns[j] != -1)
                {
                    curObjects.GetString(m_mapColumns[j], buffer);
                    m_arrObjects.SetCell(nRow, j, buffer.c_str());
                }
            }
            BYTE bFlags[2] =
            {
                m_nStatusColumn  != -1 && curObjects.ToString(m_nStatusColumn)  == "INVALID"  ? 1 : 0,
                m_nEnabledColumn != -1 && curObjects.ToString(m_nEnabledColumn) == "DISABLED" ? 1 : 0,
            };
            m_arrObjects.SetRowData(nRow, bFlags, sizeof bFlags);
        }

    }
    _DEFAULT_HANDLER_

    RedrawItems(nItem, nItem);
    UpdateWindow();
}

void CDbObjListCtrl::RefreshList (bool bValid, bool bInvalid)
{
    m_bValid   = bValid;
    m_bInvalid = bInvalid;

    int nRows = m_arrObjects.GetRows();
    int iItem = 0;
    LV_ITEM lvitem;
    memset(&lvitem, 0, sizeof lvitem);
    CListCtrl::DeleteAllItems();

    for (int i = 0; i < nRows; i++)
    {
        BYTE* rowData = m_arrObjects.GetRowData(i);
        if (((rowData[0] ? false : true) && m_bValid)
        || ((rowData[0] ? true : false) && m_bInvalid))
            for (int j = 0; j < m_Data.m_nColumns; j++)
            {
                lvitem.mask      = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE|LVIF_STATE;
                lvitem.iImage    = m_Data.m_nImageId;
                lvitem.iItem     = (j == 0)? i : iItem;
                lvitem.iSubItem  = j;
                lvitem.stateMask = LVIS_STATEIMAGEMASK;
//              lvitem.state     = get_item_state(rowData);
                lvitem.pszText   = LPSTR_TEXTCALLBACK;
                lvitem.lParam    = i;

                if (!j)
                    iItem = CListCtrl::InsertItem(&lvitem);
                else
                    CListCtrl::SetItem(&lvitem);
            }
    }
    lvitem.mask      = LVIF_STATE;
    lvitem.stateMask = lvitem.state = LVIS_SELECTED|LVIS_FOCUSED;

    m_nDirection  = 1;
    m_nSortColumn = 0;
    SortItems(CompProc, (LPARAM)this);

    CListCtrl::SetItemState(CListCtrl::GetNextItem(-1, LVNI_ALL), &lvitem);
}

void CDbObjListCtrl::ApplyQuickFilter (bool valid, bool invalid) 
{
    Common::ListCtrlManager::FilterCollection filter;
    GetFilter(filter);

    if (filter.empty())
        return;

    if (m_nStatusColumn != -1)
    {
        const char* val = 0;

        if (valid && invalid)
            val = "";
        else if (valid)
            val = "VALID";
        else if  (invalid)
            val = "INVALID";
        else
            val = "NA";

        for (int j(0); j < m_Data.m_nColumns; j++)
            if (m_mapColumns[j] == m_nStatusColumn)
            {
                if (val != filter.at(j).value
                || Common::ListCtrlManager::EXACT_MATCH != filter.at(j).operation)
                {
                    filter.at(j).value = val;
                    filter.at(j).operation = Common::ListCtrlManager::EXACT_MATCH;
                    SetFilter(filter);
                }

                break;
            }
    }
}

void CDbObjListCtrl::Refresh (const CString& strSchema, const CString& strFilter,
                              bool bValid, bool bInvalid, bool bForce)
{
    bool bNeedsInit = false;

    if (m_adapter.getColCount() == 0)
        bNeedsInit = true;
    
    if (m_bDirty || bForce)
    {
        m_bValid = bValid;
        m_bInvalid = bInvalid;

        m_bDirty = false;
        ExecuteQuery(strSchema, strFilter);
        // RefreshList(bValid, bInvalid);
        if (bNeedsInit)
        {
            Init();
            SetStyle();
        }
        else
            CManagedListCtrl::Refresh(true);

        ApplyQuickFilter(m_bValid, m_bInvalid);
    }
    else if (m_bValid != bValid || m_bInvalid != bInvalid)
    {
        m_bValid = bValid;
        m_bInvalid = bInvalid;

        // RefreshList(bValid, bInvalid);
        if (bNeedsInit)
        {
            Init();
            SetStyle();
        }
        else
            CManagedListCtrl::Refresh(true);

        ApplyQuickFilter(m_bValid, m_bInvalid);
    }
}

#pragma message("   Important improvement: Sorting have column type dependent!")
    int CALLBACK CDbObjListCtrl::CompProc (LPARAM nRow1, LPARAM nRow2, LPARAM lparam)
    {
        CDbObjListCtrl* m_This = (CDbObjListCtrl*)lparam;

        const char* szStr1 = m_This->m_arrObjects.GetCellStr(nRow1, m_This->m_nSortColumn);
        const char* szStr2 = m_This->m_arrObjects.GetCellStr(nRow2, m_This->m_nSortColumn);

        return m_This->m_nDirection * strcmpi(szStr1, szStr2);
    }

void CDbObjListCtrl::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    CManagedListCtrl::OnLvnColumnclick(pNMHDR, pResult);
    /*
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if (m_nSortColumn != pNMListView->iSubItem)
        m_nDirection = 1;
    else
        m_nDirection *= -1;
    m_nSortColumn = pNMListView->iSubItem;

    SortItems(CompProc, (LPARAM)this);

    *pResult = 0;
    */
}

void CDbObjListCtrl::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    CManagedListCtrl::OnLvnGetdispinfo(pNMHDR, pResult);

    /*
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

//  strncpy(pDispInfo->item.pszText,
//          (char*)m_arrObjects.GetCellStr(pDispInfo->item.lParam, pDispInfo->item.iSubItem),
//          pDispInfo->item.cchTextMax);
    pDispInfo->item.pszText = (char*)m_arrObjects.GetCellStr(pDispInfo->item.lParam, pDispInfo->item.iSubItem);
    pDispInfo->item.state   = get_item_state(m_arrObjects.GetRowData(pDispInfo->item.lParam));
//  pDispInfo->item.mask = LVIF_TEXT | LVIF_STATE;

    *pResult = 0;
    */
}

void CDbObjListCtrl::OnContextMenu (CWnd*, CPoint point)
{
    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_LS_LIST_POPUP));
    CMenu* pPopup = menu.GetSubMenu(0);
    ASSERT(pPopup != NULL);
    pPopup->SetDefaultItem(IDC_DS_LOAD);
    pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CDbObjListCtrl::OnSelectAll()
{
    int i = 0;
    while (SetItemState(i++, LVIS_SELECTED, LVIS_SELECTED));
}

BOOL CDbObjListCtrl::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    BOOL retVal = CManagedListCtrl::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    if (!retVal)
    {
        CWnd* wndOwner = GetOwner();
        if (wndOwner)
            return wndOwner->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    }
    return retVal;
}

void CDbObjListCtrl::OnInitMenuPopup (CMenu* pPopupMenu, UINT /*nIndex*/, BOOL bSysMenu)
{
    if (!bSysMenu)
    {
        if (!pPopupMenu->SetDefaultItem(IDC_DS_LOAD))
            return;

        UINT flag;
        flag = (!m_Data.m_bCanCompile) ? MF_BYCOMMAND|MF_GRAYED : MF_BYCOMMAND|MF_ENABLED;
        pPopupMenu->EnableMenuItem(IDC_DS_COMPILE,  flag);

        flag = (!m_Data.m_bCanDisable) ? MF_BYCOMMAND|MF_GRAYED : MF_BYCOMMAND|MF_ENABLED;
        pPopupMenu->EnableMenuItem(IDC_DS_ENABLE,  flag);
        pPopupMenu->EnableMenuItem(IDC_DS_DISABLE, flag);

        flag = (!m_Data.m_bCanDrop) ? MF_BYCOMMAND|MF_GRAYED : MF_BYCOMMAND|MF_ENABLED;
        pPopupMenu->EnableMenuItem(IDC_DS_DELETE,  flag);

        DWORD style = LVS_TYPEMASK & GetWindowLong(m_hWnd, GWL_STYLE);
        pPopupMenu->CheckMenuItem(IDC_DS_AS_LIST,  (LVS_LIST == style) ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);
        pPopupMenu->CheckMenuItem(IDC_DS_AS_REPORT,(LVS_REPORT == style) ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);

        if (strcmp(m_Data.m_szType, "RECYCLEBIN") == 0)
        {
            pPopupMenu->EnableMenuItem(IDC_DS_LOAD, MF_BYCOMMAND|MF_GRAYED);
            pPopupMenu->EnableMenuItem(IDC_DS_LOAD_ALL_IN_ONE, MF_BYCOMMAND|MF_GRAYED);
            pPopupMenu->EnableMenuItem(ID_SQL_DESCRIBE, MF_BYCOMMAND|MF_GRAYED);
            pPopupMenu->AppendMenu(MF_SEPARATOR);
            pPopupMenu->AppendMenu(MF_STRING, ID_DS_FLASHBACK,       "&Flashback");
            pPopupMenu->AppendMenu(MF_STRING, ID_DS_PURGE_ALL,       "&Purge All");
        }

        // Ugly hook
        if (!strcmp(m_Data.m_szType, "VIEW"))
        {
            pPopupMenu->DeleteMenu(ID_DS_TRUNCATE, MF_BYCOMMAND);
        }
        else if (strcmp(m_Data.m_szType, "TABLE"))
        {
            pPopupMenu->DeleteMenu(0,    MF_BYCOMMAND);
            pPopupMenu->DeleteMenu(ID_DS_QUERY,    MF_BYCOMMAND);
            pPopupMenu->DeleteMenu(ID_DS_DELETE,   MF_BYCOMMAND);
            pPopupMenu->DeleteMenu(ID_DS_TRUNCATE, MF_BYCOMMAND);
        }
    }
}

void CDbObjListCtrl::OnLButtonDblClk (UINT /*nFlags*/, CPoint /*point*/)
{
    CWnd* wndOwner = GetOwner();
    if (wndOwner)
        wndOwner->SendMessage(WM_COMMAND, IDC_DS_LOAD);
}

string CDbObjListCtrl::GetListSelectionAsText()
{
    LV_ITEM lvi;
    memset(&lvi, 0, sizeof lvi);
    lvi.mask = LVIF_TEXT|LVIF_PARAM;
	const int NAME_MAX_SIZE = 128+1; // resized to 128+1 because db link name length can be up to 128 chars
    char szBuff[NAME_MAX_SIZE]; 
    lvi.pszText = szBuff;
    lvi.cchTextMax = sizeof szBuff;

	string s_theTextList;
	string s_delimiter = "";
    string s_schema = "";
	bool lower_items = GetSQLToolsSettings().m_bLowerNames;
    bool b_schema_name = GetSQLToolsSettings().m_bShemaName;

    if (b_schema_name)
        s_schema = string(((CDbSourceWnd *) GetOwner())->m_strSchema) + ".";

    if (lower_items)
        s_schema = string(CString(s_schema.c_str()).MakeLower());
    
    int index = -1;
    while ((index = GetNextItem(index, LVNI_SELECTED))!=-1) 
    {
        lvi.iItem      = index;
        lvi.iSubItem   = 0;
        lvi.pszText    = szBuff;
        lvi.cchTextMax = sizeof szBuff;
        if (!GetItem(&lvi)) {
            AfxMessageBox("Unknown error!");
            AfxThrowUserException();
        }

        s_theTextList += s_delimiter + s_schema + string(lower_items ? CString(lvi.pszText).MakeLower() : CString(lvi.pszText));
		s_delimiter = ", ";
    }

	return s_theTextList;
}

void CDbObjListCtrl::OnBeginDrag (NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	string s_theTextList;

	// const char* pszText = (char*)m_arrObjects.GetCellStr(GetItemData(pNMListView->iItem), pNMListView->iSubItem);

	s_theTextList = GetListSelectionAsText();

	Common::SimpleDragDataSource(s_theTextList.c_str()).DoDragDrop(DROPEFFECT_COPY);

    *pResult = 0;
}

LRESULT CDbObjListCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CManagedListCtrl::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
