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

/*
    14.08.2002 bug fix, Find/Replace dialog - garbage in combo box history
    25.09.2002 bug fix, Find/Replace dialog - [:xxxx:] exps have been replaced
    16.12.2002 bug fix, regexp replace fails on \1,...
    16.12.2002 bug fix, Find/Replace dialog auto position
    21.03.2003 bug fix, missing entry in fing what/replace with histoty
    26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
    26.05.2003 bug fix, Find/Replace dialog: "Transform backslash expressions" has beed added to handle \t\b\ddd...
    01.06.2004 bug fix, Find/Replace dialog: replace skips empty lines, e.g. "^$"
    30.06.2004 improvement/bug fix, text search/replace interface has been changed
    17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report
    30.03.2005 (ak) bug fix, hanging on trying to replace 'a' with 'aa' in the current selection 
*/

#include "stdafx.h"
#include <algorithm>
#include <COMMON/AppGlobal.h>
#include <COMMON/AppUtilities.h>
#include <COMMON/ExceptionHelper.h>
#include <COMMON/StrHelpers.h>
#include "OpenEditor/OEFindReplaceDlg.h"
#include "OpenEditor/OEView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OpenEditor;

/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDlg dialog

CPoint CFindReplaceDlg::m_ptTopLeft(-1,-1);
BOOL CFindReplaceDlg::m_MessageOnEOF;

CFindReplaceDlg::CFindReplaceDlg(BOOL replace, COEditorView* pView)
: CDialog(!replace ? IDD_OE_EDIT_FIND : IDD_OE_EDIT_REPLACE, NULL)
{
    m_pView          = pView;
    m_ReplaceMode    = replace;
    m_Modified       = TRUE;
	//{{AFX_DATA_INIT(CFindReplaceDlg)
	//}}AFX_DATA_INIT

    if (pView)
    {
        // 26.05.2003 bug fix, Find/Replace dialog: "Transform backslash expressions" has beed added to handle \t\b\ddd...
        m_BackslashExpressions = AfxGetApp()->GetProfileInt("Editor", "BackslashExpressions", FALSE);

        string buff;
        toPrintableStr(m_pView->GetSearchText(), buff);
        m_SearchText = buff.c_str();

        /*
        bool backward, wholeWords, matchCase, regExpr, searchAll;
        pView->GetSearchOption(backward, wholeWords, matchCase, regExpr, searchAll);

        m_MatchCase      = matchCase;
        m_MatchWholeWord = wholeWords;
        m_AllWindows     = searchAll;
        m_RegExp         = regExpr;
        m_Direction      = backward ? 0 : 1;
        */
        m_MatchCase      = AfxGetApp()->GetProfileInt("Editor", "SearchMatchCase",      FALSE);
        m_MatchWholeWord = AfxGetApp()->GetProfileInt("Editor", "SearchMatchWholeWord", FALSE);
        m_AllWindows     = AfxGetApp()->GetProfileInt("Editor", "SearchAllWindows",     FALSE);
        m_RegExp         = AfxGetApp()->GetProfileInt("Editor", "SearchRegExp",         FALSE);
        m_MessageOnEOF   = AfxGetApp()->GetProfileInt("Editor", "MessageOnEOF",         FALSE);
        // 26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
        m_Direction      = m_ReplaceMode ? 1 : AfxGetApp()->GetProfileInt("Editor", "SearchDirection", 1);

        m_WhereReplace   = 1;
    }
    else
        _ASSERTE(0);
}


void CFindReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindReplaceDlg)
	//}}AFX_DATA_MAP
	DDX_CBString(pDX, IDC_EF_SEARCH_TEXT, m_SearchText);
	DDX_Control(pDX, IDC_EF_SEARCH_TEXT, m_wndSearchText);

    if (m_ReplaceMode)
    {
	    DDX_CBString(pDX, IDC_EF_REPLACE_TEXT, m_ReplaceText);
  	    DDX_Control(pDX, IDC_EF_REPLACE_TEXT, m_wndReplaceText);
	    DDX_Radio(pDX, IDC_EF_REPLACE_IN_SELECTION, m_WhereReplace);
    }
    else
    {
	    DDX_Radio(pDX, IDC_EF_UP, m_Direction);
        DDX_Check(pDX, IDC_EF_MSG_BOX_ON_EOF, m_MessageOnEOF);
    }

    DDX_Check(pDX, IDC_EF_MATCH_CASE, m_MatchCase);
	DDX_Check(pDX, IDC_EF_MATCH_WHOLE_WORD, m_MatchWholeWord);
	DDX_Check(pDX, IDC_EF_ALL_WINDOWS, m_AllWindows);
	DDX_Check(pDX, IDC_EF_REGEXP, m_RegExp);
    DDX_Check(pDX, IDC_EF_TRANSFORM_BACKSLASH_EXPR, m_BackslashExpressions);
}


BEGIN_MESSAGE_MAP(CFindReplaceDlg, CDialog)
	//{{AFX_MSG_MAP(CFindReplaceDlg)
	ON_BN_CLICKED(IDC_EF_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_EF_REGEXP, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_ALL_WINDOWS, OnAllWindows)
	ON_CBN_EDITCHANGE(IDC_EF_SEARCH_TEXT, OnSearchText)
	ON_CBN_SELCHANGE(IDC_EF_SEARCH_TEXT, OnSearchText)
	ON_CBN_EDITCHANGE(IDC_EF_REPLACE_TEXT, OnChangeSettings)
	ON_CBN_SELCHANGE(IDC_EF_REPLACE_TEXT, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_REPLACE, OnReplace)
	ON_BN_CLICKED(IDC_EF_REPLACE_ALL, OnReplaceAll)
	ON_BN_CLICKED(IDC_EF_MATCH_CASE, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_MATCH_WHOLE_WORD, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_UP, OnChangeSettings)
	ON_BN_CLICKED(IDC_EF_DOWN, OnChangeSettings)
    ON_BN_CLICKED(IDC_EF_REPLACE_IN_SELECTION, OnReplaceWhere)
	ON_BN_CLICKED(IDC_EF_REPLACE_IN_WHOLE_FILE, OnReplaceWhere)
	ON_BN_CLICKED(IDC_EF_COUNT, OnCount)
	ON_BN_CLICKED(IDC_EF_MARK_ALL, OnMarkAll)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_EF_REGEXP_FIND, OnBnClicked_RegexpFind)
    ON_BN_CLICKED(IDC_EF_REGEXP_REPLACE, OnBnClicked_RegexpReplace)
    ON_COMMAND_RANGE(ID_REGEXP_FIND_TAB, ID_REGEXP_FIND_QUOTED, OnInsertRegexpFind)
    ON_COMMAND_RANGE(ID_REGEXP_REPLACE_WHAT_FIND, ID_REGEXP_REPLACE_TAGEXP_9, OnInsertRegexpReplace)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDlg message handlers

void CFindReplaceDlg::toPrintableStr (const char* from, string& _to)
{
    if (m_BackslashExpressions)
        Common::to_printable_str(from, _to);
    else
        _to = from;
}

void CFindReplaceDlg::toUnprintableStr (const char* from, string& _to, bool skipEscDgt)
{
    if (m_BackslashExpressions)
        Common::to_unprintable_str(from, _to, skipEscDgt);
    else
        _to = from;
}

void CFindReplaceDlg::AdjustPosition ()
{
    if (m_pView)
    {
        CPoint pt;
        m_pView->GetCaretPosition(pt);
        m_pView->ClientToScreen(&pt);

        CRect rc;
        GetWindowRect(&rc);

        if (pt.y >= rc.top && pt.y <= rc.bottom)
        {
            CRect frmRc, newRc(rc);
            AfxGetMainWnd()->GetClientRect(&frmRc);
            AfxGetMainWnd()->ClientToScreen(&frmRc);

            if ((rc.top + rc.bottom) / 2 < (frmRc.top + frmRc.bottom) / 2) // top window half
            {
                newRc.bottom = frmRc.bottom;
                newRc.top    = newRc.bottom - rc.Height();
            }
            else // bottom window half
            {
                newRc.top    = frmRc.top;
                newRc.bottom = newRc.top + rc.Height();
            }
            MoveWindow(newRc, TRUE);
        }
    }
}

