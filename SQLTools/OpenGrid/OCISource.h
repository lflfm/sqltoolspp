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
// 16.12.2004 (Ken Clubok) Add IsStringField method, to support CSV prefix option
// 03.01.2005 (ak) Rename IsStringField to IsTextField

#ifndef __OCISOURCE_H__
#define __OCISOURCE_H__

#include <memory>
#include <OCI8/ACursor.h>
#include "GridSourceBase.h"

class OciGridSource : public OG2::GridStringSource
{
public:
    OciGridSource ();
    ~OciGridSource ();

    void SetCursor (std::auto_ptr<OCI8::AutoCursor>& cursor);
    void ApplyColumnFit ();

    virtual void ExportText (std::ostream&, int row = -1, int col = -1) const;
    virtual const string& GetCellStrForExport (int line, int col) const;
    bool IsTextField (int col) const; 

    void Fetch (int fetchedRows /*= 60*/);
    bool AllRowsFetched () const    { return m_AllRowsFetched; }

protected:
    std::auto_ptr<OCI8::AutoCursor> m_Cursor;
    bool m_AllRowsFetched;
    bool m_ShouldDeleteCursor;
    static const std::string m_emptyString;
};

#endif//__OCISOURCE_H__
