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
#include "SQLTools.h"
#include "DbObjListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OCI8;

/////////////////////////////////////////////////////////////////////////////
// CDbSourceWnd

#define DECL_COL_DATA(type)  static CDbObjListCtrl::CData::CColumn g_##type##Columns[]
#define DECL_DATA(type)          const CDbObjListCtrl::CData CDbObjListCtrl::sm_##type##Data
#define INIT_COL_REF(type)   sizeof g_##type##Columns/sizeof g_##type##Columns[0], \
                                                         g_##type##Columns

///////////////////////////////////////////////////////////

DECL_COL_DATA(Table) =
{
    { "table_name",     "Name",       200, LVCFMT_LEFT  },
    { "table_type",     "Type",        60, LVCFMT_LEFT  },
    { "tablespace_name","Tablespace", 100, LVCFMT_LEFT  },
    { "initial_extent", "Initial",     80, LVCFMT_RIGHT },
    { "next_extent",    "Next",        80, LVCFMT_RIGHT },
    { "pct_increase",   "Increase",    60, LVCFMT_RIGHT },
    { "max_extents",    "Max Ext",     60, LVCFMT_RIGHT },
    { "pct_free",       "Free",        60, LVCFMT_RIGHT },
    { "pct_used",       "Used",        60, LVCFMT_RIGHT },
    { "cluster_name",   "Cluster",    100, LVCFMT_LEFT  },
};

DECL_DATA(Table) =
{
    esvServer73X,
    IDII_TABLE,
    "TABLE",
    "Tables",
    "SELECT /*+ RULE*/ t.*,"
      " Decode(iot_type, NULL,(Decode(TEMPORARY, 'Y', 'TEMPORARY', 'REGULAR')), 'INDEX') table_type"
    " FROM sys.all_tables t WHERE owner = :p_owner"
    " AND iot_name IS NULL", // iot_name is null because of overflow tables
    // for 73
    "SELECT /*+ RULE*/ t.*, null table_type"
    " FROM sys.all_tables t WHERE owner = :p_owner",
    " AND table_name = :p_name",
    "table_name",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(Table)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Index) =
{
    { "index_name",     "Index",      200, LVCFMT_LEFT  },
    { "index_type",     "Type",        60, LVCFMT_LEFT  },
    { "uniqueness",     "Unique",      40, LVCFMT_LEFT  },
//    { "table_owner",    "Table Owner", 80, LVCFMT_LEFT  },
    { "table_name",     "Table",      160, LVCFMT_LEFT  },
    { "tablespace_name","Tablespace", 100, LVCFMT_LEFT  },
    { "initial_extent", "Initial",     80, LVCFMT_RIGHT },
    { "next_extent",    "Next",        80, LVCFMT_RIGHT },
    { "pct_increase",   "Increase",    60, LVCFMT_RIGHT },
    { "max_extents",    "Max Ext",     60, LVCFMT_RIGHT },
    { "pct_free",       "Free",        60, LVCFMT_RIGHT },
};

