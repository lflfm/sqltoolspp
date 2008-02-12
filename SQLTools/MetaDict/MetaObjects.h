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

#ifndef __METAOBJECTS_H__
#define __METAOBJECTS_H__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4430 )

//
// Purpose: Class library for storing oracle db objects in meta-dictionary
// Note: Copy-constraction & assign-operation is not supported for all classes
//

// 04.08.2002 bug fix: debug privilege for 9i
// 17.09.2002 bug fix: cluster reengineering fails
// 06.04.2003 bug fix, some oracle 9i privilege are not supported
// 03.08.2003 improvement, domain index support

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "arg_shared.h"


namespace OraMetaDict 
{
    class Dictionary;
    class TextOutput;

    using std::string;
    using std::set;
    using std::map;
    using std::list;
    using std::vector;
    using arg::counted_ptr;

    class _WriteSettings 
    {
    public:
        // WriteSettings struct is optimized for persistent store/restore (BOOL instead bool)
        // Common settings
        bool m_bComments;               // table, view & column comments
        bool m_bGrants;                 // grants
        bool m_bLowerNames;             // safe lowercase object names
        bool m_bShemaName;              // schema as object names prefix
        bool m_bSQLPlusCompatibility;   // view do not have no white space lines, truncate end-line white space 
        // Table Specific
        bool m_bCommentsAfterColumn;    // column comments as end line remark
        int  m_nCommentsPos;            // end line comments position
        bool m_bConstraints;            // constraints (table writing)
        bool m_bIndexes;                // indexes (table writing)
        bool m_bNoStorageForConstraint; // no storage for primary key & unique key
        int  m_nStorageClause;          // table & index storage clause
        bool m_bAlwaysPutTablespace;    // always put tablespace in storage clause (independent of previous flag)
        bool m_bTableDefinition;        // short table definition (table writing)
        bool m_bTriggers;               // table triggers (table writing)
        // Othes
        bool m_bSequnceWithStart;       // sequence with START clause
        bool m_bViewWithTriggers;       // view triggers (view writing)
        bool m_bViewWithForce;          // use FORCE for view statement

        _WriteSettings () { memset(this, 0, sizeof(*this)); }
    };

    class WriteSettings : public _WriteSettings
    {
    public:
        string m_EndOfShortStatement; // what put after comments and grants 

        WriteSettings () : m_EndOfShortStatement(";") {}
    };

    /// Abstract base class for all db object classes ///////////////////////
    class DbObject 
    {
    public:
        DbObject (Dictionary& dictionary) 
            : m_Dictionary(dictionary) {};
        virtual ~DbObject () {};

        Dictionary& GetDictionary ()      const { return m_Dictionary; }
        const char* GetOwner  ()          const { return m_strOwner.c_str(); }
        const char* GetName   ()          const { return m_strName.c_str(); }
  
        virtual bool IsCode ()      const       { return false; }
        virtual bool IsLight ()     const       { return false; }
        virtual bool IsGrantable () const       { return false; }
        virtual bool IsGenerated () const       { return false; }

        virtual const char* GetDefExt ()  const { return "sql"; };
        virtual const char* GetTypeStr () const = 0;

        // return code offset for functions,procedures,packages & views
        virtual int  Write (TextOutput& out, const WriteSettings&) const = 0;
        virtual void WriteGrants (TextOutput& out, const WriteSettings&) const;
        // DBMS_METADATA base support
        static bool UseDbms_MetaData();
        virtual void SetDBMS_MetaDataOption(const char *option, const char *value) const;
        virtual void SetDBMS_MetaDataOption(const char *option, const bool value) const;
        virtual void SetDBMS_MetaDataOption(const char *option, const int  value) const;
        virtual void InitDBMS_MetaData() const;
        virtual void InitDBMS_MetaDataStorage(const WriteSettings & settings) const;
        virtual string DBMS_MetaDataGetDDL(const char *type, 
                                           const char *name = 0,
                                           const char *owner = 0) const;
        virtual string DBMS_MetaDataGetDependentDDL(const char *type, 
                                                    const char *name = 0,
                                                    const char *owner = 0) const;
        virtual string DBMS_MetaDataGetGrantedDDL(const char *type, 
                                                  const char *grantee) const;
    protected:
        Dictionary& m_Dictionary;
    public:
        string m_strOwner,
               m_strName;

