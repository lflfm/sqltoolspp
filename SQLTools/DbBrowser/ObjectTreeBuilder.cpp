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

// 23.04.02 Command "Find Object" fails on an TYPE object
// 09.11.2003 small improvement, type and uniqueness have been added to index description in "Object Viewer"
// 18.11.2003 bug fix, When using Ctrl +F12 (Load DDL Script) on a table name, only index script is loaded, 
// not table creation script. Its happens only when index name is identical to table name.
// 25.12.2004 (ak) changed default value for abortDlgShowDelay to avoid moving focus to the main window 
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

#include "stdafx.h"
#include "ObjectTreeBuilder.h"
#include "SQLTools.h"
#include "AbortThread/AbortThread.h"
#include "COMMON\StrHelpers.h"
#include <OCI8/BCursor.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CStaticDescs::TData __StaticDescs [] = {
    { titRoot,   titSchema,    "Current User",   11, 1 },
    { titRoot,   titOthers,    "Others Schemas",  0, 1 },
    { titOthers, titPublic,    "PUBLIC",         11, 1 },
    { titSchema, titTables,    "Tables",    0, 1 },
    { titSchema, titViews,     "Views",     0, 1 },
    //{ titSchema, titCode,      "Code",      0, 1 },
    { titSchema, titSequences, "Sequences", 0, 1 },
    { titSchema, titSynonyms,  "Synonyms",  0, 1 },

    { titSchema, titProcedures,    "Procedures",     0, 1 },
    { titSchema, titFunctions,     "Functions",      0, 1 },
    { titSchema, titPackages,      "Packages",       0, 1 },
    { titSchema, titPackageBodies, "Package Bodies", 0, 1 },
    { titSchema, titTriggers,      "Triggers",  0, 1 },

    { titSchema, titIndexes,   "Indexes",   0, 1 },
    { titSchema, titClusters,  "Clusters",  0, 1 },

    { titPublic, titSynonyms,  "Synonyms",  0, 1 },
    /*
    { titCode, titProcedures,    "Procedures",     0, 1 },
    { titCode, titFunctions,     "Functions",      0, 1 },
    { titCode, titPackages,      "Packages",       0, 1 },
    { titCode, titPackageBodies, "Package Bodies", 0, 1 },
    { titCode, titTriggers,      "Triggers",  0, 1 },
    */
    { titTable, titConstraints,      "Constraints", 0, 1 },
    { titTable, titTableIndexes,     "Indexes",     0, 1 },
    { titTable, titTableTriggers,    "Triggers",    0, 1 },
    { titTable, titReferencedTables, "Referenced Tables",    0, 1 },

    { titView,        titDependentOn,   "Dependent On",       0, 1 },
    { titPackage,     titDependentOn,   "Dependent On",       0, 1 },
    { titPackageBody, titDependentOn,   "Dependent On",       0, 1 },
    { titFunction,    titDependentOn,   "Dependent On",       0, 1 },
    { titProcedure,   titDependentOn,   "Dependent On",       0, 1 },
    { titTrigger,     titDependentOn,   "Dependent On",       0, 1 },

    // 23.04.02 Command "Find Object" fails on an TYPE object
    { titType,        titDependentOn,   "Dependent On",       0, 1 },
    { titTypeBody,    titDependentOn,   "Dependent On",       0, 1 },

    { titTable,       titDependentObjs, "Dependent Objects",  0, 1 },
    { titView,        titDependentObjs, "Dependent Objects",  0, 1 },
    { titSequence,    titDependentObjs, "Dependent Objects",  0, 1 },
    { titFunction,    titDependentObjs, "Dependent Objects",  0, 1 },
    { titProcedure,   titDependentObjs, "Dependent Objects",  0, 1 },
    { titPackage,     titDependentObjs, "Dependent Objects",  0, 1 },
    { titSynonym,     titDependentObjs, "Dependent Objects",  0, 1 },

    // 23.04.02 Command "Find Object" fails on an TYPE object
    { titType,        titDependentObjs, "Dependent Objects",  0, 1 },

    { titTable,     titGrantedTo, "Granted Privileges", 0, 1 },
    { titView,      titGrantedTo, "Granted Privileges", 0, 1 },
    { titSequence,  titGrantedTo, "Granted Privileges", 0, 1 },
    { titFunction,  titGrantedTo, "Granted Privileges", 0, 1 },
    { titProcedure, titGrantedTo, "Granted Privileges", 0, 1 },
    { titPackage,   titGrantedTo, "Granted Privileges", 0, 1 },

    // 23.04.02 Command "Find Object" fails on an TYPE object
    { titType,      titGrantedTo, "Granted Privileges", 0, 1 },
    //{ titSynonym,   titGrantedTo, "Granted Privileges", 0, 1 },

    { titCluster, titClusterTables,    "Cluster Tables",    0, 1 },
    /*
    { titSchema, titDatabaseLinks, "", "Database Links", 0, 1 },
    { titSchema, titRefreshGroups, "", "Refresh Groups", 0, 1 }
    */
};

