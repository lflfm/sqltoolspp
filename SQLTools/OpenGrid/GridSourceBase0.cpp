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

/***************************************************************************/
/*      Purpose: Text data source implementation for grid component        */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/
// 16.12.2004 (Ken Clubok) Add CSV prefix option

#include "stdafx.h"
#include <sstream>
#include <COMMON\SimpleDragDataSource.h>         // for Drag & Drop
#include "GridSourceBase.h"
#include "CellEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{
    ETextFormat GridStringSource::m_OutputFormat = etfPlainText;
    char GridStringSource::m_FieldDelimiterChar = ',';
    char GridStringSource::m_QuoteChar          = '"';
    char GridStringSource::m_QuoteEscapeChar    = '"';
	char GridStringSource::m_PrefixChar         = '=';
    bool GridStringSource::m_OutputWithHeader   = false;
    bool GridStringSource::m_ColumnNameAsAttr   = false;

GridSourceBase::GridSourceBase ()
: m_ColumnInfo("GridSourceBase::m_ColumnInfo")
{
    SetFixSize(eoVert, edHorz, 6);
    SetFixSize(eoVert, edVert, 1);
    SetFixSize(eoHorz, edHorz, 30);
    SetFixSize(eoHorz, edVert, 1);
    m_Editor = 0;
    m_ShowFixCol = m_ShowFixRow = 
    m_ShowRowEnumeration = true;
    m_ShowTransparentFixCol = false;
    m_MaxShowLines = 1;
    m_ImageList = 0;
    m_bDeleteImageList = false;
    m_Orientation = eoVert;//eoHorz; 
}

GridSourceBase::~GridSourceBase ()
{
    try { EXCEPTION_FRAME;

    if (m_bDeleteImageList)
        delete m_ImageList;
    }
    _DESTRUCTOR_HANDLER_;
}

// Cell management
int GridSourceBase::GetFirstFixCount (eDirection dir) const
{
    return (dir == edHorz && m_ShowFixCol || dir == edVert && m_ShowFixRow) ? 1 : 0;
}

int GridSourceBase::GetLastFixCount (eDirection) const
{
    return 0;
}

int GridSourceBase::GetSize (int pos, eDirection dir) const
{
    if (IsTableOrientation() && dir == edHorz)
        return GetColCharWidth(pos);
    else if (IsFormOrientation() && dir == edVert)
        return GetColCharHeight(pos);

    return 1;
}

int GridSourceBase::GetFirstFixSize (int pos, eDirection dir) const
{
    _ASSERTE(!pos);
    return GetFixSize(m_Orientation, dir);
}

int GridSourceBase::GetLastFixSize (int, eDirection) const
{
    _ASSERTE(0);
    return 1;
}

