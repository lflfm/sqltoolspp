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

// 19.10.2002 bug fix, goto the last record does not fetch all rows
// 07.11.2003 bug fix, "Invalid vector<T> subscript" happens continually
// if a query returns error during fetching the first portion of records
// 12.10.2002 bug fix, grid column autofit has been brocken since build 38

#include "stdafx.h"
#include "resource.h"

#pragma message ("it should be replaced with grid own settings")
#include "SQLTools.h"

#include <COMMON\ExceptionHelper.h>
#include <OCI8\ACursor.h>
#include "GridManager.h"
#include "OCISource.h"
#include "OCIGridView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;

    class OciGridManager : public GridManager
    {
    public:
        OciGridManager (CWnd* client , OciGridSource* source)
        : GridManager(client, source, FALSE) {}

        OciGridSource* GetSource () { return static_cast<OciGridSource*>(m_pSource); }
        const OciGridSource* GetSource () const { return static_cast<const OciGridSource*>(m_pSource); }

        void fetch (eDirection dir, int rows)
        {
            OciGridSource* source = GetSource();

            if ((dir == edVert && source->IsTableOrientation())
            || (dir == edHorz && !source->IsTableOrientation()))
            {
                if (!source->AllRowsFetched())
                {
                    source->Fetch(rows);
                    RecalcRuller(dir, false/*adjust*/, true/*invalidate*/);
                    if (GetSQLToolsSettings().GetGridAutoResizeColWidthFetch())
                        if (! ((OciGridView *) m_pClientWnd)->GetKeepColSize())
                            ((OciGridView *) m_pClientWnd)->ApplyColumnFit();
                }
            }
        }

        virtual void AfterMove (eDirection dir)
        {
            if (dir == edVert
            && (m_Rulers[edVert].GetUnusedCount() 
                || m_Rulers[edVert].GetLastVisiblePos() == m_pSource->GetCount(edVert)))
                fetch(dir, GetPrefetch());
            GridManager::AfterMove(dir);
        }

        virtual void AfterScroll (eDirection dir)
        {
            if (dir == edVert
            && (m_Rulers[edVert].GetUnusedCount() 
                || m_Rulers[edVert].GetLastVisiblePos() == m_pSource->GetCount(edVert)))
                fetch(dir, GetPrefetch());
            GridManager::AfterScroll(dir);
        }

        virtual bool MoveToEnd (eDirection dir)
        {
            fetch(dir, INT_MAX);
            return GridManager::MoveToEnd(dir);
        }
        
        virtual bool ScrollToEnd (eDirection dir)
        {
            fetch(dir, INT_MAX);
            return GridManager::ScrollToEnd(dir);
        }

        static int GetPrefetch ()
        {
            int limit = GetSQLToolsSettings().GetGridPrefetchLimit();
            return limit == 0 ? INT_MAX : (limit < 60 ? 60 : limit);
        }
    };

/////////////////////////////////////////////////////////////////////////////
// GridView

IMPLEMENT_DYNCREATE(OciGridView, GridView)

OciGridView::OciGridView()
{
    SetVisualAttributeSetName("Data Grid Window");
    m_pOciSource = new OciGridSource;
    m_pManager = new OciGridManager(this, m_pOciSource);
    m_pManager->m_Options.m_Editing = true;
    m_pManager->m_Options.m_ExpandLastCol = true;
    m_pManager->m_Options.m_Rotation = true;
    m_lastRow = 0;
    m_bKeepColSize = false;
}

OciGridView::~OciGridView()
{
    try { EXCEPTION_FRAME;

        delete m_pManager;
        delete m_pOciSource;
    }
    _DESTRUCTOR_HANDLER_;
}

BEGIN_MESSAGE_MAP(OciGridView, GridView)
	//{{AFX_MSG_MAP(OciGridView)
	//}}AFX_MSG_MAP
    ON_WM_INITMENUPOPUP()
    ON_COMMAND_RANGE(ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, OnChangeColumnFit)
    ON_UPDATE_COMMAND_UI(ID_OCIGRID_COLS_TO_HEADERS, OnUpdate_OciGridColsToHeaders)
    ON_UPDATE_COMMAND_UI(ID_OCIGRID_DATA_FIT, OnUpdate_OciGridDataFit)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCIGRID, OnUpdate_OciGridIndicator)
    ON_COMMAND(ID_GRID_ROTATE, OnRotate)   