CStaticDescs _StaticDescs = { sizeof __StaticDescs / sizeof __StaticDescs[0], __StaticDescs };

const char* p_obj_sttm =
"SELECT /*+RULE*/ object_name FROM sys.all_objects"
" WHERE object_type=:p_type AND owner=:p_schema";
const char* p_dep_on_sttm =
"SELECT /*+RULE*/ DISTINCT DECODE(referenced_owner,:p_schema,referenced_name,referenced_owner||'.'||referenced_name)"
",referenced_type FROM sys.all_dependencies"
" WHERE owner=:p_schema"
" AND name=:p_parent"
" ORDER BY referenced_type";
const char* p_dep_objs_sttm =
"SELECT /*+RULE*/ DISTINCT DECODE(owner,:p_schema,name,owner||'.'||name), type FROM sys.all_dependencies"
" WHERE referenced_owner=:p_schema"
" AND referenced_name=:p_parent"
" ORDER BY type";
const char* p_syn_sttm =
"SELECT /*+RULE*/ decode(o.owner,:p_schema,o.object_name,o.owner||'.'||o.object_name), o.object_type"
" FROM sys.all_synonyms s, sys.all_objects o"
" WHERE s.table_owner = o.owner"
" AND s.table_name = o.object_name"
" AND s.synonym_name = :p_parent"
" AND s.owner =:p_schema"
" ORDER BY o.object_type";
const char* p_constr_sttm =
"SELECT /*+RULE*/ constraint_name,constraint_type"
" FROM sys.all_constraints"
" WHERE owner=:p_schema and table_name=:p_parent"
" ORDER BY decode(constraint_type,'P',1,'U',2,'R',3,'C',4,5)";
const char* p_others_sttm =
"SELECT /*+RULE*/ username from sys.all_users au"
" WHERE EXISTS (SELECT * FROM sys.all_objects ao WHERE ao.owner = au.username)"
" ORDER BY username";
const char* p_tab_col_sttm =
"select /*+RULE*/ t.column_name||'   '||lower(data_type)"
"||decode(t.data_type,'NUMBER',decode(t.data_precision || t.data_scale,null,null,"
"'('||decode(t.data_precision,null, '*',t.data_precision)||decode(t.data_scale,null,null,','||t.data_scale)||')'),"
"'FLOAT','('||t.data_precision||')',"
"'LONG',null,"
"'LONG RAW',null,"
"'BLOB',null,"
"'CLOB',null,"
"'NCLOB',null,"
"'BFILE',null,"
"'CFILE',null,"
"'DATE',null,'('||t.data_length||')')"
"||'   '||decode(nullable,'N','not null',null) "
"||'   '||decode(c.comments,null,null,'/* '||c.comments||' */')"
" from sys.all_tab_columns t,sys.all_col_comments c"
" where t.owner=:p_schema and t.table_name=:p_parent"
" and t.owner=c.owner and t.table_name=c.table_name"
" and t.column_name=c.column_name"
" order by t.<ORDERBY_COLUMN>";
const char* p_tab_idx_sttm =
"select /*+RULE*/ decode(owner,:p_schema,index_name,owner||'.'||index_name)||decode(index_type,'NORMAL',NULL,'   '||lower(index_type))||decode(uniqueness,'NONUNIQUE',NULL,'   '||Lower(uniqueness)) from sys.all_indexes"
" where table_owner=:p_schema and table_name=:p_parent";
const char* p_tab_trg_sttm =
"select /*+RULE*/ decode(owner,:p_schema,trigger_name,owner||'.'||trigger_name) from sys.all_triggers"
" where table_owner=:p_schema and table_name=:p_parent";
const char* p_view_col_sttm =
"select /*+RULE*/ column_name from sys.all_tab_columns"
" where owner=:p_schema and table_name=:p_parent";
const char* p_all_idx_sttm =
"select /*+RULE*/ column_name from sys.all_ind_columns"
" where index_name = :p_parent and index_owner = :p_schema";
const char* p_idx_col_sttm =
"select /*+RULE*/ t.column_name||'   '||lower(t.data_type)"
"||decode(t.data_type,'NUMBER',decode(t.data_precision || t.data_scale,null,null,"
"'('||decode(t.data_precision,null, '*',t.data_precision)||decode(t.data_scale,null,null,','||t.data_scale)||')'),"
"'FLOAT','('||t.data_precision||')',"
"'LONG',null,"
"'LONG RAW',null,"
"'BLOB',null,"
"'CLOB',null,"
"'NCLOB',null,"
"'BFILE',null,"
"'CFILE',null,"
"'DATE',null,'('||t.data_length||')')"
" ||'   '||decode(t.nullable,'N','not null',null)"
" from sys.all_tab_columns t, sys.all_ind_columns i"
" where i.index_owner=:p_schema"
" and i.index_name=:p_parent"
" and t.owner=i.table_owner"
" and t.table_name=i.table_name"
" and t.column_name=i.column_name"
" order by i.column_position";
const char* p_con_col_sttm =
"select /*+RULE*/ t.column_name||'   '||lower(data_type)"
"||decode(t.data_type,'NUMBER',decode(t.data_precision || t.data_scale,null,null,"
"'('||decode(t.data_precision,null, '*',t.data_precision)||decode(t.data_scale,null,null,','||t.data_scale)||')'),"
"'FLOAT','('||t.data_precision||')',"
"'LONG',null,"
"'LONG RAW',null,"
"'BLOB',null,"
"'CLOB',null,"
"'NCLOB',null,"
"'BFILE',null,"
"'CFILE',null,"
"'DATE',null,'('||t.data_length||')')"
"||'   '||decode(nullable,'N','not null',null) "
" from sys.all_tab_columns t, sys.all_cons_columns c"
" where t.owner=:p_schema and t.owner=c.owner"
" and t.table_name=c.table_name"
" and c.constraint_name=:p_parent"
" and t.column_name=c.column_name"
" order by c.position";
const char* p_cls_tab_sttm =
"select /*+RULE*/ table_name from sys.all_tables"
" where cluster_name = :p_parent and owner = :p_schema";
const char* p_granted_obj_privs =
"select /*+RULE*/ grantee||\' can \'||privilege from sys.all_tab_privs"
" where table_name = :p_parent"
" and table_schema = :p_schema";
const char* p_ref_tab_sttm =
"select /*+RULE*/ distinct decode(owner,:p_schema,null,owner||'.')||table_name from sys.all_constraints"
" where (r_owner,r_constraint_name)"
" in (select owner,constraint_name from sys.all_constraints"
" where owner = :p_schema"
" and table_name = :p_parent"
" and constraint_type in (\'P\',\'U\'))";


