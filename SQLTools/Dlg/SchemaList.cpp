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
#include "COMMON\ExceptionHelper.h"
#include "Dlg/SchemaList.h"
#include <OCI8/BCursor.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSchemaList

CSchemaList::CSchemaList (OciConnect& connect, BOOL bAllSchemas)
: m_connect(connect),
m_bAllSchemas(bAllSchemas)
{
}

BEGIN_MESSAGE_MAP(CSchemaList, CComboBox)
	//{{AFX_MSG_MAP(CSchemaList)
	ON_CONTROL_REFLECT(CBN_SETFOCUS, OnSetFocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSchemaList message handlers

  LPSCSTR cszSchemasSttm =
    "SELECT username FROM sys.all_users a_u"
    " WHERE username <> USER"
    " AND (Exists (SELECT NULL FROM sys.all_source a_s WHERE a_s.owner = a_u.username)"
    " OR Exists (SELECT NULL FROM sys.all_triggers a_t WHERE a_t.owner = a_u.username))";

  LPSCSTR cszAllSchemasSttm =
    "SELECT username from sys.all_users a_u"
    " WHERE username <> USER";

void CSchemaList::OnSetFocus()
{
    if (GetCount() <= 1) {
        CWaitCursor wait;

        try { EXCEPTION_FRAME;

            OciCursor cursor(m_connect, m_bAllSchemas ? cszAllSchemasSttm : cszSchemasSttm);

            cursor.Execute();

            while (cursor.Fetch())
                AddString(cursor.ToString(0).c_str());

        }
        _DEFAULT_HANDLER_
    }
}

void CSchemaList::UpdateSchemaList ()
{
    ResetContent();

    try { EXCEPTION_FRAME;

        OciCursor cursor(m_connect, "SELECT USER FROM sys.dual", 1);
        cursor.Execute();
        cursor.Fetch();
        SetCurSel(AddString(cursor.ToString(0).c_str()));

    }
    _DEFAULT_HANDLER_
}

void CSchemaList::PreSubclassWindow()
{
	CComboBox::PreSubclassWindow();
    UpdateSchemaList();
}

