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
#include "COMMON\ListCtrlDataProvider.h"
#include "COMMON\ManagedListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog

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

    class ConnectInfoAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        mutable char m_buffer[128];

        const char* getAliasStr (const ListEntry& entry) const { 
            if (entry.directConnection) {
                strncpy(m_buffer, entry.sid, sizeof m_buffer);
                strncat(m_buffer, ":"         , sizeof m_buffer);
                strncat(m_buffer, entry.host, sizeof m_buffer);
                m_buffer[sizeof(m_buffer)-1] = 0;
                return m_buffer; 
            }
            return entry.tnsAlias;
        }

        static int compAliases (const ListEntry& entry1, 
                                const ListEntry& entry2) { 
            if (int r = comp(entry1.directConnection ? entry1.sid : entry1.tnsAlias, entry2.directConnection ? entry2.sid : entry2.tnsAlias))
                return r;
            return comp(entry1.directConnection ? entry1.host : CString(), entry2.directConnection ? entry2.host : CString());
        }

        const std::vector<ListEntry>& m_entries;

    public:
        ConnectInfoAdapter (const std::vector<ListEntry>& entries) : m_entries(entries) {}

        const ListEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 4; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String;
            case 1: return String;
            case 2: return Number;
            case 3: return String;
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "User";       
            case 1: return "Alias";
            case 2: return "Usage Count";      
            case 3: return "Last usage";    
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).user);
            case 1: return getAliasStr(data(row));
            case 2: return getStr(atoi(data(row).counter));
            case 3: return getStr(data(row).lastUsage);
            }
            return "Unknown";
        }

        bool IsVisibleRow (int /*row*/) const {
            return true;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).user, data(row2).user);
            case 1: return compAliases(data(row1), data(row2));
            case 2: return comp(atoi(data(row1).counter), atoi(data(row2).counter));
            case 3: return comp(data(row1).lastUsage, data(row2).lastUsage);
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 40 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }
    };

class CConnectDlg : public CDialog
{
    CString m_callbackBuffer;

    std::vector<ListEntry> m_data;

    int m_sortColumn, m_current;

    Common::ListCtrlManager::ESortDir m_direction;
    
    void setupConnectionType ();
    void makeTnsString (CString&);

    static void filetime_to_string (FILETIME&, CString&);
    void writeProfileListConfig(void);

public:
	CConnectDlg (CWnd* pParent = NULL);


	//{{AFX_DATA(CConnectDlg)
	enum { IDD = IDD_CONNECT_DIALOG };
    ConnectInfoAdapter m_adapter;
    Common::CManagedListCtrl m_profiles;
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
    Common::ListCtrlManager::FilterCollection m_filter;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CConnectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CConnectDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnGetDispInfo_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanged_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDataChanged_SavePassword ();
	afx_msg void OnTest();
	afx_msg void OnDelete();
	virtual void OnOK();
    virtual void OnCancel();
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
