#include "stdafx.h"
#include <set>
#include <string>
#include "ListCtrlDataProvider.h"
#include "ManagedListCtrlFilterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace Common
{

ListCtrlManager::ListCtrlManager (CListCtrl& list, ListCtrlDataProvider& dataAdapter)
: m_list(list), 
m_dataAdapter(dataAdapter),
m_sortColumn(0), 
m_sortDir(ASC), 
m_filterEmpty(true), 
m_initalized(false),
m_settingFilter(false)
{
}

void ListCtrlManager::OnCreate ()
{
    m_list.ModifyStyle(0, LVS_REPORT|LVS_SINGLESEL|LVS_OWNERDATA);
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_HEADERDRAGDROP);

    CHeaderCtrl* header = m_list.GetHeaderCtrl();
    header->ModifyStyle(0, HDS_FILTERBAR);	

    HDITEM hdItem;
    memset(&hdItem, 0, sizeof(hdItem));

    m_settingFilter = true;

    for (int i = 0, count = m_dataAdapter.getColCount(); i < count; ++i)
    {
        m_list.InsertColumn(i, m_dataAdapter.getColHeader(i), 
            m_dataAdapter.getColType(i) == ListCtrlDataProvider::Number ? LVCFMT_RIGHT : LVCFMT_LEFT);

        hdItem.mask = HDI_FILTER;
        hdItem.type = (m_dataAdapter.getColType(i) == ListCtrlDataProvider::Number)
            ? (HDFT_ISNUMBER|HDFT_HASNOVALUE) : (HDFT_ISSTRING|HDFT_HASNOVALUE);

        header->SetItem(i, &hdItem);
    }

    m_settingFilter = false;

    if (m_filter.empty())
        m_filter.resize(m_dataAdapter.getColCount());

    m_initalized = true;

    OnRefresh(true);
}

void ListCtrlManager::setItemFilter (int col, const char* str)
{
    assert(col < m_dataAdapter.getColCount());

    HDITEM hdItem;
    memset(&hdItem, 0, sizeof(hdItem));
	hdItem.mask = HDI_FILTER;

    if (m_dataAdapter.getColType(col) == ListCtrlDataProvider::Number)
    {
		hdItem.type = HDFT_ISNUMBER;
        if (!str || !strlen(str)) 
            hdItem.type |= HDFT_HASNOVALUE;
        __int64 val = str ? _atoi64(str) : 0;
        hdItem.pvFilter = &val;
        m_list.GetHeaderCtrl()->SetItem(col, &hdItem);
    }
    else
    {
		hdItem.type = HDFT_ISSTRING;
        if (!str || !strlen(str)) 
            hdItem.type |= HDFT_HASNOVALUE;
	    HDTEXTFILTER hdTextFilter;
        memset(&hdTextFilter, 0, sizeof(hdTextFilter));
	    hdTextFilter.pszText = (LPSTR)str;
	    //hdTextFilter.cchTextMax = m_filter.at(i).size()+1;
        hdItem.pvFilter = &hdTextFilter;
        m_list.GetHeaderCtrl()->SetItem(col, &hdItem);
    }
}

void ListCtrlManager::SetFilter (const FilterCollection& filter)
{
    ASSERT(filter.empty() || filter.size() == (unsigned)m_dataAdapter.getColCount());

    if (filter.empty() || filter.size() == (unsigned)m_dataAdapter.getColCount())
    {
        m_filter = filter;

        m_settingFilter = true;

        for (int i = 0, count = m_dataAdapter.getColCount(); i < count; ++i)
            setItemFilter(i, !filter.empty() ?  filter.at(i).value.c_str() : 0);

        m_settingFilter = false;

        OnRefresh();
    }
}

void ListCtrlManager::GetColumnHeaders (std::vector<std::string>& _headers) const
{
    std::vector<std::string> headers;

    for (int col = 0, colCount = m_dataAdapter.getColCount(); col < colCount; ++col)
	    headers.push_back(m_dataAdapter.getColHeader(col));

    std::swap(_headers, headers);
}

void ListCtrlManager::GetColumnValues (int col, std::set<std::string>& _data) const
{
    std::set<std::string> data;

    for (int row = 0, rowCount = m_dataAdapter.getRowCount(); row < rowCount; ++row)
        if (m_dataAdapter.IsVisibleRow(row))
	        data.insert(m_dataAdapter.getString(row, col));

    std::swap(_data, data);
}

