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
#include <COMMON\AppGlobal.h>
#include "resource.h"
#include "SQLToolsSettings.h"
#include "SQLTools.h"
#include "StatView.h"
#include <OCI8/BCursor.h>
#include <OpenGrid/GridSourceBase.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


    using namespace std;

StatSet StatGauge::m_StatSet;

void StatSet::Load ()
{
    std::string path;
    Global::GetSettingsPath(path);
    path += "\\sesstat.dat";
    
    m_StatNames.clear();
    m_MapNameToLine.clear();
  
    std::ifstream is(path.c_str());
    std::string buffer;
    for (int row = 0; std::getline(is, buffer, '\n'); row++) 
    {
        m_StatNames.push_back(buffer);
        m_MapNameToLine.insert(std::map<string,int>::value_type(buffer, row));
    }
}

void StatSet::Flush ()
{
    m_StatNames.clear();
    m_MapNameToLine.clear();
}

StatGauge::StatGauge ()
{
}

void StatGauge::Open (OciConnect& connect)
{
    LoadStatSet();

    try 
    {
        /*
		{
            OciCursor curs(connect, "select sid from v$session where audsid = userenv('SESSIONID')", 1);
            
            curs.Execute();
            
            if (curs.Fetch())
                curs.GetString(0, m_SID);

            curs.Close();
        }
		*/

		m_SID = connect.GetSessionSid();

        m_MapRowToNum.clear();

        OciCursor curs(connect, "select name, statistic# from v$statname", 300);
        
        curs.ExecuteShadow();
        while (curs.Fetch()) 
        {
            map<string,int>::const_iterator 
                it = m_StatSet.m_MapNameToLine.find(curs.ToString(0));

            if (it != m_StatSet.m_MapNameToLine.end()) 
            {
                m_MapRowToNum.insert(std::map<int,int>::value_type(it->second, curs.ToInt(1)));
            }
        }

        curs.Close();

		connect.SetSession();

        Calibrate(connect);
    } 
    catch (...) 
    {
		connect.SetSession();

        m_StatisticAccessible = false;
        throw;
    }
    m_StatisticAccessible = true;
}

void StatGauge::Close ()
{
    m_StatisticAccessible = false;
}

void StatGauge::LoadStatistics (OciConnect& connect, map<int,int>& data)
{
    data.clear();

    OciCursor stats(connect, "select statistic#,value from v$sesstat where sid = :sid", 300);
    stats.Bind(":sid", m_SID.c_str());
    stats.ExecuteShadow();
    while(stats.Fetch()) 
        data.insert(map<int,int>::value_type(stats.ToInt(0), stats.ToInt(1)));

	connect.SetSession();
}

void StatGauge::Calibrate (OciConnect& connect)
{
    m_CalibrateData.clear();

    map<int,int> probes[2];
    LoadStatistics(connect, probes[0]);
    LoadStatistics(connect, probes[1]);

    map<int,int>::const_iterator 
        it(probes[0].begin()), end(probes[0].end()),
        it2, end2(probes[1].end());

    for (; it != end; it++)
    {
        it2 = probes[1].find(it->first);
        if (it2 != end2)
        {
            m_CalibrateData.insert(
                map<int,int>::value_type(
                    it->first, it2->second - it->second));
        }
        else
            _ASSERTE(0);
    }
}

void StatGauge::BeginMeasure (OciConnect& connect)
{
    if (m_StatisticAccessible)
        LoadStatistics(connect, m_Statistics[0]);
}

void StatGauge::EndMeasure (OciConnect& connect)
{
    if (m_StatisticAccessible)
        LoadStatistics(connect, m_Statistics[1]);
}

int StatGauge::GetStatistic (int row)
{
    if (m_StatisticAccessible)
    {
        map<int,int>::const_iterator 
            rowIt = m_MapRowToNum.find(row);

        if (rowIt != m_MapRowToNum.end())
        {
            map<int,int>::const_iterator 
                deltaIt(m_CalibrateData.find(rowIt->second)),
                prb1It (m_Statistics[0].find(rowIt->second)),
                prb2It (m_Statistics[1].find(rowIt->second));

            if (deltaIt != m_CalibrateData.end()
                && prb1It != m_Statistics[0].end()
                && prb2It != m_Statistics[1].end())
            {
                return (prb2It->second) - (prb1It->second) - (deltaIt->second);
            }
        }
        //_ASSERT(0);
    }
    return INT_MAX;
}

