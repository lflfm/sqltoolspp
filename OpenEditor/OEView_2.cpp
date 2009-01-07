/* 
    Copyright (C) 2002-2005 Aleksey Kochetov

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
    28.03.2003 improvement, optimization of GDI resources usage - it's a very critical issue for Win95/Win98
    29.06.2003 bug fix, selected text foreground color cannot be changed for "Text" class
    11.10.2004 improvement, Ruler for columns has been added
    11.10.2004 improvement, Columnar markers has been added (can be defined in class property page)
    16.04.2005 improvement, optimization of DrawHorzRuler/DrawVertRuler
*/

#include "stdafx.h"
#include "MemDC.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/OEHighlighter.h"
#include "SQLTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OpenEditor;

/////////////////////////////////////////////////////////////////////////////
// COEditorView drawing

    // arguments should be normalized
    // it's necessary to decrement blk.end.column
    inline
    bool _is_block_visible (const Square& scr, const Square& blk, EBlockMode mode)
    {
        switch (mode)
        {
        case ebtColumn:
            return intersect_not_null(scr, blk);
        case ebtStream:
            if (blk.start.line == blk.end.line) return intersect_not_null(scr, blk);// 1-line block
        }
        return scr.line_in_rect(blk.start.line) || blk.line_in_rect(scr.start.line);
    }

void COEditorView::DrawLineNumber (CDC& dc, RECT rc, int line)
{
    rc.right -= (m_Rulers[0].m_CharSize*3)/4;

    char buff[40];
    itoa(line + 1, buff, 10);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
    
    CFont* pOrgFont = m_paintAccessories->SelectFont(dc, 0);
    dc.DrawText(buff, strlen(buff), &rc, DT_RIGHT|DT_TOP);
    dc.SelectObject(pOrgFont);
}

void COEditorView::DrawBookmark (CDC& dc, RECT rc)
{
    CBrush* pOrgBrush = dc.SelectObject(&m_paintAccessories->m_BmkBrush);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BmkPen);
    dc.RoundRect(&rc, CPoint(5, 5));
    dc.SelectObject(pOrgBrush);
    dc.SelectObject(pOrgPen);
}

void COEditorView::DrawRandomBookmark (CDC& dc, RECT rc, int index, bool halfclip)
{
    if (halfclip)
    {
        rc.left = m_Rulers[0].m_Indent / 2;
        dc.IntersectClipRect(&rc);
        rc.left = 0;
    }

    CBrush* pOrgBrush = dc.SelectObject(&m_paintAccessories->m_RndBmkBrush);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_RndBmkPen);
    dc.RoundRect(&rc, CPoint(5, 5));
    dc.SelectObject(pOrgBrush);
    dc.SelectObject(pOrgPen);

    if (halfclip)
        dc.SelectClipRgn(NULL);

    char buff[40];
    itoa(index, buff, 10);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(m_paintAccessories->m_RndBmkForeground);

    CFont* pOrgFont = m_paintAccessories->SelectFont(dc, GetSettings().GetLineNumbers() ? HighlighterBase::efmBold : HighlighterBase::efmNormal);
    dc.DrawText(buff, strlen(buff), &rc, DT_CENTER|DT_TOP);
    dc.SelectObject(pOrgFont);
}

void COEditorView::DrawHorzRuler (CDC& realDc, const CRect& rc, int start, int end)
{
    CMemDC dc(&realDc, &rc);
    ::FillRect(dc, &rc, GetSysColorBrush(COLOR_BTNFACE));

    COLORREF bkColor = dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
    COLORREF txtColor = dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
    CFont* pOrgFont = dc.SelectObject(&m_paintAccessories->m_RulerFont);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BmkPen);

    dc.MoveTo(rc.left, rc.bottom-1);
    dc.LineTo(rc.right, rc.bottom-1);

    for (int i = start; i <= end; i++)
    {
        int column = m_Rulers[0].m_Topmost + i;

        int x = m_Rulers[0].m_Indent + m_Rulers[0].m_CharSize * i;

        int hight = (!(column % 5)) ? 4 : 2;

        dc.MoveTo(x, rc.bottom-1-hight);
        dc.LineTo(x, rc.bottom-1);

        if (!(column % 10))
        {
            char buffer[80];
            itoa(column/*+1*/, buffer, 10);
            int buflen = strlen(buffer);
            int width = buflen * m_paintAccessories->m_RulerCharSize.cx;
            dc.DrawText(buffer, buflen, 
                CRect(x - width, 0, x + width, m_paintAccessories->m_RulerCharSize.cy), 
                DT_CENTER|DT_TOP);
        }
    }

    dc.SetBkColor(bkColor);
    dc.SetTextColor(txtColor);
    dc.SelectObject(pOrgFont);
    dc.SelectObject(pOrgPen);
}