void GridSourceBase::PaintCell (DrawCellContexts& dcc) const
{
    CPen* oldPen;
    CRect _rc = dcc.m_Rect;
    _rc.InflateRect(dcc.m_CellMargin);
    int bkClrIndex = IsTableOrientation() ? dcc.m_Row & 1 : dcc.m_Col & 1;

    if (dcc.IsDataCell()) 
    {
        if (IsEmpty() || dcc.m_Select == esNone) 
        {
            dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_BkBrush[bkClrIndex]);
            dcc.m_Dc->SetBkColor(dcc.m_BkColor[bkClrIndex]);
            dcc.m_Dc->SetTextColor(dcc.m_TextColor);
        } 
        else if (dcc.m_Select == esRect) 
        {
            dcc.m_Dc->SetBkColor(dcc.m_HightLightColor);
            dcc.m_Dc->SetTextColor(dcc.m_HightLightTextColor);
            dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_HightLightBrush);
        } 
        else if (dcc.m_Select == esBorder) 
        {
            oldPen = dcc.m_Dc->SelectObject(&dcc.m_HightLightPen);
            dcc.m_Dc->Rectangle(dcc.m_Rect);
            dcc.m_Dc->SelectObject(oldPen);
        }
        if (!IsEmpty()) 
        {
            //_ASSERTE(GetCount(edVert));
            if (GetCount(edVert)) 
            {
                const string& text = GetCellStr(dcc.m_Row, dcc.m_Col);
                dcc.m_Dc->DrawText(text.data(), text.length(), _rc, 
                    GetColAlignment((IsTableOrientation() ? dcc.m_Col : dcc.m_Row)) == ecaLeft 
                    ? DT_LEFT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX
                    : DT_RIGHT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX);
            }
        }
    } else {
        dcc.m_Dc->SetBkColor(dcc.m_FixBkColor);
        dcc.m_Dc->SetTextColor(dcc.m_FixTextColor);
        if (IsEmpty()) 
        {
            DrawDecFrame(dcc);
        } 
        else 
        {
            if (dcc.m_Type[IsTableOrientation() ? 0 : 1] == efNone && dcc.m_Type[IsTableOrientation() ? 1 : 0] == efFirst) 
            {
                DrawDecFrame(dcc);
                const string& text = GetColHeader(IsTableOrientation() ? dcc.m_Col : dcc.m_Row);
                dcc.m_Dc->DrawText(text.data(), text.length(), _rc, DT_LEFT|DT_VCENTER|DT_NOPREFIX);
            } 
            else if (dcc.m_Type[IsTableOrientation() ? 1 : 0] == efNone && dcc.m_Type[IsTableOrientation() ? 0 : 1] == efFirst) 
            {
                if (!m_ShowTransparentFixCol)
                    DrawDecFrame(dcc);
                else
                    dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_BkBrush[bkClrIndex]);
                if (m_ShowRowEnumeration) 
                {
                    char buffer[18];
                    dcc.m_Dc->DrawText(itoa(IsTableOrientation() ? dcc.m_Row + 1 : dcc.m_Col + 1, buffer, 10), -1, _rc, DT_RIGHT|DT_VCENTER|DT_NOPREFIX);
                }
                if (m_ImageList) 
                {
                    int nIndex = GetImageIndex(dcc.m_Row);
                    if (nIndex != -1)
                        m_ImageList->Draw(dcc.m_Dc, nIndex, _rc.TopLeft(), ILD_TRANSPARENT);
                } 
            } 
            else 
            {
                DrawDecFrame(dcc);
            }
        }
    }
}

void GridSourceBase::CalculateCell (DrawCellContexts& dcc, size_t maxTextLength) const
{
    dcc.m_Rect.SetRectEmpty();

    if (!IsEmpty()) 
    {
        if (dcc.IsDataCell())
        {
            _ASSERTE(GetCount(edVert));
            if (GetCount(edVert)) 
            {
                const string& text = GetCellStr(dcc.m_Row, dcc.m_Col);
                    
                dcc.m_Dc->DrawText(text.data(), min(text.length(), maxTextLength), dcc.m_Rect, DT_CALCRECT|DT_EXPANDTABS|DT_NOPREFIX);

                dcc.m_Rect.InflateRect(-dcc.m_CellMargin.cx, -dcc.m_CellMargin.cy);
            }
        }
        else
        {
            if (dcc.m_Type[IsTableOrientation() ? 0 : 1] == efNone 
            && dcc.m_Type[IsTableOrientation() ? 1 : 0] == efFirst) 
            {
                const string& text = GetColHeader(IsTableOrientation() ? dcc.m_Col : dcc.m_Row);

                dcc.m_Dc->DrawText(text.data(), min(text.length(), maxTextLength), dcc.m_Rect, DT_CALCRECT|DT_NOPREFIX);

                dcc.m_Rect.InflateRect(-dcc.m_CellMargin.cx, -dcc.m_CellMargin.cy);
            } 
        }
    }
}

void GridSourceBase::DrawDecFrame (DrawCellContexts& dcc) const
{
    dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_FixBrush);

    CPen lgtPen(PS_SOLID, 1, dcc.m_LightColor);
    CPen* oldPen = dcc.m_Dc->SelectObject(&lgtPen);

    dcc.m_Dc->MoveTo(dcc.m_Rect.left, dcc.m_Rect.bottom - 1);
    dcc.m_Dc->LineTo(dcc.m_Rect.left, dcc.m_Rect.top);
    dcc.m_Dc->LineTo(dcc.m_Rect.right - 1, dcc.m_Rect.top);

    CPen shdPen(PS_SOLID, 1, dcc.m_FixShdColor);
    dcc.m_Dc->SelectObject(&shdPen);

    dcc.m_Dc->MoveTo(dcc.m_Rect.left, dcc.m_Rect.bottom - 1);
    dcc.m_Dc->LineTo(dcc.m_Rect.right - 1, dcc.m_Rect.bottom - 1);
    dcc.m_Dc->LineTo(dcc.m_Rect.right - 1, dcc.m_Rect.top);

    dcc.m_Dc->SelectObject(*oldPen);
}

