/* 
    Copyright (C) 2002 Aleksey Kochetov

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

// 22.07.2002 bug fix, stream block delete fails below EOF
// 03.08.2002 bug fix, block copy is too slow on 10M file
// 14.08.2002 bug fix, indent/undent block does not work if selection is not normalized
// 17.08.2002 bug fix, block copy fails below EOF
// 07.09.2002 bug fix, copy/paste operation loses the first blank line
// 17.09.2002 performance tuning, insert/delete block operation
// 13.01.2003 bug fix, useless selection on delete word undo
// 07.03.2003 bug fix, wrong columnar selection after undo
// 16.03.2003 bug fix, autoindent always uses a previos line as base even the line is empty
// 16.03.2003 bug fix, scroll position fails on delete boolmark lines operation
// 26.05.2003 bug fix, selection may still be out of visible text after undo
// 03.06.2003 bug fix, sql find match fails on select/insert/... if there is no ending ';'
// 10.06.2004 improvement, text has been sort implemented (currently only 1 sort key)
// 11.10.2004 improvement, columnar edit mode has been added
// 11.10.2004 improvement, columnar indent and undent mode has been added (if columnar mode is on)
// 24.10.2004 improvement, indent and undent align each line individually to the neatest indent level
// 30.03.2005 bug fix, sorting does not completely remove duplicate rows if a number of dups is even

#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include <COMMON/AppGlobal.h>
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEContext.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _CHECK_ALL_PTR_ _CHECK_AND_THROW_(m_pStorage, "Editor has not been initialized properly!")

namespace OpenEditor
{
    using namespace std;


    void space4insertion (string& buff, int from, int to, bool use_tab, int tab_spacing)
    {
        _ASSERTE(to - from >= 0);

        buff.erase();

        int cur_tabs = from / tab_spacing;
        int new_tabs = to / tab_spacing;

        if (use_tab)
        {
            int spaces   = to - new_tabs * tab_spacing;
            int tabs     = new_tabs - cur_tabs;
            buff.append(tabs, '\t');
            buff.append(spaces, ' ');
        }
        else
            buff.append(to - from, ' ');
    }


void EditContext::PushInUndoStack (Position pos)
{
    pos.column = pos2inx(pos.line, pos.column, true); // 07.03.2003 bug fix, wrong columnar selection after undo
    m_pStorage->PushInUndoStack(pos);
}

void EditContext::PushInUndoStack (EBlockMode mode, Square sel)
{
    sel.start.column = pos2inx(sel.start.line, sel.start.column, true); // 07.03.2003 bug fix, wrong columnar selection after undo
    sel.end.column = pos2inx(sel.end.line, sel.end.column, true);
    m_pStorage->PushInUndoStack(mode, sel);
}

void EditContext::expandVirtualSpace (int line, int column)
{
    m_pStorage->ExpandLinesTo(line);

    int length = GetLineLength(line);
    if (column > length)
    {
        string buff;
        space4insertion(buff, length, column, !GetTabExpand(), GetTabSpacing());
        InsertLinePart(line, length, buff.c_str(), buff.length());
    }
}

// string mustn't have '\r' or '\n'
void EditContext::InsertLine (int line, const char* str, int len)
{
    if (line)
        m_pStorage->ExpandLinesTo(line - 1);

    m_pStorage->InsertLine(line, str, len);
}

// string mustn't have '\r' or '\n'
void EditContext::InsertLinePart (int line, int column, const char* str, int len)
{
    expandVirtualSpace(line, column);
    m_pStorage->InsertLinePart(line, !column ? 0 : pos2inx(line, column), str, len);
}

void EditContext::DeleteLinePart (int line, int from, int to)
{
    if (line < GetLineCount())
    {
        m_pStorage->DeleteLinePart(line, !from ? 0 : pos2inx(line, from), !to   ? 0 : pos2inx(line, to));
    }
}

void EditContext::SplitLine (int line, int pos)
{
    if (line < GetLineCount())
    {
        m_pStorage->SplitLine(line, !pos ? 0 : pos2inx(line, pos, true));
    }
}

void EditContext::DoIndent (bool moveOnly)
{
    UndoGroup undoGroup(*this);

    Position pos   = GetPosition();
    int indentSize = GetIndentSpacing();
    int newColumn  = (pos.column / indentSize + 1) * indentSize;

    if (!moveOnly && (!GetCursorBeyondEOL()
    || (pos.line < GetLineCount() && pos.column < GetLineLength(pos.line))))
    {
        string buff;
        space4insertion(buff, pos.column, newColumn, !GetTabExpand(), GetTabSpacing());
        InsertLinePart(pos.line, pos.column, buff.c_str(), buff.length());
    }
    pos.column = newColumn;

    MoveTo(pos);
}

void EditContext::DoUndent ()
{
    int indentSize = GetIndentSpacing();
    Position pos   = GetPosition();
    pos.column = max(0, (pos.column / indentSize
                         - ((pos.column % indentSize) ? 0 : 1)) * indentSize);
    MoveTo(pos);
}

void EditContext::DoCarriageReturn ()
{
    UndoGroup undoGroup(*this);

    int column;

    if (m_curPos.line < GetLineCount())
        //column = m_curPos.column;
        column = min(m_curPos.column, GetLineLength(m_curPos.line));
    else
        column = 0;

    expandVirtualSpace(m_curPos.line, column);
    m_pStorage->Insert('\r', m_curPos.line, !column ? 0 : pos2inx(m_curPos.line, column));

    if (GoToDown(true) && GoToStart(true))
    {
        switch (GetIndentType())
        {
        case eiAuto:
            if (m_curPos.line > 0 && m_curPos.line < GetLineCount())
            {
                int len = 0;
                const char* str = 0;
                Position pos = GetPosition();

                // 16.03.2003 bug fix, autoindent always uses a previos line as base even the line is empty
                for (int baseLine = pos.line - 1; baseLine >= 0 && len == 0; baseLine--)
                    GetLine(baseLine, str, len);

                for (int i(0); i < len && isspace(str[i]); i++);

                pos.column = inx2pos(str, len, i);

                if (!(GetCursorBeyondEOL() && GetLineLength(pos.line) == 0))
                {
                    string buff;
                    space4insertion(buff, 0, pos.column, !GetTabExpand(), GetTabSpacing());
                    InsertLinePart(pos.line, 0, buff.c_str(), buff.length());
                }

                MoveTo(pos, true);
            }
            break;
        default:; // eiNone & others
        }
    }
    PushInUndoStack(m_curPos);
}

void EditContext::Insert (char chr)
{
    _CHECK_ALL_PTR_

    switch (chr)
    {
    case '\r':
        DoCarriageReturn();
        break;
    case '\t':
        DoIndent();
        break;
    default:
        {
            UndoGroup undoGroup(*this);

            expandVirtualSpace(m_curPos.line, m_curPos.column);
            m_pStorage->Insert(chr, m_curPos.line, pos2inx(m_curPos.line, m_curPos.column));
            GoToRight(true);
        }
    }
}

void EditContext::Backspace ()
{
    _CHECK_ALL_PTR_

    int nlines = GetLineCount();
    int length = (m_curPos.line < nlines) ? GetLineLength(m_curPos.line) : 0;

    if (m_curPos.line < nlines && m_curPos.column <= length)
    {
        UndoGroup undoGroup(*this);

        if (!m_curPos.column)
        {
            if (GoToLeft())
            {
                int inx = pos2inx(m_curPos.line, m_curPos.column, true);
                m_pStorage->Delete(m_curPos.line, inx);
            }
            return;
        }

        int inx = pos2inx(m_curPos.line, m_curPos.column);

        char chr(0);
        try
        {
            chr = m_pStorage->GetChar(m_curPos.line, inx-1);
            m_pStorage->Delete(m_curPos.line, inx-1);
        }
		catch (std::out_of_range&) {}

        if (chr == '\t')
        {
            m_curPos.column =
            m_orgHrzPos     = inx2pos(m_curPos.line, inx-1);
        }
        else
            GoToLeft();
    }
    else
    {
        if (m_curPos.column > 0)
        {
            int indentSize = GetIndentSpacing();
            int lastIndent = (length / indentSize + 1) * indentSize;

            if (m_curPos.column <= lastIndent)
                GoToEnd();
            else
                DoUndent();
        }
        else
            GoToLeft();
    }
}

void EditContext::Overwrite (char chr)
{
    _CHECK_ALL_PTR_

    UndoGroup undoGroup(*this);

    switch (chr)
    {
    case '\r':
        if (GoToDown()) GoToStart();
        break;
    case '\t':
        DoIndent();
        break;
    default:
        expandVirtualSpace(m_curPos.line, m_curPos.column + 1);
        m_pStorage->Overwrite(chr, m_curPos.line, pos2inx(m_curPos.line, m_curPos.column));
        GoToRight(true);
        break;
    }
}

void EditContext::Delete ()
{
    _CHECK_ALL_PTR_

    if (m_curPos.line < GetLineCount())
    {
        UndoGroup undoGroup(*this);

        expandVirtualSpace(m_curPos.line, m_curPos.column);
        m_pStorage->Delete(m_curPos.line, pos2inx(m_curPos.line, m_curPos.column));
    }
}

void EditContext::DeleteLine ()
{
    if (m_curPos.line < GetLineCount())
    {
        UndoGroup undoGroup(*this);

        // save pos because it can be changed by notification handler
        Position pos = GetPosition();
        m_pStorage->DeleteLine(pos.line);
        MoveTo(pos);

        AdjustCaretPosition();
        ClearSelection();
    }
}

void EditContext::DeleteWordToLeft ()
{
    Square blkPos;
    blkPos.start = m_curPos;
    blkPos.end   = wordLeft(false);

    if (!blkPos.is_empty())
    {
        EBlockMode blkMode = GetBlockMode();
        SetBlockMode(ebtStream);
        SetSelection(blkPos);
        DeleteBlock(false);
        SetBlockMode(blkMode);
        AdjustCaretPosition();
    }
}

void EditContext::DeleteWordToRight ()
{
    Square blkPos;
    blkPos.start = wordRight(false);
    blkPos.end   = m_curPos;

    if (!blkPos.is_empty())
    {
        EBlockMode blkMode = GetBlockMode();
        SetBlockMode(ebtStream);
        SetSelection(blkPos);
        DeleteBlock(false);
        SetBlockMode(blkMode);
        AdjustCaretPosition();
    }
}

void EditContext::Undo ()
{
    _CHECK_ALL_PTR_

    UndoContext cxt(*m_pStorage);

    if (m_pStorage->Undo(cxt))
    {
        if (cxt.m_position.line < GetLineCount())
            cxt.m_position.column = inx2pos(cxt.m_position.line, cxt.m_position.column);
        // 26.06.2003 bug fix, selection may still be out of visible text after undo
        MoveToAndCenter(cxt.m_position);

        if (cxt.m_selMode != ebtUndefined)
        {
            if (cxt.m_selection.start.line < GetLineCount())
                cxt.m_selection.start.column = inx2pos(cxt.m_selection.start.line, cxt.m_selection.start.column);
            if (cxt.m_selection.end.line < GetLineCount())
                cxt.m_selection.end.column = inx2pos(cxt.m_selection.end.line, cxt.m_selection.end.column);
            SetBlockMode(cxt.m_selMode);
            SetSelection(cxt.m_selection);
        }
        else
            ClearSelection();

        //if (cxt.m_position.line < GetLineCount())
        //    cxt.m_position.column = inx2pos(cxt.m_position.line, cxt.m_position.column);
        //MoveTo(cxt.m_position);
    }
}

void EditContext::Redo ()
{
    _CHECK_ALL_PTR_

    UndoContext cxt(*m_pStorage);

    if (m_pStorage->Redo(cxt))
    {
        if (cxt.m_selMode != ebtUndefined)
        {
            if (cxt.m_selection.start.line < GetLineCount())
                cxt.m_selection.start.column = inx2pos(cxt.m_selection.start.line, cxt.m_selection.start.column);
            if (cxt.m_selection.end.line < GetLineCount())
                cxt.m_selection.end.column = inx2pos(cxt.m_selection.end.line, cxt.m_selection.end.column);
            SetBlockMode(cxt.m_selMode);
            SetSelection(cxt.m_selection);
        }
        else
            ClearSelection();

        if (cxt.m_position.line < GetLineCount())
            cxt.m_position.column = inx2pos(cxt.m_position.line, cxt.m_position.column);
        MoveTo(cxt.m_position);
    }
}

void EditContext::ClearSelection ()
{
    if (!m_blkPos.is_empty())
    {
        if (m_BlockMode == ebtColumn)
            InvalidateSquare(m_blkPos);
        else
            InvalidateLines(m_blkPos.start.line, m_blkPos.end.line);

        m_blkPos.start = m_blkPos.end;

        if (m_bDirtyPosition)
            adjustCursorPosition();
    }
}

void EditContext::ConvertSelectionToLines ()
{
    if (GetBlockMode() != ebtStream)
        SetBlockMode(ebtStream);

    Square pos;
    GetSelection(pos);

    if (!pos.is_empty())
    {
        pos.normalize();
        pos.normalize();

        bool reselect = false;

        if (pos.start.column > 0)
        {
            reselect = true;
            pos.start.column = 0;
        }

        if (pos.end.column > 0 && GetLineLength(pos.end.line))
        {
            reselect = true;
            pos.end.line++;
            pos.end.column = 0;
        }

        if (reselect)
        {
            SetSelection(pos);
            MoveTo(pos.end, true);
        }
    }
}

void EditContext::SetSelection (const Square& blkPos)
{
    if (m_blkPos != blkPos)
    {
        ClearSelection();

        m_blkPos = blkPos;

        if (GetBlockMode() == ebtColumn)
            InvalidateSquare(m_blkPos);
        else
            InvalidateLines(m_blkPos.start.line, m_blkPos.end.line);
    }
}

void EditContext::SelectAll()
{
    if (GetLineCount())
    {
        Square selection;
        selection.start.line   = 0;
        selection.end.line     = GetLineCount() - 1;
        selection.start.column = 0;
        selection.end.column   = GetLineLength(selection.end.line);

        SetBlockMode(ebtStream);
        MoveTo(selection.end);
        SetSelection(selection);

        InvalidateLines(selection.start.line, selection.end.line);
    }
}

void EditContext::SelectLine (int line)
{
    ClearSelection();
    if (GetCursorBeyondEOF() || line < GetLineCount())
    {
        Square selection;
        selection.start.line   = line;
        selection.end.line     = line + 1;
        selection.start.column = 0;
        selection.end.column   = 0;

        SetBlockMode(ebtStream);
        MoveTo(selection.end);
        SetSelection(selection);

        InvalidateLines(selection.start.line, selection.end.line);
    }
}

void EditContext::SelectByCursor (Position pos)
{
    if (m_blkPos.is_empty())
        m_blkPos.start = m_blkPos.end = pos;
    else if (m_BlockMode == ebtColumn)
        InvalidateSquare(m_blkPos);

    int old_end_line = m_blkPos.end.line;

    // it's necessary to add checks here
    m_blkPos.end = GetPosition();

    if (m_BlockMode == ebtColumn)
        InvalidateSquare(m_blkPos);
    else
        InvalidateLines(old_end_line, m_blkPos.end.line);
}

void EditContext::GetBlock (string& str, const Square* sqr) const
{
    _CHECK_ALL_PTR_

    Square pos;

    if (sqr)
        pos = *sqr;
    else
        GetSelection(pos);

    if (!pos.is_empty())
    {
        pos.normalize();

        int niLength;
        const char* pszLine;
        int nLines = GetLineCount();

        switch (GetBlockMode())
        {
        case ebtStream:
            {
                int reserve = 0;
                for (int i(pos.start.line); i <= pos.end.line; i++)
                    // 03/08/2002 bug fix, block copy is too slow on 10M file
                    // 17/08/2002 bug fix, block copy fails below EOF
                    if (i < nLines) 
                        reserve += m_pStorage->GetLineLength(i) + (sizeof("\r\n")-1);
                    else
                        break;

                str.reserve(reserve);

                if (pos.start.line == pos.end.line)
                {
                    line2buff(pos.start.line, pos.start.column, pos.end.column, str);
                }
                else
                {
                    line2buff(pos.start.line, pos.start.column, INT_MAX, str);
                    for (int i(pos.start.line + 1); i <= pos.end.line - 1; i++)
                    {
                        str += "\r\n";
                        if (i < nLines)
                        {
                            GetLine(i, pszLine, niLength);
                            str.append(pszLine, niLength);
                        }
                    }
                    str += "\r\n";
                    line2buff(pos.end.line, 0, pos.end.column, str);
                }
                TRACE("Copy Block reserved = %d, actual length = %d\n",  reserve, str.length());

                if ((pos.end.line < nLines
                  && pos.end.column > GetLineLength(pos.end.line))
                || (pos.end.line >= nLines
                  && pos.end.column > 0))
                        str += "\r\n";
            }
            break;
        case ebtColumn:
            {
                if ((pos.end.column - pos.start.column) < 0)
                    swap(pos.end.column, pos.start.column);

                str.reserve((pos.end.line - pos.start.line)
                           *(pos.end.column - pos.start.column));

                for (int i(pos.start.line); i <= pos.end.line; i++)
                {
                    if (i > pos.start.line)
                        str += "\r\n";

                    line2buff(i, pos.start.column, pos.end.column, str, true);
                }
            }
            break;
        }
    }
}

void EditContext::InsertBlock (const char* str)
{
    InsertBlock(str, !GetBlockKeepMarking());
}

void EditContext::InsertBlock (const char* str, bool hideSelection, bool putSelInUndo)
{
    _CHECK_ALL_PTR_

    string buff;
    istringstream io(str);
    bool with_CR;
    Position orgPos = m_curPos;

    UndoGroup undoGroup(*this);
    PushInUndoStack(m_curPos);
    Square sel;
    GetSelection(sel);
    if (putSelInUndo)
        PushInUndoStack(GetBlockMode(), sel);

    switch (GetBlockMode())
    {
    case ebtStream:
        {
            if (getLine(io, buff, with_CR))
            {
                if (with_CR) 
                    SplitLine(m_curPos.line, m_curPos.column); // multiline selection

                InsertLinePart(m_curPos.line, m_curPos.column, buff.c_str(), buff.length());

                if (with_CR) // multiline selection
                {
                    StringArray lines(1024, 16 * 1024);

                    m_curPos.column = 0;
                    while (getLine(io, buff, with_CR))
                    {
                        if (with_CR)
                            lines.append().assign(buff.c_str(), buff.length(), true);
                        else
                        {
                            InsertLinePart(m_curPos.line+1, 0, buff.c_str(), buff.length());
                            m_curPos.column = inx2pos(buff.c_str(), buff.size(), buff.size());
                        }
                    }

                    if (lines.size())
                        m_pStorage->InsertLines(m_curPos.line+1, lines);

                    m_curPos.line += lines.size() + 1;
                }
                else
                    m_curPos.column = inx2pos(m_curPos.line, pos2inx(m_curPos.line, m_curPos.column) + buff.size());

                MoveTo(m_curPos);
            }
        }
        break;
    case ebtColumn:
        {
            int lastLineLen = 0;
            int line = m_curPos.line;

            Storage::NotificationDisabler disabler(m_pStorage);

            while (getLine(io, buff, with_CR))
            {
                InsertLinePart(line, m_curPos.column, buff.c_str(), buff.length());
                line++;
                lastLineLen = buff.size();
            }
            orgPos.line  += line - m_curPos.line - 1;
            orgPos.column = inx2pos(orgPos.line, pos2inx(orgPos.line, m_curPos.column + lastLineLen));

            disabler.Enable();
        }
        break;
    }

    switch (GetBlockMode())
    {
    case ebtStream:
        sel.start = orgPos;
        sel.end   = m_curPos;
        SetSelection(sel);
        break;
    case ebtColumn:
        sel.start = m_curPos;
        sel.end   = orgPos;
        SetSelection(sel);
        if (GetColBlockCursorToStartAfterPaste())
            MoveTo(orgPos);
        break;
    }

    if (putSelInUndo)
    {
        GetSelection(sel);
        PushInUndoStack(GetBlockMode(), sel);
    }

    if (hideSelection)
        ClearSelection();

    AdjustCaretPosition();
    PushInUndoStack(m_curPos);
}

void EditContext::DeleteBlock (bool putSelInUndo)
{
    _CHECK_ALL_PTR_

    Square sel;
    GetSelection(sel);
    sel.normalize();
    int nLines = GetLineCount();

    if (sel.start.line < nLines 
    && (sel.start.line != nLines-1 || sel.start.column < GetLineLength(nLines-1))
    && !sel.is_empty())
    {
        UndoGroup undoGroup(*this);
        PushInUndoStack(m_curPos);
        if (putSelInUndo) 
            PushInUndoStack(GetBlockMode(), sel);

        switch (GetBlockMode())
        {
        case ebtStream:
            {
                // for single-line selection
                if (sel.start.line == sel.end.line)
                {
                    DeleteLinePart(sel.start.line, sel.start.column, sel.end.column);
                }
                else if (sel.start.line < nLines) // 22/07/2002 bug fix, stream block delete fails below EOF
                {
                    // delete selection, skip first and last lines
                    if (sel.end.line-sel.start.line > 1)
                        m_pStorage->DeleteLines(sel.start.line + 1, 
                            sel.end.line - sel.start.line - 1);

                    DeleteLinePart(sel.start.line, sel.start.column, USHRT_MAX);
                    expandVirtualSpace(sel.start.line, sel.start.column);
                    DeleteLinePart(sel.start.line + 1, 0, sel.end.column);
                    m_pStorage->MergeLines(sel.start.line);
                }

                MoveTo(sel.start);
            }
            ClearSelection();
            break;
        case ebtColumn:
            {
                sel.normalize_columns();

                Storage::NotificationDisabler disabler(m_pStorage);

                bool deleteSpace = GetColBlockDeleteSpaceAfterMove();
                string space;

                if (!deleteSpace)
                    space.resize(sel.end.column > sel.start.column
                                 ? sel.end.column - sel.start.column
                                 : sel.start.column - sel.end.column, ' ');

                for (int i(sel.start.line); i <= sel.end.line && i < nLines; i++)
                {
                    DeleteLinePart(i, sel.start.column, sel.end.column);

                    if (!deleteSpace)
                        InsertLinePart(i, sel.start.column, space.c_str(), space.length());
                }

                disabler.Enable();

                Position pos;
                pos.line = m_curPos.line;
                pos.column = sel.start.column;
                MoveTo(pos);
            }
            if (GetColBlockEditMode())
            {
                sel.end.column = sel.start.column;
                SetSelection(sel);
            }
            else
                ClearSelection();
            break;
        }

        AdjustCaretPosition();
        
        if (putSelInUndo)
        {
            GetSelection(sel); // save empty selection in undo stack
            PushInUndoStack(GetBlockMode(), sel);
        }
        PushInUndoStack(m_curPos);
    }
    else if (!sel.is_empty())
    {
        if (GetBlockMode() != ebtColumn || !GetColBlockEditMode())
        {
            ClearSelection();
            MoveTo(sel.start);
        }
    }
}

void EditContext::ColumnarInsert (char ch)
{
    ASSERT(GetBlockMode() == ebtColumn);

    Square blk;
    GetSelection(blk);
    blk.normalize();
    blk.normalize_columns();
    Position pos = GetPosition();

    int width = blk.end.column - blk.start.column;
    string text, line(width ? width : 1, ch);

    line += "\r\n";
    for (int i = 0; i < blk.end.line - blk.start.line + 1; i++)
        text.append(line);

    UndoGroup undoGroup(*this);
    PushInUndoStack(ebtColumn, blk);
    PushInUndoStack(pos);

    if (width) DeleteBlock(false);
    MoveTo(blk.start, true/*force*/);
    InsertBlock(text.c_str(), false, width ? true : false);

    if (!width) 
    {
        pos.column++;
        blk.start.column++, blk.end.column++;
        SetSelection(blk);
        PushInUndoStack(ebtColumn, blk);
    }

    MoveTo(pos);
    PushInUndoStack(pos);
}

