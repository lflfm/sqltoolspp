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

#include "stdafx.h"
#include "AboutDlg.h"
#include "VersionInfo.rh"

// CAboutDlg dialog

IMPLEMENT_DYNAMIC(CAboutDlg, CDialog)
CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg::IDD, pParent)
//    , m_license(_T(""))
{
    m_license = SQLTOOLS_VERSION
        "\r\nDevelopment tools for Oracle 8/8i/9i"
        "\r\n\r\n" SQLTOOLS_COPYRIGHT
        "\r\nwww.sqltools.net"
         "\r\n\r\nThis program is free software; you can redistribute it and/or modify it" 
         " under the terms of the GNU General Public License as published by the Free Software Foundation;" 
         " either version 2 of the License, or (at your option) any later version." 
         "\r\n\r\nThis program is distributed in the hope that it will be useful,"
         " but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY"
         " or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.";
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_OEA_LICENSE, m_license);
}


//BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
//END_MESSAGE_MAP()

// CAboutDlg message handlers

LRESULT CAboutDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}