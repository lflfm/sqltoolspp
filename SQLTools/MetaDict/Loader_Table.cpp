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
    29.05.2002  bug fix, using number intead of integer 
    10.02.2003  bug fix, wrong declaration length of character type column for fixed charset
    14.04.2003 bug fix, Oracle9i and UTF8 data truncation
    09.02.2004 bug fix, DDL reengineering: Oracle Server 8.0.5 support (tables)
    17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
    08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
*/

#include "stdafx.h"
#include "config.h"
//#pragma warning ( disable : 4786 )
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    using namespace std;
    using namespace Common;

    const int cn_rec_type = 0;

    const int cn_tbl_owner           = 1;
    const int cn_tbl_table_name      = 2;
    const int cn_tbl_tablespace      = 3;
    const int cn_tbl_cluster         = 4;
    const int cn_tbl_initial_extent  = 5;
    const int cn_tbl_next_extent     = 6;
    const int cn_tbl_min_extents     = 7;
    const int cn_tbl_max_extents     = 8;
    const int cn_tbl_pct_increase    = 9;
    const int cn_tbl_freelists       = 10;
    const int cn_tbl_freelist_groups = 11;
    const int cn_tbl_pct_free        = 12;
    const int cn_tbl_pct_used        = 13;
    const int cn_tbl_ini_trans       = 14;
    const int cn_tbl_max_trans       = 15;
    const int cn_tbl_cache           = 16;
    const int cn_tbl_temporary       = 17;
    const int cn_tbl_duration        = 18;
    const int cn_tbl_iot_type        = 19;
    const int cn_tbl_logging         = 20;

    const int cn_cmt_owner           = 1;
    const int cn_cmt_table_name      = 2;
    const int cn_cmt_comments        = 3;

    LPSCSTR csz_tab_sttm =
        "SELECT /*+RULE*/ 1,"
            "owner,"
            "table_name,"
            "tablespace_name,"
            "cluster_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "Nvl(freelists, 1),"
            "Nvl(freelist_groups, 1),"
            "pct_free,"
            "Nvl(pct_used, 40),"
            "ini_trans,"
            "max_trans,"
            "cache,"
            "<TEMPORARY>,"
            "<DURATION>,"
            "<IOT_TYPE>,"
            "<LOGGING> "
        "FROM sys.all_tables "
        "WHERE owner = :owner AND table_name <EQUAL> :name "
            "<AND_IOT_NAME_IS_NULL>"
        "UNION ALL "
        "SELECT 2,"
	        "owner,"
	        "table_name,"
	        "comments,"
	        "NULL,"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "To_Number(NULL),"
	        "NULL,"
	        "NULL,"
	        "NULL,"
	        "NULL,"
	        "NULL "
        "FROM sys.all_tab_comments "
        "WHERE owner = :owner AND table_name <EQUAL> :name "
	        "AND table_type = 'TABLE' "
			"<AND_NOT_RECYCLEBIN>"
        "ORDER BY 1";

    const int cn_col_owner          = 0;
    const int cn_col_table_name     = 1;
    const int cn_col_column_id      = 2;
    const int cn_col_column_name    = 3;
    const int cn_col_data_type      = 4;
    const int cn_col_data_precision = 5;
    const int cn_col_data_scale     = 6;
    const int cn_col_data_length    = 7;
    const int cn_char_col_decl_length = 8;
    const int cn_col_nullable       = 9;
    const int cn_col_data_default   = 10;
    const int cn_col_comments       = 11;

    LPSCSTR csz_col_sttm =
        "SELECT /*+RULE*/ "
            "v1.owner,"
            "v1.table_name,"
            "v1.column_id,"
            "v1.column_name,"
            "v1.data_type,"
            "v1.data_precision,"
            "v1.data_scale,"
            "v1.data_length,"
            "<V1_CHAR_COL_DECL_LENGTH>,"
            "v1.nullable,"
            "v1.data_default,"
            "v2.comments "
        "FROM sys.all_tab_columns v1, sys.all_col_comments v2, sys.all_tables v3 "
        "WHERE v1.owner = :owner AND v1.table_name <EQUAL> :name "
            "AND v1.owner = v2.owner AND v1.table_name = v2.table_name "
            "AND v1.owner = v3.owner AND v1.table_name = v3.table_name "
            "AND v1.column_name = v2.column_name";


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

    const int cn_iot_owrflw_compression     = 0;
    const int cn_iot_owrflw_prefix_length   = 1;
    const int cn_iot_owrflw_pct_threshold   = 2;
    const int cn_iot_owrflw_include_column  = 3;
    const int cn_iot_owrflw_tablespace_name = 4;
    const int cn_iot_owrflw_ini_trans       = 5;
    const int cn_iot_owrflw_max_trans       = 6;
    const int cn_iot_owrflw_initial_extent  = 7;
    const int cn_iot_owrflw_next_extent     = 8;
    const int cn_iot_owrflw_min_extents     = 9;
    const int cn_iot_owrflw_max_extents     = 10;
    const int cn_iot_owrflw_pct_increase    = 11;
    const int cn_iot_owrflw_freelists       = 12;
    const int cn_iot_owrflw_freelist_groups = 13;
    const int cn_iot_owrflw_pct_free        = 14;
    const int cn_iot_owrflw_pct_used        = 15;
    const int cn_iot_owrflw_logging         = 16;

    LPSCSTR csz_iot_owrflw_sttm =
        "SELECT /*+RULE*/"
          "v1.compression,"
          "v1.prefix_length,"
          "v1.pct_threshold,"
          "v1.include_column,"
          "v2.tablespace_name,"
          "v2.ini_trans,"
          "v2.max_trans,"
          "v2.initial_extent,"
          "v2.next_extent," 
          "v2.min_extents," 
          "v2.max_extents,"
          "v2.pct_increase,"
          "Nvl(v2.freelists, 1),"
          "Nvl(v2.freelist_groups, 1),"
          "v2.pct_free," 
          "Nvl(v2.pct_used, 40)," 
          "v2.logging " 
        "FROM sys.all_indexes v1, sys.all_tables v2 "
          "WHERE v1.index_type = \'IOT - TOP\' " 
            "AND v1.table_name = v2.iot_name "
            "AND v1.table_name = :name "
            "AND v1.table_owner = v2.owner "
            "AND v1.table_owner = :owner";

    static void prepare_cursor    (OCI8::EServerVersion, OciCursor&, const char* sttm, const char* owner, const char* name, bool useLike);
    static void load_table_body   (OCI8::EServerVersion, OciCursor&, Dictionary&, bool useDba);
    static void load_table_column (OciCursor&, Dictionary&);
    static void load_clu_columns  (OciConnect& con, Table& table, bool useDba);


