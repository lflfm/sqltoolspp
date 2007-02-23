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

#pragma once
#ifndef __STATVIEW_H__
#define __STATVIEW_H__

#include <map>
#include <set>
#include <vector>
#include <string>
#include <OpenGrid/GridView.h>

    class CSQLToolsApp;
    using std::set;
    using std::map;
    using std::vector;
    using std::string;
    using namespace OG2;

    struct StatSet  // One on an application
    {
        vector<string>  m_StatNames;
        map<string,int> m_MapNameToLine;
        std::string     m_stat_mode;

        bool IsEmpty () const                   { return m_StatNames.empty(); }
        int  GetRows () const                   { return m_StatNames.size(); }
        const char* GetStatName (int row) const { return m_StatNames[row].c_str(); }
        const string& GetStatMode() const       { return m_stat_mode; }

        void Load  (OciConnect& connect);
        void Flush ();
    };

    class StatGauge  // On on a OciConnect
    {
    public:
        StatGauge ();

        void LoadStatSet (OciConnect& connect, bool force = false)         { if (force || m_StatSet.IsEmpty()) m_StatSet.Load(connect); };
        void OnChangeStatSet ();

        bool StatisticAccessible ()                   { return m_StatisticAccessible; }
        int  GetRows () const                         { return m_StatSet.GetRows(); }
        const char* GetStatName (int row) const       { return m_StatSet.GetStatName(row); }
        const string& GetStatMode() const             { return m_StatSet.GetStatMode(); }

        void Open  (OciConnect& connect);
        void Close ();

        void LoadStatistics (OciConnect& connect, map<int,int>&);
        void Calibrate (OciConnect& connect);

        void BeginMeasure (OciConnect& connect);
        void EndMeasure   (OciConnect& connect);

        int GetStatistic (int row);
        int GetSessionStatistic (int row);

    private:
        static StatSet m_StatSet;

        bool m_StatisticAccessible;

        map<int,int> m_MapRowToNum, 
                     m_CalibrateData;
        map<int,int> m_Statistics[2];

        string m_SID;
    };


class CStatView : public StrGridView
{
public:
	CStatView();
	~CStatView();

    CSQLToolsApp* GetApp () const              { return (CSQLToolsApp*)AfxGetApp(); }

    void LoadStatNames (OciConnect& connect);
    void BeginMeasure (OciConnect& connect)    { m_StatGauge.BeginMeasure(connect); }
    void EndMeasure (OciConnect& connect)      { m_StatGauge.EndMeasure(connect); }
    void ShowStatistics ();

    bool IsEmpty () const            { return m_pManager->m_pSource->GetCount(edVert) ? false : true; }

    static void OpenAll  (OciConnect& connect);
    static void CloseAll ();

    //{{AFX_VIRTUAL(CStatView)
    protected:
    //}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CStatView)
	//}}AFX_MSG
//    DECLARE_MESSAGE_MAP()
	DECLARE_DYNCREATE(CStatView)

    // SettingsSubscriber
    virtual void OnSettingsChanged ();
private:
    static StatGauge m_StatGauge;
    static CImageList m_imageList;
    static set<CStatView*> m_GridFamily;
};

//{{AFX_INSERT_LOCATION}}

#endif//__STATVIEW_H__