CWnd* GridSourceBase::BeginEdit (int row, int col, CWnd* parent, const CRect& rect)  // get editor for grid window
{
    _ASSERTE(!m_Editor && !IsEmpty());
    
    const string& text = GetCellStr(row, col);
    
    if (!text.empty()) 
    {
        std::istringstream istr(text);
        std::ostringstream ostr;
        std::string buff;
        
        int nlines = 0;
        while (std::getline(istr, buff, '\n')) 
        {
            if (nlines++)
                ostr << '\r' << '\n';
            ostr << buff;
        }
        ostr << std::ends;

        DWORD dwStyle = WS_VISIBLE|WS_CHILD|
                        ES_AUTOHSCROLL|ES_AUTOVSCROLL|
                        ES_NOHIDESEL|ES_READONLY;
        if (nlines > 1)
            dwStyle |= ES_MULTILINE|WS_VSCROLL|WS_HSCROLL;

        m_Editor = new CellEditor;
        m_Editor->Create(dwStyle, rect, parent, 999);
        m_Editor->SetWindowText(ostr.str().data());

        if (nlines <= 1)
            m_Editor->SetSel(0, ostr.str().length());
    }
    return m_Editor;
}

void GridSourceBase::PostEdit () // event for commit edit result &
{
    _ASSERTE(!IsEmpty());
    m_Editor->DestroyWindow();
    delete m_Editor;
    m_Editor = 0;
}

void GridSourceBase::CancelEdit () // end edit session
{
    _ASSERTE(!IsEmpty());
    m_Editor->DestroyWindow();
    delete m_Editor;
    m_Editor = 0;
}

// clipboard operation support
// need Open & Close
void GridSourceBase::DoEditCopy (int row, int col, ECopyWhat what)
{
    if (EmptyClipboard()) 
    {
        std::string buff;
        CopyToString(buff, row, col, what);
        if (!buff.empty()) 
        {
            HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, buff.length()+1);
            char* dest = (char*)GlobalLock(hData);
            if (dest) memcpy(dest, buff.c_str(), buff.length());
            SetClipboardData(CF_TEXT, hData);
        }
    }
}

void GridSourceBase::CopyColumnNames (std::string& buff) const
{
    int nCols = m_ColumnInfo.size();
    for (int i(0); i < nCols; i++) 
    {
        buff += GetColHeader(i);
        if (i != nCols - 1)
            buff += ", ";
    }
}

void GridSourceBase::CopyToString (std::string& buff, int row, int col, ECopyWhat what)
{
    if (!IsEmpty())
    {
        switch (what) 
        {
        case ecDataCell:
            buff = GetCellStr(row, col);
            break;
        case ecFieldNames:
        case ecDragTopLeftCorner:
            CopyColumnNames(buff);
            break;
        case ecColumnHeader: 
        case ecDragColumnHeader: 
            buff = GetColHeader(col);
            break;
        case ecRowHeader:
            {
                std::stringstream of;
                of << row;
                buff = of.str();
            }
            break;
        case ecColumn:
        case ecRow:
        case ecDragRowHeader:
        case ecEverything:
            switch (what)
            {
            case ecColumn: 
                row = -1;
                break;
            case ecRow:
            case ecDragRowHeader:
                col = -1;
                break;
            case ecEverything:
                row = col = -1;
            }
            std::stringstream of;
            ExportText(of, row, col);
            buff = of.str();
            break;
        }
    }
}

void GridSourceBase::DoEditPaste ()
{
    // Not implement
}

void GridSourceBase::DoEditCut ()
{
    // Not implement
}

BOOL GridSourceBase::DoDragDrop (int row, int col, ECopyWhat what)
{
    if (!IsEmpty()) 
    {
        std::string buff;
        CopyToString(buff, row, col, what);
        if (!buff.empty()) 
        {
            Common::SimpleDragDataSource(buff.c_str()).DoDragDrop(DROPEFFECT_COPY/*|DROPEFFECT_MOVE*/);
            return TRUE;
        }
    }
    return FALSE;
}

DROPEFFECT GridSourceBase::DoDragOver (COleDataObject*, DWORD)
{
    // Not implement
    return DROPEFFECT_NONE;
}

BOOL GridSourceBase::DoDrop (COleDataObject*, DROPEFFECT)
{
    // Not implement
    return FALSE;
}