void EditContext::ColumnarIndent ()
{
    ASSERT(GetBlockMode() == ebtColumn);

    Square blk;
    GetSelection(blk);
    blk.normalize();
    blk.normalize_columns();
    Position pos = GetPosition();

    UndoGroup undoGroup(*this);
    PushInUndoStack(ebtColumn, blk);
    PushInUndoStack(pos);

    int nlines = GetLineCount();
    for (int line = blk.start.line; line <= blk.end.line && line < nlines; line++)
    {
        Position pos; 
        pos.line = line;
        pos.column = blk.start.column;
        MoveTo(pos);
        DoIndent(false);
    }
    int offset = GetPosition().column - blk.start.column;
    blk.end.column += offset;
    blk.start.column += offset;
    SetSelection(blk);
    pos.column += offset;
    MoveTo(pos);

    PushInUndoStack(ebtColumn, blk);
    PushInUndoStack(pos);
}

void EditContext::ColumnarUndent ()
{
    ASSERT(GetBlockMode() == ebtColumn);

    Square blk;
    GetSelection(blk);
    blk.normalize();
    blk.normalize_columns();

    int indentSize = GetIndentSpacing();
    int newIndent = max(0, 
        (blk.start.column / indentSize - ((blk.start.column % indentSize) ? 0 : 1)) * indentSize);

    if (newIndent == blk.start.column) return;

    int nlines = GetLineCount();
    for (int line = blk.start.line; line <= blk.end.line && line < nlines; line++)
    {
        int len;
        const char* str;
        GetLine(line, str, len);

        int indentInx = pos2inx(str, len, blk.start.column, true);
        int newIndentInx = pos2inx(str, len, newIndent, true);

        if (newIndentInx < len)
        {
            for (int i = min(indentInx-1, len-1); newIndentInx <= i && isspace(str[i]); i--) {}
            newIndent = max<int>(newIndent, inx2pos(str, len, i + 1));
        }
    }

    if (newIndent == blk.start.column) return;

    UndoGroup undoGroup(*this);
    PushInUndoStack(ebtColumn, blk);
    PushInUndoStack(GetPosition());

    for (int line = blk.start.line; line <= blk.end.line; line++)
        DeleteLinePart(line, newIndent, blk.start.column);

    int offset = blk.start.column - newIndent;
    Position pos = GetPosition();
    pos.column -= offset;
    MoveTo(pos);

    blk.end.column -= offset;
    blk.start.column = newIndent;
    SetSelection(blk);

    PushInUndoStack(ebtColumn, blk);
    PushInUndoStack(GetPosition());
}

