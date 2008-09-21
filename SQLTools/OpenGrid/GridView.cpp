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
/*      Purpose: Grid implementation                                       */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-2005 Aleksey Kochetov                                     */
/***************************************************************************/

// 27.10.2003 bug fix, export settings do not affect on quick html/csv viewer launch
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
// 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
// 16.12.2004 (Ken Clubok) Add CSV prefix option

#pragma message("All source operation through grid manager only")

#include "stdafx.h"
#include <list>
#include <fstream>
#include <sstream>
#include "GridManager.h"
#include "GridView.h"
#include "GridSourceBase.h"
#include "Dlg/PropGridOutputPage.h"
#include "COMMON/TempFilesManager.h"
#include "COMMON\AppUtilities.h"

// the following section is included only for OnGridSettings
#include "COMMON/VisualAttributesPage.h"
#include "Dlg/PropGridPage.h"
#include "Dlg/PropGridOutputPage.h"
#include "Dlg/PropHistoryPage.h"
#include "COMMON/PropertySheetMem.h"

#include "SQLWorksheet/SQLWorksheetDoc.h" // it's bad because of dependency on CPLSWorksheetDoc


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2 /* OG = OpenGrid V2 */
{

    UINT GridView::m_uWheelScrollLines; // cached value for MS Weel support
    CPopupFrameWnd* GridView::pTextPopupWnd = 0;
    static const char g_szClassName[] = "Kochware.OpenGrid.V2";

    IMPLEMENT_DYNCREATE(GridView, CView)
    IMPLEMENT_DYNCREATE(StrGridView, GridView)


StrGridView::StrGridView ()
{
    m_pStrSource = new GridStringSource;
    m_pManager = new GridManager(this, m_pStrSource, TRUE);
    m_pManager->m_Options.m_Editing = true;
    m_ShouldDeleteManager = true;
}

void StrGridView::Clear ()
{
    m_pStrSource->DeleteAll();
    m_pManager->Clear();
}

GridView::GridView()
: m_Erase(FALSE),
  m_pManager(0),
  m_ShouldDeleteManager(FALSE),
  m_toolbarNavigationDir(edVert)
{
    GetSQLToolsSettings().AddSubscriber(this);
}

GridView::~GridView()
{
    try { EXCEPTION_FRAME;

        GetSQLToolsSettings().RemoveSubscriber(this);
        
        if (m_ShouldDeleteManager) 
            delete m_pManager;
    }
    _DESTRUCTOR_HANDLER_;
}

bool GridView::IsEmpty () const
{
    _ASSERTE(m_pManager && GetGridSource());
    return GetGridSource()->IsEmpty();
}

void GridView::InitOutputOptions ()
{
    _ASSERTE(m_pManager && GetGridSource());

#pragma warning (disable : 4189) // because of compiler failure
    GridStringSource* source = dynamic_cast<GridStringSource*>(GetGridSource());
    const SQLToolsSettings& setting = GetSQLToolsSettings();

    switch (setting.GetGridExpFormat())
    {
    case 0: source->SetOutputFormat(etfPlainText);           break;
    case 1: source->SetOutputFormat(etfQuotaDelimited);      break;
    case 2: source->SetOutputFormat(etfTabDelimitedText);    break;
    case 3: source->SetOutputFormat(etfXmlElem);             break;
    case 4: source->SetOutputFormat(etfXmlAttr);             break;
    case 5: source->SetOutputFormat(etfHtml);                break;
    default: _ASSERTE(0);
    }

    source->SetFieldDelimiterChar (setting.GetGridExpCommaChar()[0]);
    source->SetQuoteChar          (setting.GetGridExpQuoteChar()[0]);
    source->SetQuoteEscapeChar    (setting.GetGridExpQuoteEscapeChar()[0]);
	source->SetPrefixChar         (setting.GetGridExpPrefixChar()[0]);
    source->SetOutputWithHeader   (setting.GetGridExpWithHeader());
    source->SetColumnNameAsAttr   (setting.GetGridExpColumnNameAsAttr());
}

BEGIN_MESSAGE_MAP(GridView, CView)
	//{{AFX_MSG_MAP(GridView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_GETDLGCODE()
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditGroup)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditGroup)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditGroup)
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_GETFONT, OnGetFont)
	ON_WM_CONTEXTMENU()
    ON_WM_SETTINGCHANGE()
    ON_WM_INITMENUPOPUP()

	ON_COMMAND(ID_GRID_POPUP, OnGridPopup)
    ON_COMMAND(ID_GRID_OUTPUT_OPTIONS, OnGridOutputOptions)
	ON_COMMAND(ID_GRID_OUTPUT_SAVE, OnFileSave)
    ON_COMMAND(ID_GRID_OUTPUT_SAVE_AND_OPEN, OnGridOutputSaveAndOpen)

    ON_COMMAND(ID_GRID_COPY_COL_HEADER, OnGridCopyColHeader)
    ON_COMMAND(ID_GRID_COPY_HEADERS, OnGridCopyHeaders)
    ON_COMMAND(ID_GRID_COPY_ROW, OnGridCopyRow)
    ON_COMMAND(ID_GRID_COPY_ALL, OnGridCopyAll)
    ON_COMMAND(ID_GRID_COPY_COL, OnGridCopyCol)

    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_SAVE, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_SAVE_AND_OPEN, OnUpdateEditGroup)

	ON_UPDATE_COMMAND_UI(ID_GRID_COPY_ALL, OnUpdateEditGroup)
	ON_UPDATE_COMMAND_UI(ID_GRID_COPY_COL_HEADER, OnUpdateEditGroup)
	ON_UPDATE_COMMAND_UI(ID_GRID_COPY_HEADERS, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_ROW, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_COPY_COL, OnUpdateEditGroup)

    ON_COMMAND(ID_GRID_OUTPUT_OPEN, OnGridOutputOpen)
    ON_UPDATE_COMMAND_UI(ID_GRID_OUTPUT_OPEN, OnUpdateEditGroup)
    ON_WM_SYSCOLORCHANGE()

    ON_COMMAND(ID_GRID_MOVE_UP,      OnMoveUp)     
    ON_COMMAND(ID_GRID_MOVE_DOWN,    OnMoveDown)   
    ON_COMMAND(ID_GRID_MOVE_PGUP,    OnMovePgUp)   
    ON_COMMAND(ID_GRID_MOVE_PGDOWN,  OnMovePgDown) 
    ON_COMMAND(ID_GRID_MOVE_END,     OnMoveEnd)    
    ON_COMMAND(ID_GRID_MOVE_HOME,    OnMoveHome)   

    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_UP,     OnUpdate_MoveUp    )     
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_DOWN,   OnUpdate_MoveDown  )   
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_PGUP,   OnUpdate_MovePgUp  )   
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_PGDOWN, OnUpdate_MovePgDown) 
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_END,    OnUpdate_MoveEnd   )    
    ON_UPDATE_COMMAND_UI(ID_GRID_MOVE_HOME,   OnUpdate_MoveHome  )   

    ON_COMMAND(ID_GRID_CLEAR, OnGridClear)
    ON_UPDATE_COMMAND_UI(ID_GRID_CLEAR, OnUpdate_GridClear)

    ON_COMMAND(ID_GRID_ROTATE, OnRotate)   
    ON_UPDATE_COMMAND_UI(ID_GRID_ROTATE, OnUpdate_Rotate)

    ON_COMMAND(ID_GRID_SETTINGS, OnGridSettings)
    ON_UPDATE_COMMAND_UI(ID_GRID_SETTINGS, OnUpdate_GridSettings)

    ON_COMMAND(ID_GRID_OPEN_WITH_IE, OnGridOpenWithIE)
    ON_COMMAND(ID_GRID_OPEN_WITH_EXCEL, OnGridOpenWithExcel)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_IE, OnUpdateEditGroup)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_EXCEL, OnUpdateEditGroup)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GridView message handlers