void CFindReplaceDlg::SaveOption ()
{
    if (m_Modified)
    {

        Common::AppSaveHistory(m_wndSearchText, "Editor", "SearchText", 10);
        // 21.03.2003 bug fix, missing entry in fing what/replace with histoty
        Common::AppRestoreHistory(m_wndSearchText, "Editor", "SearchText", 10);

        if (m_ReplaceMode)
        {
            Common::AppSaveHistory(m_wndReplaceText, "Editor", "ReplaceText", 10);
            // 21.03.2003 bug fix, missing entry in fing what/replace with histoty
            Common::AppRestoreHistory(m_wndReplaceText, "Editor", "ReplaceText", 10);
        }

        if (m_pView)
        {
            string buff;
            toUnprintableStr(m_SearchText, buff, false);
            m_pView->SetSearchText(buff.c_str());
            m_pView->SetSearchOption(m_Direction == 0 ? true : false,
                                     m_MatchWholeWord ? true : false,
                                     m_MatchCase      ? true : false,
                                     m_RegExp         ? true : false,
                                     m_AllWindows     ? true : false);

            AfxGetApp()->WriteProfileInt("Editor", "SearchMatchCase",      m_MatchCase     );
            AfxGetApp()->WriteProfileInt("Editor", "SearchMatchWholeWord", m_MatchWholeWord);
            AfxGetApp()->WriteProfileInt("Editor", "SearchAllWindows",     m_AllWindows    );
            AfxGetApp()->WriteProfileInt("Editor", "SearchRegExp",         m_RegExp        );
            AfxGetApp()->WriteProfileInt("Editor", "MessageOnEOF",         m_MessageOnEOF  );
            // 26.05.2003 bug fix, Find/Replace dialog: a replace mode depends on a search direction
            if (!m_ReplaceMode)
                AfxGetApp()->WriteProfileInt("Editor", "SearchDirection", m_Direction);

            AfxGetApp()->WriteProfileInt("Editor", "BackslashExpressions", m_BackslashExpressions);
        }

        m_Modified = FALSE;
    }
}

void CFindReplaceDlg::OnFindNext()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        UpdateData();
        SaveOption();

        if (m_pView && !m_SearchText.IsEmpty())
        {
            COEditorView* pView;
            if (m_pView->RepeadSearch(esdDefault, pView))
            {
                m_pView = pView;
                AdjustPosition();

                if (!m_ReplaceMode)
                    OnOK();
            }
        }
    }
    _OE_DEFAULT_HANDLER_;
}

void CFindReplaceDlg::OnReplace ()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        UpdateData();
        SaveOption();

        Square blk;

        if (m_pView)
        {
            m_pView->GetSelection(blk);
            blk.normalize();

            // 01.06.2004 bug fix, Find/Replace dialog: replace skips empty lines, e.g. "^$"
            if (/*!blk.is_empty() &&*/ blk.start.line == blk.end.line)
            {
                FindCtx ctx;
                // 16.12.2002 bug fix, regexp replace fails on \1,...
                toUnprintableStr(m_ReplaceText, ctx.m_text, m_RegExp ? true : false /*skipEscDgt*/);
                ctx.m_line  = blk.start.line;
                ctx.m_start = blk.start.column;
                ctx.m_end   = blk.end.column;
                m_pView->Replace(ctx);
            }
        }

        OnFindNext();
    }
    _OE_DEFAULT_HANDLER_;
}

void CFindReplaceDlg::OnReplaceAll ()
{
    SearchBatch(esbReplace);
}

void CFindReplaceDlg::OnCount ()
{
    SearchBatch(esbCount);
}

