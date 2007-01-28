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

/*
    09.02.2004 bug fix, bug-reporting activation for oracle errors on grid fetching
	16.12.2004 (Ken Clubok) Add IsStringField method, to support CSV prefix option
    03.01.2005 (ak) Rename IsStringField to IsTextField
*/

#include "stdafx.h"
#include "OCISource.h"
#pragma message ("it should be replaced with grid own settings")
#include "SQLTools.h" // it should be replaced with grid own settings

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OG2;
    using namespace OCI8;

    const std::string OciGridSource::m_emptyString;

OciGridSource::OciGridSource ()
{
    m_AllRowsFetched = true;
    m_ShouldDeleteCursor = false;
}

OciGridSource::~OciGridSource ()
{
}

void OciGridSource::SetCursor (std::auto_ptr<OCI8::AutoCursor>& cursor)
{
    m_AllRowsFetched  = false;

    m_Cursor = cursor;

    Clear();

    if (m_Cursor.get())
    {
        ASSERT(m_Cursor->IsOpen());
        SetCols(m_Cursor->GetFieldCount());
        SetMaxShowLines(GetSQLToolsSettings().GetGridMultilineCount());

        for (int i = 0; i < GetColumnCount(); i++)
        {
            SetColHeader(i, m_Cursor->GetFieldName(i).c_str());
            SetColAlignment(i, m_Cursor->IsNumberField(i) ? ecaRight : ecaLeft);
        }
    }
}

void OciGridSource::Fetch (int fetchedRows)
{
    try { EXCEPTION_FRAME;

        CAbortController abortCtrl(*GetAbortThread(), &m_Cursor->GetConnect());
        abortCtrl.SetActionText("Fetching...");
        time_t stepTime = time(0);

        // move this check to the start of the method
        if (!m_AllRowsFetched && m_Cursor->IsOpen())
        {
            CWaitCursor wait;
            NotificationDisabler disabler(this);
            int nrows = GetCount(IsTableOrientation() ? edVert : edHorz);

            int cols = m_Cursor->GetFieldCount();
            for (int i = 0; i < fetchedRows ; i++)
            {
                if (m_Cursor->Fetch())
                {
                    int row = Append();
                    for (int j = 0; j < cols; j++)
                        m_Cursor->GetString(j, GetString(row, j));

                    if (fetchedRows%100)
                    {
                        time_t curTime = time(0);
                        if (stepTime != curTime)
                        {
                            stepTime = curTime;
                            CString text;
                            text.Format("Fetching, %d records...", row+1);
                            abortCtrl.SetActionText(text);
                        }
                    }
                }
                else
                {
                    m_AllRowsFetched = true;
                    m_Cursor->Close();
                    break;
                }
            }

            if (m_AllRowsFetched)
                m_Cursor->Close();

            disabler.Enable();

            if (IsTableOrientation())
                Notify_ChangedRowsQuantity(nrows, i);
            else
                Notify_ChangedColsQuantity(nrows, i);

            // it's a good idea to autofit colomn size here, just notification needed !!!
        }
    }
    catch (const OciException& x)
    {
        m_AllRowsFetched = true;
        try { m_Cursor->Close(true); } catch (...) {}

        const char* text = (x != 24374) ? x.what()
            : "The result of the query is not shown"
            "\nbecause all columns types are currently not supported.";

        MessageBeep(MB_ICONHAND);
        AfxMessageBox(text, MB_OK|MB_ICONHAND);
    }
    catch (const std::exception& x)
    {
        m_AllRowsFetched = true;
        try { m_Cursor->Close(true); } catch (...) {}
        DEFAULT_HANDLER(x);
    }
    catch (...)
    {
        m_AllRowsFetched = true;
        try { m_Cursor->Close(true); } catch (...) {}
        DEFAULT_HANDLER_ALL;
    }
}

void OciGridSource::ExportText (std::ostream& of, int row, int col) const
{
    try
    {
#pragma message("\t!Test this statement!")
        if ((row == -1 && IsTableOrientation())
        || (col == -1 && !IsTableOrientation()))
        {
            OciGridSource* This = const_cast<OciGridSource*>(this);
            This->Fetch(INT_MAX);
        }
        GridStringSource::ExportText(of, row, col);
    }
    catch (const OciException& x)
    {
        MessageBeep(MB_ICONHAND);
        AfxMessageBox(x);
    }
}

const string& OciGridSource::GetCellStrForExport (int line, int col) const
{
    const string& str = GridStringSource::GetCellStrForExport(line, col);
    return GetSQLToolsSettings().GetGridNullRepresentation() != str ? str : m_emptyString;
}

bool OciGridSource::IsTextField (int col) const
{
	return !(m_Cursor->IsNumberField(col) || m_Cursor->IsDatetimeField(col));
}