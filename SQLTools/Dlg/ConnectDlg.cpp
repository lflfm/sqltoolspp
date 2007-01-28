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

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog
//      1/02/2002 - Connection dialog has been re implemented 
//                  for supporting non-tnsnames connection strings. 
//                  It's slightly over engineering because of compatibility issue.
//

/*
    27.03.2003 bug fix, a leak of registry key handles - it's a very critical bug for Win95/Win98/WinMe
    16.02.2004 bug fix, a connection info might not be displayed properly after changing an existing connection profile
*/

#include "stdafx.h"
#include <fstream>
#include "resource.h"
#include "ConnectDlg.h"
#include <COMMON/StrHelpers.h>
#include <COMMON/DlgDataExt.h>
#include <OCI8/Connect.h>

    using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    static void GetTnsEntries (std::vector<string>& entries);

    const char* cszProfilesKey    = "Profiles";
    const char* cszProfilesExKey  = "ProfilesEx";
    const char* cszCurrProfileKey = "CurrProfile";
    const char* cszUserKey        = "User";
    const char* cszPasswordKey    = "Password";
    const char* cszConnectKey     = "Connect";
    const char* cszCounterKey     = "Counter";
    const char* cszHostKey        = "Host";
    const char* cszTcpPortKey     = "TcpPort";
    const char* cszSIDKey         = "SID";  
    const char* cszModeKey        = "Mode";  
    const char* cszSafetyKey      = "Safety";  

    const char* cszServiceInsteadOfSidKey = "ServiceInsteadOfSid";
    
    const char* cszDirectConnectionKey = "DirectConnection";  
    const char* cszSortColumnKey       = "SortColumn";
    const char* cszSortDirectionKey    = "SortDirection";

    const int   cnUserColumn        = 0;
    const int   cnAliasColumn       = 1;
    const int   cnCounterColumn     = 2;
    const int   cnLastUsageColumn   = 3;
    const int   cnColumns           = 4;

CConnectDlg::CConnectDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CConnectDlg::IDD, pParent),
    m_sortColumn(-1), m_direction(0)
{
    SetHelpID(CConnectDlg::IDD);
    //{{AFX_DATA_INIT(CConnectDlg)
    m_user = _T("");
    m_password = _T("");
    m_tnsAlias = _T("");
    m_directConnection = FALSE;
    m_host = _T("");
    m_savePassword = FALSE;
    m_sid = _T("");
    m_tcpPort = _T("");
    m_serviceInsteadOfSid = FALSE;
    m_connectionMode = 0;
    m_safety = 0;
    //}}AFX_DATA_INIT
    m_savePassword = AfxGetApp()->GetProfileInt("Server", "SavePassword", TRUE) ? true : false;
}

void CConnectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConnectDlg)
    DDX_Control(pDX, IDC_C_PROFILES, m_profiles);
    DDX_Text(pDX, IDC_C_USER, m_user);
    DDX_Text(pDX, IDC_C_PASSWORD, m_password);
    DDX_Text(pDX, IDC_C_CONNECT, m_tnsAlias);
    DDX_Check(pDX, IDC_C_DIRECT_CONNECTION, m_directConnection);
    DDX_Text(pDX, IDC_C_HOST, m_host);
    DDX_Check(pDX, IDC_C_SAVE_PASSWORD, m_savePassword);
    DDX_Text(pDX, IDC_C_SID, m_sid);
    DDX_Text(pDX, IDC_C_TCP_PORT, m_tcpPort);
    DDX_Check(pDX, IDC_C_SERVICE_INSTEAD_OF_SID, m_serviceInsteadOfSid);
    //}}AFX_DATA_MAP
    DDX_CBIndex(pDX, IDC_C_MODE, m_connectionMode);
    DDX_CBIndex(pDX, IDC_C_SAFETY, m_safety);
}