void CFindReplaceDlg::OnMarkAll ()
{
    SearchBatch(esbMark);
}

void CFindReplaceDlg::SearchBatch (OpenEditor::ESearchBatch mode)
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        int counter = 0;

        UpdateData();
        SaveOption();

        if (m_pView)
        {
            // replace in file(s) or count/mark
            if (m_WhereReplace == 1 || mode == esbCount || mode == esbMark )
            {
                if (!m_AllWindows || mode == esbCount || mode == esbMark
                || AfxMessageBox("All occurances will be replaced for all windows! Do You confirm?", MB_OKCANCEL|MB_ICONWARNING) == IDOK)
                {
                    string buff;
                    toUnprintableStr(m_ReplaceText, buff, false);
                    counter = m_pView->SearchBatch(buff.c_str(), mode);
                }
            }
            else // replace in selection
            {
                Square blk;
                m_pView->GetSelection(blk);
                Position pos = m_pView->GetPosition();

                bool _backward = m_pView->IsBackwardSearch();
                m_pView->SetBackwardSearch(false);

                try
                {
                    Square blk;
                    m_pView->GetSelection(blk);

                    if (!blk.is_empty())
                    {
                        blk.normalize();
                        switch (m_pView->GetBlockMode())
                        {
                        default:
                            THROW_APP_EXCEPTION("Unsupported block type for counting or replacing.");
                        case ebtStream:
                            ;
                        }

                        const COEditorView* pView = 0;

                        COEditorView::UndoGroup undoGroup(*m_pView);
                        Position pos = m_pView->GetPosition();
                        m_pView->PushInUndoStack(pos);

                        FindCtx ctx;
                        ctx.m_line = blk.start.line;
                        ctx.m_start = blk.start.column;
                        ctx.m_end = blk.end.column;
                        ctx.m_skipFirstNull = false;
                        // 27.05.2002 bug fix, regexp replace fails on \1,... for ReplaceAll
                        toUnprintableStr(m_ReplaceText, ctx.m_text, m_RegExp ? true : false /*skipEscDgt*/);

                        while (m_pView->Find(pView, ctx)
                        && !ctx.m_eofPassed && pView == m_pView)
                        {
                            if ((ctx.m_line >= blk.start.line && ctx.m_line < blk.end.line)
                            || (ctx.m_line == blk.end.line && ctx.m_end <= blk.end.column))
                            {
                                if (mode == esbReplace)
                                {
                                    m_pView->Replace(ctx);
                                    // 30.03.2005 (ak) bug fix, hanging on trying to replace 'a' with 'aa' in the current selection 
                                    ctx.m_start = ctx.m_end;
                                }
                                counter++;
                            }
                            else
                                break;
                        }
                    }
                }
                catch (...)
                {
                    m_pView->SetBackwardSearch(_backward);
                    throw;
                }

                m_pView->SetBackwardSearch(_backward);
                m_pView->MoveTo(pos);
                m_pView->SetSelection(blk);
            }

            if (mode == esbReplace && counter)
                AdjustPosition();
        }

        char buff[100];
        itoa(counter, buff, 10);

        switch (mode)
        {
        case esbReplace:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been replaced!"));
            break;
        case esbMark:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been marked!"));
            break;
        case esbCount:
            Global::SetStatusText(strcat(buff, " occurrence(s) have been found!"));
            MessageBox(buff, "Find");
            break;
        }
    }
    _OE_DEFAULT_HANDLER_;
}