DECL_DATA(Index) =
{
    esvServer73X,
    IDII_INDEX,
    "INDEX",
    "Indexes",
    "SELECT /*+ RULE*/ * FROM sys.all_indexes t WHERE owner = :p_owner"
    " AND index_type != \'IOT - TOP\'",
    // for 73
    "SELECT /*+ RULE*/ t.*, null index_type FROM sys.all_indexes t WHERE owner = :p_owner",
    " AND index_name = :p_name",
    "index_name",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(Index)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(ChkConstraint) =
{
    { "constraint_name",   "Constraint",     240, LVCFMT_LEFT  },
    { "table_name",        "Table",          240, LVCFMT_LEFT  },
    { "search_condition",  "Condition",      200, LVCFMT_LEFT  },
    { "deferrable",        "Deferrable",      80, LVCFMT_LEFT  },
    { "deferred",          "Deferred",        80, LVCFMT_LEFT  },
};

DECL_DATA(ChkConstraint) =
{
    esvServer73X,
    IDII_CHK_CONSTRAINT,
    "CONSTRAINT",
    "Checks",
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, search_condition, status,"
      " decode(deferrable,'DEFERRABLE','Y') deferrable, decode(deferred,'DEFERRED','Y') deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'C'",
    // for 73
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, search_condition, status,"
      " null deferrable, null deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'C'",
    " AND constraint_name = :p_name",
    "constraint_name",
    0,
    0, "status",
    false, true, true,
    INIT_COL_REF(ChkConstraint)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(PkConstraint) =
{
    { "constraint_name",   "Constraint",     240, LVCFMT_LEFT  },
    { "table_name",        "Table",          240, LVCFMT_LEFT  },
    { "deferrable",        "Deferrable",      80, LVCFMT_LEFT  },
    { "deferred",          "Deferred",        80, LVCFMT_LEFT  },
};

DECL_DATA(PkConstraint) =
{
    esvServer73X,
    IDII_PK_CONSTRAINT,
    "CONSTRAINT",
    "PKs",
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, status,"
      " decode(deferrable,'DEFERRABLE','Y') deferrable, decode(deferred,'DEFERRED','Y') deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'P'",
    // for 73
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, status,"
      " null deferrable, null deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'P'",
    " AND constraint_name = :p_name",
    "constraint_name",
    0,
    0, "status",
    false, true, true,
    INIT_COL_REF(PkConstraint)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(UkConstraint) =
{
    { "constraint_name",   "Constraint",     240, LVCFMT_LEFT  },
    { "table_name",        "Table",          240, LVCFMT_LEFT  },
    { "deferrable",        "Deferrable",      80, LVCFMT_LEFT  },
    { "deferred",          "Deferred",        80, LVCFMT_LEFT  },
};

DECL_DATA(UkConstraint) =
{
    esvServer73X,
    IDII_UK_CONSTRAINT,
    "CONSTRAINT",
    "UKs",
    "SELECT /*+ RULE*/"
      " constraint_name, constraint_type, table_name, status,"
      " decode(deferrable,'DEFERRABLE','Y') deferrable, decode(deferred,'DEFERRED','Y') deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'U'",
    // for 73
    "SELECT /*+ RULE*/"
      " constraint_name, constraint_type, table_name, status,"
      " null deferrable, null deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'U'",
    " AND constraint_name = :p_name",
    "constraint_name",
    0,
    0, "status",
    false, true, true,
    INIT_COL_REF(UkConstraint)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(FkConstraint) =
{
    { "constraint_name",   "Constraint",     240, LVCFMT_LEFT  },
    { "table_name",        "Table",          160, LVCFMT_LEFT  },
    { "r_owner",           "Ref Owner",       70, LVCFMT_LEFT  },
    { "r_constraint_name", "Ref Constraint", 160, LVCFMT_LEFT  },
    { "delete_rule",       "Delete Rule",    100, LVCFMT_LEFT  },
    { "deferrable",        "Deferrable",      80, LVCFMT_LEFT  },
    { "deferred",          "Deferred",        80, LVCFMT_LEFT  },
};

DECL_DATA(FkConstraint) =
{
    esvServer73X,
    IDII_FK_CONSTRAINT,
    "CONSTRAINT",
    "FK",
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, decode(owner, r_owner, null, r_owner) r_owner, r_constraint_name,"
      " decode(delete_rule,'NO ACTION', null, delete_rule) delete_rule, status,"
      " decode(deferrable,'DEFERRABLE','Y') deferrable, decode(deferred,'DEFERRED','Y') deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'R'",
    // for 73
    "SELECT /*+ RULE*/"
      " constraint_name, table_name, decode(owner, r_owner, null, r_owner) r_owner, r_constraint_name,"
      " decode(delete_rule,'NO ACTION', null, delete_rule) delete_rule, status,"
      " null deferrable, null deferred"
    " FROM sys.all_constraints t WHERE owner = :p_owner AND constraint_type = 'R'",
    " AND constraint_name = :p_name",
    "constraint_name",
    0,
    0, "status",
    false, true, true,
    INIT_COL_REF(FkConstraint)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Sequence) =
{
    { "sequence_name", "Name",        200, LVCFMT_LEFT  },
    { "last_number",   "Last Number",  90, LVCFMT_RIGHT },
    { "min_value",     "Min Value",    80, LVCFMT_RIGHT },
    { "max_value",     "Max Value",    80, LVCFMT_RIGHT },
    { "increment_by",  "Interval",     60, LVCFMT_RIGHT },
    { "cycle_flag",    "Cycle",        60, LVCFMT_RIGHT },
    { "order_flag",    "Order",        60, LVCFMT_RIGHT },
    { "cache_size",    "Cache",        60, LVCFMT_RIGHT },
};

DECL_DATA(Sequence) =
{
    esvServer73X,
    IDII_SEQUENCE,
    "SEQUENCE",
    "Sequences",
    "SELECT /*+RULE*/ * FROM sys.all_sequences"
    " WHERE sequence_owner = :p_owner",
    NULL,
    " AND sequence_name = :p_name",
    "sequence_name",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(Sequence)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(View) =
{
    { "object_name",   "Name",        320, LVCFMT_LEFT  },
    { "text_length",   "Text Length", 100, LVCFMT_RIGHT },
    { "created",       "Created",     100, LVCFMT_LEFT  },
    { "last_ddl_time", "Modified",    100, LVCFMT_LEFT  },
};

DECL_DATA(View) =
{
    esvServer73X,
    IDII_VIEW,
    "VIEW",
    "Views",
    "SELECT /*+RULE*/"
    " o.object_name,"
    " o.created,"
    " o.last_ddl_time,"
    " o.status,"
    " v.text_length"
    " FROM sys.all_objects o, sys.all_views v"
    " WHERE o.owner = :p_owner"
    " AND v.owner = :p_owner"
    " AND object_name = view_name"
    " AND object_type = 'VIEW'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(View)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Function) =
{
    { "object_name",    "Name",    320, LVCFMT_LEFT },
    { "created",        "Created", 100, LVCFMT_LEFT },
    { "last_ddl_time", "Modified", 100, LVCFMT_LEFT },
};

DECL_DATA(Function) =
{
    esvServer73X,
    IDII_FUNCTION,
    "FUNCTION",
    "Functions",
    "SELECT /*+RULE*/ * FROM sys.all_objects"
    " WHERE owner = :p_owner"
    " AND object_type = 'FUNCTION'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(Function)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Procedure) =
{
    { "object_name",   "Name",     320, LVCFMT_LEFT },
    { "created",       "Created",  100, LVCFMT_LEFT },
    { "last_ddl_time", "Modified", 100, LVCFMT_LEFT },
};

DECL_DATA(Procedure) =
{
    esvServer73X,
    IDII_PROCEDURE,
    "PROCEDURE",
    "Procedures",
    "SELECT /*+RULE*/ * FROM sys.all_objects"
    " WHERE owner = :p_owner"
    " AND object_type = 'PROCEDURE'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(Procedure)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Package) =
{
    { "object_name",   "Name",     320, LVCFMT_LEFT },
    { "created",       "Created",  100, LVCFMT_LEFT },
    { "last_ddl_time", "Modified", 100, LVCFMT_LEFT },
};

DECL_DATA(Package) =
{
    esvServer73X,
    IDII_PACKAGE,
    "PACKAGE",
    "Packages",
    "SELECT /*+RULE*/ * FROM sys.all_objects"
    " WHERE owner = :p_owner"
    " AND object_type = 'PACKAGE'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(Package)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(PackageBody) =
{
    { "object_name",   "Name",     320, LVCFMT_LEFT },
    { "created",       "Created",  100, LVCFMT_LEFT },
    { "last_ddl_time", "Modified", 100, LVCFMT_LEFT },
};

DECL_DATA(PackageBody) =
{
    esvServer73X,
    IDII_PACKAGE_BODY,
    "PACKAGE BODY",
    "Package Bodies",
    "SELECT /*+RULE*/ * from sys.all_objects"
    " WHERE owner = :p_owner"
    " AND object_type = 'PACKAGE BODY'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(PackageBody)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Trigger) =
{
    { "object_name",     "Name",        275, LVCFMT_LEFT },
    { "table_owner",     "Table Owner", 120, LVCFMT_LEFT },
    { "table_name",      "Table",       120, LVCFMT_LEFT },
    { "trigger_type",    "Type",        120, LVCFMT_LEFT },
    { "triggering_event","Event",       120, LVCFMT_LEFT },
    { "when_clause",     "When",        120, LVCFMT_LEFT },
    { "created",         "Created",     100, LVCFMT_LEFT },
    { "last_ddl_time",   "Modified",    100, LVCFMT_LEFT },
};

DECL_DATA(Trigger) =
{
    esvServer73X,
    IDII_TRIGGER,
    "TRIGGER",
    "Triggers",
    " SELECT /*+RULE*/ "
    " o.object_name,"
    " o.created,"
    " o.last_ddl_time,"
    " o.status,"
    " t.status enabled,"
    " t.table_owner,"
    " t.table_name,"
    " t.trigger_type,"
    " t.triggering_event,"
    " t.when_clause"
    " FROM sys.all_objects o, sys.all_triggers t"
    " WHERE o.owner = :p_owner"
    " AND t.owner = :p_owner"
    " AND o.owner = t.owner"
    " AND object_name = trigger_name"
    " AND object_type = 'TRIGGER'",
    NULL,
    " AND object_name = :p_name",
    "object_name",
    "status", ":p_status",
    "enabled",
    true, true, true,
    INIT_COL_REF(Trigger)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Synonym) =
{
    { "synonym_name", "Synonym", 300, LVCFMT_LEFT },
    { "table_owner",  "Owner",   120, LVCFMT_LEFT },
    { "table_name",   "Object",  300, LVCFMT_LEFT },
    { "db_link",      "DB Link", 120, LVCFMT_LEFT },
};

DECL_DATA(Synonym) =
{
    esvServer73X,
    IDII_SYNONYM, // m_nIconId, m_nImageId
    "SYNONYM",        // m_szType
    "Synonyms",       // m_szTitle
    " SELECT /*+RULE*/ s.*, 'VALID' status" // m_szSqlStatement
    " FROM (SELECT * FROM sys.all_synonyms WHERE owner = :p_owner) s, sys.all_objects o"
    " WHERE s.table_name  = o.object_name"
    " AND s.table_owner = o.owner"
    " UNION"
    " ("
    " SELECT s.*, 'INVALID' status FROM"
    " ("
    " SELECT * FROM sys.all_synonyms WHERE owner = :p_owner"
    " MINUS"
    " SELECT s.*"
    " FROM (SELECT * FROM sys.all_synonyms WHERE owner = :p_owner) s, sys.all_objects o"
    " WHERE s.table_name  = o.object_name"
    " AND s.table_owner = o.owner"
    " ) s"
    " )",
    NULL,
    0, //m_szRefreshWhere
    "synonym_name", // m_szFilterColumn
    "status", ":p_status", // m_szStatusColumn, m_szStatusParam
    0, //m_szEnabledColumn
    false, false, true, // m_bCanCompile, m_bCanDisable
    INIT_COL_REF(Synonym)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Grantee) =
{
    { "grantee", "Grantee", 450, LVCFMT_LEFT },
};

DECL_DATA(Grantee) =
{
    esvServer73X,
    IDII_GRANTEE, // m_nIconId, m_nImageId
    "GRANTEE",              // m_szType
    "Grantee",           // m_szTitle
    " SELECT /*+RULE NO_MERGE*/"
    " DISTINCT grantee"
    " FROM sys.all_tab_privs"
    " WHERE table_schema = :p_owner"
    " UNION"
    " SELECT /*+NO_MERGE*/"
    " DISTINCT grantee"
    " FROM sys.all_col_privs"
    " WHERE table_schema = :p_owner",
    NULL,
    0, //m_szRefreshWhere
    "grantee", // m_szFilterColumn
    0, 0, // m_szStatusColumn, m_szStatusParam
    0, //m_szEnabledColumn
    false, false, false, // m_bCanCompile, m_bCanDisable
    INIT_COL_REF(Grantee)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Cluster) =
{
    { "cluster_name",   "Name",       200, LVCFMT_LEFT  },
    { "cluster_type",   "Type",        60, LVCFMT_LEFT  },
    { "function",       "Function",   100, LVCFMT_LEFT  },
    { "hashkeys",       "Hashkeys",    70, LVCFMT_RIGHT },
    { "key_size",       "Key size",    70, LVCFMT_RIGHT },
    { "tablespace_name","Tablespace", 100, LVCFMT_LEFT  },
    { "initial_extent", "Initial",     80, LVCFMT_RIGHT },
    { "next_extent",    "Next",        80, LVCFMT_RIGHT },
    { "pct_increase",   "Increase",    60, LVCFMT_RIGHT },
    { "max_extents",    "Max Ext",     60, LVCFMT_RIGHT },
    { "pct_free",       "Free",        60, LVCFMT_RIGHT },
    { "pct_used",       "Used",        60, LVCFMT_RIGHT },
};

DECL_DATA(Cluster) =
{
    esvServer73X,
    IDII_CLUSTER,
    "CLUSTER",
    "Clusters",
    "SELECT /*+RULE*/ * FROM sys.all_clusters WHERE owner = :p_owner",
    NULL,
    " AND cluster_name = :p_name",
    "cluster_name",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(Cluster)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(DbLink) =
{
    { "db_link",  "Db Link",  200, LVCFMT_LEFT  },
    { "username", "Username", 200, LVCFMT_LEFT  },
    { "host",     "Host",     100, LVCFMT_LEFT  },
};

DECL_DATA(DbLink) =
{
    esvServer73X,
    IDII_DBLINK,
    "DATABASE LINK",
    "DB Links",
    "SELECT /*+RULE*/ * FROM sys.all_db_links WHERE owner = :p_owner",
    NULL,
    " AND db_link = :p_name",
    "db_link",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(DbLink)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(Snapshot) =
{
    { "name",           "Snapshot",     180, LVCFMT_LEFT },
    { "master_owner",   "Master Owner", 100, LVCFMT_LEFT },
    { "master",         "Master Table", 100, LVCFMT_LEFT },
    { "master_link",    "Master Link",  100, LVCFMT_LEFT },
    { "can_use_log",    "Use Log",       50, LVCFMT_LEFT },
    { "updatable",      "Updatable",     50, LVCFMT_LEFT },
// there isn't the "refresh_method" field in version 7
//  { "refresh_method", "Method",        80, LVCFMT_LEFT },
    { "type",           "Type",          80, LVCFMT_LEFT },
    { "last_refresh",   "Last Refresh",  90, LVCFMT_LEFT },
    { "next",           "Next",          90, LVCFMT_LEFT },
    { "start_with",     "Start with",    90, LVCFMT_LEFT },
    { "query",          "Query",        350, LVCFMT_LEFT },

};

DECL_DATA(Snapshot) =
{
    esvServer73X,
    IDII_SNAPSHOT,
    "SNAPSHOT",
    "Snapshots",
    "SELECT "
            "/*+RULE*/ v.*, decode(error,0,'VALID','INVALID') as status "
        "FROM sys.all_snapshots v WHERE owner = :p_owner",
    NULL,
    " AND name = :p_name",
    "name",
    0,
    "status", 0,
    false, false, true,
    INIT_COL_REF(Snapshot)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(SnapshotLog) =
{
    { "master",      "Master Table", 200, LVCFMT_LEFT },
    { "log_table",   "Log Table",    200, LVCFMT_LEFT },
    { "log_trigger", "Log Trigger",  200, LVCFMT_LEFT },
};

DECL_DATA(SnapshotLog) =
{
    esvServer73X,
    IDII_SNAPSHOT_LOG,
    "SNAPSHOT LOG",
    "Snapshot Logs",
// there isn't the "all_snapshot_logs" view in version 7
    "SELECT /*+RULE*/ * FROM sys.all_snapshot_logs WHERE log_owner = :p_owner",
    NULL,
    " AND log_table = :p_name",
    "master",
    0,
    0, 0,
    false, false, true,
    INIT_COL_REF(SnapshotLog)
};

///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////

DECL_COL_DATA(Type) =
{
    { "type_name",  "Name",      300, LVCFMT_LEFT  },
    { "typecode",   "Code",      100, LVCFMT_LEFT  },
    { "attributes", "Attributes", 80, LVCFMT_RIGHT },
    { "methods",    "Methods",    80, LVCFMT_RIGHT },
    { "incomplete", "Incomplete", 80, LVCFMT_RIGHT },
};

DECL_DATA(Type) =
{
    esvServer80X,
    IDII_TYPE,
    "TYPE",
    "Types",
    "SELECT /*+RULE*/ t1.type_name, t1.typecode, t1.attributes, t1.methods, t1.incomplete, t2.status"
    " FROM sys.all_types t1, sys.all_objects t2 WHERE t1.owner = :p_owner"
    " AND t1.owner = t2.owner AND t1.type_name = t2.object_name AND t2.object_type = 'TYPE'",
    NULL,
    " AND t1.type_name = :p_name",
    "type_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(Type)
};

///////////////////////////////////////////////////////////

DECL_COL_DATA(TypeBody) =
{
    { "type_name",  "Name",      300, LVCFMT_LEFT  },
    { "typecode",   "Code",      100, LVCFMT_LEFT  },
    { "attributes", "Attributes", 80, LVCFMT_RIGHT },
    { "methods",    "Methods",    80, LVCFMT_RIGHT },
    { "incomplete", "Incomplete", 80, LVCFMT_RIGHT },
};

DECL_DATA(TypeBody) =
{
    esvServer80X,
    IDII_TYPE+1,
    "TYPE BODY",
    "Type Bodies",
    "SELECT /*+RULE*/ t1.type_name, t1.typecode, t1.attributes, t1.methods, t1.incomplete, t2.status"
    " FROM sys.all_types t1, sys.all_objects t2 WHERE t1.owner = :p_owner"
    " AND t1.owner = t2.owner AND t1.type_name = t2.object_name AND t2.object_type = 'TYPE BODY'",
    NULL,
    " AND t1.type_name = :p_name",
    "type_name",
    "status", ":p_status",
    0,
    true, false, true,
    INIT_COL_REF(TypeBody)
};

///////////////////////////////////////////////////////////
/*
///////////////////////////////////////////////////////////

DECL_COL_DATA(...) =
{
    { "", "", 200, LVCFMT_LEFT },
};

DECL_DATA(...) =
{
    IDI_...,
    "Sequences",
    "select +RULE * from all_sequences where owner = :p_owner",
    "sequence_name",
    INIT_COL_REF(...)
};

///////////////////////////////////////////////////////////
*/