BEGIN_MESSAGE_MAP(CConnectDlg, CDialog)
    //{{AFX_MSG_MAP(CConnectDlg)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_C_PROFILES, OnColumnClick_Profiles)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_C_PROFILES, OnGetDispInfo_Profiles)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_C_PROFILES, OnItemChanged_Profiles)
    ON_BN_CLICKED(IDC_C_TEST, OnTest)
    ON_BN_CLICKED(IDC_C_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_C_SAVE_PASSWORD, OnDataChanged_SavePassword)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_NOTIFY(NM_DBLCLK, IDC_C_PROFILES, OnDblClk_Profiles)
	ON_BN_CLICKED(IDC_C_DIRECT_CONNECTION, OnClk_DirectConnectin)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_C_TNSCOPY, OnBnClicked_TnsCopy)
    ON_BN_CLICKED(IDHELP, OnBnClicked_Help)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg message handlers

void CConnectDlg::filetime_to_string (FILETIME& filetime, CString& str)
{
    SYSTEMTIME systemTime;
    FileTimeToLocalFileTime(&filetime, &filetime);
    FileTimeToSystemTime(&filetime, &systemTime);

    char buff[40];
    int dateLen = GetDateFormat(LOCALE_USER_DEFAULT, NULL, &systemTime, "yyyy.MM.dd", buff, sizeof(buff));
    buff[dateLen - 1] = ' ';
    GetTimeFormat(LOCALE_USER_DEFAULT, NULL, &systemTime, "HH:mm", buff + dateLen, sizeof(buff) - dateLen);
    str = buff;
}

