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
#include "COMMON\AppUtilities.h"
#include "COMMON\ExceptionHelper.h"
#include "COMMON\DirSelectDlg.h"
#include <COMMON/DlgDataExt.h>
#include "ExtractSchemaDlg.h"
#include "ExtractDDLSettings.h"
#include "ExtactSchemaMainPage.h"
#include "extactschemamainpage.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using namespace Common;
using namespace OraMetaDict;

/////////////////////////////////////////////////////////////////////////////
// CExtactSchemaMainPage

CExtactSchemaMainPage::CExtactSchemaMainPage (ExtractDDLSettings& DDLSettings,
                                              OciConnect& connect) 
: CPropertyPage(CExtactSchemaMainPage::IDD),
  m_connect(connect),
  m_DDLSettings(DDLSettings),
  m_wndSchemaList(connect)
{
}

void CExtactSchemaMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX,  IDC_ES_SCHEMA, m_wndSchemaList);
	DDX_Control(pDX,  IDC_ES_FOLDER, m_wndFolder);
    DDX_Control(pDX,  IDC_ES_TABLE_TABLESPACE, m_wndTableTablespace);
    DDX_Control(pDX,  IDC_ES_INDEX_TABLESPACE, m_wndIndexTablespace);

	DDX_CBString(pDX, IDC_ES_SCHEMA, m_DDLSettings.m_strSchema);
	DDX_CBString(pDX, IDC_ES_FOLDER, m_DDLSettings.m_strFolder);
    DDX_CBString(pDX, IDC_ES_TABLE_TABLESPACE, m_DDLSettings.m_strTableTablespace);
    DDX_CBString(pDX, IDC_ES_INDEX_TABLESPACE, m_DDLSettings.m_strIndexTablespace);

}

BEGIN_MESSAGE_MAP(CExtactSchemaMainPage, CPropertyPage)
	//{{AFX_MSG_MAP(CExtactSchemaMainPage)
	ON_BN_CLICKED(IDC_ES_SELECT_FOLDER,  OnSelectFolder)
	ON_BN_CLICKED(IDC_ES_FOLDER_OPTIONS, OnFolderOptions)
	ON_CBN_EDITCHANGE(IDC_ES_FOLDER, OnInvalidateFullPath)
	ON_CBN_EDITCHANGE(IDC_ES_SCHEMA, OnInvalidateFullPath)
	ON_CBN_SELCHANGE(IDC_ES_FOLDER,  OnInvalidateFullPath)
	ON_CBN_SELCHANGE(IDC_ES_SCHEMA,  OnInvalidateFullPath)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_ES_USE_DB_ALIAS, OnUseDbAlias)
    ON_COMMAND_RANGE(ID_ES_AS_DB_SCHEMA, ID_ES_AS_DB_BKS_SCHEMA, OnUseDbAliasAs)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtactSchemaMainPage message handlers

void CExtactSchemaMainPage::SetStatusText (const char* szMessage)
{
    HWND hStatusWnd = ::GetDlgItem(*this, IDC_ES_STATUS);

    if (hStatusWnd) 
    {
        ::SetWindowText(hStatusWnd, szMessage);
        UpdateWindow();
    }
}

void CExtactSchemaMainPage::ShowFullPath ()
{
    UpdateData();
    AfxSetWindowText(::GetDlgItem(m_hWnd, IDC_ES_FULL_PATH), CExtractSchemaDlg::GetFullPath(m_DDLSettings).c_str()); 
}

void CExtactSchemaMainPage::OnSelectFolder() 
{
    UpdateData();
    CDirSelectDlg dirDlg("Put In", this, m_DDLSettings.m_strFolder.c_str());

    if (dirDlg.DoModal() == IDOK) 
    {
        m_DDLSettings.m_strFolder = dirDlg.GetPath();
        UpdateData(FALSE);
        ShowFullPath();
    }
    
    m_wndFolder.SetFocus();
}

