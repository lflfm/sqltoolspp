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
#include "PropServerPage.h"
#include <COMMON/DlgDataExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropServerPage property page

CPropServerPage::CPropServerPage(SQLToolsSettings& settings) 
    : CPropertyPage(CPropServerPage::IDD),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CPropServerPage)
	//}}AFX_DATA_INIT
}

void CPropServerPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPropServerPage)
    //}}AFX_DATA_MAP
    DDX_Check(pDX, IDC_SERV_PROP_AUTOCOMMIT,    m_settings.m_Autocommit);
    DDX_CBIndex(pDX, IDC_SERV_PROP_ACTION_ON_DISCONNECT, m_settings.m_CommitOnDisconnect);
    DDX_Check(pDX, IDC_SERV_PROP_OUTPUT_ENABLE, m_settings.m_OutputEnable);
    DDX_Check(pDX, IDC_SERV_PROP_SESSTAT,       m_settings.m_SessionStatistics);
    DDX_CBString(pDX, IDC_SERV_PROP_SESSTAT_MODE, m_settings.m_SessionStatisticsMode);
    DDX_Check(pDX, IDC_SERV_PROP_SAVE_PASSWORD, m_settings.m_SavePassword);
    DDX_Text(pDX, IDC_SERV_PROP_DATE_FORMAT,    m_settings.m_DateFormat);
    DDX_Text(pDX, IDC_SERV_PROP_PLAN_TABLE,     m_settings.m_PlanTable);

    DDV_MaxChars(pDX, m_settings.m_PlanTable, 65);

    DDX_Text(pDX, IDC_SERV_PROP_OUTPUT_SIZE, m_settings.m_OutputSize);
	DDV_MinMaxUInt(pDX, m_settings.m_OutputSize, 0, 1000000000);
    SendDlgItemMessage(IDC_SERV_PROP_OUTPUT_SIZE_SPIN, UDM_SETRANGE32, 1, 1000000000);
    
    UDACCEL accels[10];
    int count = SendDlgItemMessage(IDC_SERV_PROP_OUTPUT_SIZE_SPIN, UDM_GETACCEL, 
        (WPARAM)sizeof(accels)/sizeof(accels[0]), (LPARAM)&accels);

    for (int i = 0; i < count && i < sizeof(accels)/sizeof(accels[0]); i++)
        accels[i].nInc *= 10000;
    
    SendDlgItemMessage(IDC_SERV_PROP_OUTPUT_SIZE_SPIN, UDM_SETACCEL, (WPARAM)i, (LPARAM)&accels);

    DDX_Text(pDX, IDC_SERV_PROP_CANCEL_QUERY_DELAY, m_settings.m_CancelQueryDelay);
	DDV_MinMaxUInt(pDX, m_settings.m_CancelQueryDelay, 0, 60 * 60);
    SendDlgItemMessage(IDC_SERV_PROP_CANCEL_QUERY_DELAY_SPIN, UDM_SETRANGE32, 0, 60 * 60);

    DDX_Check(pDX, IDC_SERV_PROP_TOPMOST_CANCEL_QUERY, m_settings.m_TopmostCancelQuery);
    DDX_Check(pDX, IDC_SERV_PROP_DISPLAY_CURSOR, m_settings.m_DbmsXplanDisplayCursor);
    DDX_Check(pDX, IDC_SERV_PROP_WHITESPACE_DELIM, m_settings.m_WhitespaceLineDelim);

    OnServPropSesstat();
}

BEGIN_MESSAGE_MAP(CPropServerPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPropServerPage)
	ON_BN_CLICKED(IDC_SERV_PROP_SESSTAT, OnServPropSesstat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPropServerPage::OnServPropSesstat() 
{
    if (m_hWnd) {
        HWND hCheck = ::GetDlgItem(m_hWnd, IDC_SERV_PROP_SESSTAT);
        HWND hText = ::GetDlgItem(m_hWnd, IDC_SERV_PROP_SESSTAT_MODE);
        ::EnableWindow(hText, ::SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
}