void ListCtrlManager::OnRefresh (bool autosizeColumns)
{
    CWaitCursor wait;

    m_list.SetRedraw(false);

    m_list.DeleteAllItems();

    if (autosizeColumns
    && (LVS_TYPEMASK & m_list.GetStyle()) == LVS_LIST)
    {
        m_list.SetColumnWidth(0, m_dataAdapter.GetMinDefColWidth(0));
    }

    int colCount = m_dataAdapter.getColCount();
    int rowCount = m_dataAdapter.getRowCount();

    LV_ITEM lvitem;
    memset(&lvitem, 0, sizeof lvitem);
    lvitem.mask = LVIF_TEXT|LVIF_PARAM;
    if (m_list.GetImageList(LVSIL_SMALL)) lvitem.mask |= LVIF_IMAGE;
    //if (m_list.GetImageList(LVSIL_STATE)) lvitem.mask |= LVIF_STATE;

    for (int row = 0; row < rowCount; ++row)
    {
        if (m_dataAdapter.IsVisibleRow(row) && isMatchedToFilter(row))
        {
            int inx = -1;
            for (int col = 0; col < colCount; ++col)
            {
                if (!col)
                {
                    lvitem.iImage    = m_dataAdapter.getImageIndex(row);
                    lvitem.iItem     = row;
                    //lvitem.stateMask = LVIS_STATEIMAGEMASK;
                    //lvitem.state     = (m_dataAdapter.getStateImageIndex(row) +1) << (8 + 4);
                    lvitem.pszText   = LPSTR_TEXTCALLBACK;
                    lvitem.lParam    = row;

                    inx = m_list.InsertItem(&lvitem);
                }
                else
                    m_list.SetItemText(inx, col, LPSTR_TEXTCALLBACK);
            }
        }
    }

    if (autosizeColumns)
    {
        for (int i = 0, count = m_dataAdapter.getColCount(); i < count; ++i)
        {
            m_list.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
            if (m_list.GetColumnWidth(i) < m_dataAdapter.GetMinDefColWidth(i))
                m_list.SetColumnWidth(i, m_dataAdapter.GetMinDefColWidth(i));
            if (m_list.GetColumnWidth(i) > m_dataAdapter.GetMaxDefColWidth(i))
                m_list.SetColumnWidth(i, m_dataAdapter.GetMaxDefColWidth(i));
        }

        m_list.GetHeaderCtrl()->Invalidate();
    }

    doSort();

    m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
    m_list.EnsureVisible(0, FALSE);

    m_list.SetRedraw(true);
}

void ListCtrlManager::OnFilterChange (int)
{
    if (m_initalized)
    {
        refreshFilter();
        if (! m_settingFilter)
            OnRefresh();
    }
}

void ListCtrlManager::OnFilterBtnClick (int col)
{
    CRect rc;
    m_list.GetHeaderCtrl()->GetItemRect(col, rc);
    m_list.GetHeaderCtrl()->ClientToScreen(rc);

    assert(m_filter.empty() || col < (int)m_filter.size());

    if (m_filter.empty() || col < (int)m_filter.size())
    {
        CManagedListCtrlFilterDlg(&m_list, rc, *this, col).DoModal();
        m_list.SetFocus();
        m_list.Invalidate();
        m_list.GetHeaderCtrl()->Invalidate();
    }
}

void ListCtrlManager::OnSort (int col)
{
    m_sortDir = (m_sortColumn != col || m_sortDir == DESC) ? ASC : DESC;
    m_sortColumn = col;
    doSort();
}

void ListCtrlManager::refreshFilter ()
{
    char buff[1024];
    bool filterEmpty = true;
    FilterCollection filter;

	HDITEM hdItem;
    memset(&hdItem, 0, sizeof(hdItem));
	hdItem.mask = HDI_FILTER;
	HDTEXTFILTER hdTextFilter;
    memset(&hdTextFilter, 0, sizeof(hdTextFilter));
    
	hdTextFilter.pszText = buff;
	hdTextFilter.cchTextMax = sizeof(buff)-1;
    
    CHeaderCtrl* header = m_list.GetHeaderCtrl();
    for (unsigned int i = 0, count = header->GetItemCount(); i < count; ++i)
    {
        if (m_dataAdapter.getColType(i) == ListCtrlDataProvider::Number)
        {
            __int64 val = 0;
		    hdItem.type = HDFT_ISNUMBER;
            hdItem.pvFilter = &val;

            if (header->GetItem(i, &hdItem) && !(HDFT_HASNOVALUE & hdItem.type))
			    _i64toa(val, buff, 10);
            else
			    buff[0] = 0;
        }
        else
        {
		    hdItem.type = HDFT_ISSTRING;
		    hdItem.pvFilter = &hdTextFilter;

		    if (header->GetItem(i, &hdItem) && !(HDFT_HASNOVALUE & hdItem.type))
			    buff[sizeof(buff)-1] = 0;
            else
			    buff[0] = 0;
        }

        if (i < m_filter.size())
            filter.push_back(FilterComponent(buff,  m_filter.at(i).operation));
        else
            filter.push_back(FilterComponent(buff,  
                m_dataAdapter.getColType(i) == ListCtrlDataProvider::Number
                ? ListCtrlManager::START_WITH : ListCtrlManager::CONTAIN));

        if (!filter.back().value.empty())
            filterEmpty = false;
    }

    std::swap(m_filter, filter);
    std::swap(m_filterEmpty, filterEmpty);
}

