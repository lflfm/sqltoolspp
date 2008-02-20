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

#ifndef __OCISTRGRIDVIEW_H__
#define __OCISTRGRIDVIEW_H__

#include "GridView.h"

    class OciGridSource;

class OciGridView : public OG2::GridView
{
protected:
	DECLARE_DYNCREATE(OciGridView)
    OciGridSource* m_pOciSource;
    
    CString m_statusText;

    struct StatusCxt {
        int m_curRec, m_numOfRec;
        bool m_allRowsFetched;

        StatusCxt (int curRec, int numOfRec, bool allRowsFetched) 
            : m_curRec(curRec), m_numOfRec(numOfRec), m_allRowsFetched(allRowsFetched) {}
        StatusCxt () 
            : m_curRec(-1), m_numOfRec(-1), m_allRowsFetched(false) {}

        bool operator != (const StatusCxt& other)
            { return m_curRec != other.m_curRec || m_numOfRec != other.m_numOfRec || m_allRowsFetched != other.m_allRowsFetched; }
    } 
    m_statusCxt;

    int m_lastRow;
    bool m_bKeepColSize;

public:
	OciGridView();
	virtual ~OciGridView();

// Operations for SOURCE
    void Clear ();
    void Refresh ();
    void SetCursor (std::auto_ptr<OCI8::AutoCursor>& cursor);
	void ApplyColumnFit (int nItem = -1);
    void ResetLastRow() { m_lastRow = 0;}
    bool GetKeepColSize() { return m_bKeepColSize; }

    int  GetCurrentRecord () const;
    int  GetRecordCount () const;

protected:
	DECLARE_MESSAGE_MAP()

    afx_msg void OnInitMenuPopup (CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnChangeColumnFit (UINT);
    afx_msg void OnUpdate_OciGridColsToHeaders(CCmdUI *pCmdUI);
    afx_msg void OnUpdate_OciGridDataFit(CCmdUI *pCmdUI);
    afx_msg void OnRotate ();   
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

public:
    afx_msg void OnUpdate_OciGridIndicator (CCmdUI* pCmdUI);
};

#endif//__OCISTRGRIDVIEW_H__
