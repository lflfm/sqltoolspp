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
#include "TreeViewer.h"
#include "ObjectTreeBuilder.h"
#include <COMMON\SimpleDragDataSource.h>         // for Drag & Drop
#include <COMMON\ExceptionHelper.h>
#include "MainFrm.h"
#include "OpenEditor/OEView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeViewer

IMPLEMENT_DYNCREATE(CTreeViewer, CTreeCtrlEx)
/*
CTreeViewer::CTreeViewer()
{
}

CTreeViewer::~CTreeViewer()
{
}
*/
BOOL CTreeViewer::Create (CWnd* pFrameWnd)
{
    CRect rect;
    pFrameWnd->GetClientRect(rect);
    return CTreeCtrlEx::Create(WS_CHILD|WS_VISIBLE, rect, pFrameWnd, 1);
}

void CTreeViewer::LoadAndSetImageList (UINT nResId)
{
    m_Images.Create(nResId, 16, 64, RGB(0,255,0));
    SetImageList(&m_Images, TVSIL_NORMAL);
}


BEGIN_MESSAGE_MAP(CTreeViewer, CTreeCtrlEx)
	//{{AFX_MSG_MAP(CTreeViewer)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTreeViewer diagnostics

#ifdef _DEBUG
void CTreeViewer::AssertValid() const
{
	CTreeCtrlEx::AssertValid();
}

void CTreeViewer::Dump(CDumpContext& dc) const
{
	CTreeCtrlEx::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTreeViewer message handlers

void CTreeViewer::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CTreeCtrlEx::OnLButtonDblClk(nFlags, point);

	UINT nHitFlags = 0;
	HTREEITEM hClickedItem = HitTest( point, &nHitFlags );

    HTREEITEM hNextItem = GetChildItem( hClickedItem );

	DWORD type = GetItemData(hClickedItem);

    if ((! hNextItem) && (type == titColumn))
    {
        string sText = GetItemStrippedText(hClickedItem, true);

        COEditorView* pView = ((COEditorView *)((CMDIMainFrame *)AfxGetMainWnd())->GetMyActiveView());
        
        if (pView)
        {
            try { EXCEPTION_FRAME;
                pView->DeleteBlock();
                pView->InsertBlock(sText.c_str(), !pView->GetBlockKeepMarkingAfterDragAndDrop_Public()/*hideSelection*/);
                pView->SetFocus();
            } 
            _DEFAULT_HANDLER_
        }
    }
}

void CTreeViewer::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
    try { EXCEPTION_FRAME;

	    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
        HTREEITEM    hItem = pNMTreeView->itemNew.hItem;

        if (pNMTreeView->action == TVE_EXPAND
        && !GetChildItem(hItem)) 
        {

            ETreeItemType itemType = (ETreeItemType)GetItemData(hItem);
            CString       itemText = GetItemText(hItem);

            TV_ITEM item;
            item.hItem     = hItem;
            item.mask      = TVIF_CHILDREN;

            item.cChildren = AddItemByDynamicDesc(GetApp()->GetConnect(), *this, hItem, itemType, itemText);
            item.cChildren += AddItemByStaticDesc(*this, hItem, itemType);

            SetItem(&item);
        }
    } 
    _DEFAULT_HANDLER_

	*pResult = 0;
}

/*! \fn void CTreeViewer::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
 *
 * \brief Processing dragging into editor from ObjectViewer
 * \param pNMHDR View object.
 * \param pResult We served this?
 *
 * TEST CASE of this functionality:
   CREATE TABLE test (
	"lower singlespace" NUMBER,
	"lower  doublespace" NUMBER,
	"lower   triple_BUG" NUMBER,
	test_lower VARCHAR2(1),
	TEST_UPPER VARCHAR2(1),
	test_MIXED NUMBER,
	"test MIXED" NUMBER
   )
 */