CDynamicDescs::TData __DynamicDescs [] = {
    { titOthers,    titSchema,   11, 1, NULL,    p_others_sttm },

    { titTables,    titTable,     2, 1, "TABLE",    p_obj_sttm },
    { titIndexes,   titIndex,     5, 1, "INDEX",    p_obj_sttm },
    { titTriggers,  titTrigger,   9, 1, "TRIGGER",  p_obj_sttm },
    { titClusters,  titCluster,   3, 1, "CLUSTER",  p_obj_sttm },
    { titSequences, titSequence,  7, 1, "SEQUENCE", p_obj_sttm },
    { titSynonyms,  titSynonym,   8, 1, "SYNONYM",  p_obj_sttm },
    { titViews,     titView,     10, 1, "VIEW",     p_obj_sttm },

    { titPackages,      titPackage,       6, 1, "PACKAGE",      p_obj_sttm },
    // 23.04.02 Command "Find Object" fails on an TYPE object
    { titTypes,         titType,         25, 1, "TYPE",         p_obj_sttm },
    //{ titPackageBodies, titPackageBody,  24, 1, "PACKAGE BODY", p_obj_sttm },
    { titFunctions,     titFunction,     12, 1, "FUNCTION",     p_obj_sttm },
    { titProcedures,    titProcedure,    13, 1, "PROCEDURE",    p_obj_sttm },

    { titTable,         titColumn, 14, 0, NULL, p_tab_col_sttm   },
    { titView,          titColumn, 14, 0, NULL, p_tab_col_sttm/*p_view_col_sttm*/  },
    { titIndex,         titColumn, 14, 0, NULL, p_idx_col_sttm   },
    { titConstraint,    titColumn, 14, 0, NULL, p_con_col_sttm   },

    { titTableIndexes,  titIndex,    5, 1, NULL, p_tab_idx_sttm  },
    { titTableTriggers, titTrigger,  9, 1, NULL, p_tab_trg_sttm  },
    { titConstraints,   titUnknown, -1, 1, NULL, p_constr_sttm   },
    { titDependentObjs, titUnknown, -1, 1, NULL, p_dep_objs_sttm },
    { titDependentOn,   titUnknown, -1, 1, NULL, p_dep_on_sttm   },
    { titSynonym,       titUnknown, -1, 1, NULL, p_syn_sttm      },
    { titClusterTables, titTable,    2, 1, NULL, p_cls_tab_sttm  },

    { titGrantedTo, titGrantedObjPrivilege, 23, 0, NULL, p_granted_obj_privs },
    { titReferencedTables, titTable,  2, 1, NULL, p_ref_tab_sttm  },
};

