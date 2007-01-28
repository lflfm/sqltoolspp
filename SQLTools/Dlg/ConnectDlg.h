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

#ifndef __CONNECTDLG_H__
#define __CONNECTDLG_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog

class CConnectDlg : public CDialog
{
    struct ListEntry 
    {
        CString 
            user, password, tnsAlias, 
            host, tcpPort, sid,
            counter, lastUsage;

        bool directConnection, serviceInsteadOfSid;
        DWORD mode;
        DWORD safety;
    };

    CString m_callbackBuffer;

    std::vector<ListEntry> m_data;

    int m_direction, m_sortColumn, m_current;
    
    CImageList m_sortImages;

    void sortProfiles (int col, int dir);
    void setupConnectionType ();
    void makeTnsString (CString&);

    static void filetime_to_string (FILETIME&, CString&);
    static int CALLBACK comp_proc (LPARAM, LPARAM, LPARAM lparam);

public:
	CConnectDlg (CWnd* pParent = NULL);


	//{{AFX_DATA(CConnectDlg)
	enum { IDD = IDD_CONNECT_DIALOG };
	CListCtrl m_profiles;
	CString	m_user;
	CString	m_password;
	CString	m_tnsAlias;
	bool	m_directConnection;
	CString	m_host;
	bool	m_savePassword;
	CString	m_sid;
	CString	m_tcpPort;
    bool    m_serviceInsteadOfSid;
    int     m_connectionMode;
    int     m_safety;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CConnectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CConnectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnColumnClick_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetDispInfo_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanged_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDataChanged_SavePassword ();
	afx_msg void OnTest();
	afx_msg void OnDelete();
	virtual void OnOK();
	afx_msg void OnDblClk_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClk_DirectConnectin ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClicked_TnsCopy();
    afx_msg void OnBnClicked_Help();
protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}

#endif//__CONNECTDLG_H__