const string CTreeViewer::GetItemStrippedText(HTREEITEM hItem, const bool b_force_alt)
{
	int pos, pos_space;
	CString strText, strTmp;
	bool lower_items = GetSQLToolsSettings().m_bLowerNames;
	bool is_alt_pressed = bool((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) || b_force_alt;
    HTREEITEM hChildItem = hItem;

	if (hItem) {
		DWORD type = GetItemData(hItem);
		if ((type == titTable || type == titView || type == titIndex || type == titConstraint) && ItemHasChildren(hItem) &&
		is_alt_pressed == false ) // 'expand' item only when alt isn't pressed
		{
			// without expanding we have an empty list for never expanded items (for example 'new' Indexes)
			Expand(hItem, TVE_EXPAND);
			// paste column list separated by comma (only if Table)
			hChildItem = GetChildItem(hItem);
		    while (hChildItem != NULL) {
				type = GetItemData(hChildItem);
				// only columns without 'Constraints, Indexes, ...'
				if (type == titColumn) {
					strTmp = GetItemText(hChildItem);
					// find 3 SPACEs and shorten buffer (comment below)
					pos = strTmp.Find("   ");
					if (pos != -1) {
						strTmp.GetBufferSetLength(pos);
						strTmp.ReleaseBuffer();
						pos_space = strTmp.Find(" ");// check existence of single space inside item name
						if (pos_space != -1 && pos_space < pos) {
							strText += ("\"" + strTmp + "\"");// add ""
						} else {// upper/lower only items without ""
							if(lower_items) {
								strTmp.MakeLower();
							} else {
								strTmp.MakeUpper();
							}
							strText += strTmp;
						}
						strText += ",";
					}
				}
				hChildItem = GetNextItem(hChildItem, TVGN_NEXT);
			}
			strText.TrimRight(",");
		} else {
			// paste only a selected item
		    strTmp = GetItemText(hItem);
			// We have the following data:
			// "ITEM_NAME datatype /* Comment on item */", 
			// for example:
			// "CUSTOMER_NAME  VARCHAR2(100) /* Short title of customer */"
			//
			// We must extract only ITEM_NAME by searching SPACE inside this string.
			pos = strTmp.Find("   ");// Quick fix, but not enough! (Watch B#1427749 - Dragging in ObjectViewer partially handle spaces)
			// If we have colum with any spaces we should surround it with "". How to recognize the proper end of item name?
			if (pos != -1)  {
				strTmp.GetBufferSetLength(pos);
				strTmp.ReleaseBuffer();
				pos_space = strTmp.Find(" ");// check existence of single space inside item name
				if (pos_space != -1 && pos_space < pos) {
					strText += ("\"" + strTmp + "\"");// add ""
				} else {// upper/lower only items without ""
					if(lower_items) {
						strTmp.MakeLower();
					} else {
						strTmp.MakeUpper();
					}
					strText += strTmp;
				}
			} else {
				if(lower_items) {
					strTmp.MakeLower();
				} else {
					strTmp.MakeUpper();
				}
				
                strText += strTmp;
			}
		}
	}

	return string(strText);
}

const string CTreeViewer::GetSelectedItemsAsText(const bool b_force_alt)
{
    bool b_schema_name = GetSQLToolsSettings().m_bShemaName;
	bool lower_items = GetSQLToolsSettings().m_bLowerNames;

    HTREEITEM hItem = GetFirstSelectedItem();

    string theText, theDelimiter, theOwner;

    if (b_schema_name)
    {
        theOwner = GetItemText(GetRootItem());

        string::size_type nIndex = theOwner.find('.');

        if (nIndex != string::npos)
        {
            theOwner = theOwner.substr(nIndex + 1) + ".";
            if (lower_items)
                theOwner = CString(theOwner.c_str()).MakeLower();
    	}
        else
            theOwner = "";
	}

	if (hItem) 
    {
        for (; hItem != NULL; hItem = GetNextSelectedItem(hItem))
        {
    		theText += theDelimiter + theOwner + GetItemStrippedText(hItem, b_force_alt);
            theDelimiter = ", ";
		}
	}

    return theText;
}

void CTreeViewer::OnBeginDrag(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	bool is_alt_pressed = bool((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

    string theText = GetSelectedItemsAsText();

    if (theText.size()) {
		// differ drag&drop icon depending on keyboard state
		if(is_alt_pressed == true) {
			Common::SimpleDragDataSource(theText.c_str()).DoDragDrop(DROPEFFECT_LINK);
		} else {
			Common::SimpleDragDataSource(theText.c_str()).DoDragDrop(DROPEFFECT_COPY);
		}
	}
	*pResult = 0;
}

int CTreeViewer::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeCtrlEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	ModifyStyle(0, /*TVS_FULLROWSELECT|*/TVS_HASBUTTONS|TVS_HASLINES, 0);
	
	return 0;
}