void EditContext::ColumnarBackspace ()
{
    ASSERT(GetBlockMode() == ebtColumn);

    Square blk;
    GetSelection(blk);
    blk.normalize();
    blk.normalize_columns();

    if (blk.end.column == blk.start.column)
    {
        if (blk.start.column > 0)
        {
            UndoGroup undoGroup(*this);
            PushInUndoStack(ebtColumn, blk);
            PushInUndoStack(GetPosition());
            blk.start.column--;
            SetSelection(blk);
            DeleteBlock(false);
            blk.end.column--;
            SetSelection(blk);
            PushInUndoStack(ebtColumn, blk);
        }
    }
    else
        DeleteBlock();
}

void EditContext::ColumnarDelete ()
{
    ASSERT(GetBlockMode() == ebtColumn);

    Square blk;
    GetSelection(blk);
    blk.normalize();
    blk.normalize_columns();

    if (blk.end.column == blk.start.column)
    {
        UndoGroup undoGroup(*this);
        PushInUndoStack(ebtColumn, blk);
        PushInUndoStack(GetPosition());
        blk.end.column++;
        SetSelection(blk);
        DeleteBlock(false);
        blk.end.column--;
        SetSelection(blk);
        PushInUndoStack(ebtColumn, blk);
    }
    else
        DeleteBlock();
}

