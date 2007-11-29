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

#ifndef __DBOBJLISTCTRL_H__
#define __DBOBJLISTCTRL_H__
#pragma once

#include <COMMON\RecArray.h>
#include <OCI8/Connect.h>
#include "COMMON\ListCtrlDataProvider.h"
#include "COMMON\ManagedListCtrl.h"

#define IDII_FUNCTION     0
#define IDII_PROCEDURE    1
#define IDII_PACKAGE      2
#define IDII_PACKAGE_BODY 3
#define IDII_TRIGGER      4
#define IDII_VIEW         5
#define IDII_TABLE        6
#define IDII_SEQUENCE     7
#define IDII_SYNONYM      8
#define IDII_GRANTEE      9
#define IDII_CLUSTER     10
#define IDII_DBLINK       11
#define IDII_SNAPSHOT     12
#define IDII_SNAPSHOT_LOG 13
#define IDII_TYPE         14
#define IDII_TYPE_BODY    15
#define IDII_INDEX        16
#define IDII_CHK_CONSTRAINT 17
#define IDII_PK_CONSTRAINT  18
#define IDII_UK_CONSTRAINT  19
#define IDII_FK_CONSTRAINT  20
#define IDII_RECYCLEBIN   21

class CDbSourceWnd;

    struct CData 
    {
        OCI8::EServerVersion m_MinVer;
        
        int m_nImageId;

        const char* m_szType;
        const char* m_szTitle;
        const char* m_szSqlStatement;
        const char* m_szSqlStatement73;
        const char* m_szRefreshWhere;
        const char* m_szFilterColumn;
        const char* m_szStatusColumn;
        const char* m_szStatusParam;
        const char* m_szEnabledColumn;

        bool m_bCanCompile, 
             m_bCanDisable, 
             m_bCanDrop;

        int  m_nColumns;

        struct CColumn 
        {
            const char* m_szName;
            const char* m_szTitle;

            int m_szWidth, m_dwAttr;

            Common::ListCtrlDataProvider::Type m_colType;

            // CColumn() { m_colType = Common::ListCtrlDataProvider::String; }

        } *m_pColumns;
    };

    class ObjListInfoAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const Common::RecArray& m_entries;
        const CData& m_objListDef;

    public:
        ObjListInfoAdapter(const Common::RecArray& entries, const CData & objListDef) 
            : m_entries(entries), m_objListDef(objListDef) {}

        virtual int getRowCount () const { return m_entries.GetRows(); }

        virtual int getColCount () const { return m_entries.GetColumns(); }

        virtual Type getColType (int col) const {
            return m_objListDef.m_pColumns[col].m_colType;
        }

        virtual const char* getColHeader (int col) const {
            return m_objListDef.m_pColumns[col].m_szTitle;
        }

        virtual const char* getString (int row, int col) const {
            return m_entries.GetCellStr(row, col);
        }

        bool IsVisibleRow (int /*row*/) const {
            return true;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (m_objListDef.m_pColumns[col].m_colType) {
            case String: return comp(m_entries.GetCellStr(row1, col), m_entries.GetCellStr(row2, col));
            case Number: return comp(atoi(m_entries.GetCellStr(row1, col)), atoi(m_entries.GetCellStr(row2, col)));
            case Date:   return comp(m_entries.GetCellStr(row1, col), m_entries.GetCellStr(row2, col));
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 40 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return m_objListDef.m_nImageId; }

        static DWORD get_item_state (const BYTE state[2])
        {
            DWORD retVal = 0;
            if (state[0] && !state[1])
            {
                retVal = 1;
            }
            else if (!state[0] && state[1])
            {
                retVal = 2;
            }
            else if (state[0] && state[1])
            {
                retVal = 3;
            }
            return retVal;
        }

        virtual int getStateImageIndex (int row) const {
            return get_item_state(m_entries.GetRowData(row));
        }
    };


    class CDbObjListCtrl : public Common::CManagedListCtrl
{
    friend CDbSourceWnd;
    static int CALLBACK CompProc (LPARAM nRow1, LPARAM nRow2, LPARAM lparam);

public:

    static const CData  sm_TableData, 
                        sm_ChkConstraintData, 
                        sm_PkConstraintData, 
                        sm_UkConstraintData, 
                        sm_FkConstraintData, 
                        sm_IndexData, 
                        sm_ViewData, 
                        sm_SequenceData,
                        sm_FunctionData, 
                        sm_ProcedureData, 
                        sm_PackageData, 
                        sm_PackageBodyData,
                        sm_TriggerData, 
                        sm_SynonymData, 
                        sm_GranteeData,
                        sm_ClusterData,
                        sm_DbLinkData,
                        sm_SnapshotData,
                        sm_SnapshotLogData,
                        sm_TypeData,
                        sm_TypeBodyData,
                        sm_RecyclebinData;

private:
  const CData& m_Data;
  bool m_bDirty;
  bool m_bValid, m_bInvalid;
  int  m_nStatusColumn,
       m_nEnabledColumn,
       m_nSortColumn, 
       m_nDirection;

  CDWordArray m_mapColumns;
  Common::RecArray m_arrObjects;
  CImageList m_Images, m_StateImages;
  ObjListInfoAdapter m_adapter;
  
  OciConnect& m_connect;

  void SetStyle(void);

// Construction
public:
    CDbObjListCtrl (OciConnect& connect, const CData& data);

// Attributes
public:

// Operations
public:
  bool IsDirty () const               { return m_bDirty; }
  void Dirty ()                       { m_bDirty = true; }

  void Refresh      (const CString& strSchema, const CString& strFilter, 
                     bool bValid, bool bInvalid, bool bForce = false);
  void RefreshList  (bool bValid, bool bInvalid);
  void ExecuteQuery (const CString& strSchema, const CString& strFilter);
  void RefreshRow   (int nItem, int nRow, const char* szOwner, const char* szName);
  void ApplyQuickFilter (bool valid, bool invalid);

  string GetListSelectionAsText();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDbObjListCtrl)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDbObjListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDbObjListCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSelectAll();
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
#endif//__DBOBJLISTCTRL_H__