BOOL CExtactSchemaMainPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

    try { EXCEPTION_FRAME;

        CWaitCursor wait;

	    AppRestoreHistory(m_wndFolder, "ExtactSchemaDDL", "FolderHistory",  7);
        ShowFullPath();
	
        m_wndTableTablespace.ResetContent();
        m_wndIndexTablespace.ResetContent();

        OciCursor curs(m_connect, "SELECT tablespace_name FROM user_tablespaces ORDER BY 1");

        string tablespace;
        curs.Execute();
        while (curs.Fetch())
        {
            curs.GetString(0, tablespace);
            m_wndTableTablespace.AddString(tablespace.c_str());
            m_wndIndexTablespace.AddString(tablespace.c_str());
        }
    } 
    _DEFAULT_HANDLER_

	return TRUE;
}

void CExtactSchemaMainPage::OnFolderOptions() 
{
    HWND hButton = ::GetDlgItem(m_hWnd, IDC_ES_FOLDER_OPTIONS);
    
    _ASSERTE(hButton);
  
    if (hButton
    && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED) 
    {
        ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

        CRect rect;
        ::GetWindowRect(hButton, &rect);

  
        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_ES_FOLDER_OPTIONS));

        CMenu* pPopup = menu.GetSubMenu(0);
        _ASSERTE(pPopup != NULL);
        
        pPopup->CheckMenuItem(ID_ES_USE_DB_ALIAS, 
                              (m_DDLSettings.m_bUseDbAlias ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
        
        pPopup->CheckMenuRadioItem(ID_ES_AS_DB_SCHEMA, ID_ES_AS_DB_BKS_SCHEMA, 
                                   ID_ES_AS_DB_SCHEMA + m_DDLSettings.m_nUseDbAliasAs, MF_BYCOMMAND);
        
        if (!m_DDLSettings.m_bUseDbAlias)
            for (int i(0); i < 4; i++)
                pPopup->EnableMenuItem(ID_ES_AS_DB_SCHEMA + i, MF_GRAYED|MF_BYCOMMAND);

        TPMPARAMS param;
        param.cbSize = sizeof param;
        param.rcExclude.left   = 0;
        param.rcExclude.top    = rect.top;
        param.rcExclude.right  = USHRT_MAX;
        param.rcExclude.bottom = rect.bottom;

        if (::TrackPopupMenuEx(*pPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                               rect.left, rect.bottom + 1, *this, &param)) 
        {
		      MSG msg;
		      ::PeekMessage(&msg, hButton, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
        }

        ::PostMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0L);
    }
}

void CExtactSchemaMainPage::OnUseDbAlias ()
{
    m_DDLSettings.m_bUseDbAlias = !m_DDLSettings.m_bUseDbAlias;

    ShowFullPath();
}

void CExtactSchemaMainPage::OnUseDbAliasAs (UINT uID)
{
    m_DDLSettings.m_nUseDbAliasAs = uID - ID_ES_AS_DB_SCHEMA;

    if (m_DDLSettings.m_nUseDbAliasAs < 0) m_DDLSettings.m_nUseDbAliasAs = 0;
    if (m_DDLSettings.m_nUseDbAliasAs > 2) m_DDLSettings.m_nUseDbAliasAs = 2;

    ShowFullPath();
}

void CExtactSchemaMainPage::OnInvalidateFullPath() 
{
	if (!SetTimer(1, 250, NULL))
        ShowFullPath();
}

void CExtactSchemaMainPage::OnTimer(UINT nIDEvent) 
{
	_ASSERTE(nIDEvent == 1);

    KillTimer(1);

    ShowFullPath();
}

void CExtactSchemaMainPage::UpdateDataAndSaveHistory ()
{
    UpdateData();

    AppSaveHistory(m_wndFolder, "ExtactSchemaDDL", "FolderHistory",  7);
    AppRestoreHistory(m_wndFolder, "ExtactSchemaDDL", "FolderHistory",  7);

    CPropertyPage::OnOK();
}
