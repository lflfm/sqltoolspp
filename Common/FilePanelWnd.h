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

#pragma once
#ifndef __FilePanelWnd_h__
#define __FilePanelWnd_h__


#include "Common/FakeRClick.h"
#include "DirTreeCtrl/DirTreeCtrl.h"


class CFilePanelWnd : public CTabCtrl
{
	DECLARE_DYNAMIC(CFilePanelWnd)

public:
	CFilePanelWnd ();
	virtual ~CFilePanelWnd ();

    CListCtrl&  GetOpenFilesListCtrl () { return m_openFilesList; }
    CImageList& GetSysImageList() { return m_explorerTree.GetSysImageList(); }

    void OpenFiles_Append (LVITEM&);
    void OpenFiles_UpdateByParam (LPARAM param, LVITEM&);
    void OpenFiles_RemoveByParam (LPARAM param);
    void OpenFiles_ActivateByParam (LPARAM param);
    LPARAM OpenFiles_GetCurSelParam ();

    const CString& GetCurDrivePath () const;
    void  SetCurDrivePath (const CString&);
    BOOL  SetCurPath (const CString&);

protected:
    int OpenFiles_FindByParam (LPARAM param);

protected:
    BOOL         m_isExplorerInitialized;
    BOOL         m_isDrivesInitialized;
    CString      m_curDrivePath;

    CImageList   m_explorerStateImageList;
    CListCtrl    m_openFilesList;
    CDirTreeCtrl m_explorerTree;
    CComboBoxEx  m_drivesCBox, m_filterCBox;
    CFakeRClick  m_drivesRClick, m_filterRClick;
    CStringArray m_driverPaths;

    void SelectDrive (const CString&, BOOL force = FALSE);
    void DisplayDrivers (BOOL force = FALSE, BOOL curOnly = FALSE);
    void ChangeTab (int);
    void ActivateOpenFile ();

protected:
	DECLARE_MESSAGE_MAP()

public:
    afx_msg int  OnCreate (LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy ();
    afx_msg void OnSize (UINT nType, int cx, int cy);
    afx_msg void OnTab_SelChange (NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDrive_SetFocus ();
    afx_msg void OnDrive_SelChange ();
    afx_msg void OnOpenFiles_Click (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnOpenFiles_KeyDown (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnOpenFiles_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_DblClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnExplorerTree_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrivers_RClick (NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnFpwOpen();
    afx_msg void OnFpwRefresh();
    afx_msg void OnFpwSetWorDir();
    afx_msg void OnFpwRefreshDrivers();
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

inline
const CString& CFilePanelWnd::GetCurDrivePath () const
    { return m_curDrivePath; }

inline
void CFilePanelWnd::SetCurDrivePath (const CString& curDrivePath)
    { SelectDrive(curDrivePath); }

#endif//__FilePanelWnd_h__