BOOL GridView::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wndClass;
	BOOL bRes = CView::PreCreateWindow(cs);
	HINSTANCE hInstance = AfxGetInstanceHandle();

	// see if the class already exists
	if (!::GetClassInfo(hInstance, g_szClassName, &wndClass))
    {
		// get default stuff
		::GetClassInfo(hInstance, cs.lpszClass, &wndClass);
		wndClass.lpszClassName = g_szClassName;
		wndClass.style &= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wndClass.hbrBackground = 0;
		// register a new class
		if (!AfxRegisterClass(&wndClass))
			AfxThrowResourceException();
	}
    cs.lpszClass = g_szClassName;

	return bRes;
}

LRESULT GridView::OnGetFont (WPARAM, LPARAM)
{
    if (m_paintAccessories.get() && (HFONT)m_paintAccessories->m_Font)
    {
        return (LRESULT)(HFONT)m_paintAccessories->m_Font;
    }
    else if (GetParent())
    {
        return GetParent()->SendMessage(WM_GETFONT, 0, 0L);
    }
    else
        return (LRESULT)::GetStockObject(DEFAULT_GUI_FONT);
}

void GridView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CView::OnPrepareDC(pDC, pInfo);

    if (m_paintAccessories.get() && (HFONT)m_paintAccessories->m_Font)
    {
        pDC->SelectObject(&m_paintAccessories->m_Font);
    }
    else if (GetParent())
    {
        HFONT hfont = (HFONT)GetParent()->SendMessage(WM_GETFONT, 0, 0L);
        if (hfont)
            pDC->SelectObject(hfont);
    }
    else
        pDC->SelectObject(CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT)));
}

