/*
    Copyright (C) 2005 Aleksey Kochetov

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

#pragma once
#include <assert.h>
#include <string>
#include <vector>
#include <set>
#include <shlwapi.h>

    class CImageList;
    
namespace Common
{

class ListCtrlDataProvider
{
public:
    static const int MIN_DEF_COL_WIDTH = 80;
    static const int MAX_DEF_COL_WIDTH = 200;

    enum Type { String, Number, Date };

    virtual ~ListCtrlDataProvider () {}

    virtual int getRowCount () const = 0;
    virtual int getColCount () const = 0;
    virtual Type getColType (int col) const = 0;
    virtual const char* getColHeader (int col) const = 0;
    virtual const char* getString (int row, int col) const = 0;
    virtual bool IsVisibleRow (int row) const = 0;
    virtual int compare (int row1, int row2, int col) const = 0;

    virtual int GetMinDefColWidth (int /*col*/) const { return MIN_DEF_COL_WIDTH;  }
    virtual int GetMaxDefColWidth (int /*col*/) const { return MAX_DEF_COL_WIDTH; }

    // optional icons support
    virtual int getImageIndex (int /*row*/) const { return -1; }
    virtual int getStateImageIndex (int /*row*/) const { return -1; }
};

class ListCtrlDataProviderHelper
{
    mutable char m_buffer[128];

protected:
    const char* getStr (const CString& str) const { return str.GetString(); }
    const char* getStr (const std::string& str) const { return str.c_str(); }
    const char* getStr (__int64 i) const { return _i64toa(i, m_buffer, 10); }
    const char* getStr (unsigned __int64 i) const { return _ui64toa(i, m_buffer, 10); }
    const char* getStr (int i) const { return itoa(i, m_buffer, 10); }
    const char* getStr (tm time) const { 
        static tm null = { 0 };
        if (memcmp(&time, &null, sizeof(tm))) {
            strftime(m_buffer, sizeof m_buffer, "%Y-%m-%d %H:%M:%S", &time);
            m_buffer[sizeof(m_buffer)-1] = 0;
        } else
            m_buffer[0] = 0;
        return m_buffer; 
    }
    const char* getStrTimeT (time_t t) const { 
        if (const tm* ptm = localtime(&t)) {
            strftime(m_buffer, sizeof m_buffer, "%Y-%m-%d %H:%M:%S", ptm);
            m_buffer[sizeof(m_buffer)-1] = 0;
        } else
            m_buffer[0] = 0;
        return m_buffer; 
    }

    static int comp (const CString& s1, const CString& s2) { return stricmp(s1.GetString(), s2.GetString()); }
    static int comp (const std::string& s1, const std::string& s2) { return stricmp(s1.c_str(), s2.c_str()); }
    static int comp (const char *s1, const char *s2) { return stricmp(s1, s2); }
    static int comp (int val1, int val2) { return val1 == val2 ? 0 : (val1 < val2 ? -1 : 1); }
    static int comp (__int64 val1, __int64 val2) { return val1 == val2 ? 0 : (val1 < val2 ? -1 : 1); }
    static int comp (unsigned __int64 val1, unsigned __int64 val2) { return val1 == val2 ? 0 : (val1 < val2 ? -1 : 1); }
    static int compTimeT (time_t val1, time_t val2) { return val1 == val2 ? 0 : (val1 < val2 ? -1 : 1); }
    static int comp (tm val1, tm val2) 
    { 
        if (val1.tm_year != val2.tm_year)
            return val1.tm_year < val2.tm_year ? -1 : 1;

        if (val1.tm_mon != val2.tm_mon)
            return val1.tm_mon < val2.tm_mon ? -1 : 1;

        if (val1.tm_mday != val2.tm_mday)
            return val1.tm_mday < val2.tm_mday ? -1 : 1;

        if (val1.tm_hour != val2.tm_hour)
            return val1.tm_hour < val2.tm_hour ? -1 : 1;

        if (val1.tm_min != val2.tm_min)
            return val1.tm_min < val2.tm_min ? -1 : 1;

        if (val1.tm_sec != val2.tm_sec)
            return val1.tm_sec < val2.tm_sec ? -1 : 1;

        return 0; 
    }
};


class ListCtrlManager
{
public:
    enum ESortDir { ASC = 1, DESC = -1 };
    enum EFilterOperation { CONTAIN = 0, START_WITH = 1, EXACT_MATCH = 2 }; 
    struct FilterComponent
    {
        EFilterOperation operation;
        string           value;

        FilterComponent () : operation(CONTAIN) {}
        FilterComponent (const char* v, EFilterOperation op = CONTAIN) : operation(op), value(v) {}
        FilterComponent (const string& v, EFilterOperation op = CONTAIN) : operation(op), value(v) {}
    };
    typedef std::vector<FilterComponent> FilterCollection;

private:
    CListCtrl& m_list; 
    ListCtrlDataProvider& m_dataAdapter;
    int m_sortColumn;
    FilterCollection m_filter;
    bool m_filterEmpty;
    bool m_initalized;
    bool m_settingFilter;
    bool m_bAutoResize;
    enum ESortDir m_sortDir;

public:
    ListCtrlManager (CListCtrl&, ListCtrlDataProvider&, bool bAutoResize = true);

    CListCtrl& GetListCtrl () { return m_list; }
    const CListCtrl& GetListCtrl () const { return m_list; }

    void SetSortColumn (int col, ESortDir sortDir = ASC);
    void GetSortColumn (int& col, ESortDir& sortDir) const;
    void SetFilter (const FilterCollection&);
    void GetFilter (FilterCollection&) const;

    void GetColumnHeaders (std::vector<std::string>&) const;
    void GetColumnValues (int col, std::set<std::string>&) const;

    const char* GetString (int row, int col) const { return m_dataAdapter.getString(row, col); }
    int GetStateImageIndex (int row) const         { return m_dataAdapter.getStateImageIndex(row); }

    void OnCreate ();
    void OnRefresh (bool autosizeColumns = false);
    void OnFilterChange (int col);
    void OnFilterBtnClick (int col);
    void OnSort (int col);

    void OnAppendEntry ();
    void OnUpdateEntry (int entry);
    void OnDeleteEntry (int entry);

    void SelectEntry (int entry);

protected:
    void setItemFilter (int col, const char* str);
    void refreshFilter ();
    bool isMatchedToFilter (int row);
    void doSort();
    static int CALLBACK CompProc (LPARAM lparam1, LPARAM lparam2, LPARAM lparam3);
};

inline
void ListCtrlManager::SetSortColumn (int col, ESortDir sortDir) 
{ 
    m_sortColumn = col; 
    m_sortDir = sortDir; 
}

inline
void ListCtrlManager::GetSortColumn (int& col, ESortDir& sortDir) const
{
    col = m_sortColumn; 
    sortDir = m_sortDir; 
}

inline
void ListCtrlManager::GetFilter (FilterCollection& filter) const
{
    //if (!m_filterEmpty)
        filter = m_filter;
    //else
    //    filter.clear();
}


}//namespace Common