#pragma message("\tTODO: \"smart\" indent/undent, test in case tab <> indent\n")
void EditContext::IndentBlock ()
{
    _CHECK_ALL_PTR_

    Square sel;
    GetSelection(sel);
    sel.normalize(); // 14/08/2002 bug fix, indent/undent block does not work if selection is not normalized

    if (!sel.is_empty())
    {
        UndoGroup undoGroup(*this);
        PushInUndoStack(m_curPos);
        PushInUndoStack(GetBlockMode(), sel);

        int nLines = GetLineCount();
        int indentSize = GetIndentSpacing();

        ConvertSelectionToLines();

        Storage::NotificationDisabler disabler(m_pStorage);
        
        // 24.10.2004 improvement, indent and undent align each line individually to the neatest indent level
        for (int line = sel.start.line; line < sel.end.line && line < nLines; line++)
        {
            int len;
            const char* str;
            GetLine(line, str, len);

            int indentLevel = -1;

            for (int i = 0; i < len; i++)
                if (!isspace(str[i])) 
                {
                    indentLevel = i;
                    break;
                }

            if (indentLevel != -1)
            {
                indentLevel = inx2pos(str, len, indentLevel);
                int newIndentLevel = (indentLevel / indentSize + 1) * indentSize;

                string buff;
                space4insertion (buff, indentLevel, newIndentLevel, !GetTabExpand(), GetTabSpacing());
                InsertLinePart(line, indentLevel, buff.c_str(), buff.length());
            }
        }

        disabler.Enable();
        PushInUndoStack(m_curPos);
    }
}