int GridView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (m_pManager == 0 || CView::OnCreate(lpCreateStruct) == -1)
		return -1;

    m_paintAccessories = CGridViewPaintAccessories::GetPaintAccessories(GetVisualAttributeSetName());
    m_paintAccessories->OnSettingsChanged(GetSQLToolsSettings().GetVASet(GetVisualAttributeSetName()));

    CRect rect;
    GetClientRect(rect);

    TEXTMETRIC tm;
    CClientDC dc(this);
    OnPrepareDC(&dc, 0);
    dc.GetTextMetrics(&tm);

    CSize size((2 * tm.tmAveCharWidth + tm.tmMaxCharWidth) / 3, tm.tmHeight);
    m_pManager->EvOpen(rect, size);

    SetTimer(1, 100, NULL);

    return 0;
}

void GridView::OnDestroy()
{
    m_pManager->EvClose();

    KillTimer(1);

	CView::OnDestroy();
}

void GridView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && (nType == SIZE_MAXIMIZED || nType == SIZE_RESTORED))
    {
        m_pManager->EvSize(CSize(cx, cy));
        Invalidate(TRUE);
        m_Erase = TRUE;
    }
}

void GridView::OnHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar*)
{
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
    SCROLLINFO scrollInfo;
	scrollInfo.cbSize = sizeof(scrollInfo);
	scrollInfo.fMask = SIF_TRACKPOS;
	GetScrollInfo(SB_HORZ, &scrollInfo);

    m_pManager->EvScroll(edHorz, nSBCode, scrollInfo.nTrackPos);
}

void GridView::OnVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar*)
{
// 16.02.2002 bug fix, exception on scrolling if grid contains more then 32K rows
    SCROLLINFO scrollInfo;
	scrollInfo.cbSize = sizeof(scrollInfo);
	scrollInfo.fMask = SIF_TRACKPOS;
	GetScrollInfo(SB_VERT, &scrollInfo);

    m_pManager->EvScroll(edVert, nSBCode, scrollInfo.nTrackPos);
}

UINT GridView::OnGetDlgCode()
{
    return ((!m_pManager->m_Options.m_Tabs) ? 0 : DLGC_WANTTAB)
             | DLGC_WANTARROWS | DLGC_WANTCHARS
	         | CView::OnGetDlgCode();
}

HBRUSH GridView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_EDIT
    || nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetBkColor(RGB(255,255,255));
        return (HBRUSH)::GetStockObject(WHITE_BRUSH);
    }
    else
        return CView::OnCtlColor(pDC, pWnd, nCtlColor);
}

void GridView::OnPaint()
{
	CPaintDC dc(this);
    OnPrepareDC(&dc, 0);

    m_pManager->Paint(dc, (m_Erase && dc.m_ps.fErase), dc.m_ps.rcPaint);

    m_Erase = FALSE;
}

void GridView::OnSetFocus(CWnd* pOldWnd)
{
    m_pManager->EvSetFocus(TRUE);

    CView::OnSetFocus(pOldWnd);
    
    SendMessage(WM_NCACTIVATE, TRUE);

    if (GetParent())
        GetParent()->SendNotifyMessage(GetDlgCtrlID(), 1, (LPARAM)this);
}

