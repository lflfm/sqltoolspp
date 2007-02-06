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

#ifndef __DBSOURCEWND_H__
#define __DBSOURCEWND_H__
#pragma once

#include <OCI8/Connect.h>
#include "DbObjListCtrl.h"


    class SQLToolsSettings;

class CDbSourceWnd : public CWnd
{
    static CPtrList m_thisDialogs;
// Construction
public:
	CDbSourceWnd (OciConnect& connect);

// Attributes
public:
    OciConnect& m_connect;
    CImageList m_Images;
    CMenu m_menuOther, m_menuOptions;
    CDbObjListCtrl* m_wndTabLists[19];
    int m_nItems, m_nSelItems;

    CDbObjListCtrl* GetCurSel () const;

// Dialog Data
	//{{AFX_DATA(CDbSourceWnd)
	enum { IDD = IDD_DB_SOURCE };
	CTabCtrl	m_wndTab;
	BOOL	m_bValid;
	BOOL	m_bInvalid;
	CString	m_strSchema;
	CString	m_strFilter;
	int		m_nViewAs;
	//}}AFX_DATA
	CComboBox	m_wndSchemaList;
	CComboBox	m_wndFilterList;
	CStatic     m_StaticSchema;
	CStatic     m_StaticFilter;
	BOOL m_AllSchemas;
    BOOL m_bShowTabTitles;

// Operations
public:
    BOOL IsVisible () const; 
    BOOL Create (CWnd* pFrameWnd);
    void UpdateSchemaList ();
    void DirtyObjectsLists ();

    void EvOnConnect ();
    void EvOnDisconnect ();
    static void EvOnConnectAll ();
    static void EvOnDisconnectAll ();
    void OnLoad (bool bAsOne);

    struct CDoContext 
    {
        int m_nRow;
        int m_nItem;
        const char* m_szOwner;
        const char* m_szName;
        const char* m_szTable; // for constraint only
        const char* m_szType;
        void* m_pvParam;
        void* m_pvParam2;
        void* m_pvParam3;
        CAbortController* m_pAbortCtrl;

        CDoContext () { memset(this, 0, sizeof(*this)); }
    };

    void Do (bool (CDbSourceWnd::*pmfnDo)(CDoContext&), void* = 0, void* = 0, void* = 0, CAbortController* = 0);

    bool DoLoad        (CDoContext&);
    bool DoDelete      (CDoContext&);
    bool DoAlter       (CDoContext&);
    bool DoCompileBody (CDoContext&);
    bool DoQuery       (CDoContext&);
    bool DoDataDelete  (CDoContext&);
    bool DoTruncate    (CDoContext&);
    bool DoTruncatePre (CDoContext&);

// Overrides
	//{{AFX_VIRTUAL(CDbSourceWnd)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL
    void DelayShow (BOOL bShow);

// Implementation
public:
	virtual ~CDbSourceWnd();
	void OnCopy_Public();

	// Generated message map functions
protected:
    //{{AFX_MSG(CDbSourceWnd)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocusSchema();
    afx_msg void OnDataChanged();
    afx_msg void OnDataChanged2();
    afx_msg void OnRefresh();
    afx_msg void OnDblClikList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLoad();
    afx_msg void OnCopy();
    afx_msg void OnLoadAsOne();
    afx_msg void OnCompile();
    afx_msg void OnDelete();
    afx_msg void OnDisable();
    afx_msg void OnEnable();
    afx_msg void OnShowAsList();
    afx_msg void OnShowAsReport();
    afx_msg void OnDestroy();
    afx_msg void OnSelectAll();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnShowOptions();
    afx_msg void OnSelChangeTab(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnCancel();
	afx_msg void OnRClickOnTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
    afx_msg LRESULT OnInitDialog (WPARAM, LPARAM);
    afx_msg void OnIdleUpdateCmdUI ();
    afx_msg LRESULT OnHelpHitTest(WPARAM, LPARAM lParam);
    afx_msg void OnMenuButtons (UINT);
    afx_msg void OnQuery ();
    afx_msg void OnDataDelete ();
    afx_msg void OnTruncate ();

    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
#endif//__DBSOURCEWND_H__