void Loader::LoadTables (const char* owner, const char* name, bool useLike)
{
    {
#if TEST_LOADING_OVERFLOW
    OciCursor cur(m_connect, 50, 32);
#else
    OciCursor cur(m_connect, 50, 196);
#endif
    
    prepare_cursor(m_connect.GetVersion(), cur, csz_tab_sttm, owner, name, useLike);

    bool useDba = m_strCurSchema.compare(owner) ? true : false;
    
    cur.Execute();
    while (cur.Fetch())
        load_table_body(m_connect.GetVersion(), cur, m_dictionary, useDba);
    }

    {
#if TEST_LOADING_OVERFLOW
    OciCursor cur(m_connect, 50, 32);
#else
    OciCursor cur(m_connect, 50, 196);
#endif
    
    prepare_cursor(m_connect.GetVersion(), cur, csz_col_sttm, owner, name, useLike);

    cur.Execute();
    while (cur.Fetch())
        load_table_column(cur, m_dictionary);
    }

    loadIndexes(owner, name, true/*byTable*/, false/*isCluster*/, useLike);
    loadConstraints(owner, name, true/*byTable*/, useLike);
}


    static
    void prepare_cursor (OCI8::EServerVersion servVer, OciCursor& cur, const char* sttm, const char* owner, const char* name, bool useLike)
    {
        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst.AddPair("<DURATION>",  (servVer > OCI8::esvServer80X) ? "duration" : "NULL");

        subst.AddPair("<TEMPORARY>", (servVer > OCI8::esvServer73X) ? "temporary" : "NULL");
        subst.AddPair("<IOT_TYPE>",  (servVer > OCI8::esvServer73X) ? "iot_type"  : "NULL");
        subst.AddPair("<LOGGING>",   (servVer > OCI8::esvServer73X) ? "logging"   : "'YES'");

        subst.AddPair("<AND_IOT_NAME_IS_NULL>", (servVer > OCI8::esvServer73X) ?  "AND iot_name IS NULL " : "");
        subst.AddPair("<V1_CHAR_COL_DECL_LENGTH>", (servVer > OCI8::esvServer73X) ?  "v1.char_col_decl_length" : "NULL");
		// B#1407225, Skip objects from recyclebin showed in 'SYS.ALL_TAB_COMMENTS' (unfortunately we have only table_name!)
		// If we want to do it 100% properly, we should MINUS with 'SELECT object_name FROM recyclebin WHERE type='TABLE'', 
		// but I think that current solution is good enough.
		subst.AddPair("<AND_NOT_RECYCLEBIN>", (servVer >= OCI8::esvServer10X) ? "AND table_name NOT LIKE 'BIN$%' " : "");

        subst << sttm;

        cur.Prepare(subst.GetResult());
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
    }

    static
    void load_table_body (OCI8::EServerVersion servVer, OciCursor& cur, Dictionary& dict, bool useDba)
    {
        if (cur.ToInt(cn_rec_type) == 1)
        {
            Table& table = dict.CreateTable(cur.ToString(cn_tbl_owner).c_str(), cur.ToString(cn_tbl_table_name).c_str());
            
            cur.GetString(cn_tbl_owner, table.m_strOwner);
            cur.GetString(cn_tbl_table_name, table.m_strName);

            if (Loader::IsYes(cur.ToString(cn_tbl_temporary)))
            {
                table.m_TableType = ettTemporary;

                if (cur.ToString(cn_tbl_duration) == "SYS$TRANSACTION")
                    table.m_TemporaryTableDuration = ettdTransaction;
                else if (cur.ToString(cn_tbl_duration) == "SYS$SESSION")
                    table.m_TemporaryTableDuration = ettdSession;
                else
                    _CHECK_AND_THROW_(0, "Table loading error:\nunknown temporary table duration!");
            }
            else if (cur.IsNull(cn_tbl_iot_type))
            {
                table.m_TableType = ettNormal;

                cur.GetString(cn_tbl_tablespace, table.m_strTablespaceName);
                table.m_nInitialExtent  = cur.ToInt(cn_tbl_initial_extent);
                table.m_nNextExtent     = cur.ToInt(cn_tbl_next_extent);
                table.m_nMinExtents     = cur.ToInt(cn_tbl_min_extents);
                table.m_nMaxExtents     = cur.ToInt(cn_tbl_max_extents);
                table.m_nPctIncrease    = cur.ToInt(cn_tbl_pct_increase);
            
                table.m_nFreeLists      = cur.ToInt(cn_tbl_freelists);
                table.m_nFreeListGroups = cur.ToInt(cn_tbl_freelist_groups);
                table.m_nPctFree        = cur.ToInt(cn_tbl_pct_free);
                table.m_nPctUsed        = cur.ToInt(cn_tbl_pct_used);
                table.m_nIniTrans       = cur.ToInt(cn_tbl_ini_trans);
                table.m_nMaxTrans       = cur.ToInt(cn_tbl_max_trans);

                cur.GetString(cn_tbl_cluster, table.m_strClusterName);
                if (!table.m_strClusterName.empty())
                    load_clu_columns(cur.GetConnect(), table, useDba);
            }
            else if (cur.ToString(cn_tbl_iot_type) == "IOT")
            {
                table.m_TableType = ettIOT;

                OciCursor iotCur(cur.GetConnect(), 1);
                prepare_cursor(servVer, iotCur, csz_iot_owrflw_sttm, table.m_strOwner.c_str(), table.m_strName.c_str(), false);
                
                iotCur.Execute();
                if (iotCur.Fetch())
                {
                    iotCur.GetString(cn_iot_owrflw_tablespace_name, table.m_IOTOverflow_Storage.m_strTablespaceName);
                    
                    table.m_IOTOverflow_Storage.m_nInitialExtent  = iotCur.ToInt(cn_iot_owrflw_initial_extent);
                    table.m_IOTOverflow_Storage.m_nNextExtent     = iotCur.ToInt(cn_iot_owrflw_next_extent);
                    table.m_IOTOverflow_Storage.m_nMinExtents     = iotCur.ToInt(cn_iot_owrflw_min_extents);
                    table.m_IOTOverflow_Storage.m_nMaxExtents     = iotCur.ToInt(cn_iot_owrflw_max_extents);
                    table.m_IOTOverflow_Storage.m_nPctIncrease    = iotCur.ToInt(cn_iot_owrflw_pct_increase);
            
                    table.m_IOTOverflow_Storage.m_nFreeLists      = iotCur.ToInt(cn_iot_owrflw_freelists);
                    table.m_IOTOverflow_Storage.m_nFreeListGroups = iotCur.ToInt(cn_iot_owrflw_freelist_groups);
                    table.m_IOTOverflow_Storage.m_nPctFree        = iotCur.ToInt(cn_iot_owrflw_pct_free);
                    table.m_IOTOverflow_Storage.m_nPctUsed        = iotCur.ToInt(cn_iot_owrflw_pct_used);
                    table.m_IOTOverflow_Storage.m_nIniTrans       = iotCur.ToInt(cn_iot_owrflw_ini_trans);
                    table.m_IOTOverflow_Storage.m_nMaxTrans       = iotCur.ToInt(cn_iot_owrflw_max_trans);

                    table.m_IOTOverflow_PCTThreshold  = iotCur.ToInt(cn_iot_owrflw_pct_threshold);
                    table.m_IOTOverflow_includeColumn = iotCur.ToInt(cn_iot_owrflw_include_column);
                }
            }
            else
                _CHECK_AND_THROW_(0, "Table loading error:\nunknown table type!");

            table.m_bCache   = Loader::IsYes(cur.ToString(cn_tbl_cache));
            table.m_bLogging = Loader::IsYes(cur.ToString(cn_tbl_logging));
        }
        else
        {
            Table& table = dict.LookupTable(cur.ToString(cn_cmt_owner).c_str(), cur.ToString(cn_cmt_table_name).c_str());

            if (cur.IsGood(cn_cmt_comments))
                cur.GetString(cn_cmt_comments, table.m_strComments);
            else 
            {
                OciCursor ovrfCur(cur.GetConnect(), "SELECT comments FROM sys.all_tab_comments WHERE owner = :owner AND table_name = :name", 1, 4 * 1024);
                
                ovrfCur.Bind(":owner", table.m_strOwner.c_str());
                ovrfCur.Bind(":name",  table.m_strName.c_str());

                ovrfCur.Execute();
                ovrfCur.Fetch();

                _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Table comments loading error:\nsize exceed 4K!");
                
                ovrfCur.GetString(0, table.m_strComments);
            }
        }
    }

    static
    void load_table_column (OciCursor& cur, Dictionary& dict)
    {
        counted_ptr<TabColumn> p_column(new TabColumn());

        cur.GetString(cn_col_column_name, p_column->m_strColumnName);
        cur.GetString(cn_col_data_type, p_column->m_strDataType);

        // bug fix, using number intead of integer 
        p_column->m_nDataPrecision = cur.IsNull(cn_col_data_precision) ? -1 : cur.ToInt(cn_col_data_precision);
        p_column->m_nDataScale     = cur.IsNull(cn_col_data_scale) ? -1 : cur.ToInt(cn_col_data_scale);

        // bug fix, wrong declaration length of character type column for fixed charset
        if (!cur.IsNull(cn_char_col_decl_length))
            p_column->m_nDataLength = cur.ToInt(cn_char_col_decl_length);
        else
            p_column->m_nDataLength = cur.ToInt(cn_col_data_length);

        p_column->m_bNullable = Loader::IsYes(cur.ToString(cn_col_nullable));

        if (cur.IsGood(cn_col_comments) && cur.IsGood(cn_col_data_default))
        {
            cur.GetString(cn_col_comments, p_column->m_strComments);
            cur.GetString(cn_col_data_default, p_column->m_strDataDefault);
        } 
        else 
        {
            OciCursor ovrfCur(
                cur.GetConnect(),
                "SELECT /*+RULE*/ v2.comments,v1.data_default "
                    "FROM sys.all_tab_columns v1, sys.all_col_comments v2 "
                    "WHERE v1.owner = v2.owner "
                        "AND v1.table_name = v2.table_name "
                        "AND v1.column_name = v2.column_name "
                        "AND v1.owner = :owner "
                        "AND v1.table_name = :name "
                        "AND v1.column_name = :col_name",
                1,
                4 * 1024
                );

            ovrfCur.Bind(":owner",    cur.ToString(cn_col_owner).c_str());
            ovrfCur.Bind(":name",     cur.ToString(cn_col_table_name).c_str());
            ovrfCur.Bind(":col_name", p_column->m_strColumnName.c_str());

            ovrfCur.Execute();
            ovrfCur.Fetch();

            _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Column comments loading error:\nsize exceed 4K!");
            _CHECK_AND_THROW_(ovrfCur.IsGood(1), "Column default data value loading error:\nsize exceed 4K!");
            
            ovrfCur.GetString(0 ,p_column->m_strComments);
            ovrfCur.GetString(1, p_column->m_strDataDefault);
        }

        trim_symmetric(p_column->m_strDataDefault);

        Table& table = dict.LookupTable(cur.ToString(cn_col_owner).c_str(), cur.ToString(cn_col_table_name).c_str());
        table.m_Columns[cur.ToInt(cn_col_column_id) - 1] = p_column;
    }

    static
    void load_clu_columns (OciConnect& con, Table& table, bool useDba)
    {
        try
        {
            Common::Substitutor subst;

            subst.AddPair("<SRC>",         useDba  ? "dba" : "user");
            subst.AddPair("<OWNER_EXPRS>", useDba  ? "AND t.owner = c.owner AND c.owner = :owner" : "");

            subst << csz_clu_col_sttm;

            OciCursor cur(con, subst.GetResult(), 20);

            if (useDba) cur.Bind(":owner", table.m_strOwner.c_str());
            cur.Bind(":clu_name", table.m_strClusterName.c_str());
            cur.Bind(":tab_name", table.m_strName.c_str());

            cur.Execute();
            while (cur.Fetch())
                table.m_clusterColumns[cur.ToInt(cn_clu_column_id) - 1] = cur.ToString(cn_clu_tab_col_name);
        }
        catch (const OciException& x)
        {
            if (useDba && x == 942)
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