CDynamicDescs _DynamicDescs = { sizeof __DynamicDescs / sizeof __DynamicDescs[0], __DynamicDescs };

CTypeMap::TData __TypeMap [] = {
    { titTable,       2, 1, "TABLE"        },
    { titSequence,    7, 1, "SEQUENCE"     },
    { titSynonym,     8, 1, "SYNONYM"      },
    { titView,       10, 1, "VIEW"         },
    { titIndex,       5, 1, "INDEX"        },
    { titPackage,     6, 1, "PACKAGE"      },
    { titPackageBodies, 24, 1, "PACKAGE BODY" },
    { titFunction,   12, 1, "FUNCTION"     },
    { titProcedure,  13, 1, "PROCEDURE"    },
    { titTrigger,     9, 1, "TRIGGER"      },

    // 23.04.02 Command "Find Object" fails on an TYPE object
    { titType,       25, 1, "TYPE"      },
    { titTypeBodies, 26, 1, "TYPE BODY" },

    { titConstraint, 16, 1, "P" },
    { titConstraint, 17, 1, "U" },
    { titConstraint, 18, 1, "R" },
    { titConstraint, 15, 1, "C" },

    //{ titUnknown,    -1, 1, "NON-EXISTENT" },
};

CTypeMap _TypeMap = { sizeof __TypeMap / sizeof __TypeMap[0], __TypeMap };

void GetObjectName (CTreeCtrl& tree, HTREEITEM hItem, CString& object, CString& schema)
{
    schema.Empty();
    object.Empty();

    DWORD type = tree.GetItemData(hItem);
    while (hItem
        && (type != titSchema && type != titPublic)
        && type >= titObjectGroup) {
            hItem = tree.GetParentItem(hItem);
            if (hItem)
                type = tree.GetItemData(hItem);
        }

        if (hItem && (type == titSchema || type == titPublic)) {
            schema = tree.GetItemText(hItem);
            return;
        }

        if (hItem && tree.GetItemData(hItem) < titObjectGroup) {
            object = tree.GetItemText(hItem);
            int chr_pos = object.Find('.');
            if (chr_pos != -1) {
                int len = object.GetLength() - chr_pos;
                char* buff = object.GetBuffer(len);

                memcpy(schema.GetBuffer(chr_pos), buff, chr_pos);
                memmove(buff, buff + chr_pos + 1, len);

                schema.ReleaseBuffer(chr_pos);
                object.ReleaseBuffer(len);
            }

            chr_pos = object.Find(' ');
            if (chr_pos != -1)
                object.Truncate(chr_pos);
        }

        if (hItem && schema.IsEmpty()) {
            CString dummy;
            hItem = tree.GetParentItem(hItem);
            if (hItem)
                GetObjectName(tree, hItem, dummy, schema);
            //schema = GetSchemaName(tree, hItem);
        }
}

