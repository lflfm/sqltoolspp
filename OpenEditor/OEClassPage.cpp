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

// 17.01.2005 (ak) simplified settings class hierarchy 

#include "stdafx.h"
#include <sstream>
#include "resource.h"
#include "COMMON/StrHelpers.h"
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEClassPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COEClassPage property page

COEClassPage::COEClassPage(SettingsManager& manager) 
: CPropertyPage(COEClassPage::IDD),
m_manager(manager)
{
    m_dataInitialized = false;
	//{{AFX_DATA_INIT(COEClassPage)
	//}}AFX_DATA_INIT
}

COEClassPage::~COEClassPage()
{
}

void COEClassPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COEClassPage)
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COEClassPage, CPropertyPage)
	//{{AFX_MSG_MAP(COEClassPage)
	ON_CBN_SELCHANGE(IDC_OEC_CLASS, OnSelChangeLanguage)
	ON_EN_CHANGE(IDC_OEC_TAB_SIZE, OnChangeData)
	ON_EN_CHANGE(IDC_OEC_DELIMITERS, OnChangeData)
	ON_EN_CHANGE(IDC_OEC_FILE_EXTENSIONS, OnChangeData)
	ON_CBN_SELCHANGE(IDC_OEC_FILE_TYPE, OnChangeData)
	ON_EN_CHANGE(IDC_OEC_INDENT_SIZE, OnChangeData)
	ON_BN_CLICKED(IDC_OEC_INDENT_TYPE_DEFAULT, OnChangeData)
	ON_BN_CLICKED(IDC_OEC_INDENT_TYPE_NONE, OnChangeData)
	ON_BN_CLICKED(IDC_OEC_INDENT_TYPE_SMART, OnChangeData)
    ON_BN_CLICKED(IDC_OEC_INSERT_SPACES, OnChangeData)
    ON_BN_CLICKED(IDC_OEC_KEEP_TABS, OnChangeData)
    ON_BN_CLICKED(IDC_OEC_NORMALIZE_KEYWORDS, OnChangeData)
    ON_BN_CLICKED(IDC_OEC_COLUMN_MARKERS, OnChangeData)
    ON_EN_CHANGE(IDC_OEC_COLUMN_MARKERS_SET, OnChangeData)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COEClassPage message handlers

LRESULT COEClassPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
    try { EXCEPTION_FRAME;

        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
    _OE_DEFAULT_HANDLER_;

    return 0;
}


BOOL COEClassPage::OnInitDialog() 
{
    m_dataInitialized = false;

	CPropertyPage::OnInitDialog();

    SendDlgItemMessage(IDC_OEC_CLASS, CB_RESETCONTENT);
    
    int count = m_manager.GetClassCount();
    for (int i(0); i < count; i++)
        SendDlgItemMessage(IDC_OEC_CLASS, CB_ADDSTRING, 0, 
              (LPARAM)m_manager.GetClassByPos(i)->GetName().c_str());
    
    SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_RESETCONTENT);
    SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_ADDSTRING, 0, (LPARAM)"Dos (LF/CR)");
    SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_ADDSTRING, 0, (LPARAM)"Unix (LF)");
    SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_ADDSTRING, 0, (LPARAM)"Mac (CR/LF)");
	
    SendDlgItemMessage(IDC_OEC_CLASS, CB_SETCURSEL, 0);
    OnSelChangeLanguage();
    
    SendDlgItemMessage(IDC_OEC_INDENT_SIZE_SPIN, UDM_SETRANGE32, 0, 32);
    SendDlgItemMessage(IDC_OEC_TAB_SIZE_SPIN,    UDM_SETRANGE32, 0, 32);

	return TRUE;
}


