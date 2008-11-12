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

// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters

#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "COMMON\StrHelpers.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    const int cn_snp_owner           = 0;
    const int cn_snp_table_name      = 1;
    const int cn_snp_type            = 2;
    const int cn_snp_start_with      = 3;
    const int cn_snp_next_time       = 4;
    const int cn_snp_query           = 5;
    const int cn_snp_tablespace      = 6;
    const int cn_snp_pct_free        = 7;
    const int cn_snp_pct_used        = 8;
    const int cn_snp_ini_trans       = 9;
    const int cn_snp_max_trans       = 10;
    const int cn_snp_initial_extent  = 11;
    const int cn_snp_next_extent     = 12;
    const int cn_snp_min_extents     = 13;
    const int cn_snp_max_extents     = 14;
    const int cn_snp_pct_increase    = 15;
    const int cn_snp_freelists       = 16;
    const int cn_snp_freelist_groups = 17;
    const int cn_snp_cluster_name    = 18;

    LPSCSTR csz_snp_sttm =
        "SELECT /*+RULE*/ "
          "s.owner,"
          "s.name,"
          "s.type,"
          "s.start_with,"
          "s.next,"
          "s.query," 
          "t.tablespace_name,"
          "t.pct_free,"
          "Nvl(t.pct_used, 30) as pct_used," 
          "t.ini_trans," 
          "t.max_trans," 
          "t.initial_extent,"
          "t.next_extent," 
          "t.min_extents," 
          "t.max_extents," 
          "t.pct_increase, "
          "Nvl(t.freelists, 1),"
          "Nvl(t.freelist_groups, 1),"
          "t.cluster_name " 
        "FROM <V_SNAPSHOTS> s, sys.all_tables t "
        "WHERE s.owner = t.owner "
          "AND s.table_name = t.table_name "
          "AND s.owner = :owner AND s.name <EQUAL> :name";

    const int cn_clu_tab_col_name = 0;
    const int cn_clu_column_id    = 1;

    LPSCSTR csz_clu_col_sttm =
        "SELECT /*+RULE*/ "
            //"<OWNER>,"                                // if (dba) then "c.owner" else "USER"
            //"c.cluster_name,"
            //"c.table_name," 
	        "c.tab_column_name," 
	        //"c.clu_column_name," 
	        "t.column_id "
        "FROM <SRC>_tab_columns t, <SRC>_clu_columns c "// if (dba) then "dba" else "user"
        "WHERE t.table_name = c.cluster_name "
            "AND t.column_name = c.clu_column_name "
            "AND c.cluster_name = :clu_name "
            "AND c.table_name = :tab_name " 
            "<OWNER_EXPRS>";                            // if (dba) then "AND t.owner = c.owner "
                                                        //               "AND c.owner = :owner"
   static void load_snp_clu_columns  (OciConnect& con, Snapshot& table, bool useDba);

void Loader::LoadSnapshots (const char* owner, const char* name, bool useLike)
{
    bool useAll = m_strCurSchema.compare(owner) ? true : false;

    Common::Substitutor subst;
    subst.AddPair("<V_SNAPSHOTS>", 
            useAll ? (m_connect.GetVersion() >= OCI8::esvServer80X ? "sys.all_snapshots" 
                : "dba_snapshots")
                         : "user_snapshots");

    subst.AddPair("<EQUAL>", useLike ? "like" : "=");
    subst << csz_snp_sttm;

    OciCursor cur(m_connect, subst.GetResult(), useLike ? 50 : 1, 64 * 1024U);
    
    cur.Bind(":owner", owner);
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        Snapshot& snp 
            = m_dictionary.CreateSnapshot(cur.ToString(cn_snp_owner).c_str(), 
                                          cur.ToString(cn_snp_table_name).c_str());
        
        cur.GetString(cn_snp_tablespace, snp.m_strTablespaceName);

        snp.m_nInitialExtent  = cur.ToInt(cn_snp_initial_extent);
        snp.m_nNextExtent     = cur.ToInt(cn_snp_next_extent);
        snp.m_nMinExtents     = cur.ToInt(cn_snp_min_extents);
        snp.m_nMaxExtents     = cur.ToInt(cn_snp_max_extents);
        snp.m_nPctIncrease    = cur.ToInt(cn_snp_pct_increase);
        
        snp.m_nFreeLists      = cur.ToInt(cn_snp_freelists);
        snp.m_nFreeListGroups = cur.ToInt(cn_snp_freelist_groups);
        snp.m_nPctFree        = cur.ToInt(cn_snp_pct_free);
        snp.m_nPctUsed        = cur.ToInt(cn_snp_pct_used);
        snp.m_nIniTrans       = cur.ToInt(cn_snp_ini_trans);
        snp.m_nMaxTrans       = cur.ToInt(cn_snp_max_trans);

        cur.GetString(cn_snp_owner,      snp.m_strOwner);
        cur.GetString(cn_snp_table_name, snp.m_strName);

        cur.GetString(cn_snp_type, snp.m_strRefreshType);
        cur.GetString(cn_snp_start_with, snp.m_strStartWith);
        cur.GetString(cn_snp_next_time, snp.m_strNextTime);
        cur.GetString(cn_snp_query, snp.m_strQuery);
        cur.GetString(cn_snp_cluster_name, snp.m_strClusterName);
        if (!snp.m_strClusterName.empty())
            load_snp_clu_columns(cur.GetConnect(), snp, useAll);
        
    }

}

    static
    void load_snp_clu_columns (OciConnect& con, Snapshot& snp, bool useAll)
    {
        try
        {
            Common::Substitutor subst;

            subst.AddPair("<SRC>",         useAll  ? "all" : "user");
            subst.AddPair("<OWNER_EXPRS>", useAll  ? "AND t.owner = c.owner AND c.owner = :owner" : "");

            subst << csz_clu_col_sttm;

            OciCursor cur(con, subst.GetResult(), 20);

            if (useAll) cur.Bind(":owner", snp.m_strOwner.c_str());
            cur.Bind(":clu_name", snp.m_strClusterName.c_str());
            cur.Bind(":tab_name", snp.m_strName.c_str());

            cur.Execute();
            while (cur.Fetch())
                snp.m_clusterColumns[cur.ToInt(cn_clu_column_id) - 1] = cur.ToString(cn_clu_tab_col_name);
        }
        catch (const OciException& x)
        {
            if (useAll && x == 942)
            {
                _CHECK_AND_THROW_(0, "DBA_TAB_COLUMNS or DBA_CLU_COLUMNS is not accessble! "
                                   "Can't capture claster table definition. "
                                   "Reconnect as schema owner and repeat");
            }
            else
                throw;
        }
    }

}// END namespace OraMetaDict
