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

// 17.09.02 bug fix: cluster reengineering fails
// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 03.08.2003 improvement, domain index support
// 09.02.2004 bug fix, DDL reengineering: Oracle Server 8.0.5 support (indexes)
// 21.03.2004 bug fix, DDL reengineering: domain index bug (introduced in 141b46)
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters

#include "stdafx.h"
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
    //
    //  Attention: in cluster case user_indexes or dba_indexes must be used
    //  if schema is equal user, then we can use user_indexes
    //  else we must use dba_indexes (user must have select grant)
    //

    LPSCSTR csz_tab_index_sttm =
        "SELECT /*+RULE*/ "
            "1,"
            "owner,"
            "index_name,"
            "<INDEX_TYPE>,"    //NORMAL for 7.3
            "table_owner,"
            "table_name,"
            "table_type,"
            "uniqueness,"
            "tablespace_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "Nvl(freelists, 1),"
            "Nvl(freelist_groups, 1),"
            "pct_free,"
            "ini_trans,"
            "max_trans,"
            "<TEMPORARY>,"     //N for 7.3
            "<GENERATED>,"     //N for 7.3
            "<LOGGING>,"       //YES for 7.3
            "<COMPRESSION>,"   //NULL
            "<PREFIX_LENGTH>," //To_Number(NULL)
            "<ITYP_OWNER>,"    //NULL
            "<ITYP_NAME>,"     //NULL
            "<PARAMETERS> "    //NULL
        "FROM sys.all_indexes "
        "WHERE <OWNER> = :owner "
            "AND <NAME> <EQUAL> :name "
        "UNION ALL "
        "SELECT "
            "2,"
            "v2.index_owner,"
            "v2.index_name,"
            "NULL,"
            "v2.table_owner,"
            "v2.table_name,"
            "v2.column_name,"
            "NULL,"
            "NULL,"
            "v2.column_position,"
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
            "To_Number(NULL),"
            "NULL,"
            "NULL,"
            "NULL "
        "FROM sys.all_indexes v1, sys.all_ind_columns v2 "
        "WHERE v2.index_owner = v1.owner "
            "AND v2.index_name = v1.index_name "
            "AND v1.<OWNER> = :owner "
            "AND v1.<NAME> <EQUAL> :name";

    LPSCSTR csz_ind_exp_sttm =
        "SELECT column_position, column_expression "
            "FROM sys.all_ind_expressions "
            "WHERE index_owner = :owner "
                "AND index_name = :name";

    LPSCSTR csz_clu_index_sttm =
        "SELECT /*+RULE*/ "
            "1,"
            "<INDEX_OWNER>,"   //if (dba) then "INDEX_OWNER" else "USER"
            "index_name,"
            "<INDEX_TYPE>,"    //NORMAL for 7.3
            "table_owner,"
            "table_name,"
            "table_type,"
            "uniqueness,"
            "tablespace_name,"
            "initial_extent,"
            "next_extent,"
            "min_extents,"
            "max_extents,"
            "pct_increase,"
            "Nvl(freelists, 1),"
            "Nvl(freelist_groups, 1),"
            "pct_free,"
            "ini_trans,"
            "max_trans, "
            "<TEMPORARY>,"     //N for 7.3
            "<GENERATED>,"     //N for 7.3
            "<LOGGING>,"       //YES for 7.3
            "<COMPRESSION>,"   //NULL
            "<PREFIX_LENGTH>," //To_Number(NULL)
            "<ITYP_OWNER>,"    //NULL
            "<ITYP_NAME>,"     //NULL
            "<PARAMETERS> "    //NULL
        "FROM <SRC_INDEXES> "           // if (dba) then "dba_indexes" else "user_indexes"
        "WHERE table_owner = :owner "
            "AND table_name <EQUAL> :name "
            "AND EXISTS "
                "("
                    "SELECT NULL FROM <SRC_CLUSTERS> "// if (dba) then "dba_clusters" else "user_clusters"
                    "WHERE <CLUSTER_OWNER> = :owner "
                        "AND cluster_name <EQUAL> :name "
                ")";

    const int cn_inx_rec_type           = 0;

    const int cn_inx_owner              = 1;
    const int cn_inx_index_name         = 2;
    const int cn_inx_index_type         = 3;
    const int cn_inx_table_owner        = 4;
    const int cn_inx_table_name         = 5;
    const int cn_inx_table_type         = 6;
    const int cn_inx_uniqueness         = 7;
    const int cn_inx_tablespace_name    = 8;
    const int cn_inx_initial_extent     = 9;
    const int cn_inx_next_extent        = 10;
    const int cn_inx_min_extents        = 11;
    const int cn_inx_max_extents        = 12;
    const int cn_inx_pct_increase       = 13;
    const int cn_inx_freelists          = 14;
    const int cn_inx_freelist_groups    = 15;
    const int cn_inx_pct_free           = 16;
    const int cn_inx_ini_trans          = 17;
    const int cn_inx_max_trans          = 18;
    const int cn_inx_temporary          = 19;
    const int cn_inx_generated          = 20;
    const int cn_inx_logging            = 21;
    const int cn_inx_compression        = 22;
    const int cn_inx_prefix_length      = 23;
    const int cn_inx_domain_owner       = 24; // 03.08.2003 improvement, domain index support
    const int cn_inx_domain             = 25;
    const int cn_inx_domain_params      = 26;


    const int cn_inx_column_name         = 6;
    const int cn_inx_column_position     = 9;

    const int cn_inx_exp_column_position     = 0;
    const int cn_inx_exp_column_expressoin   = 1;