void COEClassPage::OnSelChangeLanguage() 
{
    OpenEditor::ClassSettingsPtr settings 
        = m_manager.GetClassByPos(SendDlgItemMessage(IDC_OEC_CLASS, CB_GETCURSEL));

    m_dataInitialized = false;

    char buff[80];

    std::string delim;
    Common::to_printable_str(settings->GetDelimiters().c_str(), delim);
    SetDlgItemText(IDC_OEC_DELIMITERS,     delim.c_str());           
    SetDlgItemText(IDC_OEC_FILE_EXTENSIONS,settings->GetExtensions().c_str());           
    SetDlgItemText(IDC_OEC_INDENT_SIZE,    itoa(settings->GetIndentSpacing(), buff, 10));
    SetDlgItemText(IDC_OEC_TAB_SIZE,       itoa(settings->GetTabSpacing(), buff, 10));   

#if (!(IDC_OEC_INDENT_TYPE_NONE < IDC_OEC_INDENT_TYPE_DEFAULT && IDC_OEC_INDENT_TYPE_DEFAULT < IDC_OEC_INDENT_TYPE_SMART))
#error("check resource indentifiers defenition: IDC_OEC_INDENT_TYPE_NONE, IDC_OEC_INDENT_TYPE_DEFAULT, IDC_OEC_INDENT_TYPE_SMART")
#endif
    CheckRadioButton(IDC_OEC_INDENT_TYPE_NONE, 
                     IDC_OEC_INDENT_TYPE_NONE + 2, 
                     IDC_OEC_INDENT_TYPE_NONE + settings->GetIndentType());

#if (!(IDC_OEC_INSERT_SPACES < IDC_OEC_KEEP_TABS))
#error("check resource indentifiers defenition: IDC_OEC_INSERT_SPACES, IDC_OEC_KEEP_TABS")
#endif
    CheckRadioButton(IDC_OEC_INSERT_SPACES, 
                     IDC_OEC_INSERT_SPACES + 1, 
                     IDC_OEC_INSERT_SPACES + (settings->GetTabExpand() ? 0 : 1));

    SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_SETCURSEL, settings->GetFileCreateAs());

    CheckDlgButton(IDC_OEC_NORMALIZE_KEYWORDS, settings->GetNormalizeKeywords() ? BST_CHECKED : BST_UNCHECKED);

    std::ostringstream out;
    std::vector<int> markers = settings->GetColumnMarkersSet();
    for (std::vector<int>::const_iterator it = markers.begin(); it != markers.end(); ++it)
    {
        if (it != markers.begin()) out << ',';
        out << (*it + 1);
    }
    SetDlgItemText(IDC_OEC_COLUMN_MARKERS_SET, out.str().c_str());           

    m_dataInitialized = true;
}



void COEClassPage::OnChangeData() 
{
    if (m_dataInitialized)
    {
        OpenEditor::ClassSettingsPtr settings 
            = m_manager.GetClassByPos(SendDlgItemMessage(IDC_OEC_CLASS, CB_GETCURSEL));

        char buff[256];
        GetDlgItemText(IDC_OEC_DELIMITERS, buff, sizeof(buff));
        
        std::string delim;
        Common::to_unprintable_str (buff, delim);
	    settings->SetDelimiters(delim, false);

        GetDlgItemText(IDC_OEC_FILE_EXTENSIONS, buff, sizeof(buff));
	    settings->SetExtensions(buff, false);

        GetDlgItemText(IDC_OEC_INDENT_SIZE, buff, sizeof(buff));
	    settings->SetIndentSpacing(atoi(buff), false);
        GetDlgItemText(IDC_OEC_TAB_SIZE, buff, sizeof(buff));
	    settings->SetTabSpacing(atoi(buff), false);

        if (IsDlgButtonChecked(IDC_OEC_INDENT_TYPE_SMART))
	        settings->SetIndentType(2, false);
        else if (IsDlgButtonChecked(IDC_OEC_INDENT_TYPE_DEFAULT))
	        settings->SetIndentType(1, false);
        else
	        settings->SetIndentType(0, false);

        if (IsDlgButtonChecked(IDC_OEC_INSERT_SPACES))
	        settings->SetTabExpand(true, false);
        else
	        settings->SetTabExpand(false, false);

        settings->SetFileCreateAs(SendDlgItemMessage(IDC_OEC_FILE_TYPE, CB_GETCURSEL), false);
        settings->SetNormalizeKeywords(IsDlgButtonChecked(IDC_OEC_NORMALIZE_KEYWORDS) ? true : false, false);

        GetDlgItemText(IDC_OEC_COLUMN_MARKERS_SET, buff, sizeof(buff));
        while (char* comma = strchr(buff, ','))
            *comma = ' ';

        std::vector<int> markers;
        std::istringstream in(buff);

        while (true)
        {
            int val = 0;

            in >> val;

            if (!in)
            {
                if (in.eof()) break;

                if (in.bad() || in.fail()) 
                    AfxMessageBox("Cannot parse column markers input. Please use ',' or ' ' as delimiters.", MB_OK|MB_ICONSTOP);
                AfxThrowUserException();
            }

            markers.push_back(val - 1);
        }
        
        settings->SetColumnMarkersSet(markers);
    }
}