void COEditorView::DrawVertRuler (CDC& realDc, const CRect& rc, int start, int end, bool lineNumbers)
{
    CMemDC dc(&realDc, &rc);
    COLORREF bkColor = dc.SetBkColor(RGB(255,255,255));
    COLORREF txtColor = dc.SetTextColor(GetSysColor(COLOR_BTNFACE));
    dc.FillRect(&rc, dc.GetHalftoneBrush());
    dc.SetBkColor(bkColor);
    dc.SetTextColor(txtColor);

    RECT cell;
    cell.left   = 0;
    cell.right  = m_Rulers[0].m_Indent - 1;
    int lim = min(GetLineCount() - m_Rulers[1].m_Topmost, end);

    for (int i = start; i < lim; i++)
    {
        int line = m_Rulers[1].m_Topmost + i;
        cell.top    = m_Rulers[1].m_Indent + m_Rulers[1].m_CharSize * i;
        cell.bottom = cell.top + m_Rulers[1].m_CharSize;

        RandomBookmark bookmarkId;
        bool isBmkLine = IsBookmarked(line, eBmkGroup1);
        bool isRandomBmkLine = IsRandomBookmarked(line, bookmarkId);

        if (isRandomBmkLine)
        {
            if (isBmkLine) DrawBookmark(dc, cell);
            DrawRandomBookmark(dc, cell, bookmarkId.GetId(), isBmkLine);
        }
        else
        {
            if (isBmkLine) DrawBookmark(dc, cell);
            if (lineNumbers) DrawLineNumber(dc, cell, line);
        }
    }
}