void Loader::LoadIndexes (const char* owner, const char* table, bool useLike)
{
    loadIndexes(owner, table, false/*byTable*/, false/*isCluster*/, useLike);
}

void Loader::loadIndexes (const char* owner, const char* table, bool byTable, bool isCluster, bool useLike)
{
    bool useDba = isCluster && m_strCurSchema.compare(owner);
    try
    {
        OCI8::EServerVersion ver = m_connect.GetVersion();
        Common::Substitutor subst;

        subst.AddPair("<OWNER>", byTable  ? "table_owner" : "owner");
        subst.AddPair("<NAME>",  byTable  ? "table_name"  : "index_name");
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");

        subst.AddPair("<COMPRESSION>"  , (ver > OCI8::esvServer80X) ? "compression"   : "NULL"           );
        subst.AddPair("<PREFIX_LENGTH>", (ver > OCI8::esvServer80X) ? "prefix_length" : "To_Number(NULL)");
        subst.AddPair("<ITYP_OWNER>"   , (ver > OCI8::esvServer80X) ? "ityp_owner"    : "NULL"           );
        subst.AddPair("<ITYP_NAME>"    , (ver > OCI8::esvServer80X) ? "ityp_name"     : "NULL"           );
        subst.AddPair("<PARAMETERS>"   , (ver > OCI8::esvServer80X) ? "parameters"    : "NULL"           );

        subst.AddPair("<INDEX_TYPE>", (ver > OCI8::esvServer73X) ? "index_type" : "'NORMAL'");
        subst.AddPair("<TEMPORARY>" , (ver > OCI8::esvServer73X) ? "temporary"  : "'N'"     );
        subst.AddPair("<GENERATED>" , (ver > OCI8::esvServer73X) ? "generated"  : "'N'"     );
        subst.AddPair("<LOGGING>"   , (ver > OCI8::esvServer73X) ? "logging"    : "'YES'"   );

        if (isCluster)
        {
            subst.AddPair("<INDEX_OWNER>",  !useDba  ? "user" : "owner");
            subst.AddPair("<CLUSTER_OWNER>",!useDba  ? "user" : "owner");
            subst.AddPair("<SRC_INDEXES>",  !useDba  ? "user_indexes" : "dba_indexes");
            subst.AddPair("<SRC_CLUSTERS>", !useDba  ? "user_clusters" : "dba_clusters");
        }
    
        subst << (!isCluster ? csz_tab_index_sttm : csz_clu_index_sttm);

        OciCursor cur(m_connect, subst.GetResult(), 50);
        
        cur.Bind(":owner", owner);
        cur.Bind(":name",  table);

        cur.Execute();
        while (cur.Fetch())
        {
            if (cur.ToInt(cn_inx_rec_type) == 1)  // load index data
            {
                Index& index = m_dictionary.CreateIndex(cur.ToString(cn_inx_owner).c_str(),
                                                        cur.ToString(cn_inx_index_name).c_str());

                cur.GetString(cn_inx_owner,      index.m_strOwner);
                cur.GetString(cn_inx_index_name, index.m_strName);
                              
                cur.GetString(cn_inx_table_owner,index.m_strTableOwner);
                cur.GetString(cn_inx_table_name, index.m_strTableName);

                string type;
                cur.GetString(cn_inx_index_type, type);

                if (type == "NORMAL")
                    index.m_Type = eitNormal;
                else if (type == "NORMAL/REV")
                {
                    index.m_Type = eitNormal;
                    index.m_bReverse = true;
                }
                else if (type == "BITMAP")
                    index.m_Type = eitBitmap;
                else if (type == "CLUSTER")
                    index.m_Type = eitCluster;
                else if (type == "FUNCTION-BASED NORMAL")
                    index.m_Type = eitFunctionBased;
                else if (type == "IOT - TOP")
                    index.m_Type = eitIOT_TOP;
                else if (type == "DOMAIN") // 03.08.2003 improvement, domain index support
                    index.m_Type = eitDomain;
                else
                    _CHECK_AND_THROW_(0 ,"Index loading error:\nunsupporded index type!");

                index.m_bUniqueness = (cur.ToString(cn_inx_uniqueness) == "UNIQUE") ? true : false;
                index.m_bTemporary  = IsYes(cur.ToString(cn_inx_temporary));

                if (index.m_Type == eitFunctionBased)
                {
                    OciCursor cur_exp(m_connect, csz_ind_exp_sttm, 1, 4 * 1024);
                    
                    cur_exp.Bind(":owner", index.m_strOwner.c_str());
                    cur_exp.Bind(":name",  index.m_strName.c_str());

                    cur_exp.Execute();
                    while (cur_exp.Fetch())
                    {
                        _CHECK_AND_THROW_(cur_exp.IsGood(cn_inx_exp_column_expressoin), "Index loading error:\nexpression is too long!");
                        index.m_Columns[cur_exp.ToInt(cn_inx_exp_column_position) - 1] = cur_exp.ToString(cn_inx_exp_column_expressoin);
                    }
                }

                index.m_bGenerated = Loader::IsYes(cur.ToString(cn_inx_generated));
                index.m_bLogging = Loader::IsYes(cur.ToString(cn_inx_logging));
                
                index.m_bCompression = cur.ToString(cn_inx_compression) == "ENABLED";
                
                if (index.m_bCompression)
                    index.m_nCompressionPrefixLength = cur.ToInt(cn_inx_prefix_length);

                cur.GetString(cn_inx_tablespace_name, index.m_strTablespaceName);

                index.m_nInitialExtent  = cur.ToInt(cn_inx_initial_extent);
                index.m_nNextExtent     = cur.ToInt(cn_inx_next_extent);
                index.m_nMinExtents     = cur.ToInt(cn_inx_min_extents);
                index.m_nMaxExtents     = cur.ToInt(cn_inx_max_extents);
                index.m_nPctIncrease    = cur.ToInt(cn_inx_pct_increase);

                index.m_nFreeLists      = cur.ToInt(cn_inx_freelists);
                index.m_nFreeListGroups = cur.ToInt(cn_inx_freelist_groups);
                index.m_nPctFree        = cur.ToInt(cn_inx_pct_free);
                index.m_nPctUsed        = -1;
                index.m_nIniTrans       = cur.ToInt(cn_inx_ini_trans);
                index.m_nMaxTrans       = cur.ToInt(cn_inx_max_trans);

                string strKey;
                strKey = index.m_strOwner + '.' + index.m_strName;

                if (index.m_Type == eitDomain) // 03.08.2003 improvement, domain index support
                {
                    index.m_strDomainOwner  = cur.ToString(cn_inx_domain_owner);
                    index.m_strDomain       = cur.ToString(cn_inx_domain);
                    index.m_strDomainParams = cur.ToString(cn_inx_domain_params);
                }

                try
                {
                    if (index.m_Type == eitCluster)
                    {
                        Cluster& cluster = m_dictionary.LookupCluster(index.m_strTableOwner.c_str(), 
                                                                      index.m_strTableName.c_str());
                        cluster.m_setIndexes.insert(strKey);
                    }
                    else
                    {
                        Table& table = m_dictionary.LookupTable(index.m_strTableOwner.c_str(), 
                                                                index.m_strTableName.c_str());
                        table.m_setIndexes.insert(strKey);
                    }
                }
                catch (const XNotFound&) 
                {
                    if (byTable) throw;
                }

            }
            else // load index columns
            {
                Index& index = m_dictionary.LookupIndex(cur.ToString(cn_inx_owner).c_str(), 
                                                        cur.ToString(cn_inx_index_name).c_str());
            
                int pos = cur.ToInt(cn_inx_column_position);
            
                if (index.m_Columns.find(pos - 1) == index.m_Columns.end())
                    index.m_Columns[pos - 1] = cur.ToString(cn_inx_column_name);
                else if (index.m_Type != eitFunctionBased)
                    _CHECK_AND_THROW_(0, "Index reengineering algorithm failure!");
            }
        }
    }
    catch (const OciException& x)
    {
        if (useDba && x == 942)
        {
            _CHECK_AND_THROW_(0, "DBA_INDEXES or DBA_CLUSTERS is not accessble! "
                               "Can't capture claster table definition. "
                               "Reconnect as schema owner and repeat");
        }
        else
            throw;
    }
}

}// END namespace OraMetaDict
