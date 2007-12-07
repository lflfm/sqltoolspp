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
#include "PlusPlusPage.h"
#include <COMMON/DlgDataExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlusPlusPage dialog


CPlusPlusPage::CPlusPlusPage(SQLToolsSettings& settings)
	: CPropertyPage(CPlusPlusPage::IDD, NULL),
  m_Settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
	//{{AFX_DATA_INIT(CPlusPlusPage)
	//}}AFX_DATA_INIT
}


void CPlusPlusPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlusPlusPage)
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_PROP_PLUS_DISPLAY_CURSOR,    m_Settings.m_DbmsXplanDisplayCursor);
	DDX_Check(pDX, IDC_PROP_PLUS_WHITESPACE_DELIM,  m_Settings.m_WhitespaceLineDelim);
	DDX_Check(pDX, IDC_PROP_PLUS_EMPTYLINE_DELIM,   m_Settings.m_EmptyLineDelim);
	DDX_Check(pDX, IDC_PROP_PLUS_UNLIMITED_OUTPUT,  m_Settings.m_UnlimitedOutputSize);
	DDX_Text(pDX,  IDC_PROP_PLUS_EXTERNAL_TOOL_CMD, m_Settings.m_ExternalToolCommand);
	DDX_Text(pDX,  IDC_PROP_PLUS_EXTERNAL_TOOL_PAR, m_Settings.m_ExternalToolParameters);
	DDX_Check(pDX, IDC_PROP_PLUS_USE_DBMS_METADATA, m_Settings.m_UseDbmsMetaData);
	DDX_Check(pDX, IDC_PROP_PLUS_HALT_ON_ERRORS,    m_Settings.m_HaltOnErrors);
	DDX_Check(pDX, IDC_PROP_PLUS_SAVE_BEF_EXECUTE,  m_Settings.m_SaveFilesBeforeExecute);
}

BEGIN_MESSAGE_MAP(CPlusPlusPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDBSOthersPage)
	//}}AFX_MSG_MAP
    ON_WM_CREATE()
    ON_COMMAND(IDC_PROP_PLUS_SELECT_DIR, OnSelectDir)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlusPlusPage message handlers

void CPlusPlusPage::OnSelectDir()
{
    UpdateData(TRUE);
    CString toolName = m_Settings.m_ExternalToolCommand.c_str();

	CFileDialog dlgFile(TRUE, NULL, NULL,
		OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_NOCHANGEDIR, 
		NULL, NULL, 0);

	CString title = "Select external tool";

	dlgFile.m_ofn.Flags |= OFN_FILEMUSTEXIST;

	CString strFilter;
	// append the "*.*" all files filter
	strFilter += "Executable Files (*.exe;*.cmd;*.bat)";
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.exe;*.cmd;*.bat");
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += "All Files (*.*)";
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
    
    strFilter += '\0'; // close

	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
    dlgFile.m_ofn.nFilterIndex = 0;

// 31.03.2003 bug fix, the programm fails on attempt to open multiple file selection in Open dialog
#if _MAX_PATH*256 < 64*1024-1
#define PATH_BUFFER _MAX_PATH*256
#else
#define PATH_BUFFER 64*1024-1
#endif
    dlgFile.m_ofn.nMaxFile = PATH_BUFFER;
	dlgFile.m_ofn.lpstrFile = toolName.GetBuffer(dlgFile.m_ofn.nMaxFile);
    //dlgFile.m_ofn.lpstrFile[0] = 0; // 26.05.2003 bug fix, no file name on "Save As" or "Save" for a new file
	
    // overwrite initial dir
    dlgFile.m_ofn.lpstrInitialDir = NULL;

    if (dlgFile.DoModal() == IDOK)
    {
        m_Settings.m_ExternalToolCommand = toolName;
        UpdateData(FALSE);
    }
    
	toolName.ReleaseBuffer();
}
