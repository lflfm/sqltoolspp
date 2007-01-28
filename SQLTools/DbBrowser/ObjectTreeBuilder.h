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

#ifndef OBJECTTREEBUILDER_H
#define OBJECTTREEBUILDER_H

enum ETreeItemType {
  titUnknown,
  titRoot,
  titSchema,
  titPublic,

  titCode,
  titOthers,
  titColumn,        // for tables, indexs & views
  titConstraint,    // for tables

  titTable,
  titIndex,
  titTrigger,
  titCluster,
  titSequence,
  titSynonym,
  titView,

  titPackageBody,
  titPackage,
  titFunction,
  titProcedure,

  titGrantedObjPrivilege,

  titDatabaseLink,
  titRefreshGroup,
  
  // 23.04.02 Command "Find Object" fails on an TYPE object
  titType,
  titTypeBody,
  /////////////////////////////////////////////////////////
  titObjectGroup = 0xff00, // for all object groups
  titDependentOn,          // for views & all code
  titDependentObjs,        // for all base objects
  titColumns,              // for tables, indexs & views
  titConstraints,          // for tables

  titTables, titClusterTables,
  titIndexes, titTableIndexes,
  titTriggers, titTableTriggers,
  titClusters,
  titSequences,
  titSynonyms,
  titViews,

  titPackageBodies,
  titPackages,
  titFunctions,
  titProcedures,

  titGrantedTo,
  titReferencedTables,

  titDatabaseLinks,
  titRefreshGroups,

  // 23.04.02 Command "Find Object" fails on an TYPE object
  titTypes,
  titTypeBodies
};

struct CStaticDescs {
  int m_Size;
  struct TData {
    ETreeItemType m_Type;
    ETreeItemType m_ChildType;
    const char*   m_DisplayName;
    int           m_ImageId;
    int           m_Children;
    //int           m_State;
  } *m_Data;
};

struct CDynamicDescs {
  int m_Size;
  struct TData {
    ETreeItemType m_Type;
    ETreeItemType m_ChildType;
    int           m_ImageId;
    int           m_Children;
    //int           m_State;
    const char*   m_ObjectType;
    const char*   m_SqlStatement;
  } *m_Data;
};

struct CTypeMap {
  int m_Size;
  struct TData {
    ETreeItemType m_Type;
    int           m_ImageId;
    int           m_Children;
    //int           m_State;
    const char*   m_Key;
  } *m_Data;
};

extern CStaticDescs  _StaticDescs;
extern CDynamicDescs _DynamicDescs;
extern CTypeMap      _TypeMap;

int AddItemByStaticDesc  (CTreeCtrl& tree, HTREEITEM hItem, ETreeItemType type,
                          const CStaticDescs& descs = _StaticDescs);
int AddItemByDynamicDesc (OciConnect& connect, 
                          CTreeCtrl& tree, HTREEITEM hItem, 
                          ETreeItemType itemType, CString& itemText,
                          const CDynamicDescs& descs = _DynamicDescs, 
                          const CTypeMap& _map = _TypeMap);

BOOL DescribeObject      (OciConnect& connect, CTreeCtrl& tree, 
                          const char* str, const CTypeMap& _map = _TypeMap);

bool FindObject (OciConnect& connect, const char* str, 
                 std::string& owner, std::string& name, std::string& type);

#endif//OBJECTTREEBUILDER_H