void GridView::OnKillFocus(CWnd* pNewWnd)
{
    m_pManager->EvSetFocus(FALSE);

	CView::OnKillFocus(pNewWnd);

    SendMessage(WM_NCACTIVATE, FALSE);
    
    if (GetParent())
        GetParent()->SendNotifyMessage(GetDlgCtrlID(), 2, (LPARAM)this);
}

void GridView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (!m_pManager->EvKeyDown(nChar, nRepCnt, nFlags))
	    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void GridView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    m_pManager->EvLButtonDblClk(nFlags, point);
}

void GridView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (this != GetFocus())
        SetFocus();

    m_pManager->EvLButtonDown(nFlags, point);

    if (!m_pManager->ResizingProcess()
    && GetGridSource())            // Drag & Drop
    {
        InitOutputOptions();

        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();

        if (m_pManager->IsFixedCorner(point))
            GetGridSource()->DoDragDrop(row, col, ecDragTopLeftCorner);
        else if (m_pManager->IsFixedCol(point.x) && GetGridSource()->GetCount(edVert) > row)
            GetGridSource()->DoDragDrop(row, col, ecDragRowHeader);
        else if (m_pManager->IsFixedRow(point.y))
            GetGridSource()->DoDragDrop(row, col, ecDragColumnHeader);
        else if (m_pManager->IsDataCell(point))
            GetGridSource()->DoDragDrop(row, col, ecDataCell);
    }
}

void GridView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CView::OnLButtonUp(nFlags, point);

    m_pManager->EvLButtonUp(nFlags, point);
}

void GridView::OnMouseMove(UINT nFlags, CPoint point)
{
    m_pManager->EvMouseMove(nFlags, point);

    //if (m_pManager
    //&& !m_pManager->ResizingProcess()
    ///*&& m_pManager->m_Options.m_Tooltips*/)
    //{
    //    {
    //        if (!m_wndToolTip.m_hWnd)
    //        {
    //            m_wndToolTip.Create(this);

    //            TOOLINFO ti;
    //            memset(&ti, 0, sizeof(TOOLINFO));
    //            ti.cbSize   = sizeof(TOOLINFO);
    //            ti.uFlags   = TTF_IDISHWND;
    //            ti.hwnd     = m_hWnd;
    //            ti.hinst    = AfxGetApp()->m_hInstance;
    //            ti.uId      = UINT(m_hWnd);
    //            ti.lpszText = "";
    //            ::SendMessage(m_wndToolTip.m_hWnd, TTM_ADDTOOL, 0, LPARAM(LPTOOLINFO(&ti)));
    //        }

    //        const char* pszLegend = NULL;

    //        if (m_pManager->IsFixedCorner(point))
    //            pszLegend = "You can drag && drop it to get the list of columns.";
    //        else if (m_pManager->IsFixedRow(point.y))
    //            pszLegend = "You can drag && drop it to get the column name.";
    //        else if (m_pManager->IsFixedCol(point.x))
    //            pszLegend = "You can drag && drop it to get the whole row.";

    //        TOOLINFO ti;
    //        memset(&ti, 0, sizeof(TOOLINFO));
    //        ti.cbSize = sizeof(TOOLINFO);
    //        ti.uFlags = TTF_IDISHWND;
    //        ti.hwnd = m_hWnd;
    //        ti.hinst = AfxGetApp()->m_hInstance;
    //        ti.uId = UINT(m_hWnd);
    //        ti.lpszText = const_cast<char*>(pszLegend);
    //        ::SendMessage(m_wndToolTip.m_hWnd, TTM_UPDATETIPTEXT, 0, LPARAM(LPTOOLINFO(&ti)));

	   //     MSG msg;
	   //     msg.hwnd    = m_hWnd;
	   //     msg.message = WM_MOUSEMOVE;
	   //     msg.wParam = (WPARAM)nFlags;
	   //     msg.lParam = (LPARAM)&point;

	   //     m_wndToolTip.RelayEvent(&msg);
    //    }
    //    //else if (m_wndToolTip.m_hWnd)
    //    //    m_wndToolTip.Pop();
    //}

    CView::OnMouseMove(nFlags, point);
}

void GridView::OnTimer (UINT)
{
    m_pManager->IdleAction();
}