#pragma message("\tTODO: \"smart\" indent/undent, test in case tab <> indent\n")
void EditContext::UndentBlock ()
{
    _CHECK_ALL_PTR_

    Square sel;
    GetSelection(sel);
    sel.normalize(); // 14/08/2002 bug fix, indent/undent block does not work if selection is not normalized

    if (!sel.is_empty())
    {
        UndoGroup undoGroup(*this);
        PushInUndoStack(m_curPos);
        PushInUndoStack(GetBlockMode(), sel);

        int nLines = GetLineCount();
        int indentSize = GetIndentSpacing();

        ConvertSelectionToLines();

        Storage::NotificationDisabler disabler(m_pStorage);

        // 24.10.2004 improvement, indent and undent align each line individually to the neatest indent level
        for (int line = sel.start.line; line < sel.end.line && line < nLines; line++)
        {
            int len;
            const char* str;
            GetLine(line, str, len);

            int indentLevel = -1;

            for (int i = 0; i < len; i++)
                if (!isspace(str[i])) 
                {
                    indentLevel = i;
                    break;
                }

            if (indentLevel > 0)
            {
                indentLevel = inx2pos(str, len, indentLevel);
                int newIndentLevel = max(0, 
                    (indentLevel / indentSize - ((indentLevel  % indentSize) ? 0 : 1)) * indentSize);
                DeleteLinePart(line, newIndentLevel, indentLevel);
            }
        }
        disabler.Enable();
        PushInUndoStack(m_curPos);
    }
}


    struct LineRef 
    {
        int m_seq, m_length;
        const char* m_str;
        LineRef () : m_seq(0), m_str(0), m_length(0) {}
    };

    class SortFunctor
    {
        const SortCtx& m_ctx;
    public:

        SortFunctor (const SortCtx& ctx) : m_ctx(ctx) {}

        bool operator () (const LineRef& e1, const LineRef& e2) const
        {
            bool res = least(e1, e2);
            return m_ctx.mKeyOrder == 0 ? res : !res;
        }

        bool least (const LineRef& e1, const LineRef& e2) const
        {
            int minlen = min(e1.m_length, e2.m_length);

            if (!minlen)
                return e1.m_length < e2.m_length ? true : false;

            int res = m_ctx.mIgnoreCase 
                ? strnicmp(e1.m_str, e2.m_str, minlen)
                : strncmp(e1.m_str, e2.m_str, minlen);
            
            if (!res)
                return e1.m_length < e2.m_length ? true : false;

            return res < 0 ? true : false;
        }
    };