BOOL CFindReplaceDlg::OnInitDialog ()
{
    BOOL enableWhereReplace =
        (!m_pView->IsSelectionEmpty()
         && m_pView->GetBlockMode() != ebtColumn
         && !m_AllWindows) ? TRUE : FALSE;

    Square blk;
    m_pView->GetSelection(blk);
    m_WhereReplace = (enableWhereReplace
        && blk.start.line != blk.end.line) ? 0 : 1;

    CDialog::OnInitDialog();

    Common::AppRestoreHistory(m_wndSearchText, "Editor", "SearchText", 10);

    if (!m_SearchText.IsEmpty())
    {
        // 14/08/2002 bug fix, Find/Replace dialog - garbage in combo box history
        m_wndSearchText.SetWindowText(m_SearchText);
    }

    if (m_ReplaceMode)
    {
        Common::AppRestoreHistory(m_wndReplaceText, "Editor", "ReplaceText", 10);

        GetDlgItem(IDC_EF_REPLACE_IN_SELECTION)->EnableWindow(enableWhereReplace);
    }

    SetupButtons();

    if (m_ptTopLeft.x != -1 && m_ptTopLeft.y != -1)
    {
        CRect rc;
        GetWindowRect(&rc);
#if _MFC_VER > 0x0600
        rc.MoveToXY(m_ptTopLeft);
#else
	rc.bottom = rc.Height() + m_ptTopLeft.y; rc.top  = m_ptTopLeft.y;
	rc.right  = rc.Width()  + m_ptTopLeft.x; rc.left = m_ptTopLeft.x;
#endif
        MoveWindow(rc);
    }
    AdjustPosition();

    if (!m_edtSearchText.SubclassComboBoxEdit(m_wndSearchText.m_hWnd))
        // hide button for Win95
        ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

    if (m_wndReplaceText.m_hWnd)
    {
        if (!m_edtReplaceText.SubclassComboBoxEdit(m_wndReplaceText.m_hWnd))
            // hide button for Win95
            ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
    }

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), m_RegExp);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), m_RegExp);

    return TRUE;
}

void CFindReplaceDlg::OnChangeSettings ()
{
    UpdateData();
    m_Modified = TRUE;
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_FIND), m_RegExp);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EF_REGEXP_REPLACE), m_RegExp);
}

void CFindReplaceDlg::OnAllWindows ()
{
    OnChangeSettings();

    if (m_ReplaceMode)
    {
        GetDlgItem(IDC_EF_REPLACE_IN_SELECTION)
            ->EnableWindow(!m_pView->IsSelectionEmpty()
                           && m_pView->GetBlockMode() != ebtColumn
                           && !m_AllWindows);

        if (m_AllWindows && m_WhereReplace == 0)
        {
            m_WhereReplace = 1;
            UpdateData(FALSE);
        }
    }

    SetupButtons();
}

void CFindReplaceDlg::OnReplaceWhere ()
{
    OnChangeSettings();
    SetupButtons();
}

void CFindReplaceDlg::OnSearchText ()
{
    OnChangeSettings();
    SetupButtons();
}

void CFindReplaceDlg::SetupButtons ()
{
    // it works not precisely but maybe later...
    BOOL hasSearchText = m_wndSearchText.GetWindowTextLength() > 0
        || m_wndSearchText.GetCurSel() != -1 && m_wndSearchText.GetLBTextLen(m_wndSearchText.GetCurSel());

    if (!m_ReplaceMode)
    {
        GetDlgItem(IDC_EF_MARK_ALL)->EnableWindow(hasSearchText);
        GetDlgItem(IDC_EF_FIND_NEXT)->EnableWindow(hasSearchText);
        GetDlgItem(IDC_EF_COUNT)->EnableWindow(hasSearchText);
    }
    else
    {
        GetDlgItem(IDC_EF_FIND_NEXT)->EnableWindow(m_WhereReplace && hasSearchText);
        GetDlgItem(IDC_EF_REPLACE)->EnableWindow(m_WhereReplace && hasSearchText);
        GetDlgItem(IDC_EF_REPLACE_ALL)->EnableWindow(hasSearchText);
    }
}
void CFindReplaceDlg::OnBnClicked_RegexpFind()
{
    ShowPopupMenu(IDC_EF_REGEXP_FIND, IDR_OE_REGEXP_FIND_POPUP);
}

void CFindReplaceDlg::OnBnClicked_RegexpReplace()
{
    ShowPopupMenu(IDC_EF_REGEXP_REPLACE, IDR_OE_REGEXP_REPLACE_POPUP);
}