int StatGauge::GetSessionStatistic (int row)
{
    if (m_StatisticAccessible)
    {
        map<int,int>::const_iterator 
            rowIt = m_MapRowToNum.find(row);

        if (rowIt != m_MapRowToNum.end())
        {
            map<int,int>::const_iterator 
                prb2It(m_Statistics[1].find(rowIt->second));

            if (prb2It  != m_Statistics[1].end())
            {
                return (prb2It->second);
            }
        }
        //_ASSERT(0);
    }
    return INT_MAX;
}

/////////////////////////////////////////////////////////////////////////////
// CStatView
StatGauge CStatView::m_StatGauge;
set<CStatView*> CStatView::m_GridFamily;

void CStatView::OpenAll (OciConnect& connect)
{
    m_StatGauge.Open(connect);

    std::set<CStatView*>::iterator it = m_GridFamily.begin();

    for (; it != m_GridFamily.end(); ++it)
        (*it)->LoadStatNames();
}

void CStatView::CloseAll ()
{
    m_StatGauge.Close();

    std::set<CStatView*>::iterator it = m_GridFamily.begin();

    for (; it != m_GridFamily.end(); ++it)
        (*it)->Clear();
}

void CStatView::LoadStatNames ()
{
    if (m_pStrSource->GetCount(edVert))
        Clear();

    m_StatGauge.LoadStatSet();
    int count = m_StatGauge.GetRows();

    for (int i(0); i < count; i++) 
    {
        int row = m_pStrSource->Append();
        m_pStrSource->SetString(row, 0, m_StatGauge.GetStatName(i));
        m_pStrSource->SetString(row, 1, "");
        m_pStrSource->SetString(row, 2, "");
        m_pStrSource->SetImageIndex(i, 6);
    }

    if (m_hWnd)
        m_pManager->Refresh();
}

void CStatView::ShowStatistics ()
{
    if (m_StatGauge.StatisticAccessible()) 
    {
        int nrows = m_pStrSource->GetCount(edVert);
        
        for (int row(0) ; row < nrows; row++) 
        {
            bool failure = false;
            char szBuff[18];
            int val = m_StatGauge.GetStatistic(row);
            failure = (val == INT_MAX);
            m_pStrSource->SetString(row, 1, (!failure) ? itoa(val, szBuff, 10) : "");
            val = m_StatGauge.GetSessionStatistic(row);
            failure |= (val == INT_MAX);
            m_pStrSource->SetString(row, 2, (!failure) ? itoa(val, szBuff, 10) : "");
            m_pStrSource->SetImageIndex(row, (!failure) ? 6 : 3);
        }

        m_pManager->Refresh();
    }
}

    CImageList CStatView::m_imageList;

IMPLEMENT_DYNCREATE(CStatView, StrGridView)

CStatView::CStatView ()
{
    SetVisualAttributeSetName("Statistics Window");

    _ASSERTE(m_pStrSource);
    m_pStrSource->SetCols(3);
    m_pStrSource->SetColCharWidth(0, 32);
    m_pStrSource->SetColCharWidth(1, 16);
    m_pStrSource->SetColCharWidth(2, 16);
    m_pStrSource->SetColHeader(0, "Name");
    m_pStrSource->SetColHeader(1, "Last");
    m_pStrSource->SetColAlignment(1, ecaRight);
    m_pStrSource->SetColHeader(2, "Total");
    m_pStrSource->SetColAlignment(2, ecaRight);
    m_pStrSource->SetShowRowEnumeration(false);
    m_pStrSource->SetShowTransparentFixCol(true);

    m_pStrSource->SetFixSize(eoVert, edHorz, 3);

    m_pManager->m_Options.m_HorzLine  = false;
    m_pManager->m_Options.m_VertLine  = false;
    m_pManager->m_Options.m_RowSelect = true;
    m_pManager->m_Options.m_ExpandLastCol = false;

    if (!m_imageList.m_hImageList)
        m_imageList.Create(IDB_IMAGELIST2, 16, 64, RGB(0,255,0));

    m_pStrSource->SetImageList(&m_imageList, FALSE);

    if (GetSQLToolsSettings().GetSessionStatistics())
        LoadStatNames();

    m_GridFamily.insert(this);
}

CStatView::~CStatView ()
{
    try { EXCEPTION_FRAME;
        m_GridFamily.erase(this);
    } _DESTRUCTOR_HANDLER_;
}

/*
BEGIN_MESSAGE_MAP(CStatView, StrGridView)
	//{{AFX_MSG_MAP(CStatView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
*/