void EditContext::Sort (const SortCtx& ctx)
{
    _CHECK_ALL_PTR_

    Square sel;
    GetSelection(sel);
    
    UndoGroup undoGroup(*this);
    PushInUndoStack(m_curPos);
    PushInUndoStack(GetBlockMode(), sel);

    if (sel.is_empty())
    {
        sel.start.column = 
        sel.end.column = 
        sel.start.line = 0;
        sel.end.line = GetLineCount();
        SetSelection(sel);
    }

    ConvertSelectionToLines();
    GetSelection(sel);
    sel.normalize();
    MoveTo(sel.start);

    int nLines = GetLineCount();
    int start  = min(sel.start.line, nLines);
    int end    = min(sel.end.line, nLines);
    int keypos = ctx.mKeyStartColumn -1;
    int keylen = !ctx.mKeyLength ? INT_MAX : ctx.mKeyLength;

    vector<LineRef> lines(end - start);

    for (unsigned long i = 0; i < lines.size(); i++)
    {
        int len;
        const char* str;
        GetLine(i + start, str, len);

        lines.at(i).m_seq = i + start;

        if (keypos < len)
        {
            lines[i].m_str = str + keypos;
            lines[i].m_length = min(len - keypos, keylen);
        }
    }

    Global::SetStatusText("Sorting...");
    sort(lines.begin(), lines.end(), SortFunctor(ctx));

    vector<int> order(lines.size());

    vector<LineRef>::const_iterator it = lines.begin();
    for (int i = 0; it != lines.end(); ++it, i++)
        order.at(i) = it->m_seq;

    Global::SetStatusText("Moving...");
    m_pStorage->Reorder(start, order);

    int removedCounter = 0;

    if (ctx.mRemoveDuplicates)
    {
        Global::SetStatusText("Removing duplicates (it may take more time than sorting)...");
        Storage::NotificationDisabler disabler(m_pStorage);

        int prevLen = 0;
        const char* prevStr = 0;
        bool firstCycle = true;

        for (int i = start; i < end; i++)
        {
            int curLen;
            const char* curStr;
            GetLine(i, curStr, curLen);

            if (!firstCycle
            && prevLen == curLen && (!prevLen || !strncmp(prevStr, curStr, prevLen)))
            {
                m_pStorage->DeleteLine(i--);
                removedCounter++;
                end--;
            }
            else 
            // 30.03.2005 bug fix, sorting does not completely remove duplicate rows if a number of dups is even
            {
                prevLen = curLen;
                prevStr = curStr;
                firstCycle = false;
            }
        }

        if (removedCounter)
        {
            sel.end.line -= removedCounter;
            SetSelection(sel);
            disabler.Enable();
            m_pStorage->Notify_ChangedLinesQuantity(sel.start.line, -removedCounter);
        }
    }

    PushInUndoStack(m_curPos);
    PushInUndoStack(GetBlockMode(), sel);

    ostringstream o;
    o << lines.size() << " line(s) sorted";
    
    if (removedCounter) 
        o << ", " << removedCounter << " duplicated line(s) removed." << ends;
    else
        o << '.' << ends;

    Global::SetStatusText(o.str().c_str());
}