    private:
        // copy-constraction & assign-operation is not supported
        DbObject (const DbObject&);
        operator = (const DbObject&);
    };


    /// USER object /////////////////////////////////////////////////////////
    class User : public DbObject 
    {
    public:
        string m_strDefTablespace;
    public:

        User (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        virtual const char* GetTypeStr () const { return "USER"; };
        virtual int Write (TextOutput&, const WriteSettings&) const { return 0; };

    private:
        // copy-constraction & assign-operation is not supported
        User (const User&);
        operator = (const User&);
    };


    /// Storage parameters for tablespaces, tables, indices & clusters //////
    class Storage /**: public DbObject */
    {
    public:
        Storage (/**Dictionary& dictionary*/);

        bool IsStorageDefault (const Storage* pDefStorage) const;
        void WriteTablespace (TextOutput& out) const;
        void WriteOpenStorage (TextOutput& out) const;
        void WriteCloseStorage (TextOutput& out) const;
        void WriteParam (TextOutput& out, const Storage* pDefStorage, const WriteSettings&) const;

    public:
        string m_strTablespaceName;
        int    m_nInitialExtent,
               m_nNextExtent,
               m_nMinExtents,
               m_nMaxExtents, 
               m_nPctIncrease;
        
    private:
        // copy-constraction & assign-operation is not supported
        Storage (const Storage&);
        operator = (const Storage&);
    };


    /// TABLESPACE object ///////////////////////////////////////////////////
    class Tablespace : public DbObject, public Storage 
    {
    public:
        Tablespace (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual const char* GetTypeStr () const { return "TABLESPACE"; };
        virtual int Write (TextOutput&, const WriteSettings&) const { return 0; };

    private:
        // copy-constraction & assign-operation is not supported
        Tablespace (const Tablespace&);
        operator = (const Tablespace&);
    };


    /// Additional storage parameters for tables & indices //////////////////
    class StorageExt : public Storage 
    {
    public:
        StorageExt (/**Dictionary& dictionary*/);

        virtual bool IsIniTransDefault () const      { return m_nIniTrans == 1; }
        virtual bool IsPctFreeDefault () const       { return m_nPctFree == 10; }
        virtual bool IsPctUsedDefault () const       { return m_nPctUsed == 40; }
        bool IsMaxTransDefault () const              { return m_nMaxTrans == 255; }
        bool IsFreeListsDefault () const             { return m_nFreeLists == 1;  }
        bool IsFreeListGroupsDefault () const        { return m_nFreeListGroups == 1; }

        void StorageExt::Write (
            TextOutput& out, const Dictionary& dictionary,
            const char* user, const WriteSettings& settings, 
            bool skipPctUsed, bool useFreeList, bool useLogging
        ) const;

    public:
        int m_nFreeLists,
            m_nFreeListGroups,
            m_nPctFree,
            m_nPctUsed,
            m_nIniTrans,
            m_nMaxTrans;
        
        bool m_bLogging;

    private:
        // copy-constraction & assign-operation is not supported
        StorageExt (const StorageExt&);
        operator = (const StorageExt&);
    };


    /// Base columns subobject (for tables & clusters) //////////////////////
    class Column 
    {
    public:
        string m_strColumnName,
               m_strDataType;
        int    m_nDataPrecision,
               m_nDataScale,
               m_nDataLength;
        bool   m_bNullable;

        void GetSpecString (string& strBuff) const;

        Column ();

    private:
        class SubtypeMap : public map<string, int> {
        public:
            SubtypeMap ();
        };
        static SubtypeMap m_mapSubtypes;

    private:
        // copy-constraction & assign-operation is not supported
        Column (const Column&);
        operator = (const Column&);
    };


    /// Table columns subobject /////////////////////////////////////////////
    class TabColumn : public Column
    {
    public:
        string m_strDataDefault,
               m_strComments;

        TabColumn () {}

    private:
        // copy-constraction & assign-operation is not supported
        TabColumn (const TabColumn&);
        operator = (const TabColumn&);
    };


    /// INDEX object ////////////////////////////////////////////////////////
    enum IndexType { eitNormal, eitBitmap, eitBitmapFunctionBased, eitCluster, eitFunctionBased, eitIOT_TOP, eitDomain };

    class Index : public DbObject, public StorageExt  
    {
    public:
        Index (Dictionary& dictionary);