END_MESSAGE_MAP()

void OciGridView::Clear ()                           
{ 
    try { EXCEPTION_FRAME;

        m_pOciSource->Clear(); 
        m_pManager->Clear(); 
    }
    _DEFAULT_HANDLER_
}

void OciGridView::Refresh ()      
{
    try { EXCEPTION_FRAME;

        m_pManager->Refresh(); 
    }
    _DEFAULT_HANDLER_
}

int OciGridView::GetCurrentRecord () const
{
    return m_pManager->GetCurrentPos(edVert); 
}

int OciGridView::GetRecordCount () const 
{ 
    return m_pOciSource->GetCount(edVert); 
}

void OciGridView::SetCursor (std::auto_ptr<OCI8::AutoCursor>& cursor)
{
    const SQLToolsSettings& settings = GetSQLToolsSettings();
    nvector<std::string> headers("OciGridView::SetCursor::headers");
    nvector<int> itemSizes("OciGridView::SetCursor::itemSizes");

#pragma message ("more general way!")
    if (m_pOciSource->IsTableOrientation())
    if (!m_pOciSource->IsEmpty() && settings.GetGridAllowRememColWidth())
    {
        itemSizes = m_pManager->GetCacheItems(edHorz);

        for (int i = 0; i < m_pOciSource->GetCount(edHorz); i++)
            headers.push_back(m_pOciSource->GetColHeader(i));
    }

    m_pOciSource->Clear(); // it has to be called before m_pManager->Clear()
    m_pManager->Clear();
    
    // 10.10.2002 bug fix, a last column with right alignment looks ugly 
    //            if the colomn is expanded to the right window boundary
    if (cursor.get() && cursor->GetFieldCount())
        m_pManager->m_Options.m_ExpandLastCol 
            = cursor->IsNumberField(cursor->GetFieldCount()-1) ? false : true;

    m_pOciSource->SetCursor(cursor); 

    m_bKeepColSize = false;
    if (m_pOciSource->IsTableOrientation())
    if (m_pOciSource->GetCount(edHorz) == static_cast<int>(headers.size()))
    {
        m_bKeepColSize = true;
        for (int i = 0; i < m_pOciSource->GetCount(edHorz); i++)
            if (m_pOciSource->GetColHeader(i) != headers[i])
            {
                m_bKeepColSize = false;
                break;
            }
    }

    if (m_bKeepColSize)
        m_pManager->SetCacheItems(edHorz, itemSizes);

    // 07.11.2003 bug fix, "Invalid vector<T> subscript" happens continually
    // if a query returns error during fetching the first portion of records

    // the manager will in the valid state after the following Refresh
    // it's required because exception handling is possible in Fetch call
    // and that requires grid window repaintig.
    m_pManager->Refresh(); 

    m_pOciSource->Fetch(OciGridManager::GetPrefetch());
    m_pManager->MoveToHome(edVert);

    // 12.10.2002 bug fix, grid column autofit has been brocken since build 38

    ResetLastRow();
    if (!m_bKeepColSize)
        ApplyColumnFit();

#pragma message("\nCHECK: grid refresh\n")
    //m_pManager->Refresh(); 
}

void OciGridView::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    GridView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (!IsEmpty() && m_pOciSource->GetCount(edVert) > 0)
    {
        pPopupMenu->EnableMenuItem(ID_OCIGRID_DATA_FIT,        MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_OCIGRID_COLS_TO_HEADERS, MF_BYCOMMAND|MF_ENABLED);
    }

    pPopupMenu->CheckMenuRadioItem(
        ID_OCIGRID_DATA_FIT, ID_OCIGRID_COLS_TO_HEADERS, 
        (!GetSQLToolsSettings().GetGridColumnFitType()) ? ID_OCIGRID_DATA_FIT : ID_OCIGRID_COLS_TO_HEADERS,
        MF_BYCOMMAND);
}

