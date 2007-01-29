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
#include "StringTable.h"

StringTable::StringTable (int ncols, int nrows) 
: m_table("StringTable::m_table"), 
m_charWidths("StringTable::m_charWidths"),
m_ColumnCount(ncols)                                  
{ 
    if (nrows != -1) 
        m_table.reserve(nrows); 
    m_charWidths.resize(ncols); 
}

void StringTable::Format (int ncols, int nrows)                     
{ 
    m_ColumnCount = ncols; 
    m_charWidths.clear();
    m_charWidths.resize(ncols); 
    m_table.clear(); 
    if (nrows != -1) 
        m_table.reserve(nrows); 
}

void StringTable::GetMaxColumnLength (nvector<size_t>& result) const
{
    result.clear();
    result.resize(m_ColumnCount, 0);
    vector<StringRow>::const_iterator rIt = m_table.begin();
    for (; rIt != m_table.end(); ++rIt)
    {
        for (int col = 0; col < m_ColumnCount; col++)
        {
            size_t len = rIt->GetString(col).size();
            if (result[col] < len) 
				result[col] = len;
        }
    }
}

void StringTable::GetMaxRowLength (nvector<size_t>& result) const
{
    result.clear();
    result.resize(m_table.size(), 0);
    for (int col = 0; col < m_ColumnCount; col++)
    {
		vector<StringRow>::const_iterator rIt = m_table.begin();
		for (int row = 0; rIt != m_table.end(); ++rIt, ++row)
		{
			size_t len = rIt->GetString(col).size();
			if (result[row] < len) 
				result[row] = len;
        }
    }
}

    int calc_lines (const string& str, int lim)
    {
        int nlines = 1;
        if (nlines < lim)
        {
            string::const_iterator it = str.begin();

            for (; it != str.end(); ++it)
                if (*it == '\n' && ++nlines == lim) 
                    break;

			if (nlines > 1
			&& str.size() > 0
			&& it == str.end())
			{
				if (*(--it) == '\n')
				{
					nlines--;
				}
			}
			
			if (it != str.end())
			{
			    if (*(it) == '\r')
				{
					nlines--;
				}
			}
        }
        return nlines;
    }

int StringTable::GetMaxLinesInColumn (int col, int lim) const
{
    int nlines = 1;
    vector<StringRow>::const_iterator rIt = m_table.begin();
    for (; rIt != m_table.end(); ++rIt)
    {
		int nl = calc_lines(rIt->GetString(col), lim);
        if (nlines < nl)
            nlines = nl;
    }
    return nlines;
}

int StringTable::GetMaxLinesInRow (int row, int lim) const
{
    int nlines = 1;
    const StringRow& roW = m_table.at(row);
    for (int col = 0; col < m_ColumnCount; col++)
    {
	    int nl = calc_lines(roW.GetString(col), lim);
        if (nlines < nl)
            nlines = nl;
    }
    return nlines;
}