//const char* GridSourceBase::GetTooltipString (TooltipCellContexts& tcc) const
//{
//    tcc.m_CellRecall = false;
//    if (tcc.IsDataCell()) 
//    {
//        return GetCellStr(tcc.m_Row, tcc.m_Col);
//    } 
//    else if (tcc.m_Type[0] == efNone && tcc.m_Type[1] == efFirst) 
//    {
//        return GetColHeader(tcc.m_Col).c_str();
//    } 
//    else if (tcc.m_Type[1] == efNone && tcc.m_Type[0] == efFirst) 
//    {
//        char buffer[18];
//        return itoa(tcc.m_Row + 1, buffer, 10);
//    }
//    return "";
//}

void GridSourceBase::SetCols (int cols)
{
    m_ColumnInfo.clear();
    m_ColumnInfo.resize(cols, ColumnInfo());
    SetEmpty(false);
}

void GridSourceBase::SetImageList (CImageList* imageList, BOOL deleteImageList)
{
    if (m_bDeleteImageList) 
        delete m_ImageList;

    m_ImageList = imageList;
    m_bDeleteImageList = deleteImageList;
}

////////////////////////////////////////////////////////////////////////

int GridStringSource::GetCount (eDirection dir) const
{
    return /*(IsEmpty()) ? 1 :*/ ((IsTableOrientation() ? dir == edHorz : dir == edVert) ? m_table.GetColumnCount() : m_table.GetRowCount());
}

void GridStringSource::SetCols (int cols)
{
    GridSourceBase::SetCols(cols);
    m_table.Format(cols);
}

void GridStringSource::Clear ()
{
    m_table.Clear();
    //SetCols(1);
    //Append();
    SetEmpty(true);
}

int GridStringSource::Append ()                                                      
{ 
    int line = m_table.Append();
    Notify_ChangedRowsQuantity(line, 1);
    return line;
}

int GridStringSource::Insert (int row)                                               
{ 
    m_table.Insert(row); 
    Notify_ChangedRowsQuantity(row, 1);
    return row;
}

void GridStringSource::Delete (int row)                                               
{ 
    ASSERT(m_table.GetRowCount() > 0);
    if (m_table.GetRowCount() > 0) 
    {
        m_table.Delete(row); 
        Notify_ChangedRowsQuantity(row, -1);
    }
}

void GridStringSource::DeleteAll ()                                                   
{ 
    if (int count = m_table.GetRowCount())
    {
        m_table.DeleteAll(); 
	    Notify_ChangedRowsQuantity(0, -count);
    }
}

int GridStringSource::GetSize (int pos, eDirection dir) const
{
    if (IsTableOrientation())
    {
        if (dir == edVert)
        {
            if (GetMaxShowLines() > 1) 
                return m_table.GetMaxLinesInRow(pos, GetMaxShowLines());
            return 1;
        }
    }
    else // IsFormOrientation()
    {
        if (dir == edVert)
        {
            if (GetMaxShowLines() > 1)
                return m_table.GetMaxLinesInColumn(pos, GetMaxShowLines());
            return 1;
        }
        return 100;
    }

    return GridSourceBase::GetSize(pos, dir);
}

void GridStringSource::CopyToString (std::string& buff, int row, int col, ECopyWhat what)
{
    if (!IsTableOrientation())
    {
        switch (what)
        {
        case ecDragRowHeader:   
        case ecRowHeader:       
            what = ecColumnHeader; 
            col = row; 
            row = -1; 
            break;
        case ecDragColumnHeader:
            what = ecColumn; 
            break;
        case ecColumnHeader:
            std::ostringstream of;
            of << col + 1;
            buff = of.str();
            return;
        }
    }
    GridSourceBase::CopyToString(buff, row, col, what);
}

void GridStringSource::GetMaxColLenght (nvector<size_t>& maxLengths) const
{
    if (IsTableOrientation())
    {
        m_table.GetMaxColumnLength(maxLengths);

        if (m_OutputWithHeader)
        {
            _ASSERTE(maxLengths.size() == m_ColumnInfo.size());

            for (int i = 0; i < static_cast<int>(maxLengths.size()); i++)
                if (maxLengths[i] < GetColHeader(i).size())
                    maxLengths[i] = GetColHeader(i).size();
        }
    }
    else
    {
        m_table.GetMaxRowLength(maxLengths);
    }
}

const string& GridStringSource::GetCellStrForExport (int line, int col) const
{
    return GetCellStr(line, col);
}

}//namespace OG2