void GridView::OnSettingsChanged ()
{
    m_paintAccessories = CGridViewPaintAccessories::GetPaintAccessories(GetVisualAttributeSetName());
    m_paintAccessories->OnSettingsChanged(GetSQLToolsSettings().GetVASet(GetVisualAttributeSetName()));

    TEXTMETRIC tm;
    CClientDC dc(this);
    OnPrepareDC(&dc, 0);
    dc.GetTextMetrics(&tm);

    CSize size((2 * tm.tmAveCharWidth + tm.tmMaxCharWidth) / 3, tm.tmHeight);
    m_pManager->EvFontChanged(size);
}

LRESULT GridView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CView::WindowProc(message, wParam, lParam); 
    }
    _DEFAULT_HANDLER_;
    return 0;
}

BOOL GridView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    try { EXCEPTION_FRAME;

        CWnd* pInplaceEditor = m_pManager->GetEditor();

	    if (pInplaceEditor != NULL && pInplaceEditor->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		    return TRUE;

	    // then pump through frame
	    if (CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		    return TRUE;

	    return FALSE;
    }
    _DEFAULT_HANDLER_;

	return TRUE;
}

void GridView::OnContextMenu (CWnd*, CPoint pos)
{
    if (GetGridSource()) 
    {
        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_GRID_POPUP));
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, this);
    }
}

void GridView::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0)
    {
        pPopupMenu->EnableMenuItem(ID_GRID_POPUP,                MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_EDIT_COPY,                 MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_COL_HEADER,      MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_ROW,             MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_ALL,             MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_SAVE,          MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_OPEN,          MF_BYCOMMAND|MF_ENABLED);
        pPopupMenu->EnableMenuItem(ID_GRID_OUTPUT_SAVE_AND_OPEN, MF_BYCOMMAND|MF_ENABLED);
    }

    if (GetGridSource())
    {
        pPopupMenu->EnableMenuItem(ID_GRID_COPY_HEADERS,         MF_BYCOMMAND|MF_ENABLED);
    }
}

void GridView::OnGridPopup ()
{
    if (! pTextPopupWnd)
    {
        pTextPopupWnd = new CPopupFrameWnd;

        DWORD Style;
        CRect Rect;

        SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
        
        Rect.left += 100;
        Rect.top += 100;
        Rect.right = (Rect.right >> 1);
        Rect.bottom = (Rect.bottom >> 1);
        
        Style = WS_POPUPWINDOW|WS_CAPTION|WS_THICKFRAME/*|WS_VISIBLE*/;
        Style |= WS_MAXIMIZEBOX|WS_MINIMIZEBOX;
        
        const char* pszClassName;

        pszClassName = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
          ::LoadCursor(NULL, IDC_ARROW),
          (HBRUSH) ::GetStockObject(WHITE_BRUSH),
          AfxGetApp()->LoadIcon(IDR_MAINFRAME));

        char MsgBuf[255];
        DWORD nError = ::GetLastError();

        if (nError != 0)
        {
            ::FormatMessage( 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                nError,
                0, // Default language
                MsgBuf,
                sizeof(MsgBuf),
                NULL 
            );
            // Process any inserts in lpMsgBuf.
            // ...
            // Display the string.
            // ::MessageBox( NULL, MsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
        }

        if (!pTextPopupWnd->CreateEx(0, pszClassName, "Popup Editor", Style, Rect, this, 0)) 
        {
	        MessageBox("The Popup Frame window was not created");
	        // return -1;
	    }
    }

    if (pTextPopupWnd)
    {
        if (GetGridSource())
        {
            int row = m_pManager->m_Rulers[edVert].GetCurrent();
            int col = m_pManager->m_Rulers[edHorz].GetCurrent();

            string theText = ((GridStringSource *) GetGridSource())->GetCellStr(row, col);

            CString theReplaceText(theText.c_str());

            theReplaceText.Replace("\r\n", "\n");

            theReplaceText.Replace("\n", "\r\n");

            theText = theReplaceText;

            pTextPopupWnd->SetPopupText(theText);
            pTextPopupWnd->ShowWindow(SW_SHOW);
            pTextPopupWnd->SetFocus();
        }
    }
}

void GridView::OnEditCopy ()
{
    if (GetGridSource()
    && OpenClipboard())
    {
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();

        GetGridSource()->DoEditCopy(row, col, ecDataCell);
        CloseClipboard();
    }
}

void GridView::OnEditPaste ()
{
    //GetGridSource()->DoEditPaste();
}

