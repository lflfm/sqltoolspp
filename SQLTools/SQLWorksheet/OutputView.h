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

#ifndef __OUTPUTVIEW_H__
#define __OUTPUTVIEW_H__
#pragma once

#include <OpenGrid/GridView.h>
#include <COMMON/QuickArray.h>

    class CSQLToolsApp;

class COutputView : public OG2::StrGridView
{
	DECLARE_DYNCREATE(COutputView)

public:
	COutputView();

    CSQLToolsApp* GetApp () const    { return (CSQLToolsApp*)AfxGetApp(); }

    void PutLine (int type, const char* str, int line, int pos, OpenEditor::LineId lineId);
    void PutError (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId());
    void PutMessage (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId());
    void PutDbmsMessage (const char* str, int line =-1, int pos =-1, OpenEditor::LineId lineId = OpenEditor::LineId());
    void Refresh ();
    void Clear ();

    bool IsEmpty () const;
    void FirstError (bool onlyError = false);
    bool NextError (bool onlyError = false);
    bool PrevError (bool onlyError = false);
    bool GetCurrError (int& line, int& pos, OpenEditor::LineId& lineId);

	//{{AFX_VIRTUAL(COutputView)
	protected:
	//}}AFX_VIRTUAL

protected:
	virtual ~COutputView();

protected:
	//{{AFX_MSG(COutputView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditGroup(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnEditClearAll();
	DECLARE_MESSAGE_MAP()

private:
    const string& COutputView::getString (int col);

    Common::QuickArray<OpenEditor::LineId> m_lineIDs;
    bool m_NoMoreError;
    static CImageList m_imageList;
public:
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

    inline
    void COutputView::PutError (const char* str, int line, int pos, OpenEditor::LineId lineId)
    {
        PutLine(4, str, line, pos, lineId);
    }
    inline
    void COutputView::PutMessage (const char* str, int line, int pos, OpenEditor::LineId lineId)
    {
        PutLine(2, str, line, pos, lineId);
    }
    inline
    void COutputView::PutDbmsMessage (const char* str, int line, int pos, OpenEditor::LineId lineId)
    {
        PutLine(5, str, line, pos, lineId);
    }
//{{AFX_INSERT_LOCATION}}

#endif//__OUTPUTVIEW_H__