void CFindReplaceDlg::ShowPopupMenu (UINT btnId, UINT menuId)
{
    HWND hButton = ::GetDlgItem(m_hWnd, btnId);

    _ASSERTE(hButton);

    if (hButton
    && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED)
    {
        ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

        CRect rect;
        ::GetWindowRect(hButton, &rect);

        CMenu menu;
        VERIFY(menu.LoadMenu(menuId));

        CMenu* pPopup = menu.GetSubMenu(0);
        _ASSERTE(pPopup != NULL);

        TPMPARAMS param;
        param.cbSize = sizeof param;
        param.rcExclude.left   = 0;
        param.rcExclude.top    = rect.top;
        param.rcExclude.right  = USHRT_MAX;
        param.rcExclude.bottom = rect.bottom;

        if (::TrackPopupMenuEx(*pPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               rect.left, rect.bottom + 1, *this, &param))
        {
		      MSG msg;
		      ::PeekMessage(&msg, hButton, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
        }

        ::PostMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0L);
    }
}


static DWORD start, end;

void CFindReplaceDlg::OnInsertRegexpFind (UINT nID)
{
    if (nID > ID_REGEXP_FIND_TAB)
        ::SendMessage(::GetDlgItem(m_hWnd, IDC_EF_REGEXP), BM_SETCHECK, BST_CHECKED, 0L);

    static
    struct { const char* text; int offset; }
        expr[] = {
            { "\\t", -1 },
            { ".",  -1 },
            { "*",  -1 },
            { "+",  -1 },
            { "^",  -1 },
            { "$",  -1 },
            { "[]",  1 },
            { "[^]", 2 },
            { "|",  -1 },
            { "\\", -1 },
            { "([a-zA-Z0-9])",              -1 },
            { "([a-zA-Z])",                 -1 },
            { "([0-9])",                    -1 },
            { "([0-9a-fA-F]+)",             -1 },
            { "([a-zA-Z_$][a-zA-Z0-9_$]*)", -1 },
            { "(([0-9]+\\.[0-9]*)|([0-9]*\\.[0-9]+)|([0-9]+))", -1 },
            { "((\"[^\"]*\")|('[^']*'))",   -1 },
        };

    _ASSERTE((nID - ID_REGEXP_FIND_TAB) < (sizeof(expr)/sizeof(expr[0])));

    if (((nID - ID_REGEXP_FIND_TAB) < (sizeof(expr)/sizeof(expr[0]))))
    {
        m_edtSearchText.InsertAtCurPos(expr[nID - ID_REGEXP_FIND_TAB].text, expr[nID - ID_REGEXP_FIND_TAB].offset);
        OnChangeSettings();
        SetupButtons();
    }
}

void CFindReplaceDlg::OnInsertRegexpReplace (UINT nID)
{
    ::SendMessage(::GetDlgItem(m_hWnd, IDC_EF_REGEXP), BM_SETCHECK, BST_CHECKED, 0L);

    static
    const char* expr[]
        = { "\\0", "\\1", "\\2", "\\3", "\\4", "\\5", "\\6", "\\7", "\\8", "\\9" };

    _ASSERTE((nID - ID_REGEXP_REPLACE_WHAT_FIND) < (sizeof(expr)/sizeof(expr[0])));

    if (((nID - ID_REGEXP_REPLACE_WHAT_FIND) < (sizeof(expr)/sizeof(expr[0]))))
    {
        m_edtReplaceText.InsertAtCurPos(expr[nID - ID_REGEXP_REPLACE_WHAT_FIND], -1);
        OnChangeSettings();
        SetupButtons();
    }

}

void CFindReplaceDlg::OnCancel()
{
    CRect rc;
    GetWindowRect(&rc);
    m_ptTopLeft = rc.TopLeft();

    CDialog::OnCancel();
}

void CFindReplaceDlg::OnOK()
{
    CRect rc;
    GetWindowRect(&rc);
    m_ptTopLeft = rc.TopLeft();

    CDialog::OnOK();
}

LRESULT CFindReplaceDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}