void EditContext::CopyBookmarkedLines (string& buff) const
{
    _CHECK_ALL_PTR_

    vector<int> lines;
    int reserve = 0;
    
    m_pStorage->GetBookmarkedLines(lines, eBmkGroup1);

    vector<int>::const_iterator it = lines.begin();
    for (; it != lines.end(); ++it)
        reserve += m_pStorage->GetLineLength(*it) + (sizeof("\r\n")-1);
    
    if (reserve)
    {
        buff.reserve(reserve);
        it = lines.begin();
        for (; it != lines.end(); ++it)
        {
            int len;
            const char* str;
            GetLine(*it, str, len);
            buff.append(str, len);
            buff += "\r\n";
        }
    }
}

void EditContext::DeleteBookmarkedLines ()
{
    _CHECK_ALL_PTR_

    vector<int> lines;
    m_pStorage->GetBookmarkedLines(lines, eBmkGroup1);
    
    UndoGroup undoGroup(*this);
    Storage::NotificationDisabler disabler(m_pStorage);

    vector<int>::reverse_iterator it = lines.rbegin();
    for (; it != lines.rend(); ++it)
        m_pStorage->DeleteLine(*it);
    
    if (lines.size())
    {
        disabler.Enable();
        // Undo/Redo operation does not support this notification :(
        m_pStorage->Notify_ChangedLinesQuantity(lines[0], -(int)lines.size()); // 16.03.2003 bug fix, scroll position fails on delete boolmark lines operation
        AdjustCaretPosition();
        ClearSelection();
    }
}