        virtual bool IsIniTransDefault () const { return m_nIniTrans == 2; }
        virtual const char* GetTypeStr () const { return "INDEX"; };
        virtual bool IsGenerated ()       const { return m_bGenerated; }
        virtual int Write (TextOutput& out, const WriteSettings&) const;
        
        void WriteIndexParams (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strTableOwner, 
               m_strTableName; // may be cluster

        string m_strDomainOwner, // 03.08.2003 improvement, domain index support
               m_strDomain,
               m_strDomainParams;

        IndexType m_Type;
        bool      m_bUniqueness;
        bool      m_bTemporary;
        bool      m_bGenerated;
        bool      m_bReverse;
        bool      m_bCompression;
        int       m_nCompressionPrefixLength;

        map<int,string> m_Columns; // or expression for eitFunctionBased

    private:
        // copy-constraction & assign-operation is not supported
        Index (const Index&);
        operator = (const Index&);
    };


    /// CONSTRAINT object ///////////////////////////////////////////////////
    class Constraint : public DbObject 
    {
    public:
        Constraint (Dictionary& dictionary);

        bool IsNotNullCheck () const;
        virtual bool IsGenerated () const { return m_bGenerated; }

        virtual const char* GetTypeStr () const { return "CONSTRAINT"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;
        void WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strTableName,
               m_strType,
               m_strROwner,
               m_strRConstraintName,
               m_strDeleteRule,
               m_strStatus,
               m_strSearchCondition;

        map<int,string> m_Columns;

        bool m_bDeferrable;
        bool m_bDeferred;
        bool m_bGenerated;

    private:
        // copy-constraction & assign-operation is not supported
        Constraint (const Constraint&);
        operator = (const Constraint&);
    };


    /// TableBase is ancestor of TABLE, SNAPSHOT & SNAPSHOT LOG /////////////
    class TableBase : public DbObject, public StorageExt 
    {
    public:
        TableBase (Dictionary& dictionary);

        void WriteIndexes (TextOutput& out, const WriteSettings&) const;
        void WriteConstraints (TextOutput& out, const WriteSettings&, char chType) const;
        void WriteTriggers (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strClusterName,
               m_strComments;
        bool   m_bCache;

        set<string> m_setConstraints, 
                    m_setIndexes,
                    m_setTriggers;
        
        map<int,string> m_clusterColumns; // map table to cluster columns

    private:
        // copy-constraction & assign-operation is not supported
        TableBase (const TableBase&);
        operator = (const TableBase&);
    };


    /// TABLE object ////////////////////////////////////////////////////////
    enum TableType              { ettNormal, ettTemporary, ettIOT, ettIOT_Overflow };
    enum TemporaryTableDuration { etttdUnknown, ettdTransaction, ettdSession };

    class Table : public TableBase
    {
    public:
        Table (Dictionary& dictionary);

        void WriteDefinition (TextOutput& out, const WriteSettings&) const;
        void WriteComments (TextOutput& out, const WriteSettings&) const;
        void WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings& settings) const;
        const Constraint* FindPrimaryKey () const;

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "TABLE"; };
        virtual bool IsGenerated () const       { return m_TableType == ettIOT_Overflow; }
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        typedef counted_ptr<TabColumn> ColumnPtr;
        typedef map<int,ColumnPtr>  ColumnContainer;

        TableType m_TableType;
        TemporaryTableDuration m_TemporaryTableDuration;
        StorageExt m_IOTOverflow_Storage;
        int m_IOTOverflow_PCTThreshold;
        int m_IOTOverflow_includeColumn;

        ColumnContainer m_Columns;
    private:
        // copy-constraction & assign-operation is not supported
        Table (const Table&);
        operator = (const Table&);
    };