void GridView::OnEditCut ()
{
    //GetGridSource()->DoEditCut();
}

void GridView::OnUpdateEditGroup(CCmdUI* pCmdUI)
{
    if ((!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0) || 
    (GetGridSource() || pCmdUI->m_nID == ID_GRID_COPY_HEADERS))
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);
}


void GridView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (this != GetFocus())
        SetFocus();

    m_pManager->EvLButtonDown(nFlags, point);

    CView::OnRButtonDown(nFlags, point);
}

void GridView::OnFileSave ()
{
    DoFileSave(FALSE);
}

void GridView::OnGridOutputSaveAndOpen()
{
    DoFileSave(TRUE);
}

void GridView::DoFileSave (BOOL open)
{
    if (GetGridSource())
    {
        InitOutputOptions();

        const char* filter = 0;
        const char* extension = 0;
        static int  g_last_format = -1;
        static CString g_FileName = "Result";

        int format = GetSQLToolsSettings().GetGridExpFormat();
        switch (format)
        {
        case 0:
            filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";
            extension = "txt";
            break;
        case 1:
            filter = "CSV Files (Comma delimited) (*.csv)|*.csv|All Files (*.*)|*.*||";
            extension = "csv";
            break;
        case 2:
            filter = "Text Files (Tab delimited) (*.txt)|*.txt|All Files (*.*)|*.*||";
            extension = "txt";
            break;
        case 3:
        case 4:
            filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*||";
            extension = "xml";
            break;
        case 5:
            filter = "HTML Files (*.html; *.htm)|*.html;*.htm|All Files (*.*)|*.*||";
            extension = "html";
            break;
        }

        if (format != g_last_format)
        {
            g_last_format = format;

            int pos = g_FileName.ReverseFind('.');
            
            if (pos != -1)
            {
                g_FileName.GetBufferSetLength(pos);
                g_FileName.ReleaseBuffer();
            }

            g_FileName += '.';
            g_FileName += extension;
        }

        // 02.06.2003 bug fix, export in file does not work on NT4
#if _MFC_VER > 0x0600
        CFileDialog dial(FALSE, extension, g_FileName,
                        OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ENABLESIZING,
                        filter, this, 0);
#else
        CFileDialog dial(FALSE, extension, g_FileName,
                        OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_ENABLESIZING,
                        filter, this);
#endif

        if (dial.DoModal() == IDOK)
        {
            g_FileName = dial.GetPathName();
			std::ofstream of(g_FileName);
            GetGridSource()->ExportText(of);
            of.close();

            if (open)
            {
                HINSTANCE result = ShellExecute( NULL, "open", g_FileName, NULL, NULL, SW_SHOW);
                if((UINT)result <= HINSTANCE_ERROR)
                {
                    MessageBeep((UINT)-1);
                    AfxMessageBox("Cannot open a default editor.");
                }
            }
        }
    }
}

BOOL GridView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (!m_uWheelScrollLines)
	    ::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &m_uWheelScrollLines, 0);

	if (m_uWheelScrollLines == WHEEL_PAGESCROLL)
	{
        OnVScroll(zDelta > 0 ? SB_PAGEDOWN : SB_PAGEUP , 0, 0);
	}
    else
    {
		int nToScroll = ::MulDiv(zDelta, m_uWheelScrollLines, WHEEL_DELTA);

        if (zDelta > 0)
            while (nToScroll--)
                OnVScroll(SB_LINELEFT, 0, 0);
        else
            while (nToScroll++)
                OnVScroll(SB_LINERIGHT, 0, 0);
    }

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void GridView::OnSettingChange (UINT, LPCTSTR)
{
    m_uWheelScrollLines = 0;
}

void GridView::OnGridOutputOptions ()
{
    SQLToolsSettings settings = GetSQLToolsSettings();
    
    CPropGridOutputPage gridOutputPage(settings);

    static UINT gStarPage = 0;
    Common::CPropertySheetMem sheet("Settings", gStarPage);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    sheet.AddPage(&gridOutputPage);

    if (sheet.DoModal() == IDOK)
    {
        GetSQLToolsSettingsForUpdate() = settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
    }
}

void GridView::OnGridCopyAll ()
{
    if (GetGridSource() && OpenClipboard())
    {
        InitOutputOptions();
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();
        GetGridSource()->DoEditCopy(row, col, ecEverything);
        CloseClipboard();
    }
}

