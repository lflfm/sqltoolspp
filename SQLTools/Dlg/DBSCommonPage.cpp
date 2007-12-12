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
#include "DBSCommonPage.h"
#include <COMMON/DlgDataExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDBSCommonPage dialog


CDBSCommonPage::CDBSCommonPage(SQLToolsSettings& settings)
	: CPropertyPage(CDBSCommonPage::IDD, NULL),
  m_DDLSettings(settings)
{
    m_bShowShowOnShiftOnly =
    m_bShowOnShiftOnly = FALSE;
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CDBSCommonPage)
	//}}AFX_DATA_INIT
}


void CDBSCommonPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBSCommonPage)
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_DBSP_SHOW_ON_SHIFT_ONLY, m_bShowOnShiftOnly);
	DDX_Check(pDX, IDC_DBSP_COMMENTS,           m_DDLSettings.m_bComments);
	DDX_Check(pDX, IDC_DBSP_GRANTS,             m_DDLSettings.m_bGrants);
	DDX_Check(pDX, IDC_DBSP_LOWER_NAMES,        m_DDLSettings.m_bLowerNames);
	DDX_Check(pDX, IDC_DBSP_SCHEMA_NAME,        m_DDLSettings.m_bShemaName);

	DDX_Check(pDX, IDC_DBSP_SQLPLUS_COMPATIBILITY, m_DDLSettings.m_bSQLPlusCompatibility);
    DDX_Check(pDX, IDC_DBSP_SPEC_WITH_BODY,     m_DDLSettings.m_bSpecWithBody);
    DDX_Check(pDX, IDC_DBSP_BODY_WITH_SPEC,     m_DDLSettings.m_bBodyWithSpec);
    DDX_Check(pDX, IDC_DBSP_VIEW_WITH_TRIGGERS, m_DDLSettings.m_bViewWithTriggers);
    DDX_Check(pDX, IDC_DBSP_PRELOAD_DICTIONARY, m_DDLSettings.m_bPreloadDictionary);
    DDX_Check(pDX, IDC_DBSP_SEQ_WITH_START,     m_DDLSettings.m_bSequnceWithStart);
    DDX_Check(pDX, IDC_PROP_PLUS_USE_DBMS_METADATA, GetSQLToolsSettingsForUpdate().m_UseDbmsMetaData);
    DDX_Check(pDX, IDC_DBSP_VIEW_WITH_FORCE,    m_DDLSettings.m_bViewWithForce);
    DDX_Text (pDX, IDC_DBSP_PRELOAD_START_PERCENT, m_DDLSettings.m_bPreloadStartPercent);
    
    DDV_MinMaxInt(pDX, m_DDLSettings.m_bPreloadStartPercent, 0, 100);

    DDX_Check(pDX, IDC_DBSP_NO_PROMPTS, m_DDLSettings.m_NoPrompts);

    
    bool alwaysSlash = m_DDLSettings.m_EndOfShortStatement != ";";
    
    DDX_Check(pDX, IDC_DBSP_ALWAYS_SLASH, alwaysSlash);
    
    if (pDX->m_bSaveAndValidate)
        m_DDLSettings.m_EndOfShortStatement = alwaysSlash ? "\n/" : ";";

//    OnDbspPreloadDictionary();
}

BEGIN_MESSAGE_MAP(CDBSCommonPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDBSOthersPage)
	ON_BN_CLICKED(IDC_DBSP_PRELOAD_DICTIONARY, OnDbspPreloadDictionary)
	//}}AFX_MSG_MAP
    ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBSCommonPage message handlers

void CDBSCommonPage::OnDbspPreloadDictionary() 
{
    if (m_hWnd) 
    {
        HWND hCheck = ::GetDlgItem(m_hWnd, IDC_DBSP_PRELOAD_DICTIONARY);
        HWND hText = ::GetDlgItem(m_hWnd, IDC_DBSP_PRELOAD_START_PERCENT);
        ::EnableWindow(hText, ::SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}

BOOL CDBSCommonPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    ::ShowWindow(::GetDlgItem(m_hWnd, IDC_DBSP_SHOW_ON_SHIFT_ONLY), m_bShowShowOnShiftOnly ? SW_RESTORE : SW_HIDE);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