bool EditContext::ExpandTemplate (int index)
{
    _CHECK_ALL_PTR_

    Square sqr;
    string buff;
    if (index != -1 || GetBlockOrWordUnderCursor(buff, sqr, false))
    {
        const TemplatePtr tmpl = GetSettings().GetTemplate(); 
        
        Position pos;
        string text;

        bool expand = false;
        
        if (index != -1)
        {
            expand = tmpl->ExpandKeyword(index, text, pos);
            GetSelection(sqr);
        }
        else
        {
            expand = tmpl->ExpandKeyword(buff, text, pos);
        }

        if (expand)
        {
            EBlockMode orgBlockMode = GetBlockMode();
            if (orgBlockMode != ebtStream)
                SetBlockMode(ebtStream);

            UndoGroup undoGroup(*this);
            SetSelection(sqr);
            DeleteBlock(false);

            bool with_CR;
            std::istringstream io(text.c_str());
            std::string line, result, indent(sqr.start.column, ' ');
            indent.insert(indent.begin(), '\n');

            if (getLine(io, line, with_CR))
                result += line;

            while (getLine(io, line, with_CR))
            {
                if (!line.empty())
                    result += indent + line;
                else if (with_CR)
                    result += '\n';
            }

            InsertBlock(result.c_str(), false, false);

            if (!GetTabExpand()) 
                ScanAndReplaceText(TabifyLeadingSpaces, false);

            SetBlockMode(orgBlockMode);
            ClearSelection();

            pos.line += sqr.start.line;
            pos.column += sqr.start.column;

            MoveTo(pos);
            PushInUndoStack(pos);

            //GetSelection(sqr);
            //PushInUndoStack(orgBlockMode, sqr);

            return true;
        }
    }
    return false;
}

bool EditContext::GetMatchInfo(LanguageSupport::Match & match, bool bPartial, bool bIsBrace)
{
    _CHECK_ALL_PTR_
    
    bool bRet;
    LanguageSupportPtr ls = m_pStorage->GetLanguageSupport();

    if (ls.get())
    {
        int line = m_curPos.line, 
            offset = pos2inx(m_curPos.line, m_curPos.column);

        ls->FindMatch(line, offset, match, false);

        if (offset > 0 && (!match.found || match.partial) && (! bIsBrace))
        {
            LanguageSupport::Match match2;
            ls->FindMatch(line, offset-1, match2, false); // step back and try again
            
            if (bPartial)
            {
                if (match2.found && !match2.partial)
                    match = match2;
            }
            else
            {
                if (! match2.partial)
                    match = match2;
            }
        }

        bRet = true;
    }
    else
        bRet = false;

    return bRet;
}

void EditContext::FindMatch (bool select)
{
/*    _CHECK_ALL_PTR_
    
    LanguageSupportPtr ls = m_pStorage->GetLanguageSupport();

    if (ls.get())
    {
        int line = m_curPos.line, 
            offset = pos2inx(m_curPos.line, m_curPos.column);

        LanguageSupport::Match match;
        ls->FindMatch(line, offset, match);

        if (offset > 0 && (!match.found || match.partial))
        {
            LanguageSupport::Match match2;
            ls->FindMatch(line, offset-1, match2); // step back and try again
            
            if (match2.found && !match2.partial)
                match = match2;
        }
*/
    _CHECK_ALL_PTR_
    LanguageSupport::Match match;
    LanguageSupportPtr ls = m_pStorage->GetLanguageSupport();


    if (GetMatchInfo(match, true))
    {
        if (match.found)
        {
            Position pos;
            pos.line   = match.line[1];
            pos.column = (match.line[1] < m_pStorage->GetLineCount()) ? inx2pos(match.line[1], match.offset[1]) : match.offset[1];
            MoveTo(pos);

            if (select && !match.broken)
            {
                Square sqr;
                GetSelection(sqr);

                if (match.partial)
                {
                    LanguageSupport::Match match2;

                    if (ls->FindMatch(match.line[1], match.offset[1], match2))
                    {
                        sqr.start.line   = match2.line[1];
                        sqr.start.column = inx2pos(match2.line[1], match2.offset[1]);
                    }
                }
                else if (sqr.is_empty()) // adjust a original cursor position to a start token position
                {
                    if (!match.backward) 
                    {
                        sqr.start.line   = match.line[0];
                        sqr.start.column = inx2pos(match.line[0], match.offset[0]);
                    }
                    else
                    {
                        sqr.end.line   = match.line[0];
                        sqr.end.column = inx2pos(match.line[0], match.offset[0] + match.length[0]);
                    }
                }

                if (match.backward) 
                {
                    sqr.start.line   = match.line[1];
                    sqr.start.column = inx2pos(match.line[1], match.offset[1]);
                }
                else
                {
                    sqr.end.line   = match.line[1];
                    sqr.end.column = inx2pos(match.line[1], match.offset[1] + match.length[1]);
                }
                SetSelection(sqr);
                MoveTo(sqr.end);
            }
            
            if (match.broken) 
                MessageBeep((UINT)-1);

            Global::SetStatusText(!match.broken ? "Match found." : "Match brocken.");
        }
        else
            Global::SetStatusText("Match not found.");
    }
}

};