bool ListCtrlManager::isMatchedToFilter (int row)
{
    if (!m_filterEmpty)
    {
        int colCount = m_dataAdapter.getColCount();
        assert(m_filter.size() == (unsigned)colCount);
        
        for (int col = 0; col < colCount; ++col)
        {
            const std::string& filter = m_filter.at(col).value;

            if (!filter.empty())
            {
                const char* val = m_dataAdapter.getString(row, col);

                switch (m_filter.at(col).operation)
                {
                case START_WITH:
                    if (::StrCmpNI(val, filter.c_str(), (int)filter.size()))
                        return false;
                    break;
                case CONTAIN:
                    if (!::StrStrI(val, filter.c_str()))
                        return false;
                    break;
                case EXACT_MATCH:
                    if (::StrCmpI(val, filter.c_str()))
                        return false;
                    break;
                }
            }
        }
    }
    return true;
}


void ListCtrlManager::doSort()
{
    if (m_sortColumn != -1)
        m_list.SortItems(CompProc, (LPARAM)this);

    CHeaderCtrl* header = m_list.GetHeaderCtrl();

    HDITEM hditem;

    for (int i = 0, count = header->GetItemCount(); i < count; i++)
    {
        hditem.mask = HDI_FORMAT;
        header->GetItem(i, &hditem);
        hditem.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP);
        header->SetItem(i, &hditem);
    }
    
    if (m_sortColumn == -1) return;

    hditem.mask = HDI_FORMAT;
    header->GetItem(m_sortColumn, &hditem);
    hditem.fmt |= (m_sortDir == ASC) ? HDF_SORTDOWN : HDF_SORTUP;
    header->SetItem(m_sortColumn, &hditem);

    int selInx = m_list.GetNextItem(-1, LVNI_SELECTED);
    if (selInx != -1) 
        m_list.EnsureVisible(selInx, FALSE);
}

int CALLBACK ListCtrlManager::CompProc (LPARAM lparam1, LPARAM lparam2, LPARAM lparam3)
{
    ListCtrlManager* This = (ListCtrlManager*)lparam3;
    int result = This->m_dataAdapter.compare((int)lparam1, (int)lparam2, This->m_sortColumn);
    return (This->m_sortDir == ASC) ? result : -result;
}


void ListCtrlManager::OnAppendEntry ()
{
    int icount = m_list.GetItemCount();
    int last_row = -1;
    for (int i = 0; i < icount; ++i)
    {
        int row = (int)m_list.GetItemData(i);
        if (last_row < row) last_row = row;
    }

    int rowCount = m_dataAdapter.getRowCount();
    int colCount = m_dataAdapter.getColCount();

    for (int row = last_row + 1; row < rowCount; ++row)
    {
        if (m_dataAdapter.IsVisibleRow(row) && isMatchedToFilter(row))
        {
            int inx = -1;
            for (int col = 0; col < colCount; ++col)
            {
                if (!col)
                {
                    //inx = m_list.InsertItem(row, LPSTR_TEXTCALLBACK);
                    //m_list.SetItemData(inx, row);

                    LV_ITEM lvitem;
                    memset(&lvitem, 0, sizeof lvitem);
                    lvitem.mask = LVIF_TEXT|LVIF_PARAM;
                    if (m_list.GetImageList(LVSIL_SMALL)) lvitem.mask |= LVIF_IMAGE;

                    lvitem.iImage    = m_dataAdapter.getImageIndex(row);
                    lvitem.iItem     = row;
                    lvitem.pszText   = LPSTR_TEXTCALLBACK;
                    lvitem.lParam    = row;

                    inx = m_list.InsertItem(&lvitem);

                    m_list.SetItemState(inx, LVIS_SELECTED, LVIS_SELECTED);
                    m_list.EnsureVisible(inx, FALSE);
                }
                else
                    m_list.SetItemText(inx, col, LPSTR_TEXTCALLBACK);
            }
        }
    }
}

void ListCtrlManager::OnUpdateEntry (int entry)
{
    int icount = m_list.GetItemCount();
    for (int i = 0; i < icount; ++i)
    {
        if (entry == (int)m_list.GetItemData(i))
        {
            int colCount = m_dataAdapter.getColCount();
            for (int col = 0; col < colCount; ++col)
                m_list.SetItemText(i, col, LPSTR_TEXTCALLBACK);
            m_list.EnsureVisible(i, FALSE);
            m_list.RedrawItems(i, i);
            m_list.UpdateWindow();
            break;
        }
    }
}

void ListCtrlManager::OnDeleteEntry (int entry)
{
    int icount = m_list.GetItemCount();
    for (int i = 0; i < icount; ++i)
    {
        if (entry == (int)m_list.GetItemData(i))
        {
            m_list.DeleteItem(i);
            if (i == icount - 1) i--;
            m_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
            m_list.EnsureVisible(i, FALSE);
            break;
        }
    }
}

void ListCtrlManager::SelectEntry (int entry)
{
    LVFINDINFO fi;
    memset(&fi, 0, sizeof(fi));
    fi.flags = LVFI_PARAM;
    fi.lParam = entry;
    int lastItem = m_list.FindItem(&fi);
    m_list.SetItemState(lastItem, LVIS_SELECTED, LVIS_SELECTED);
    m_list.EnsureVisible(lastItem, FALSE);
}

}//namespace Common
