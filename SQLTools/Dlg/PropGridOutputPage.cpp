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

// 16.12.2004 (Ken Clubok) Add CSV prefix option

#include "stdafx.h"
#include "SQLTools.h"
#include "PropGridOutputPage.h"
#include <COMMON/DlgDataExt.h>


// CPropGridOutputPage dialog

CPropGridOutputPage::CPropGridOutputPage(SQLToolsSettings& settings) 
    : CPropertyPage(CPropGridOutputPage::IDD),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
}

void CPropGridOutputPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_GOO_PLAIN_TEXT, m_settings.m_GridExpFormat);
    DDX_Text(pDX, IDC_GOO_COMMA_CHAR, m_settings.m_GridExpCommaChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpCommaChar, 1);
    DDX_Text(pDX, IDC_GOO_QUOTE_CHAR, m_settings.m_GridExpQuoteChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpQuoteChar, 1);
    DDX_Text(pDX, IDC_GOO_QUOTE_ESC_CHAR, m_settings.m_GridExpQuoteEscapeChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpQuoteEscapeChar, 1);
    DDX_Text(pDX, IDC_GOO_PREFIX_CHAR, m_settings.m_GridExpPrefixChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpPrefixChar, 1);
    DDX_Check(pDX, IDC_GOO_WITH_HEADERS, m_settings.m_GridExpWithHeader);
    DDX_Check(pDX, IDC_GOO_XML_ELEM_W_NAME, m_settings.m_GridExpColumnNameAsAttr);
}

// CPropGridOutputPage message handlers
BEGIN_MESSAGE_MAP(CPropGridOutputPage, CPropertyPage)
    ON_BN_CLICKED(IDC_GOO_HTML, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLAIN_TEXT, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_COMMA_DELIMITED, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_TAB_DELIMITED, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_XML_ELEM, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_XML_ELEM_W_NAME, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_XML_ATTR, OnExpFormatChanged)
END_MESSAGE_MAP()

void CPropGridOutputPage::setupItems ()
{
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_COMMA_CHAR), 
                        m_settings.m_GridExpFormat == 1 /*comma delimited*/);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_QUOTE_CHAR), 
                        m_settings.m_GridExpFormat == 1 /*comma delimited*/);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_QUOTE_ESC_CHAR), 
                        m_settings.m_GridExpFormat == 1 /*comma delimited*/);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_PREFIX_CHAR), 
                        m_settings.m_GridExpFormat == 1 /*comma delimited*/);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_XML_ELEM_W_NAME), 
                        m_settings.m_GridExpFormat == 3 /*xml elem*/);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_WITH_HEADERS), 
                        m_settings.m_GridExpFormat != 3 /*xml elem*/ 
                        && m_settings.m_GridExpFormat != 4 /*xml attr*/);
}

void CPropGridOutputPage::OnExpFormatChanged ()
{
    UpdateData();
    setupItems();
}

BOOL CPropGridOutputPage::OnInitDialog ()
{
    CPropertyPage::OnInitDialog();
    setupItems();
    return TRUE;  
}