int AddItemByStaticDesc  (CTreeCtrl& tree, HTREEITEM hItem, ETreeItemType type,
                          const CStaticDescs& descs)
{
    int added(0);
    TV_INSERTSTRUCT  tvstruct;
    tvstruct.hParent      = hItem;
    tvstruct.hInsertAfter = TVI_LAST;
    tvstruct.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    for (int i = 0; i < descs.m_Size; i++)
        if (descs.m_Data[i].m_Type == type) {
            added++;
            tvstruct.item.pszText   = (char*)descs.m_Data[i].m_DisplayName;
            tvstruct.item.lParam    = descs.m_Data[i].m_ChildType;
            tvstruct.item.cChildren = descs.m_Data[i].m_Children;
            tvstruct.item.iImage    = tvstruct.item.iSelectedImage = descs.m_Data[i].m_ImageId;
            hItem = tree.InsertItem(&tvstruct);
        }
        return added;
}

int AddItemByDynamicDesc (OciConnect& connect,
                          CTreeCtrl& tree, HTREEITEM hItem,
                          ETreeItemType itemType, CString& /*itemText*/,
                          const CDynamicDescs& descs,
                          const CTypeMap& _map)
{
	if (! connect.IsOpen()) {
		throw Common::AppException("AddItemByDynamicDesc: not connected to database!");
	}
	
    CWaitCursor wait;
    CAbortController abortCtrl(*GetAbortThread(), &connect, 10);
    abortCtrl.SetActionText("Loading object description...");

    int added(0);
    TV_INSERTSTRUCT  tvstruct;
    tvstruct.hParent      = hItem;
    tvstruct.hInsertAfter = TVI_LAST;
    tvstruct.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    for (int i = 0; i < descs.m_Size; i++) {
        if (descs.m_Data[i].m_Type == itemType) {
            tvstruct.item.iImage    = tvstruct.item.iSelectedImage = descs.m_Data[i].m_ImageId;
            tvstruct.item.lParam    = descs.m_Data[i].m_ChildType;
            tvstruct.item.cChildren = descs.m_Data[i].m_Children;

            CString parent_name, schema_name;
            GetObjectName(tree, hItem, parent_name, schema_name);

            bool bOrderByName = GetSQLToolsSettings().GetColumnOrderByName();
            string sOrderByDefault;
            switch (itemType)
            {
                case titTable:
                case titView:
                    sOrderByDefault = "column_id";
                    break;
                case titIndex:
                    sOrderByDefault = "column_position";
                    break;
                case titConstraint:
                    sOrderByDefault = "position";
                    break;
            }

            Common::Substitutor subst;
            subst.AddPair("<ORDERBY_COLUMN>", bOrderByName ? "column_name" : sOrderByDefault);

            subst << descs.m_Data[i].m_SqlStatement;

            OciCursor cursor(connect, subst.GetResult());

            try {  
                cursor.Bind(":p_schema", schema_name);
            } 
            catch (const OciException& x) {
                if (x != 1036) throw;
            }
            try { 
                cursor.Bind(":p_parent", parent_name);
            } 
            catch (const OciException& x) {
                if (x != 1036) throw;
            }
            if (descs.m_Data[i].m_ChildType != titUnknown) {
                try { 
                    if (descs.m_Data[i].m_ObjectType)
                        cursor.Bind(":p_type", descs.m_Data[i].m_ObjectType);
                } 
                catch (const OciException& x) {
                    if (x != 1036) throw;
                }
            }
            cursor.Execute();

            if (descs.m_Data[i].m_ChildType != titUnknown) {
                std::string object;
                while (cursor.Fetch()) {
                    added++;
                    cursor.GetString(0, object);
                    tvstruct.item.pszText = (char*)object.c_str();
                    tree.InsertItem(&tvstruct);
                }
            } else {
                std::string object, type;
                while (cursor.Fetch()) {
                    cursor.GetString(0, object);
                    cursor.GetString(1, type);
                    for (int j = 0; j < _map.m_Size; j++) {
                        if (!strcmp(_map.m_Data[j].m_Key, type.c_str())) {
                            added++;
                            tvstruct.item.pszText   = (char*)object.c_str();
                            tvstruct.item.iImage    = tvstruct.item.iSelectedImage = _map.m_Data[j].m_ImageId;
                            tvstruct.item.lParam    = _map.m_Data[j].m_Type;
                            tvstruct.item.cChildren = _map.m_Data[j].m_Children;
                            tree.InsertItem(&tvstruct);
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    return added;
}

bool FindObject (OciConnect& connect, const char* str, 
                 std::string& _owner, std::string& _name, std::string& _type)
{
	if (! connect.IsOpen()) {
		throw Common::AppException("FindObject: not connected to database!");
	}
	
    CWaitCursor wait;
    CAbortController abortCtrl(*GetAbortThread(), &connect, 10);
    abortCtrl.SetActionText("Searching for object...");

    OciStringVar a_name(30+1);
    OciStringVar b_name(30+1);
    {
        OciStatement sttm(connect);
        OciStringVar in_name(255+1);
        OciStringVar c_name(30+1);
        OciStringVar dblink(30+1);
        OciNumberVar nextpos(connect);

        in_name.Assign(str);
        sttm.Prepare(
            "begin "
            "dbms_utility.name_tokenize(:p_name,:p_a,:p_b,:p_c,:p_dblink,:p_nextpos);"
            "end;");
        sttm.Bind(":p_name",    in_name);
        sttm.Bind(":p_a",       a_name);
        sttm.Bind(":p_b",       b_name);
        sttm.Bind(":p_c",       c_name);
        sttm.Bind(":p_dblink",  dblink);
        sttm.Bind(":p_nextpos", nextpos);
        sttm.Execute(1, true);
        sttm.Close();

        if (!dblink.IsNull())
            throw OciException(0, "Describe is not support dblink!");
    }

    {
        // I am trying to get more then 1 object 
        // because Oracle uses different name resolving rules in SQL and PL/SQL
        // 18.11.2003 bug fix, When using Ctrl +F12 (Load DDL Script) on a table name, only index script is loaded, 
        // not table creation script. Its happens only when index name is identical to table name.

        OciCursor cursor(connect);
        cursor.Prepare(
            "select /*+RULE*/ owner, object_name, object_type, 1 inx"
            ",decode(object_type, 'PACKAGE BODY', 1, 'TYPE BODY', 1, 'TRIGGER', 2, 'INDEX', 3, 'CLUSTER', 4, 0) subinx"
            " from sys.all_objects"
            " where owner = user and object_name = :p_a"
            " and object_type not like '%PARTITION%'"
            " union "
            "select /*+RULE*/ owner, object_name, object_type, 2 inx"
            ",decode(object_type, 'PACKAGE BODY', 1, 'TYPE BODY', 1, 'TRIGGER', 2, 'INDEX', 3, 'CLUSTER', 4, 0) subinx"
            " from sys.all_objects"
            " where owner = 'PUBLIC' and object_name = :p_a"
            " and object_type not like '%PARTITION%'"
            " union "
            "select /*+RULE*/ owner, object_name, object_type, 3 inx"
            ",decode(object_type, 'PACKAGE BODY', 1, 'TYPE BODY', 1, 'TRIGGER', 2, 'INDEX', 3, 'CLUSTER', 4, 0) subinx"
            " from sys.all_objects"
            " where owner = :p_a and object_name = :p_b"
            " and object_type not like '%PARTITION%'"
            " order by inx, subinx"
            );

        cursor.Bind(":p_a", a_name);
        cursor.Bind(":p_b", b_name);

        cursor.Execute();
        if (cursor.Fetch()) 
        {
            cursor.GetString(0, _owner);
            cursor.GetString(1, _name);
            cursor.GetString(2, _type);
            return true;
        }
    }

    return false;
}

BOOL DescribeObject (OciConnect& connect, CTreeCtrl& tree,
                     const char* str, const CTypeMap& _map)
{
	if (! connect.IsOpen()) {
		throw Common::AppException("DescribeObject: not connected to database!");
	}
	
    CWaitCursor wait;
    CAbortController abortCtrl(*GetAbortThread(), &connect, 10);
    abortCtrl.SetActionText("Searching for object...");

    OciStringVar a_name(30+1);
    OciStringVar b_name(30+1);
    {
        OciStatement sttm(connect);
        OciStringVar in_name(255+1);
        OciStringVar c_name(30+1);
        OciStringVar dblink(30+1);
        OciNumberVar nextpos(connect);

        in_name.Assign(str);
        sttm.Prepare(
            "begin "
            "dbms_utility.name_tokenize(:p_name,:p_a,:p_b,:p_c,:p_dblink,:p_nextpos);"
            "end;");
        sttm.Bind(":p_name",    in_name);
        sttm.Bind(":p_a",       a_name);
        sttm.Bind(":p_b",       b_name);
        sttm.Bind(":p_c",       c_name);
        sttm.Bind(":p_dblink",  dblink);
        sttm.Bind(":p_nextpos", nextpos);
        sttm.Execute(1, true);
        sttm.Close();

        if (!dblink.IsNull())
            throw OciException(0, "Describe is not support dblink!");
    }

    {
        // I am trying to get more then 1 object 
        // because Oracle uses different name resolving rules in SQL and PL/SQL

        OciCursor cursor(connect);
        cursor.Prepare(
            "select /*+RULE*/ owner, object_name, object_type, 1 inx from sys.all_objects"
            " where owner = user and object_name = :p_a"
            " and object_type not like '%PARTITION%'"
            " union "
            "select /*+RULE*/ owner, object_name, object_type, 2 inx from sys.all_objects"
            " where owner = 'PUBLIC' and object_name = :p_a"
            " and object_type not like '%PARTITION%'"
            " union "
            "select /*+RULE*/ owner, object_name, object_type, 3 inx from sys.all_objects"
            " where owner = :p_a and object_name = :p_b"
            " and object_type not like '%PARTITION%'"
            " order by inx"
            );

        cursor.Bind(":p_a", a_name);
        cursor.Bind(":p_b", b_name);

        cursor.Execute();

        HTREEITEM hItem = 0;
        TV_INSERTSTRUCT  tvstruct;
        tvstruct.hParent      = TVI_ROOT;
        tvstruct.hInsertAfter = TVI_LAST;
        tvstruct.item.mask    = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

        string _owner, _name, _type;

        while (cursor.Fetch()) 
        {
            cursor.GetString(0, _owner);
            cursor.GetString(1, _name);
            cursor.GetString(2, _type);
            
            CString fullname;
            fullname.Format("%s.%s", _owner.c_str(), _name.c_str());
            
            for (int j = 0; j < _map.m_Size; j++) 
            {
                if (!strcmp(_map.m_Data[j].m_Key,(char*)_type.c_str())) 
                {
                    tvstruct.item.pszText   = (char*)(const char*)fullname;
                    tvstruct.item.iImage    =
                        tvstruct.item.iSelectedImage = _map.m_Data[j].m_ImageId;
                    tvstruct.item.lParam    = _map.m_Data[j].m_Type;
                    tvstruct.item.cChildren = _map.m_Data[j].m_Children;
                    hItem = tree.InsertItem(&tvstruct);
                    break;
                }
            }
        }

        if (hItem) {
            tree.Expand(hItem, TVE_EXPAND);
            if (tvstruct.item.lParam == titSynonym)
                tree.Expand(tree.GetChildItem(hItem), TVE_EXPAND);
            return TRUE;
        }
    }

    return FALSE;
}