void COEditorView::OnPaint ()
{
    int maxLineLength = 0;

	CPaintDC dc(this);

    if (dc.m_ps.rcPaint.left == dc.m_ps.rcPaint.right
    || dc.m_ps.rcPaint.top == dc.m_ps.rcPaint.bottom)
        return;

    if (!m_bAttached)
    {
        ::FillRect(dc, &dc.m_ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));
        return;
    }

    const Settings& settings = GetSettings();

    Square scr;
    scr.start.line   = max(0, (int)(dc.m_ps.rcPaint.top - m_Rulers[1].m_Indent) / m_Rulers[1].m_CharSize);
    scr.end.line     = max(0, (int)(dc.m_ps.rcPaint.bottom - m_Rulers[1].m_Indent - 1) / m_Rulers[1].m_CharSize);
    scr.start.column = max(0, (int)(dc.m_ps.rcPaint.left - m_Rulers[0].m_Indent) / m_Rulers[0].m_CharSize);
    scr.end.column   = max(0, (int)(dc.m_ps.rcPaint.right - m_Rulers[0].m_Indent - 1) / m_Rulers[0].m_CharSize);

    if (dc.m_ps.rcPaint.left < m_Rulers[0].m_Indent)
    {
        CRect rc(dc.m_ps.rcPaint);
        rc.right = m_Rulers[0].m_Indent;
        rc.top   = m_Rulers[1].m_Indent;
        DrawVertRuler(dc, rc, scr.start.line, scr.end.line+1, settings.GetLineNumbers());
    }

    if (dc.m_ps.rcPaint.top < m_Rulers[1].m_Indent)
    {
        CRect rc(dc.m_ps.rcPaint);
        rc.bottom = m_Rulers[1].m_Indent;
        DrawHorzRuler(dc, rc, scr.start.column, scr.end.column+1);
    }

    if (dc.m_ps.rcPaint.right >= m_Rulers[0].m_Indent
    && dc.m_ps.rcPaint.bottom >= m_Rulers[1].m_Indent)
    {
        Square blk;
        GetSelection(blk);
        bool blockExist    = !blk.is_empty();
        EBlockMode blkMode = GetBlockMode();

        if (blockExist)
        {
            blk.normalize();

            blk.start.line   -= m_Rulers[1].m_Topmost;
            blk.end.line     -= m_Rulers[1].m_Topmost;
            blk.start.column -= m_Rulers[0].m_Topmost;
            blk.end.column   -= m_Rulers[0].m_Topmost;

            if (blkMode == ebtColumn && blk.start.column > blk.end.column)
                std::swap(blk.start.column, blk.end.column);
        }

        blockExist = blockExist && _is_block_visible(scr, blk, blkMode);

        HighlighterBase* phighlighter = m_bSyntaxHighlight ? m_highlighter.get() : 0;

        LineTokenizer tokenizer(
            settings.GetVisibleSpaces(),
            GetTabSpacing(), 
            phighlighter ? phighlighter->GetDelimiters() : GetDelimiters()
            );

        if (phighlighter)
        {
            int status(0), quoteId(0);
            bool parsing(false);
            ScanMultilineQuotes(scr.start.line + m_Rulers[1].m_Topmost, status, quoteId, parsing);
            phighlighter->SetMultilineQuotesState(status, quoteId, parsing);
        }

        RECT rcLine, rcBar;
        rcLine.top    = rcBar.top    = 0;
        rcLine.bottom = rcBar.bottom = m_Rulers[1].m_CharSize;
        rcLine.left   = max(dc.m_ps.rcPaint.left,  (long)m_Rulers[0].m_Indent);
        rcLine.right  = max(dc.m_ps.rcPaint.right, (long)m_Rulers[0].m_Indent);

        CDC memDc, memSelDc;
        CBitmap bitmap, selBitmap;
        CBitmap *pOrgBitmap = 0, *pOrgSelBitmap = 0;

        memDc.CreateCompatibleDC(&dc);
        bitmap.CreateCompatibleBitmap(&dc, rcLine.right - rcLine.left, m_Rulers[1].m_CharSize);
        pOrgBitmap = memDc.SelectObject(&bitmap);
        memDc.SetBkMode(TRANSPARENT);
        memDc.SetViewportOrg(-rcLine.left, 0);

        CPen* pOrgdMemDcPen = memDc.SelectObject(&m_paintAccessories->m_RulerPen);
        memDc.SetROP2(R2_NOTXORPEN);

        bool columnMarkers = settings.GetColumnMarkers();
        const vector<int>& columnMarkersSet = settings.GetColumnMarkersSet();
        bool indentGuide = settings.GetIndentGuide();
        int indentSpacing = GetIndentSpacing();

        if (blockExist)
        {
            memSelDc.CreateCompatibleDC(&dc);
            selBitmap.CreateCompatibleBitmap(&dc, rcLine.right - rcLine.left, m_Rulers[1].m_CharSize);
            pOrgSelBitmap = memSelDc.SelectObject(&selBitmap);
            memSelDc.SetBkMode(TRANSPARENT);
            memSelDc.SetTextColor(m_paintAccessories->m_SelTextForeground);
            memSelDc.SetViewportOrg(-rcLine.left, 0);
        }

        if (!phighlighter)
        {
            m_paintAccessories->SelectTextFont(memDc);
            memDc.SetTextColor(m_paintAccessories->m_TextForeground);

            if (blockExist)
            {
                m_paintAccessories->SelectTextFont(memSelDc);
                // 29.06.2003 bug fix, selected text foreground color cannot be changed for "Text" class
                memSelDc.SetTextColor(m_paintAccessories->m_SelTextForeground);
            }
        }

        int endFileLine = GetLineCount() - m_Rulers[1].m_Topmost;
        int curLine = GetPosition().line -  m_Rulers[1].m_Topmost;

        LanguageSupport::Match match;
        bool bIsBrace = false;
        
        if (GetSQLToolsSettings().GetEnhancedVisuals())
        {
            bIsBrace = IsCurPosBrace(match);
        }

        for (int i(scr.start.line); i <= scr.end.line; i++)
        {
            int firstWordPos = -1;

            memDc.FillRect(&rcLine, (i == curLine) ? &m_paintAccessories->m_CurrentLineBrush : &m_paintAccessories->m_TextBrush);

            if (blockExist && blk.line_in_rect(i))
            {
                switch (blkMode)
                {
                case ebtColumn:
                    rcBar.left   = m_Rulers[0].InxToPix(blk.start.column);
                    rcBar.right  = m_Rulers[0].InxToPix(blk.end.column);
                    break;
                case ebtStream:
                default: // because of warning
                    if (blk.start.line == i)
                        rcBar.left = m_Rulers[0].InxToPix(blk.start.column);
                    else
                        rcBar.left = rcLine.left;

                    if (blk.end.line == i)
                        rcBar.right = m_Rulers[0].InxToPix(blk.end.column);
                    else
                        rcBar.right = rcLine.right;
                    break;
                }
                if (rcBar.left == rcBar.right) rcBar.right++; // make it visible
                memSelDc.FillRect(&rcBar, &m_paintAccessories->m_SelTextBrush);
            }
            else
                rcBar.left = rcBar.right = -1;

            if (i < endFileLine)
            {
                const char* currentLine; 
                int currentLineLength;
                GetLine(i + m_Rulers[1].m_Topmost, currentLine, currentLineLength);
                // recalculation for maximum of visible line lengthes
                if (currentLineLength)
                {
                    int length = inx2pos(currentLine, currentLineLength, currentLineLength - 1);
                    if (maxLineLength < length)
                        maxLineLength = length;
                }

                if (phighlighter)
                {
                    phighlighter->NextLine(currentLine, currentLineLength);
                    tokenizer.EnableProcessSpaces(phighlighter->IsSpacePrintable() || m_paintAccessories->m_TextFont & 0x04);
                }
                else
                {
                    tokenizer.EnableProcessSpaces(m_paintAccessories->m_TextFont & 0x04 ? true : false);
                }

                tokenizer.StartScan(currentLine, currentLineLength);

                while (!tokenizer.Eol())
                {
                    int pos, len;
                    const char* str;

                    tokenizer.GetCurentWord(str, pos, len);
                    
                    if (firstWordPos == -1) 
                        firstWordPos = pos;

                    if (phighlighter)
                    {
                        phighlighter->NextWord(str, len, pos);
                        tokenizer.EnableProcessSpaces(phighlighter->IsSpacePrintable() || m_paintAccessories->m_TextFont & 0x04);
                    }

                    if (pos <= scr.end.column + m_Rulers[0].m_Topmost)
                    {
                        if (pos + len > scr.start.column + m_Rulers[0].m_Topmost
                        && pos <= scr.end.column + m_Rulers[0].m_Topmost)
                        {
                            if (!blockExist || (rcBar.left == rcBar.right)
                            || !(pos >= blk.start.column + m_Rulers[0].m_Topmost
                              && pos + len <= blk.end.column + m_Rulers[0].m_Topmost))
                            {
                                if (phighlighter)
                                {
                                    m_paintAccessories->SelectFont(memDc, phighlighter->GetFontIndex());
                                    memDc.SetTextColor(phighlighter->GetTextColor());
                                    
                                    if (GetSQLToolsSettings().GetEnhancedVisuals())
                                    {
                                        if (bIsBrace)
                                        {
                                            int nOffset0 = (match.line[0] < GetLineCount()) ? inx2pos(match.line[0], match.offset[0]) : match.offset[0];
                                            int nOffset1 = (match.line[1] < GetLineCount()) ? inx2pos(match.line[1], match.offset[1]) : match.offset[1];
                                            if (((i + m_Rulers[1].m_Topmost == match.line[0]) && (pos == nOffset0)) ||
                                                (match.found && ! match.broken && (i + m_Rulers[1].m_Topmost == match.line[1]) && (pos == nOffset1)))
                                            {
                                                if (! phighlighter->IsPlainText() || ! IsBrace(*str))
                                                {
                                                    bIsBrace = false;
                                                }
                                                else
                                                {
                                                    RECT rcToken;
                                                    rcToken.left = m_Rulers[0].PosToPix(pos);
                                                    rcToken.right = rcToken.left + m_Rulers[0].m_CharSize * len;
                                                    rcToken.top = rcLine.top;
                                                    rcToken.bottom = rcLine.bottom;
                                                    static CBrush brush(RGB(255,0,0));
                                                    memDc.FillRect(&rcToken, 
                                                                   (match.found && ! match.broken) ? 
                                                                   &m_paintAccessories->m_RndBmkBrush : &brush);
                                                                   // &m_paintAccessories->m_BmkBrush);
                                                }
                                            }
                                        }

                                        int nMaxIdentLength = GetSQLToolsSettings().GetMaxIdentLength();

                                        if ((nMaxIdentLength > 0) && 
                                            phighlighter->IsIdentifier(str, len) &&
                                            phighlighter->GetActualTokenLength(str, len) > nMaxIdentLength)
                                        {
                                            RECT rcToken;
                                            rcToken.left = m_Rulers[0].PosToPix(pos);
                                            rcToken.right = rcToken.left + m_Rulers[0].m_CharSize * len;
                                            rcToken.top = rcLine.top;
                                            rcToken.bottom = rcLine.bottom;
                                            memDc.SetTextColor(m_paintAccessories->m_SelTextForeground);
                                            memDc.FillRect(&rcToken, &m_paintAccessories->m_BmkBrush);
                                            // CBrush brush(RGB(255,0,0));
                                            // memDc.FillRect(&rcToken, &brush);
                                        }
                                    }
                                }

                                memDc.TextOut(m_Rulers[0].PosToPix(pos), rcLine.top, str, len);
                            }

                            if (blockExist && (rcBar.left != rcBar.right)
                                && ((blkMode != ebtColumn)
                                    || ((blkMode == ebtColumn)
                                        && pos + len > blk.start.column + m_Rulers[0].m_Topmost
                                        && pos <= blk.end.column + m_Rulers[0].m_Topmost
                                       )
                                   )
                               )
                            {
                                if (phighlighter)
                                {
                                    m_paintAccessories->SelectFont(memSelDc, phighlighter->GetFontIndex());
                                }
                                memSelDc.TextOut(m_Rulers[0].PosToPix(pos), rcLine.top, str, len);
                            }
                        }
                    }
                    tokenizer.Next();
                }
            }
            else if (i == endFileLine && settings.GetEOFMark())
            {
                static const char str[] = ">> EOF <<";

                if (sizeof(str)-1 > m_Rulers[0].m_Topmost)
                {
                    m_paintAccessories->SelectTextFont(memDc);
                    memDc.SetTextColor(m_paintAccessories->m_EOFForeground);
                    memDc.TextOut(m_Rulers[0].PosToPix(0), rcLine.top, str, sizeof(str)-1);

                    if (blockExist)
                    {
                        m_paintAccessories->SelectTextFont(memSelDc);
                        memSelDc.TextOut(m_Rulers[0].PosToPix(0), rcLine.top, str, sizeof(str)-1);
                    }
                }
            }

            if (rcBar.left != rcBar.right)
            {
                int width = rcBar.right - rcBar.left;
                if (blkMode == ebtStream && i < endFileLine
                && !GetCursorBeyondEOL())
                {
                    int lineLength = GetLineLength(i + m_Rulers[1].m_Topmost);

                    if (blk.start.line == blk.end.line
                    && rcBar.left >= m_Rulers[0].PosToPix(lineLength))
                    {
                        width = 0;
                    }
                    else
                    {
                        rcBar.left = min(rcBar.left, (long)m_Rulers[0].PosToPix(lineLength));
                        width = min(rcBar.right, (long)m_Rulers[0].PosToPix(lineLength + 1)) - rcBar.left;
                    }
                }
                if (width)
                    memDc.BitBlt(rcBar.left, 0,
                                 width, m_Rulers[1].m_CharSize,
                                 &memSelDc, rcBar.left, 0, SRCCOPY);
            }

            if (columnMarkers)
            {
                memDc.SelectObject(&m_paintAccessories->m_RulerPen);
                for (vector<int>::const_iterator it = columnMarkersSet.begin();
                    it != columnMarkersSet.end();
                    ++it)
                {
                    memDc.MoveTo(m_Rulers[0].PosToPix(*it), 0);
                    memDc.LineTo(m_Rulers[0].PosToPix(*it), m_Rulers[1].m_CharSize);
                }
            }

            if (indentGuide
            && firstWordPos > indentSpacing)
            {
                memDc.SelectObject(&m_paintAccessories->m_IndentGuidelinePen);
                for (int pos = indentSpacing; pos < firstWordPos + m_Rulers[0].m_Topmost; pos += indentSpacing)
                {
                    memDc.MoveTo(m_Rulers[0].PosToPix(pos), 0);
                    memDc.LineTo(m_Rulers[0].PosToPix(pos), m_Rulers[1].m_CharSize);
                }
            }

            dc.BitBlt(rcLine.left, m_Rulers[1].InxToPix(i),
                    rcLine.right - rcLine.left, m_Rulers[1].m_CharSize,
                    &memDc, rcLine.left, 0, SRCCOPY);
        }

        memDc.SelectObject(pOrgBitmap);
        memDc.SelectObject(pOrgdMemDcPen);

        if (blockExist && pOrgSelBitmap)
            memSelDc.SelectObject(pOrgSelBitmap);
    }

    if (m_nMaxLineLength < maxLineLength)
    {
        m_nMaxLineLength = maxLineLength;
        AdjustHorzScroller();
    }
}