void GridView::OnGridCopyColHeader()
{
    if (GetGridSource() && OpenClipboard())
    {
        InitOutputOptions();
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();
        GetGridSource()->DoEditCopy(row, col, ecColumnHeader);
        CloseClipboard();
    }
}

void GridView::OnGridCopyHeaders()
{
    if (GetGridSource() && OpenClipboard())
    {
        InitOutputOptions();
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();
        GetGridSource()->DoEditCopy(row, col, ecFieldNames);
        CloseClipboard();
    }
}

void GridView::OnGridCopyRow()
{
    if (GetGridSource() && OpenClipboard())
    {
        InitOutputOptions();
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();
        
        // it's temporary - it has to be reimplementerd!!!
        bool orgWithHeader = false;
        const GridStringSource* source = dynamic_cast<const GridStringSource*>(GetGridSource());
        if (source) 
        {
            orgWithHeader = source->GetOutputWithHeader();
            source->SetOutputWithHeader(false);
        }

        GetGridSource()->DoEditCopy(row, col, ecRow);
        CloseClipboard();

        // it's temporary - it has to be reimplementerd!!!
        if (source) 
        {
            source->SetOutputWithHeader(orgWithHeader);
        }
    }
}

void GridView::OnGridCopyCol()
{
    if (GetGridSource() && OpenClipboard())
    {
        InitOutputOptions();
        int row = m_pManager->m_Rulers[edVert].GetCurrent();
        int col = m_pManager->m_Rulers[edHorz].GetCurrent();
        GetGridSource()->DoEditCopy(row, col, ecColumn);
        CloseClipboard();
    }
}

void GridView::OnGridOutputOpen()
{
    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0)
    {
        POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();

        if (CDocTemplate* pDocTemplate = AfxGetApp()->GetNextDocTemplate(pos))
        {
            if (CDocument* doc = pDocTemplate->OpenDocumentFile(NULL))
            {
                InitOutputOptions();
				std::ostringstream of;
                GetGridSource()->ExportText(of);
                string text = of.str();

				CMemFile mf((BYTE*)text.c_str(), text.length());
                CArchive ar(&mf, CArchive::load);
				doc->Serialize(ar);

                // it's bad because of dependency on CPLSWorksheetDoc
                ASSERT_KINDOF(CPLSWorksheetDoc, doc);
                CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)doc;

                // 03.06.2003 bug fix, "Open In Editor" does not set document type
                // 29.06.2003 bug fix, "Open In Editor" fails on comma/tab delimited formats
                const char* name = "Text";

                switch (GetSQLToolsSettings().GetGridExpFormat())
                {
                case 0:
                    name = "PL/SQL";
                    break;
                case 1:
                case 2:
                    name = "Text";
                    break;
                case 3:
                case 4:
                    name = "XML";
                    break;
                }

                pDoc->SetClassSetting(name);
                // 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
                pDoc->SetSaveAsToDefault();
            }
        }
    }
}

void GridView::OnSysColorChange()
{
    CView::OnSysColorChange();

    m_pManager->InitColors();
}


void GridView::OnMoveUp ()
{
    if (!IsEmpty())
        m_pManager->MoveToLeft(m_toolbarNavigationDir);
}

void GridView::OnMoveDown ()
{
    if (!IsEmpty())
        m_pManager->MoveToRight(m_toolbarNavigationDir);
}

void GridView::OnMovePgUp ()
{
    if (!IsEmpty())
        m_pManager->MoveToPageUp(m_toolbarNavigationDir);
}

void GridView::OnMovePgDown ()
{
    if (!IsEmpty())
        m_pManager->MoveToPageDown(m_toolbarNavigationDir);
}

void GridView::OnMoveEnd ()
{
    if (!IsEmpty())
        m_pManager->MoveToEnd(m_toolbarNavigationDir);
}

void GridView::OnMoveHome ()
{
    if (!IsEmpty())
        m_pManager->MoveToHome(m_toolbarNavigationDir);
}

