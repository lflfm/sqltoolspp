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
#include <COMMON/DlgDataExt.h>
#include "ExtractDDLSettings.h"
#include "ExtractSchemaFilterPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CExtractSchemaFilterPage::CExtractSchemaFilterPage (ExtractDDLSettings& DDLSettings) 
: CPropertyPage(CExtractSchemaFilterPage::IDD),
  m_DDLSettings(DDLSettings)
{
}


void CExtractSchemaFilterPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    DDX_Check(pDX,  IDC_ES_CODE,              m_DDLSettings.m_bExtractCode);
    DDX_Check(pDX,  IDC_ES_TABLES,            m_DDLSettings.m_bExtractTables);
    DDX_Check(pDX,  IDC_ES_TRIGGERS,          m_DDLSettings.m_bExtractTriggers);
    DDX_Check(pDX,  IDC_ES_VIEWS,             m_DDLSettings.m_bExtractViews);
    DDX_Check(pDX,  IDC_ES_SEQUENCES,         m_DDLSettings.m_bExtractSequences);
    DDX_Check(pDX,  IDC_ES_SYNONYMS,          m_DDLSettings.m_bExtractSynonyms);
    DDX_Check(pDX,  IDC_ES_GRANTS_BY_GRANTEE, m_DDLSettings.m_bExtractGrantsByGrantee);
}