void OciGridView::OnChangeColumnFit (UINT id)
{
    switch (id)
    {
    case ID_OCIGRID_DATA_FIT:           GetSQLToolsSettingsForUpdate().SetGridColumnFitType(0); break;
    case ID_OCIGRID_COLS_TO_HEADERS:    GetSQLToolsSettingsForUpdate().SetGridColumnFitType(1); break;
    }

    ResetLastRow();
    ApplyColumnFit();
}

void OciGridView::ApplyColumnFit ()
{
    if (!IsEmpty() 
    && m_pOciSource->IsTableOrientation()
    && m_pOciSource->GetCount(edVert) > 0)
    {
		{
			CClientDC dc(this);
			OnPrepareDC(&dc, 0);
			PaintGridManager::m_Dcc.m_Dc = &dc;

			bool byData     = GetSQLToolsSettings().GetGridColumnFitType() == 0;
			bool byHeader   = !byData || !GetSQLToolsSettings().GetGridAllowLessThanHeader();
			int maxColLen   = GetSQLToolsSettings().GetGridMaxColLength();
			int columnCount = m_pOciSource->GetCount(edHorz);
			int rowCount    = m_pOciSource->GetCount(edVert);

			PaintGridManager::m_Dcc.m_Type[edHorz] = efNone;

			for (int col = 0; col < columnCount; col++) 
			{
				int width = 0;

				PaintGridManager::m_Dcc.m_Col = col;

				if (byHeader)
				{
					PaintGridManager::m_Dcc.m_Row = 0;
					PaintGridManager::m_Dcc.m_Type[edVert] = efFirst;
					m_pOciSource->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
					width = PaintGridManager::m_Dcc.m_Rect.Width();
				}
				if (byData)
				{
					PaintGridManager::m_Dcc.m_Type[edVert] = efNone;
                    int row = m_lastRow;
                    if (row > 0)
                        width = m_pManager->m_Rulers[edHorz].GetPixelSize(col);

					for (; row < rowCount; row++)
					{
						PaintGridManager::m_Dcc.m_Row = row;
						m_pOciSource->CalculateCell(PaintGridManager::m_Dcc, maxColLen);
						width = max(width, PaintGridManager::m_Dcc.m_Rect.Width());
					}
				}
				m_pManager->m_Rulers[edHorz].SetPixelSize(col, width);
			}

            m_lastRow = rowCount;
		}
        m_pManager->LightRefresh(edHorz);
	}
}

void OciGridView::OnUpdate_OciGridColsToHeaders(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!IsEmpty() && m_pOciSource->GetCount(edVert) > 0 && m_pOciSource->IsTableOrientation());
    pCmdUI->SetRadio(GetSQLToolsSettings().GetGridColumnFitType());
}

void OciGridView::OnUpdate_OciGridDataFit(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!IsEmpty() && m_pOciSource->GetCount(edVert) > 0 && m_pOciSource->IsTableOrientation());
    pCmdUI->SetRadio(!GetSQLToolsSettings().GetGridColumnFitType());
}

void OciGridView::OnRotate ()
{
    GridView::OnRotate();
    if (m_pOciSource->IsTableOrientation())
    {
        ResetLastRow();
        ApplyColumnFit();
    }
}

void OciGridView::OnUpdate_OciGridIndicator (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);

    StatusCxt statusCxt(
        m_pManager->GetCurrentPos(edVert) + 1, 
        m_pOciSource->GetCount(edVert), 
        m_pOciSource->AllRowsFetched());
    
    if (m_statusCxt != statusCxt)
    {
        m_statusCxt = statusCxt;

        if (m_statusCxt.m_numOfRec) {
            m_statusText.Format("Record %d of %d", m_statusCxt.m_curRec, m_statusCxt.m_numOfRec);

            if (!m_statusCxt.m_allRowsFetched)
                m_statusText += ", last not fetched.";
        }
        else
            m_statusText = "No records";
    }

    pCmdUI->SetText(m_statusText);
}