void GridView::OnUpdate_MoveUp (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnUpdate_MoveDown (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MovePgUp (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnUpdate_MovePgDown (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MoveEnd (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveDown());
}

void GridView::OnUpdate_MoveHome (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(CanMoveUp());
}

void GridView::OnRotate ()
{
    if (m_pManager->m_Options.m_Rotation && !IsEmpty())
    {
        if (GridSourceBase* source = dynamic_cast<GridSourceBase*>(GetGridSource()))
        {
            source->SetOrientation(source->GetOrientation() == eoVert ? eoHorz : eoVert);
            m_pManager->m_Options.m_ColSizingAsOne = source->GetOrientation() == eoHorz;
            m_pManager->Rotate();

            m_toolbarNavigationDir = source->GetOrientation() == eoVert ? edVert : edHorz;

            if (m_pManager->m_Options.m_ColSizingAsOne)
            {
                int sizeAsOne = m_pManager->m_Rulers[edHorz].GetScrollSize() / 2 - 1;
                m_pManager->m_Rulers[edHorz].SetSizeAsOne(sizeAsOne);
                m_pManager->RecalcRuller(edHorz, true/*adjust*/, true/*invalidate*/);
            }
        }
    }
}

void GridView::OnUpdate_Rotate (CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pManager->m_Options.m_Rotation && !IsEmpty());
}


void GridView::OnGridClear()
{
    GetGridSource()->DeleteAll();
    m_pManager->Clear();
}

void GridView::OnUpdate_GridClear(CCmdUI* pCmdUI)
{
    if (!IsEmpty()
    && GetGridSource()
    && GetGridSourceCount(edHorz) > 0
    && GetGridSourceCount(edVert) > 0
    && GetVisualAttributeSetName() != "Statistics Window"
    && GetVisualAttributeSetName() != "Bind Window") {
        pCmdUI->Enable(true);
    } else {
        pCmdUI->Enable(false);
    }
}

void GridView::OnGridSettings()
{
    SQLToolsSettings settings = GetSQLToolsSettings();
    
    CPropGridPage1   gridPage1(settings);
    CPropGridPage2   gridPage2(settings);
    CPropGridOutputPage gridOutputPage(settings);
    CPropHistoryPage histPage(settings);

    std::vector<VisualAttributesSet*> vasets;
    vasets.push_back(&settings.GetQueryVASet());
    vasets.push_back(&settings.GetStatVASet());
    vasets.push_back(&settings.GetHistoryVASet());
    vasets.push_back(&settings.GetOutputVASet());
    CVisualAttributesPage vaPage(vasets);
    vaPage.m_psp.pszTitle = "SQLTools::Font and Color";
    vaPage.m_psp.dwFlags |= PSP_USETITLE;
    
    static UINT gStarPage = 0;
    Common::CPropertySheetMem sheet("Grid settings", gStarPage);
    sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);

    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    sheet.AddPage(&gridPage1);
    sheet.AddPage(&gridPage2);
    sheet.AddPage(&gridOutputPage);
    sheet.AddPage(&histPage);
    sheet.AddPage(&vaPage);

    int retVal = sheet.DoModal();

    if (retVal == IDOK)
    {
        GetSQLToolsSettingsForUpdate() = settings;
        GetSQLToolsSettingsForUpdate().NotifySettingsChanged();
    }
}

void GridView::OnUpdate_GridSettings (CCmdUI* pCmdUI)
{
    pCmdUI->Enable();
}

void GridView::DoOpen (ETextFormat frm, const char* ext)
{
    if (GridStringSource* source = dynamic_cast<GridStringSource*>(GetGridSource()))
    {
        // 27.10.2003 bug fix, export settings do not affect on quick html/csv viewer launch
        InitOutputOptions();
        ETextFormat orgTextFormat = source->GetOutputFormat();
        source->SetOutputFormat(frm);

        string file = TempFilesManager::CreateFile(ext);

        if (!file.empty())
        {
            std::ofstream of(file.c_str());

            GetGridSource()->ExportText(of);

            of.close();

            Common::AppShellOpenFile(file.c_str());
        }
        else
        {
                MessageBeep((UINT)-1);
                AfxMessageBox("Cannot generate temporary file name for export.", MB_OK|MB_ICONSTOP);
        }

        source->SetOutputFormat(orgTextFormat);
    }
}

void GridView::OnGridOpenWithIE ()
{
    DoOpen(etfHtml, "HTM");
}

void GridView::OnGridOpenWithExcel ()
{
    DoOpen(etfQuotaDelimited, "CSV");
}

}//namespace OG2