BOOL CConnectDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    try { EXCEPTION_FRAME;

        struct 
        {
            const char* title;
            int width;
            int format;
        }
        columns[] =
        {
            { "User",       100, LVCFMT_LEFT  },
            { "Alias",      130, LVCFMT_LEFT  },
            { "Counter",     70, LVCFMT_RIGHT },
            { "Last usage", 100, LVCFMT_RIGHT },
        };

        LV_COLUMN lvcolumn;
        lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;

        _ASSERTE(cnColumns == sizeof columns/sizeof columns[0]);

        for (int i = 0; i < sizeof columns/sizeof columns[0]; i++) 
        {
            lvcolumn.pszText  = (LPSTR)(LPCSTR)columns[i].title;
            lvcolumn.iSubItem = i;
            lvcolumn.fmt      = columns[i].format;
            lvcolumn.cx       = columns[i].width;
            m_profiles.InsertColumn(i, &lvcolumn);
        }

        if (m_sortImages.m_hImageList == NULL)
            m_sortImages.Create(IDB_SORTDIRECTIONS, 8, 16, RGB(0,255,0));

        CHeaderCtrl* header = m_profiles.GetHeaderCtrl();
        header->SetImageList(&m_sortImages);

        m_profiles.SetExtendedStyle(LVS_EX_FULLROWSELECT);

        m_directConnection 
            = AfxGetApp()->GetProfileInt(cszCurrProfileKey, cszDirectConnectionKey, 0) 
                ? true : false;
        
        if (!m_directConnection)
        {
            m_user     = AfxGetApp()->GetProfileString(cszCurrProfileKey, cszUserKey);
            m_tnsAlias = AfxGetApp()->GetProfileString(cszCurrProfileKey, cszConnectKey);
            m_connectionMode = AfxGetApp()->GetProfileInt(cszCurrProfileKey, cszModeKey, 0);
            m_safety = AfxGetApp()->GetProfileInt(cszCurrProfileKey, cszSafetyKey, 0);
        }
        else
        {
            m_user = AfxGetApp()->GetProfileString(cszProfilesExKey, cszUserKey);
            m_host = AfxGetApp()->GetProfileString(cszProfilesExKey, cszHostKey);
            m_tcpPort = AfxGetApp()->GetProfileString(cszProfilesExKey, cszTcpPortKey);
            m_sid = AfxGetApp()->GetProfileString(cszProfilesExKey, cszSIDKey);
            m_serviceInsteadOfSid 
                = AfxGetApp()->GetProfileInt(cszProfilesExKey, cszServiceInsteadOfSidKey, FALSE) 
                    ? true : false;
            m_connectionMode = AfxGetApp()->GetProfileInt(cszProfilesExKey, cszModeKey, 0);
            m_safety = AfxGetApp()->GetProfileInt(cszProfilesExKey, cszSafetyKey, 0);
        }

        m_data.clear();

        set<string> hosts, ports;

        for (int branch = 0; branch < 2; branch++)
        {
            HKEY hkey = AfxGetApp()->GetSectionKey(!branch ? cszProfilesKey : cszProfilesExKey);
            if (hkey) 
            {
                char  buff[120];
                DWORD size = sizeof buff;
                FILETIME lastWriteTime;

                LV_ITEM lvitem;
                memset(&lvitem, 0, sizeof lvitem);

                int selectedProfile = -1;

                for (
                    DWORD index = 0;
                    RegEnumKeyEx(hkey, index, buff, &size, NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS;
                    index++, size = sizeof buff
                )
                {
                    ListEntry entry;
                    entry.directConnection = branch ? TRUE : FALSE;
                    entry.mode = 0;
                    entry.safety = 0;
        
                    CString subKeyName;
                    subKeyName += !branch ? cszProfilesKey : cszProfilesExKey;
                    subKeyName += '\\';
                    subKeyName += buff;
                    HKEY hSubKey = AfxGetApp()->GetSectionKey(subKeyName);
            
                    DWORD type;

                    if (!branch)
                    {
                        size = sizeof buff;
                        if (RegQueryValueEx(hSubKey, cszConnectKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                        {
                            buff[sizeof(buff)-1] = 0;
                            entry.tnsAlias = buff;
                        }
                    }
                    else
                    {
                        size = sizeof buff;
                        if (RegQueryValueEx(hSubKey, cszHostKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                        {
                            buff[sizeof(buff)-1] = 0;
                            entry.host = buff;
                            hosts.insert(buff);
                        }
                        size = sizeof buff;
                        if (RegQueryValueEx(hSubKey, cszTcpPortKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                        {
                            buff[sizeof(buff)-1] = 0;
                            entry.tcpPort = buff;
                            ports.insert(buff);
                        }
                        size = sizeof buff;
                        if (RegQueryValueEx(hSubKey, cszSIDKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                        {
                            buff[sizeof(buff)-1] = 0;
                            entry.sid = buff;
                        }
                        size = sizeof buff;
                        if (RegQueryValueEx(hSubKey, cszServiceInsteadOfSidKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                        {
                            buff[sizeof(buff)-1] = 0;
                            entry.serviceInsteadOfSid = *(DWORD*)buff ? true : false;
                        }
                    }
            
                    size = sizeof buff;
                    if (RegQueryValueEx(hSubKey, cszUserKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                    {
                        buff[sizeof(buff)-1] = 0;
                        entry.user = buff;
                    }

                    size = sizeof buff;
                    if (RegQueryValueEx(hSubKey, cszPasswordKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                    {
                        buff[sizeof(buff)-1] = 0;
                        entry.password = buff;
                    }
            
                    size = sizeof buff;
                    if (RegQueryValueEx(hSubKey, cszCounterKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                    {
                        itoa(*(DWORD*)buff, buff, 10);
                        entry.counter = buff;
                    }

                    size = sizeof buff;
                    if (RegQueryValueEx(hSubKey, cszModeKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                    {
                        buff[sizeof(buff)-1] = 0;
                        entry.mode = *(DWORD*)buff;
                    }

                    size = sizeof buff;
                    if (RegQueryValueEx(hSubKey, cszSafetyKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                    {
                        buff[sizeof(buff)-1] = 0;
                        entry.safety = *(DWORD*)buff;
                    }

                    RegCloseKey(hSubKey); // 27.03.2003 bug fix, a leak of registry key handles - it's a very critical bug for Win95/Win98/WinMe

                    filetime_to_string (lastWriteTime, entry.lastUsage);
                    m_data.push_back(entry);

                    lvitem.iItem    =
                    lvitem.lParam   = m_data.size() - 1;
                    lvitem.iSubItem = 0;
                    lvitem.mask     = LVIF_TEXT|LVIF_PARAM;
                    lvitem.pszText  = LPSTR_TEXTCALLBACK;
            
                    lvitem.iItem = m_profiles.InsertItem(&lvitem);

                    lvitem.mask = LVIF_TEXT;
                    for (lvitem.iSubItem = 1; lvitem.iSubItem < 4; lvitem.iSubItem++)
                        m_profiles.SetItem(&lvitem);

                    if (!m_directConnection)
                    {
                        if (!stricmp(m_user, entry.user) && !stricmp(m_tnsAlias, entry.tnsAlias))
                            selectedProfile = lvitem.iItem;
                    }
                    else
                    {
                        if (
                            !stricmp(m_user, entry.user) 
                            && !stricmp(m_tnsAlias, entry.tnsAlias)
                            && !stricmp(m_host, entry.host)
                            && !stricmp(m_tcpPort , entry.tcpPort)
                            && !stricmp(m_sid , entry.sid)
                        )
                            selectedProfile = lvitem.iItem;
                    }
                }

                RegCloseKey(hkey);

                if (selectedProfile != -1)
                    m_profiles.SetItemState(selectedProfile, LVIS_SELECTED, LVIS_SELECTED);

                sortProfiles(AfxGetApp()->GetProfileInt(cszCurrProfileKey, cszSortColumnKey, cnColumns - 1),
                             AfxGetApp()->GetProfileInt(cszCurrProfileKey, cszSortDirectionKey, -1));

            }
        }

        setupConnectionType();

        {
            std::vector<string> entries;
            GetTnsEntries(entries);
            std::vector<string>::const_iterator it = entries.begin();
            for (; it != entries.end(); ++it)
                SendDlgItemMessage(IDC_C_CONNECT, CB_ADDSTRING, 0, (LPARAM)it->c_str());
        }
        {
            set<string>::const_iterator it = hosts.begin();
            for (; it != hosts.end(); ++it)
                SendDlgItemMessage(IDC_C_HOST, CB_ADDSTRING, 0, (LPARAM)it->c_str());
        }
        {
            ports.insert("1521");
            set<string>::const_iterator it = ports.begin();
            for (; it != ports.end(); ++it)
                SendDlgItemMessage(IDC_C_TCP_PORT, CB_ADDSTRING, 0, (LPARAM)it->c_str());
        }
    }   
    _DEFAULT_HANDLER_
        
    return TRUE;
}

void CConnectDlg::sortProfiles (int col, int dir)
{
    _ASSERTE((m_sortColumn < cnColumns) && (dir == 1 || dir == -1));
    m_sortColumn = (m_sortColumn < cnColumns) ? col : 0;
    m_direction  = (dir == -1) ? -1 : 1;
    
    m_profiles.SortItems(CConnectDlg::comp_proc, (LPARAM)this);

    CHeaderCtrl* header = m_profiles.GetHeaderCtrl();

    HDITEM hditem;

    for (int i = 0, count = header->GetItemCount(); i < count; i++)
    {
        hditem.mask = HDI_FORMAT;
        header->GetItem(i, &hditem);
        hditem.fmt &= ~HDF_IMAGE;
        header->SetItem(i, &hditem);
    }
    
    hditem.mask = HDI_FORMAT;
    header->GetItem(m_sortColumn, &hditem);
    hditem.mask = HDI_IMAGE | HDI_FORMAT;
    hditem.iImage= (m_direction == 1) ? 0 : 1;
    hditem.fmt |= HDF_IMAGE;
    header->SetItem(m_sortColumn, &hditem);

    int selectedProfile = m_profiles.GetNextItem(-1, LVNI_SELECTED);
    if (selectedProfile != -1) 
        m_profiles.EnsureVisible(selectedProfile, FALSE);

    AfxGetApp()->WriteProfileInt(cszCurrProfileKey, cszSortColumnKey, m_sortColumn);
    AfxGetApp()->WriteProfileInt(cszCurrProfileKey, cszSortDirectionKey, m_direction);
}

int CALLBACK CConnectDlg::comp_proc (LPARAM row1, LPARAM row2, LPARAM this_prt)
{
    try { EXCEPTION_FRAME;

        const CConnectDlg* m_This = (CConnectDlg*)this_prt;

        LPCSTR str1; LPCSTR str2;

        const ListEntry& entry1 = m_This->m_data.at(row1),
                         entry2 = m_This->m_data.at(row2);
    
        switch (m_This->m_sortColumn)
        {
        case cnUserColumn: 
            str1 = entry1.user; 
            str2 = entry2.user; 
            break;
        case cnLastUsageColumn: 
            str1 = entry1.lastUsage;
            str2 = entry2.lastUsage;
            break;

        case cnCounterColumn: 
            return m_This->m_direction 
                * (atoi(entry1.counter) - atoi(entry2.counter));

        case cnAliasColumn: 
            {
                CString buff1, buff2;

                if (!entry1.directConnection) str1 = entry1.tnsAlias;   
                else
                {
                    buff1.Format("%s:%s", (LPCSTR)entry1.sid, (LPCSTR)entry1.host);
                    str1 = buff1;
                }
            
                if (!entry2.directConnection) str2 = entry2.tnsAlias;   
                else
                {
                    buff2.Format("%s:%s", (LPCSTR)entry2.sid, (LPCSTR)entry2.host);
                    str2 = buff2;
                }

                return m_This->m_direction * stricmp(str1, str2);
            }
        
        default:
            _ASSERTE(0);
            return 0;
        }

        return m_This->m_direction * stricmp(str1, str2);
    }
    _DEFAULT_HANDLER_

    return 0;
}

void CConnectDlg::OnColumnClick_Profiles (NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    int col = pNMListView->iSubItem;
    int defDir = (col == cnUserColumn || col == cnAliasColumn) ? 1 : -1;

    sortProfiles(pNMListView->iSubItem, (m_sortColumn != col) ? defDir : -m_direction);

    *pResult = 0;
}

void CConnectDlg::OnGetDispInfo_Profiles (NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

    try { EXCEPTION_FRAME;

        const ListEntry& entry = m_data.at(pDispInfo->item.lParam);

        switch (pDispInfo->item.iSubItem)
        {
        case cnUserColumn:      pDispInfo->item.pszText = (LPSTR)(LPCSTR)entry.user;      break;
        case cnCounterColumn:   pDispInfo->item.pszText = (LPSTR)(LPCSTR)entry.counter;   break;
        case cnLastUsageColumn: pDispInfo->item.pszText = (LPSTR)(LPCSTR)entry.lastUsage; break;

        case cnAliasColumn:     
            if (!entry.directConnection)
            {
                pDispInfo->item.pszText = (LPSTR)(LPCSTR)entry.tnsAlias;   
            }
            else
            {
                m_callbackBuffer.Format("%s:%s", (LPCSTR)entry.sid, (LPCSTR)entry.host);
                pDispInfo->item.pszText = (LPSTR)(LPCSTR)m_callbackBuffer;
            }
            break;
        }
    }
    _DEFAULT_HANDLER_
        
    *pResult = 0;
}

void CConnectDlg::OnItemChanged_Profiles (NMHDR* pNMHDR, LRESULT* pResult) 
{
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pNMHDR; 

    if (pnmv->uChanged && pnmv->uNewState & LVIS_SELECTED && pnmv->lParam >= 0)
    {
        ListEntry entry = m_data.at(pnmv->lParam);

        m_user     = entry.user;
        m_password = m_savePassword ? entry.password : "";
        m_directConnection = entry.directConnection;
        m_connectionMode = entry.mode;
        m_safety = entry.safety;
        
        if (!m_directConnection)
        {
            m_tnsAlias = entry.tnsAlias;
            m_host.Empty();
            m_tcpPort.Empty();
            m_sid.Empty();
            m_serviceInsteadOfSid = FALSE;
        }
        else
        {
            m_tnsAlias.Empty();
            m_host = entry.host;
            m_tcpPort = entry.tcpPort;
            m_sid = entry.sid;
            m_serviceInsteadOfSid = entry.serviceInsteadOfSid;
        }

        setupConnectionType();

        UpdateData(FALSE);
    }
        
    *pResult = 0;
}

void CConnectDlg::OnDataChanged_SavePassword ()
{
    UpdateData(TRUE);
    AfxGetApp()->WriteProfileInt("Server", "SavePassword", m_savePassword);
}

void CConnectDlg::OnTest () 
{
    UpdateData(TRUE);

    if (m_directConnection)
        makeTnsString(m_tnsAlias);

    try { EXCEPTION_FRAME;

        OciConnect connect;
        
        OCI8::EConnectionMode mode = OCI8::ecmDefault;
        switch (m_connectionMode)
        {
        case 1: mode = OCI8::ecmSysDba; break;
        case 2: mode = OCI8::ecmSysOper; break;
        }
        connect.Open(m_user, m_password, m_tnsAlias, mode, OCI8::esReadOnly);
        connect.Close();

        CString message;

        if (!m_directConnection)
            message.Format("TNS:\t%s\t\nUser:\t%s\n\nConnection is Ok!\t", 
                (LPCSTR)m_tnsAlias, (LPCSTR)m_user);
        else
            message.Format("Host:\t%s\t\nTCP Port:\t%s\n%s:\t%s\nUser:\t%s\n\nConnection is Ok!\t", 
                (LPCSTR)m_host, (LPCSTR)m_tcpPort, 
                (!m_serviceInsteadOfSid ? "SID" : "Service_Name"),
                (LPCSTR)m_sid, (LPCSTR)m_user);

        MessageBeep(MB_OK);
        MessageBox(message, "Connection test", MB_OK|MB_ICONASTERISK);
    }
    catch (const OciException& x)
    {
        MessageBeep((UINT)-1);
        MessageBox(x.what(), "Connection test", MB_OK|MB_ICONHAND);
    }
    _COMMON_DEFAULT_HANDLER_
}

void CConnectDlg::OnDelete () 
{
    int index = m_profiles.GetNextItem(-1, LVNI_SELECTED);
    if (index != -1) 
    {
        CString entry, message;

        if (!m_directConnection)
            entry.Format("%s - %s", (LPCTSTR)m_tnsAlias, (LPCTSTR)m_user);
        else
            entry.Format("%s:%s:%s - %s", 
                (LPCSTR)m_host, (LPCSTR)m_tcpPort, (LPCSTR)m_sid, (LPCTSTR)m_user);

        entry.MakeUpper();
        message.Format("Are you sure you want to delete \"%s\"?", (LPCSTR)entry);

        if (AfxMessageBox(message, MB_YESNO) == IDYES)
        {
            HKEY hkey = AfxGetApp()->GetSectionKey(!m_directConnection ? cszProfilesKey : cszProfilesExKey);
            if (hkey) 
            {
                RegDeleteKey(hkey, entry);
                RegCloseKey(hkey);
            }

            m_profiles.DeleteItem(index);

            if (!m_profiles.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED))
                m_profiles.SetItemState(--index, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}


void CConnectDlg::OnOK () 
{
    try { EXCEPTION_FRAME;

        CDialog::OnOK();

        // extract tns alias from user
        int pos = m_user.Find('@');
        if (pos != -1) 
        {
            m_tnsAlias = m_user.GetBuffer(m_user.GetLength()) + pos + 1;
            m_user.ReleaseBuffer(pos);
        }

        // extract password from user
        pos = m_user.Find('/');
        if (pos != -1) 
        {
            if (pos != -1) 
            {
                m_password = m_user.GetBuffer(m_user.GetLength()) + pos + 1;
                m_user.ReleaseBuffer(pos);
            }
        }

        AfxGetApp()->WriteProfileInt(cszCurrProfileKey, cszDirectConnectionKey, m_directConnection);

        CString keyName;
        if (!m_directConnection)
        {
            AfxGetApp()->WriteProfileString(cszCurrProfileKey, cszUserKey, m_user);
            AfxGetApp()->WriteProfileString(cszCurrProfileKey, cszConnectKey, m_tnsAlias);
            AfxGetApp()->WriteProfileString(cszCurrProfileKey, cszPasswordKey, m_savePassword ? m_password : "");
            AfxGetApp()->WriteProfileInt(cszCurrProfileKey, cszModeKey, m_connectionMode);
            AfxGetApp()->WriteProfileInt(cszCurrProfileKey, cszSafetyKey, m_safety);

            keyName.Format("%s\\%s - %s", cszProfilesKey, (LPCTSTR)m_tnsAlias, (LPCTSTR)m_user);
            keyName.MakeUpper();

            AfxGetApp()->WriteProfileString(keyName, cszUserKey, m_user);
            AfxGetApp()->WriteProfileString(keyName, cszConnectKey, m_tnsAlias);
            AfxGetApp()->WriteProfileInt(keyName, cszModeKey, m_connectionMode);
            AfxGetApp()->WriteProfileInt(keyName, cszSafetyKey, m_safety);

            // 16.02.2004 bug fix, a connection info might not be displayed properly after changing an existing connection profile
            m_host.Empty();
            m_tcpPort.Empty();
            m_sid.Empty();
        }
        else
        {
            makeTnsString(m_tnsAlias);
            AfxGetApp()->WriteProfileString(cszProfilesExKey, cszUserKey, m_user);
            AfxGetApp()->WriteProfileString(cszProfilesExKey, cszPasswordKey, m_savePassword ? m_password : "");
            AfxGetApp()->WriteProfileString(cszProfilesExKey, cszHostKey, m_host);
            AfxGetApp()->WriteProfileString(cszProfilesExKey, cszTcpPortKey, m_tcpPort);
            AfxGetApp()->WriteProfileString(cszProfilesExKey, cszSIDKey, m_sid);
            AfxGetApp()->WriteProfileInt(cszProfilesExKey, cszServiceInsteadOfSidKey, m_serviceInsteadOfSid);
            AfxGetApp()->WriteProfileInt(cszProfilesExKey, cszModeKey, m_connectionMode);
            AfxGetApp()->WriteProfileInt(cszProfilesExKey, cszSafetyKey, m_safety);

            keyName.Format("%s\\%s:%s:%s - %s", cszProfilesExKey, 
                (LPCSTR)m_host, (LPCSTR)m_tcpPort, (LPCSTR)m_sid, (LPCTSTR)m_user);
            keyName.MakeUpper();

            AfxGetApp()->WriteProfileString(keyName, cszHostKey,    m_host);
            AfxGetApp()->WriteProfileString(keyName, cszTcpPortKey, m_tcpPort);
            AfxGetApp()->WriteProfileString(keyName, cszSIDKey,     m_sid);
            AfxGetApp()->WriteProfileInt(keyName, cszServiceInsteadOfSidKey, m_serviceInsteadOfSid);
            AfxGetApp()->WriteProfileInt(keyName, cszModeKey, m_connectionMode);
            AfxGetApp()->WriteProfileInt(keyName, cszSafetyKey, m_safety);
        }

        AfxGetApp()->WriteProfileString(keyName, cszUserKey,    m_user);
        AfxGetApp()->WriteProfileString(keyName, cszPasswordKey, m_savePassword ? m_password : "");

        int counter = AfxGetApp()->GetProfileInt(keyName, cszCounterKey, 0);
        AfxGetApp()->WriteProfileInt(keyName, cszCounterKey, ++counter);
    }
    _DEFAULT_HANDLER_
}

void CConnectDlg::OnDblClk_Profiles (NMHDR*, LRESULT* pResult) 
{
    OnOK();
    *pResult = 0;
}

void CConnectDlg::OnClk_DirectConnectin ()
{
    UpdateData(TRUE);
    setupConnectionType();
}

void CConnectDlg::setupConnectionType ()
{
    GetDlgItem(IDC_C_CONNECT)->EnableWindow(!m_directConnection);
    GetDlgItem(IDC_C_HOST)->EnableWindow(m_directConnection);
    GetDlgItem(IDC_C_SID)->EnableWindow(m_directConnection);
    GetDlgItem(IDC_C_TCP_PORT)->EnableWindow(m_directConnection);
    GetDlgItem(IDC_C_TNSCOPY)->EnableWindow(m_directConnection);
    GetDlgItem(IDC_C_SERVICE_INSTEAD_OF_SID)->EnableWindow(m_directConnection);
}

void CConnectDlg::makeTnsString (CString& str)
{
    string buff;
    OciConnect::MakeTNSString(buff, m_host, m_tcpPort, m_sid, m_serviceInsteadOfSid);
    str = buff.c_str();
}

void CConnectDlg::OnBnClicked_TnsCopy()
{
    UpdateData(TRUE);

    if (OpenClipboard() && EmptyClipboard())
    {
        CString str;
        makeTnsString(str);
        str = m_sid + "=" + str;;

        if (!str.IsEmpty())
        {
            HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, str.GetLength() + 1);
            char* dest = (char*)GlobalLock(hData);
            if (dest)
                strcpy(dest, str);
            SetClipboardData(CF_TEXT, hData);
        }

        CloseClipboard();
    }
}

void CConnectDlg::OnBnClicked_Help()
{
#if _MFC_VER <= 0x0600
#pragma message("There is no html help support for VC6!")
#else//_MFC_VER > 0x0600
    AfxGetApp()->HtmlHelp(HID_BASE_RESOURCE + m_nIDHelp);
#endif
}

bool GetOciDllPath (string& path)
{
    char fullpath[1024], *filename;
    DWORD length = SearchPath(NULL, "OCI.DLL", NULL, sizeof(fullpath), fullpath, &filename);
    
    if (length > 0 && length < sizeof(fullpath))
    {
        path.assign(fullpath, filename - fullpath);
        return true;
    }

    return false;
}

bool GetRegValue (string key, const char* name, string& value)
{
    HKEY hKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE, &hKey);

    if (lRet != ERROR_SUCCESS)
        return false;

    DWORD type;
    char buff[1024];
    DWORD length = sizeof(buff);
    lRet = RegQueryValueEx(hKey, name, NULL, &type, (LPBYTE)buff, &length);
    buff[sizeof(buff)-1] = 0;

    RegCloseKey(hKey);

    if(lRet != ERROR_SUCCESS 
    || type != REG_SZ
    || length >= sizeof(buff))
        return false;

    value = buff;
    
    return true;
}

bool GetSubkeys (string key, vector<string>& _subkeys)
{
    vector<string> subkeys;

    HKEY hKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS, &hKey);

    if (lRet != ERROR_SUCCESS)
        return false;
    
    for (DWORD i = 0; true; i++) 
    { 
        char buff[1024];
        DWORD length = sizeof(buff)-1;
        FILETIME ftLastWriteTime;
        lRet = RegEnumKeyEx(hKey, i, buff, &length, NULL, NULL, NULL, &ftLastWriteTime);
        if (ERROR_SUCCESS != lRet) break;
        buff[sizeof(buff)-1] = 0;
        subkeys.push_back(buff);
    } 

    RegCloseKey(hKey);

    swap(subkeys, _subkeys);
    return true;
}

bool GetTnsPath (string& path)
{
    string ocipath, value;

    if (GetOciDllPath(ocipath))
    {
        vector<string> subkeys;
        if (GetSubkeys("SOFTWARE\\ORACLE", subkeys))
        {
            vector<string>::const_iterator it = subkeys.begin();
            for (; it != subkeys.end(); ++it)
            {
                if (!strnicmp(it->c_str(), "KEY_", sizeof("KEY_")-1)
                || !strnicmp(it->c_str(), "HOME", sizeof("HOME")-1))
                {
                    string key = string("SOFTWARE\\ORACLE\\") + *it;
                    
                    if (GetRegValue(key, "ORACLE_HOME", value))
                    {
                        if (!stricmp(ocipath.c_str(), (value + "\\BIN\\").c_str())) 
                        {
                            if (GetRegValue(key, "TNS_ADMIN", value))
                                path = value + "\\TNSNAMES.ORA";
                            else
                                path = value + "\\NETWORK\\ADMIN\\TNSNAMES.ORA";

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

static
void GetTnsEntries (std::vector<string>& entries)
{
    string tnsnames;
    if (!GetTnsPath(tnsnames)) return;

    std::ifstream in(tnsnames.c_str());
    string line, entry;
    int balance = 0;
    bool decription = false;

    while (getline(in, line) && balance >= 0)
    {
        for (string::const_iterator it = line.begin(); it != line.end(); ++it)
        {
            if (*it == '#') break;
            
            if (!decription)
            {
                if (*it != '=')
                    entry += *it;
                else
                {
                    Common::trim_symmetric(entry);
                    Common::to_lower_str(entry.c_str(), entry);
                    entries.push_back(entry);
                    entry.clear();
                    decription = true;
                }
            }
            else
            {
                switch (*it)
                {
                case '(': 
                    balance++; 
                    break;
                case ')': 
                    if (!--balance)
                        decription = false;
                    break;
                }
            }
        }
    }
}

LRESULT CConnectDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
