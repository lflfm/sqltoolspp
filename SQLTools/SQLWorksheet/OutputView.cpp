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

/*
    22.03.2004 bug fix, Output: only the first line of multiline error/message is visible
*/

#include "stdafx.h"
#include "SQLTools.h"
#include "COMMON\AppGlobal.h"
#include "resource.h"
#include "OutputView.h"
#include "SQLWorksheetDoc.h"
#include <OpenGrid/GridSourceBase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;
    using namespace Common;

    CImageList COutputView::m_imageList;

IMPLEMENT_DYNCREATE(COutputView, StrGridView)

COutputView::COutputView()
{
    SetVisualAttributeSetName("Output Window");

    _ASSERTE(m_pStrSource);

    m_NoMoreError = false;
    m_pStrSource->SetCols(3);
    m_pStrSource->SetColCharWidth(0, 5);
    m_pStrSource->SetColCharWidth(1, 5);
    m_pStrSource->SetColCharWidth(2, 20);
    m_pStrSource->SetColHeader(0, "Line");
    m_pStrSource->SetColHeader(1, "Pos");
    m_pStrSource->SetColHeader(2, "Text");
    m_pStrSource->SetShowRowEnumeration(false);
    m_pStrSource->SetShowTransparentFixCol(true);

    m_pStrSource->SetFixSize(eoVert, edHorz, 3);

    if (!m_imageList.m_hImageList)
        m_imageList.Create(IDB_IMAGELIST2, 16, 64, RGB(0,255,0));

    m_pStrSource->SetImageList(&m_imageList, FALSE);

    m_pStrSource->SetMaxShowLines(6);
    m_pManager->m_Options.m_HorzLine = false;
    m_pManager->m_Options.m_VertLine = false;
    m_pManager->m_Options.m_RowSelect = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
}

COutputView::~COutputView()
{
}

BEGIN_MESSAGE_MAP(COutputView, StrGridView)
    //{{AFX_MSG_MAP(COutputView)
    ON_WM_CREATE()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditGroup)
    //}}AFX_MSG_MAP
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_CLEAR_ALL, OnEditClearAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR_ALL, OnUpdateEditGroup)
    ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutputView message handlers

int COutputView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (StrGridView::OnCreate(lpCreateStruct) == -1)
		return -1;

    m_pManager->Refresh();

	return 0;
}

void COutputView::PutLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId)
{
    m_NoMoreError = false;
    char szLine[18], szPos[18];
    if (line != -1) itoa(line+1, szLine, 10);
    else szLine[0] = 0;
    if (pos != -1) itoa(pos+1, szPos, 10);
    else szPos[0] = 0;

    int row = m_pStrSource->Append();
    m_pStrSource->SetString(row, 0, szLine);
    m_pStrSource->SetString(row, 1, szPos);
    m_pStrSource->SetString(row, 2, str);
    m_pStrSource->SetImageIndex(row, type);
    // 22.03.2004 bug fix, Output: only the first line of multiline error/message is visible
    m_pManager->OnChangedRows(row, row);
    
    m_lineIDs.append(lineId);

    _ASSERTE(m_pStrSource->GetCount(edVert) == static_cast<int>(m_lineIDs.size()));
}

void COutputView::Refresh ()
{
    m_pManager->MoveToHome(edVert);
    m_pManager->Refresh();
}

void COutputView::FirstError (bool onlyError)
{
    m_NoMoreError = true;
    NextError(onlyError);
}

bool COutputView::NextError (bool onlyError)
{
    if (m_NoMoreError)
    {
        m_NoMoreError = false;
        m_pManager->MoveToHome(edVert);
    }
    else
    {
        m_NoMoreError = !m_pManager->MoveToRight(edVert);
    }

    while (onlyError && !m_NoMoreError
    && m_pStrSource->GetImageIndex(m_pManager->GetCurrentPos(edVert)) != 4)
        m_NoMoreError = !m_pManager->MoveToRight(edVert);

    if (m_NoMoreError)
    {
        MessageBeep(MB_ICONHAND);
        Global::SetStatusText("No more errors!", FALSE);
    }
    else
    {
        if (m_pStrSource->GetCount(edVert))
            Global::SetStatusText(getString(2).c_str(), FALSE);
    }

    return !m_NoMoreError;
}


bool COutputView::PrevError (bool onlyError)
{
    if (m_NoMoreError)
    {
        m_NoMoreError = false;
        m_pManager->MoveToEnd(edVert);
    }
    else
    {
        m_NoMoreError = !m_pManager->MoveToLeft(edVert);
    }

    while (onlyError && !m_NoMoreError
    && m_pStrSource->GetImageIndex(m_pManager->GetCurrentPos(edVert)) != 4)
        m_NoMoreError = !m_pManager->MoveToLeft(edVert);

    if (m_NoMoreError)
    {
        MessageBeep(MB_ICONHAND);
        Global::SetStatusText("No more errors!", FALSE);
    }
    else
    {
        if (m_pStrSource->GetCount(edVert))
            Global::SetStatusText(getString(2).c_str(), FALSE);
    }

    return !m_NoMoreError;
}

bool COutputView::GetCurrError (int& line, int& pos, OpenEditor::LineId& lineId)
{
    bool retVal = false;

    if (m_pStrSource->GetCount(edVert))
    {
        const string& strLine = getString(0);
        if (strLine.size())
        {
            line = atoi(strLine.c_str()) - 1;
            pos  = atoi(getString(1).c_str()) - 1;
            lineId = m_lineIDs.at(m_pManager->GetCurrentPos(edVert));
            retVal = true;
        }
    }
    return retVal;
}

bool COutputView::IsEmpty () const
{
    return m_pStrSource->GetCount(edVert) ? false : true;
}

void COutputView::OnEditCopy()
{
    if (!IsEmpty() && OpenClipboard())
    {
        if (EmptyClipboard())
        {
            const string& text = getString(2);
            HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, text.length()+1);
            memcpy((char*)GlobalLock(hData), text.c_str(), text.length()+1);
            SetClipboardData(CF_TEXT, hData);
        }
        CloseClipboard();
    }
}

void COutputView::OnUpdateEditGroup(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!IsEmpty());
}

void COutputView::OnEditClearAll ()
{
    Clear();
}

void COutputView::Clear ()
{
    StrGridView::Clear();
    m_lineIDs.clear();
}

inline
const string& COutputView::getString (int col)
{
	return m_pStrSource->GetString(m_pManager->GetCurrentPos(edVert), col);
}


void COutputView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CDocument* doc = GetDocument();
    if (doc)
        doc->OnCmdMsg(ID_SQL_CURR_ERROR, 0, 0, 0);
    else 
        StrGridView::OnLButtonDblClk(nFlags, point);
}