void COEditorView::OnSettingsChanged ()
{
    const Settings& settings = GetSettings();
    string language = settings.GetLanguage();
    const VisualAttributesSet& set = settings.GetVisualAttributesSet();

    m_paintAccessories = COEViewPaintAccessories::GetPaintAccessories(language);
    m_paintAccessories->OnSettingsChanged(this, set);

    m_highlighter = HighlighterFactory::CreateHighlighter(language);
    
    if (m_highlighter.get())
    {
        m_highlighter->InitStorageScanner(GetMultilineQuotesScanner());
        m_highlighter->Init(set);
    }

    const VisualAttribute& textAttr = set.FindByName("Text");
    const VisualAttribute& curLinetAttr = set.FindByName("Current Line");
    HighlightCurrentLine(textAttr.m_Background != curLinetAttr.m_Background ? true : false);

    bool recalcSize = false;

    if (m_Rulers[0].m_CharSize != m_paintAccessories->m_CharSize.cx
    || m_Rulers[1].m_CharSize != m_paintAccessories->m_CharSize.cy)
    {
        m_Rulers[0].m_CharSize = m_paintAccessories->m_CharSize.cx;
        m_Rulers[1].m_CharSize = m_paintAccessories->m_CharSize.cy;
        recalcSize = true;
    }

    int leftMargin = CalculateLeftMargin();
    if (m_Rulers[0].m_Indent != leftMargin)
    {
        m_Rulers[0].m_Indent = leftMargin;
        recalcSize = true;
    }

// ruler
    int rulerHight = settings.GetRuler() ? m_paintAccessories->m_RulerCharSize.cy + 4 : 0;
    if (m_Rulers[1].m_Indent != rulerHight)
    {
        m_Rulers[1].m_Indent = rulerHight;
        recalcSize = true;
    }

    if (recalcSize)
    {
        CRect rc;
        GetClientRect(rc);
        DoSize(SIZE_RESTORED, rc.Width(), rc.Height());
        ShowCaret(::GetFocus() == m_hWnd, true/*dont_destroy_caret*/);
        AdjustCaretPosition();
    }

    // if (GetSQLToolsSettings().GetEnhancedVisuals())
    m_curCharCache.Reset();

    Invalidate(FALSE);
}