    /// TRIGGER object //////////////////////////////////////////////////////
    class Trigger : public DbObject 
    {
    public:
        Trigger (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool        IsCode ()     const { return true; };
        virtual const char* GetDefExt ()  const { return "trg"; };
        virtual const char* GetTypeStr () const { return "TRIGGER"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strTableOwner,
               m_strTableName, 
               m_strDescription,
               m_strWhenClause,
               m_strRefNames,
               m_strTriggerBody,
               m_strStatus,
			   m_strActionType;

    private:
        // copy-constraction & assign-operation is not supported
        Trigger (const Trigger&);
        operator = (const Trigger&);
    };


    /// VIEW object /////////////////////////////////////////////////////////
    class View : public DbObject 
    {
    public:
        View (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        void WriteTriggers (TextOutput& out, const WriteSettings&) const;
        void WriteComments (TextOutput& out, const WriteSettings&) const;

        virtual bool IsCode () const            { return true; };
        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "VIEW"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strText,
               m_strComments;

        map<int,string>  m_Columns;
        set<string>      m_setTriggers;
        map<int, string> m_mapColComments;

    private:
        // copy-constraction & assign-operation is not supported
        View (const View&);
        operator = (const View&);
    };


    /// SEQUENCE object /////////////////////////////////////////////////////
    class Sequence : public DbObject 
    {
    public:
        Sequence (Dictionary& dictionary);

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "SEQUENCE"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strMinValue,
               m_strMaxValue,
               m_strIncrementBy,
               m_strCacheSize,
               m_strLastNumber;
        bool   m_bCycleFlag,
               m_bOrderFlag;

    private:
        // copy-constraction & assign-operation is not supported
        Sequence (const Sequence&);
        operator = (const Sequence&);
    };


    /// Abstract base class for procedural db object classes ////////////////
    class PlsCode : public DbObject 
    {
    public:
        PlsCode (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool IsGrantable () const       { return true; }
        virtual bool IsCode () const            { return true; };
        virtual const char* GetDefExt () const  { return "sql"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strType,
               m_strText;

    private:
        // copy-constraction & assign-operation is not supported
        PlsCode (const PlsCode&);
        operator = (const PlsCode&);
    };


    /// FUNCTION object /////////////////////////////////////////////////////
    class Function : public PlsCode 
    {
    public:
        Function (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "FUNCTION"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Function (const Function&);
        operator = (const Function&);
    };


    /// PROCEDURE object ////////////////////////////////////////////////////
    class Procedure : public PlsCode 
    {
    public:
        Procedure (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PROCEDURE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Procedure (const Procedure&);
        operator = (const Procedure&);
    };


    /// PACKAGE object //////////////////////////////////////////////////////
    class Package : public PlsCode 
    {
    public:
        Package (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PACKAGE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Package (const Package&);
        operator = (const Package&);
    };


    /// PACKAGE BODY object /////////////////////////////////////////////////
    class PackageBody : public PlsCode 
    {
    public:
        PackageBody (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PACKAGE BODY"; };
        virtual const char* GetDefExt ()  const { return "plb"; };

    private:
        // copy-constraction & assign-operation is not supported
        PackageBody (const PackageBody&);
        operator = (const PackageBody&);
    };


    /// TYPE object /////////////////////////////////////////////////////////
    class Type : public PlsCode 
    {
    public:
        Type (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        void WriteAsIncopmlete (TextOutput& out, const WriteSettings&) const;

        virtual const char* GetTypeStr () const { return "TYPE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Type (const Type&);
        operator = (const Type&);
    };


    /// TYPE BODY object ////////////////////////////////////////////////////
    class TypeBody : public PlsCode 
    {
    public:
        TypeBody (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "TYPE BODY"; };
        virtual const char* GetDefExt ()  const { return "plb"; };

    private:
        // copy-constraction & assign-operation is not supported
        TypeBody (const TypeBody&);
        operator = (const TypeBody&);
    };


    /// SYNONYM object //////////////////////////////////////////////////////
    class Synonym : public DbObject 
    {
    public:
        Synonym (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool        IsLight ()   const  { return true; };
        virtual const char* GetTypeStr () const { return "SYNONYM"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strRefOwner, 
               m_strRefName, 
               m_strRefDBLink;

    private:
        // copy-constraction & assign-operation is not supported
        Synonym (const Synonym&);
        operator = (const Synonym&);
    };


    /// GRANT something (DB objects privileges only) ////////////////////////////
    struct Grant
    {
        string m_strOwner, 
               m_strName, 
               m_strGrantee;

        set<string> m_privileges;
        set<string> m_privilegesWithAdminOption;

        map<string,set<string> > m_colPrivileges;
        map<string,set<string> > m_colPrivilegesWithAdminOption;

        Grant () {}
        
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        Grant (const Grant&);
        operator = (const Grant&);
    };


    /// GrantContainer subject (USER or ROLE in DB) /////////////////////////
    //
    //  GrantContainer can be Grantor or Grantee
    //
    //  Object of Grantor type is not equal "grantor" in "all_tab_privs" view,
    //  it is "schema_name"
    //
    typedef counted_ptr<Grant>       GrantPtr;
    typedef map<string, GrantPtr>    GrantMap;
    typedef GrantMap::iterator       GrantMapIter;
    typedef GrantMap::const_iterator GrantMapConstIter;

    class GrantContainer : public DbObject
    {
    public:
        GrantContainer (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        // If it's grantor then the arguments are (grantee_name, object_name)
        // If it's grantee then the arguments are (grantor_name, object_name)
        Grant& CreateGrant (const char*, const char*);
        Grant& LookupGrant (const char*, const char* = 0);
        void   DestroyGrant (const char*, const char* = 0);
        
        void EnumGrants (void (*pfnDo)(const Grant&, void*), void* = 0, bool sort = false) const;

        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        GrantMap m_mapGrants;

    private:
        // copy-constraction & assign-operation is not supported
        GrantContainer (const GrantContainer&);
        operator = (const GrantContainer&);
    };

    class Grantor : public GrantContainer
    {
    public:
        Grantor (Dictionary& dictionary) : GrantContainer(dictionary) {}
        virtual const char* GetTypeStr () const { return "GRANTOR"; };

        void WriteObjectGrants (const char* name, TextOutput&, const WriteSettings&) const;
    };

    class Grantee : public GrantContainer
    {
    public:
        Grantee (Dictionary& dictionary) : GrantContainer(dictionary) {}
        virtual const char* GetTypeStr () const { return "GRANTEE"; };
    };


    /// CLUSTER object //////////////////////////////////////////////////////
    class Cluster : public DbObject, public StorageExt 
    {
    public:
        Cluster (Dictionary& dictionary);

        virtual bool IsGrantable () const       { return true; }
        virtual bool IsIniTransDefault () const { return m_nIniTrans == 2; }
        virtual const char* GetTypeStr () const { return "CLUSTER"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        bool   m_bIsHash;
        string m_strSize,
               m_strHashKeys,
               m_strHashExpression;
        bool   m_bCache;

        typedef counted_ptr<Column> ColumnPtr;
        typedef map<int,ColumnPtr> ColContainer;

        ColContainer m_Columns;
        set<string>  m_setIndexes;

    private:
        // copy-constraction & assign-operation is not supported
        Cluster (const Cluster&);
        operator = (const Cluster&);
    };


    /// DATABASE LINK object ////////////////////////////////////////////////
    class DBLink : public DbObject 
    {
    public:
        string m_strUser,
               m_strPassword,
               m_strHost;
    public:

        DBLink (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        virtual bool IsLight ()     const       { return true; }
        virtual const char* GetTypeStr () const { return "DATABASE LINK"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        DBLink (const DBLink&);
        operator = (const DBLink&);
    };


    /// SNAPSHOT LOG object /////////////////////////////////////////////////
    //
    // Does a snapshot have cache-option?
    //
    class SnapshotLog : public DbObject, public StorageExt 
    {
    public:
        SnapshotLog (Dictionary& dictionary)
        : DbObject(dictionary) {}

        virtual bool IsIniTransDefault () const      { return m_nIniTrans == 1; }
        virtual bool IsPctFreeDefault () const       { return m_nPctFree == 60; }
        virtual bool IsPctUsedDefault () const       { return m_nPctUsed == 30; }
        virtual const char* GetTypeStr () const      { return "SNAPSHOT LOG";   }
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        SnapshotLog (const SnapshotLog&);
        operator = (const SnapshotLog&);
    };


    /// SNAPSHOT object /////////////////////////////////////////////////////
    //
    // Does a snapshot have cache-option, constraints, indexes, triggers?
    //
    class Snapshot : public DbObject, public StorageExt 
    {
    public:
        Snapshot (Dictionary& dictionary)
        : DbObject(dictionary) {}

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "SNAPSHOT"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strRefreshType,
               m_strStartWith,
               m_strNextTime,
               m_strQuery,
               m_strClusterName,
               m_strComments;
        /*
        bool        m_bCache;
        set<string> m_setConstraints, 
                    m_setIndexes,
                    m_setTriggers;
        */
        map<int,string> m_clusterColumns; // map table to cluster columns

    private:
        // copy-constraction & assign-operation is not supported
        Snapshot (const Snapshot&);
        operator = (const Snapshot&);
    };


} // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__METAOBJECTS_H__
