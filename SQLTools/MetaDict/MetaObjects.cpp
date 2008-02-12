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

// 24.08.99 changed through oracle default clause error
// 31.08.99 fix: a serious error in skip_back_space
// 18.05.00 fix: a failure in skip_db_name
// 20.11.01 fix: a pakage body text is corrupted if a pakage body header has more 
//               then one space between "package" and "body".
// 29.05.02 bug fix: using number intead of integer 
// 04.08.02 bug fix: debug privilege for 9i
// 17.09.02 bug fix: cluster reengineering fails
// 17.09.02 bug fix: better SQLPlus compatibility
// 06.04.2003 bug fix, some oracle 9i privilege are not supported
// 03.08.2003 improvement, domain index support
// 09.11.2003 bug fix, quote character in comments for table/view DDL
// 10.11.2003 bug fix, missing public keyword for public database links
// 16.11.2003 bug fix, missing public keyword for public synonyms
// 30.11.2004 bug fix, trigger reverse-engineering fails if in OF clause if column name contains "ON_"/"_ON"/"_ON_"
// 09.01.2005 bug fix, trigger reverse-engineering fails if description contains comments
// 12.04.2005 bug fix #1181324, (ak) trigger reverse-engineering fails if description contains comments (again)
// 13.06.2005 B#1078885 - Doubled GRANT when extracting Columns Privilages (tMk).
// 14.06.2005 B#1191426 - Invalid COMPRESS index generation (tMk).
// 19.01.2006 B#1404162 - Trigger with CALL - unsupported syntax (tMk)
// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters

#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\TextOutput.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "SQLWorksheet/PlsSqlParser.h"
#include "SQLTools.h"
#include "OCI8/ACursor.h"

#pragma warning ( disable : 4800 )


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace OraMetaDict 
{
    char csz_dbms_metadata_get_ddl[] =
        "select dbms_metadata.get_ddl(:type, :name, :owner) from dual";

    char csz_dbms_metadata_get_dependent_ddl[] =
        "select dbms_metadata.get_dependent_ddl(:type, :name, :owner) from dual";

    char csz_dbms_metadata_get_granted_ddl[] =
        "select dbms_metadata.get_granted_ddl(:type, :grantee) from dual";

    char csz_dbms_metadata_set_option[] = 
                     "begin "
                     "dbms_metadata.set_transform_param("
                     "DBMS_METADATA.SESSION_TRANSFORM,"
                     ":option,"
                     ":value);"
                     "end;";

    using namespace std;
    const int cnUnlimitedVal = 2147483645;

    int skip_space (const char* str)
    {
        for (int i(0); str && str[i] && isspace(str[i]); i++);
        return i;
    }
    int skip_back_space (const char* str, int len)
    {
        if (len) 
        {
            for (int i(len - 1); str && i >= 0; i--)
                if (!isspace(str[i])) 
                {
                    i++;
                    break;
                }
             return (i < 0) ? 0 : i;
        }
        return 0;
    }
    void get_word_SP (const char* str, const char*& wordPtr, int& wordLen)
    {
        _ASSERTE(str);
        wordPtr = 0;
        wordLen = 0;
        for (int i(0); str && str[i] && (isspace(str[i]) || ispunct(str[i])); i++);
        wordPtr = str + i;
        for (; str && str[i] && !isspace(str[i]) && !ispunct(str[i]); i++, wordLen++);
    }
    int skip_space_and_comments (const char* str)
    {
        for (int i(0); str && str[i]; i++) 
        {
            if (isspace(str[i])) 
            {
                continue;
            } 
            else if (str[i] == '-' && str[i+1] == '-') 
            {
                for (i += 2; str[i] && str[i] != '\n'; i++);
                continue;
            } 
            else if (str[i] == '/' && str[i+1] == '*') 
            {
                for (i += 2; str[i] && !(str[i] == '*' && str[i+1] == '/'); i++);
                i++;
                continue;
            } 
            else 
                break;
        }
        return i;
    }
    void get_word_ID (const char* str, const char*& wordPtr, int& wordLen)
    {
        _ASSERTE(str);
        wordPtr = 0;
        wordLen = 0;
        int i = skip_space_and_comments(str);
        wordPtr = str + i;
        if (str && str[i] != '\"') 
        {
            for (; str && str[i] && (isalnum(str[i]) || strchr("_$#",str[i])); i++, wordLen++);
        } 
        else 
        {
            for (i++, wordLen++; str && str[i] && str[i] != '\"'; i++, wordLen++); 
            if (str && str[i] == '\"')
                wordLen++;
        }
    }
    int skip_db_name (const char* str)
    {
        int len;
        const char* ptr;
        get_word_ID(str, ptr, len);
        ptr += len;
        int offset = skip_space_and_comments(ptr);

        if (ptr[offset] == '.') 
        {
            ptr += offset;
            ptr++;
            get_word_ID(ptr, ptr, len);
            ptr += len;
        }
//        ptr += skip_space_and_comments(ptr);

        return (ptr - str);
    }
    int count_lines (const char* str, int len = INT_MAX)
    {
        int count = 0;
        for (int i(0); str[i] && i < len; i++)
            if (str[i] == '\n') 
                count++;

        return count;
    }
    void write_text_block (TextOutput& out, const char* text, int len, bool trunc, bool strip)
    {
        _ASSERTE(!(!trunc && strip));
        
        len = skip_back_space(text, len);

        if (!trunc)
            out.PutLine(text, len);
        else 
        {
            istringstream i(text, len);
            string buffer;

            while (getline(i, buffer, '\n')) 
            {
                int size = skip_back_space(buffer.c_str(), buffer.size());
                if (!strip || size) 
                    out.PutLine(buffer.c_str(), size);
            }
        }
    }

    /// DbObject ///////////////////////////////////////////////////////////////

    bool DbObject::UseDbms_MetaData()
    {
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();
        if ((GetSQLToolsSettings().GetUseDbmsMetaData()) && 
            (connect.GetVersion() >= OCI8::esvServer9X))
            return true;
        else
            return false;
    }

    void DbObject::SetDBMS_MetaDataOption(const char *option, const char *value) const
    {
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();
        OciAutoCursor sttm(connect);

        sttm.Prepare(csz_dbms_metadata_set_option);
        sttm.Bind(":option", option);
        sttm.Bind(":value", value);
        sttm.Execute();
    }

    void DbObject::SetDBMS_MetaDataOption(const char *option, const bool value) const
    {
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();
        OciAutoCursor sttm(connect);

        if (value)
            sttm.Prepare("begin "
                         "dbms_metadata.set_transform_param("
                         "DBMS_METADATA.SESSION_TRANSFORM,"
                         ":option,"
                         "true);"
                         "end;"
                         );
        else
            sttm.Prepare("begin "
                         "dbms_metadata.set_transform_param("
                         "DBMS_METADATA.SESSION_TRANSFORM,"
                         ":option,"
                         "false);"
                         "end;"
                         );

        sttm.Bind(":option", option);
        sttm.Execute();
    }

    void DbObject::SetDBMS_MetaDataOption(const char *option, const int  value) const
    {
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();
        OciAutoCursor sttm(connect);

        sttm.Prepare(csz_dbms_metadata_set_option);
        sttm.Bind(":option", option);
        sttm.Bind(":value", value);
        sttm.Execute();
    }

    void DbObject::InitDBMS_MetaData() const
    {
        SetDBMS_MetaDataOption("SQLTERMINATOR", true);
        SetDBMS_MetaDataOption("PRETTY", true);
        SetDBMS_MetaDataOption("SIZE_BYTE_KEYWORD", true);
    }

    void DbObject::InitDBMS_MetaDataStorage(const WriteSettings & settings) const
    {
        if (settings.m_bAlwaysPutTablespace)
            SetDBMS_MetaDataOption("TABLESPACE", true);
        else
            SetDBMS_MetaDataOption("TABLESPACE", false);

        if (settings.m_nStorageClause != 0)
            SetDBMS_MetaDataOption("STORAGE", true);
        else
            SetDBMS_MetaDataOption("STORAGE", false);
    }

    string DbObject::DBMS_MetaDataGetDDL(const char *type, 
                                         const char *name,
                                         const char *owner) const
    {
        string s_ddl = "";
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();

        OciAutoCursor meta_cursor(connect, csz_dbms_metadata_get_ddl, 100, 512*1024);

        meta_cursor.Bind(":type", type);
        meta_cursor.Bind(":name", name == 0 ? GetName() : name);
        meta_cursor.Bind(":owner", owner == 0 ? GetOwner() : owner);
        meta_cursor.Execute();

        if (meta_cursor.Fetch())
        {
            meta_cursor.GetString(0, s_ddl);
        }

        return s_ddl;
    }

    string DbObject::DBMS_MetaDataGetDependentDDL(const char *type, 
                                        const char *name,
                                        const char *owner) const
    {
        string s_ddl = "";
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();

        try
        {
            OciAutoCursor meta_cursor(connect, csz_dbms_metadata_get_dependent_ddl, 100, 512*1024);

            meta_cursor.Bind(":type", type);
            meta_cursor.Bind(":name", name == 0 ? GetName() : name);
            meta_cursor.Bind(":owner", owner == 0 ? GetOwner() : owner);
            meta_cursor.Execute();

            if (meta_cursor.Fetch())
            {
                meta_cursor.GetString(0, s_ddl);
            }
        }
        catch (const OciException& x)
        {
            if (x != 31608)
                throw;
        }

        return s_ddl;
    }

    string DbObject::DBMS_MetaDataGetGrantedDDL(const char *type, 
                                                const char *grantee) const
    {
        string s_ddl = "";
        OciConnect& connect = ((CSQLToolsApp*)AfxGetApp())->GetConnect();

        OciAutoCursor meta_cursor(connect, csz_dbms_metadata_get_granted_ddl, 100, 512*1024);

        meta_cursor.Bind(":type", type);
        meta_cursor.Bind(":grantee", grantee);
        meta_cursor.Execute();

        if (meta_cursor.Fetch())
        {
            meta_cursor.GetString(0, s_ddl);
        }

        return s_ddl;
    }

    void DbObject::WriteGrants (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData() && ! m_strOwner.empty() && (m_strOwner != ""))
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDependentDDL("OBJECT_GRANT"));

            return;
        }

        Grantor* pGrantor = 0;
        
        try 
        {
            pGrantor = &(m_Dictionary.LookupGrantor(m_strOwner.c_str()));
        } 
        catch (const XNotFound&) {}

        if (pGrantor)
            pGrantor->WriteObjectGrants(m_strName.c_str(), out, settings);
    }

    /// Storage ////////////////////////////////////////////////////////////////

    Storage::Storage (/**Dictionary& dictionary*/) 
    /**: DbObject(dictionary)*/ 
    {
        m_nInitialExtent =
        m_nNextExtent    =
        m_nMinExtents    =
        m_nMaxExtents    =
        m_nPctIncrease   = 0;
    }

    bool Storage::IsStorageDefault (const Storage* pDefStorage) const
    {
        if (pDefStorage
        && m_nInitialExtent == pDefStorage->m_nInitialExtent
        && m_nNextExtent    == pDefStorage->m_nNextExtent
        && m_nMinExtents    == pDefStorage->m_nMinExtents
        && m_nMaxExtents    == pDefStorage->m_nMaxExtents
        && m_nPctIncrease   == pDefStorage->m_nPctIncrease)
            return true;

        return false;
    }

    void Storage::WriteTablespace (TextOutput& out) const
    {
        out.PutIndent();
        out.Put("TABLESPACE ");
        out.SafeWriteDBName(m_strTablespaceName.c_str());
        out.NewLine();
    }

    void Storage::WriteParam (TextOutput& out, const Storage* pDefStorage, 
                              const WriteSettings& settings) const
    {
        if (settings.m_nStorageClause) 
        {
            char szBuff[64];
            bool wholly = (settings.m_nStorageClause == 2) ? true : false;
            
            if (wholly || !pDefStorage || m_nInitialExtent != pDefStorage->m_nInitialExtent)
            { 
                sprintf(szBuff, "INITIAL %7d K", m_nInitialExtent/1024); 
                out.PutLine(szBuff); 
            }
            
            if (wholly || !pDefStorage || m_nNextExtent != pDefStorage->m_nNextExtent)
            { 
                sprintf(szBuff, "NEXT    %7d K", m_nNextExtent/1024);    
                out.PutLine(szBuff); 
            }
            
            if (wholly || !pDefStorage || m_nMinExtents != pDefStorage->m_nMinExtents)
            { 
                sprintf(szBuff, "MINEXTENTS  %3d", m_nMinExtents);       
                out.PutLine(szBuff); 
            }

            if (wholly || !pDefStorage || m_nMaxExtents != pDefStorage->m_nMaxExtents) 
            { 
                if (m_nMaxExtents < cnUnlimitedVal) 
                {
                    sprintf(szBuff, "MAXEXTENTS  %3d", m_nMaxExtents);       
                    out.PutLine(szBuff); 
                } 
                else
                    out.PutLine("MAXEXTENTS UNLIMITED"); 
            }
            
            if (wholly || !pDefStorage || m_nPctIncrease != pDefStorage->m_nPctIncrease)
            { 
                sprintf(szBuff, "PCTINCREASE %3d", m_nPctIncrease);      
                out.PutLine(szBuff); 
            }
        }
    }

    void Storage::WriteOpenStorage (TextOutput& out) const
    {
          out.PutLine("STORAGE (");
          out.MoveIndent(2);
    }

    void Storage::WriteCloseStorage (TextOutput& out) const
    {
          out.MoveIndent(-2);
          out.PutLine(")");
    }
    
    /// StorageExt ////////////////////////////////////////////////////////////////

    StorageExt::StorageExt (/**Dictionary& dictionary*/)
    /**: Storage(dictionary) */
    {
        m_nFreeLists      =
        m_nFreeListGroups =
        m_nPctFree        =
        m_nPctUsed        =
        m_nIniTrans       =
        m_nMaxTrans       = 0;
        m_bLogging = false;

    }

    void StorageExt::Write (TextOutput& out, const Dictionary& dictionary,
                            const char* user, const WriteSettings& settings, 
                            bool skipPctUsed, bool useFreeList, bool useLogging) const
    {
        if (settings.m_nStorageClause) 
        {
            char buff[64];

            bool isDefaultTablespace = false;
            bool wholly = (settings.m_nStorageClause == 2) ? true : false;

            try 
            {
                if (m_strTablespaceName == dictionary.LookupUser(user).m_strDefTablespace)
                    isDefaultTablespace = true;
            } 
            catch (const XNotFound&) {}

            if (wholly || !isDefaultTablespace
            || settings.m_bAlwaysPutTablespace)
                WriteTablespace(out);

            if (wholly || !IsPctFreeDefault())
            { 
                sprintf(buff, "PCTFREE  %3d", m_nPctFree); 
                out.PutLine(buff); 
            }
            if (!skipPctUsed && (wholly || !IsPctUsedDefault()))
            { 
                sprintf(buff, "PCTUSED  %3d", m_nPctUsed); 
                out.PutLine(buff); 
            }
            if (wholly || !IsIniTransDefault())
            { 
                sprintf(buff, "INITRANS %3d", m_nIniTrans); 
                out.PutLine(buff); 
            }
            if (wholly || !IsMaxTransDefault())
            { 
                sprintf(buff, "MAXTRANS %3d", m_nMaxTrans); 
                out.PutLine(buff); 
            }

            const Tablespace* pTablespace = 0;
            try 
            {
                pTablespace = &dictionary.LookupTablespace(m_strTablespaceName.c_str());
            } 
            catch (const XNotFound&) {}

            if (wholly 
            || (useFreeList && !IsFreeListsDefault())
            || !IsStorageDefault(pTablespace)) 
            {
                WriteOpenStorage(out);

                if (!IsStorageDefault(pTablespace))
                    WriteParam(out, pTablespace, settings);
                
                /**switch (type[0])
                {
                case 'T': if (!strcmp(type, "TABLE"))     useFreeList = true; break;
                case 'I': if (!strcmp(type, "INDEX"))     useFreeList = true; break;
                case 'C': if (!strcmp(type, "CLUSTER"))   useFreeList = true; break;
                case 'P': if (!strcmp(type, "PARTITION")) useFreeList = true; break;
                }*/

                if (useFreeList)
                {
                    if (wholly || !IsFreeListsDefault())
                    { 
                        sprintf(buff, "FREELISTS   %3d", m_nFreeLists); 
                        out.PutLine(buff); 
                    }
                    if (wholly || !IsFreeListGroupsDefault())
                    { 
                        sprintf(buff, "FREELIST GROUPS %d", m_nFreeListGroups); 
                        out.PutLine(buff); 
                    }
                }
                WriteCloseStorage(out);
            }

            if (useLogging && (wholly || !m_bLogging))
            {
                if (m_bLogging)
                    out.PutLine("LOGGING");
                else
                    out.PutLine("NOLOGGING");
            }
        }
        else 
            if (settings.m_bAlwaysPutTablespace)
                WriteTablespace(out);
    }

    /// Column //////////////////////////////////////////////////////////////

    /// helper class ///
    Column::SubtypeMap::SubtypeMap ()
    {
        insert(value_type("CHAR",      1));
        insert(value_type("NCHAR",     1));
        insert(value_type("RAW",       1));
        insert(value_type("VARCHAR2",  1));
        insert(value_type("NVARCHAR2", 1));
        insert(value_type("NUMBER",    2));
        insert(value_type("FLOAT",     2));
        insert(value_type("LONG",      0));
        insert(value_type("DATE",      0));
        insert(value_type("LONG RAW",  0));
        insert(value_type("ROWID",     0));
        insert(value_type("MLSLABEL",  0));
        insert(value_type("CLOB",      0));
        insert(value_type("NCLOB",     0));
        insert(value_type("BLOB",      0));
        insert(value_type("BFILE",     0));
    }

    Column::SubtypeMap Column::m_mapSubtypes;

    Column::Column ()
    {
        m_nDataPrecision =
        m_nDataScale     =
        m_nDataLength    = 0;
        m_bNullable      = false;
    }

    void Column::GetSpecString (string& strBuff) const
    {
      char szBuff[20];
      strBuff = m_strDataType;
      switch (m_mapSubtypes[m_strDataType]) 
      {
      case 0: 
        break;
      case 1: 
        strBuff += '('; 
        strBuff += itoa(m_nDataLength, szBuff, 10); 
        strBuff += ')';
        break;
      case 2:
        // bug fix: using number intead of integer 
        if (m_nDataPrecision != -1 && m_nDataScale != -1) 
        {
          strBuff += '('; 
          strBuff += itoa(m_nDataPrecision, szBuff, 10); 
          strBuff += ',';
          strBuff += itoa(m_nDataScale, szBuff, 10); 
          strBuff += ')';
        } 
        else if (m_nDataPrecision != -1) 
        {
          strBuff += '('; 
          strBuff += itoa(m_nDataPrecision, szBuff, 10); 
          strBuff += ')';
        }
        else if (m_nDataScale != -1) 
        {
          strBuff = "INTEGER"; 
        }
        break;
      }
    }

    /// Index ////////////////////////////////////////////////////////////////
    Index::Index (Dictionary& dictionary)
    : DbObject(dictionary) 
    { 
        m_Type = eitNormal;
        m_bUniqueness = false; 
        m_bTemporary = false; 
        m_bGenerated = false;
        m_bReverse = false;
        m_bCompression = false;
        m_nCompressionPrefixLength = 0;
    };

    int Index::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            out.Put(DBMS_MetaDataGetDDL("INDEX"));

            return 1;
        }

        out.PutIndent();
        out.Put("CREATE ");

        if ((m_Type == eitBitmap) || (m_Type == eitBitmapFunctionBased))
            out.Put("BITMAP ");
        else
            if (m_Type != eitCluster && m_bUniqueness) 
                out.Put("UNIQUE ");

        out.Put("INDEX ");
        out.PutOwnerAndName(m_strOwner, m_strName, 
                            settings.m_bShemaName || m_strOwner != m_strTableOwner);
        out.NewLine();

        out.MoveIndent(2);
        out.PutIndent();
        out.Put("ON ");

        if (m_Type != eitCluster)
        {
            out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_bShemaName);
            out.Put(" (");
            out.NewLine();
            out.WriteColumns(m_Columns, 2, (m_Type != eitFunctionBased) && (m_Type != eitBitmapFunctionBased));
            out.PutLine(")");
        }
        else
        {
            out.Put("CLUSTER ");
            out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_bShemaName);
            out.NewLine();
        }

        // 03.08.2003 improvement, domain index support
        if (m_Type == eitDomain)
        {
            out.PutIndent();
            out.Put("INDEXTYPE IS ");
            out.PutOwnerAndName(m_strDomainOwner, m_strDomain, settings.m_bShemaName || m_strOwner != m_strDomainOwner);
            out.NewLine();

            if (!m_strDomainParams.empty())
            {
                out.MoveIndent(2);
                out.PutIndent();
                out.Put("PARAMETERS('");
                out.Put(m_strDomainParams);
                out.Put("')");
                out.NewLine();
                out.MoveIndent(-2);
            }
        }
        else
            WriteIndexParams(out, settings);

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();

        return 0;
    }
    
    void Index::WriteIndexParams (TextOutput& out, const WriteSettings& settings) const
    {
        if (m_bReverse)
            out.PutLine("REVERSE");
  
        if (!m_bTemporary)
            StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, true, true, true);

        if (m_bCompression)
        {
            out.PutIndent();
            out.Put("COMPRESS");// B#1191426 - Invalid COMPRESS index generation.

            if (!(m_bUniqueness && m_nCompressionPrefixLength == static_cast<int>(m_Columns.size()) - 1)
            && !(!m_bUniqueness && m_nCompressionPrefixLength == static_cast<int>(m_Columns.size())))
            {
                char buff[64];
                sprintf(buff, " %d", m_nCompressionPrefixLength); 
                out.Put(buff);
            }

            out.NewLine();
        }
    }

    /// Constraint ////////////////////////////////////////////////////////////////

    Constraint::Constraint (Dictionary& dictionary) 
        : DbObject(dictionary) 
    { 
        m_bDeferrable = false; 
        m_bDeferred   = false; 
        m_bGenerated  = false; 
    };

    bool Constraint::IsNotNullCheck () const
    {
        if (m_strType[0] == 'C'
        && m_Columns.size() == 1) 
        {
            string strNotNullCheck;
            strNotNullCheck = m_Columns.begin()->second;
            strNotNullCheck += " IS NOT NULL";

            if (m_strSearchCondition == strNotNullCheck)
                return true;

            strNotNullCheck = "\"";
            strNotNullCheck += m_Columns.begin()->second;
            strNotNullCheck += "\" IS NOT NULL";

            if (m_strSearchCondition == strNotNullCheck)
                return true;
        }
        return false;
    }

    int Constraint::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            if (! settings.m_bNoStorageForConstraint)
                SetDBMS_MetaDataOption("STORAGE", true);
            else
                SetDBMS_MetaDataOption("STORAGE", false);

            if (m_strType[0] == 'R')
                out.Put(DBMS_MetaDataGetDDL("REF_CONSTRAINT"));
            else
                out.Put(DBMS_MetaDataGetDDL("CONSTRAINT"));

            return 1;
        }

        out.PutIndent();
        out.Put("ALTER TABLE ");
        out.PutOwnerAndName(m_strOwner, m_strTableName, settings.m_bShemaName);
        out.NewLine();

        out.MoveIndent(2);
        out.PutIndent();
        out.Put("ADD ");
        if (!IsGenerated()) 
        {
            out.Put("CONSTRAINT ");
            out.SafeWriteDBName(m_strName);
            out.Put(" ");
        }
        switch (m_strType[0]) 
        {
        case 'C': out.Put("CHECK (");       break;
        case 'P': out.Put("PRIMARY KEY ("); break;
        case 'U': out.Put("UNIQUE (");      break;
        case 'R': out.Put("FOREIGN KEY ("); break;
        }
        out.NewLine();
        if (m_strType[0] == 'C') 
        {
            out.MoveIndent(2);
            out.PutLine(m_strSearchCondition);
            out.MoveIndent(-2);
        } else 
        {
            out.WriteColumns(m_Columns, 2);
        }

        if (m_strType[0] == 'R') 
        {
            Constraint& refKey = 
                m_Dictionary.LookupConstraint(m_strROwner.c_str(), 
                                              m_strRConstraintName.c_str());
            out.PutIndent(); 
            out.Put(") REFERENCES ");
            out.PutOwnerAndName(refKey.m_strOwner, refKey.m_strTableName, 
                                settings.m_bShemaName || m_strOwner != refKey.m_strOwner);
            out.Put(" (");
            out.NewLine();
            out.WriteColumns(refKey.m_Columns, 2);
            out.PutIndent(); 
            out.Put(")");
            if (m_strDeleteRule == "CASCADE")
                out.Put(" ON DELETE CASCADE");
            out.NewLine();
        } 
        else
        {
            out.PutLine(")");
        }

        if (m_strStatus == "DISABLED")
            out.PutLine("DISABLE");

        if (m_bDeferrable)
        {
            out.PutIndent(); 
            
            if (m_bDeferred)
                out.Put("INITIALLY DEFERRED ");

            out.Put("DEFERRABLE");
            out.NewLine();
        }


        if (!settings.m_bNoStorageForConstraint
        && (m_strType[0] == 'P' || m_strType[0] == 'U')) 
        {
            try 
            {
                Index& index = m_Dictionary.LookupIndex(m_strOwner.c_str(), m_strName.c_str());
/*
                DWORD dwBefore = out.GetPosition();

                out.PutLine("USING INDEX");
                DWORD dwBegin = out.GetPosition();

                out.MoveIndent(2);
                index.StorageExt::Write(out, m_strOwner.c_str(), true, settings);
                out.MoveIndent(-2);

                if (dwBegin == out.GetPosition()) 
                {
                    out.SetPosition(dwBefore);
                    out.SetLength(dwBefore);
                }
*/
                if (!index.m_bTemporary)
                {
                    TextOutputInMEM out2(true, 2*1024);
                    out2.SetLike(out);

                    out2.MoveIndent(2);
                    index.WriteIndexParams(out2, settings);
                    out2.MoveIndent(-2);

                    if (out2.GetLength() > 0)
                    {
                        out.PutLine("USING INDEX");
                        out.Put(out2.GetData(), out2.GetLength());
                    }
                }
            } 
            catch (const XNotFound&) {}
        }

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();

        return 0;
    }
    
    void Constraint::WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings&) const
    {
        _CHECK_AND_THROW_(m_strType[0] == 'P', "IOT error:\nit is not primary key!");

        out.PutIndent();

        if (!IsGenerated()) 
        {
            out.Put("CONSTRAINT ");
            out.SafeWriteDBName(m_strName);
            out.Put(" ");
        }
        
        out.Put("PRIMARY KEY (");
        out.NewLine();
        out.WriteColumns(m_Columns, 2);
        out.PutLine(")");
    }

    /// TableBase is ancestor of TABLE, SNAPSHOT & SNAPSHOT LOG /////////////

    TableBase::TableBase (Dictionary& dictionary) 
    : DbObject(dictionary)
    {
        m_bCache = false;
    }
    
    void TableBase::WriteIndexes (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            out.Put(DBMS_MetaDataGetDependentDDL("INDEX"));

            return;
        }

        set<string>::const_iterator it(m_setIndexes.begin()),
                                    end(m_setIndexes.end());
        for (it; it != end; it++) 
        {
            Index& index = m_Dictionary.LookupIndex(it->c_str());
            
            try 
            {
                m_Dictionary.LookupConstraint(it->c_str());
                    
                if (settings.m_bNoStorageForConstraint)    // all indexes for constraints
                    index.Write(out, settings);
            } 
            catch (const XNotFound&) 
            {
                index.Write(out, settings);                // or index without constraints
            }
        }
    }

        inline int least_constaint (const Constraint* c1, const Constraint* c2)
        {
            if (c1->m_strType[0] != c2->m_strType[0])
                return (c1->m_strType[0] < c2->m_strType[0]);

            bool first_is_null = c1->IsGenerated();
            bool second_is_null = c2->IsGenerated();

            if (!first_is_null || !second_is_null)
                return (first_is_null ? string() : c1->m_strName) 
                     < (second_is_null ? string() : c2->m_strName);

            if (c1->m_strType[0] == 'C')
                return c1->m_strSearchCondition < c2->m_strSearchCondition;

            return c1->m_Columns < c2->m_Columns;
        }

        typedef vector<const Constraint*> ConstraintVector;
        typedef ConstraintVector::const_iterator ConstraintVectorIter;

    void TableBase::WriteConstraints (TextOutput& out, const WriteSettings& settings, char chType) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            if (! settings.m_bNoStorageForConstraint)
                SetDBMS_MetaDataOption("STORAGE", true);
            else
                SetDBMS_MetaDataOption("STORAGE", false);

            if (chType == 'A' || chType == 'E')
                out.Put(DBMS_MetaDataGetDependentDDL("CONSTRAINT"));

            if (chType == 'A' || chType == 'R')
                out.Put(DBMS_MetaDataGetDependentDDL("REF_CONSTRAINT"));

            return;
        }

        set<string>::const_iterator it(m_setConstraints.begin()),
                                    end(m_setConstraints.end());
        ConstraintVector vector;

        for (it; it != end; it++) 
        {
            Constraint& constraint = m_Dictionary.LookupConstraint(m_strOwner.c_str(), 
                                                                   it->c_str());
            if (chType == constraint.m_strType[0]
            && !constraint.IsNotNullCheck())
              vector.push_back(&constraint);
        }
        stable_sort(vector.begin(), vector.end(), least_constaint);

        ConstraintVectorIter vecIt(vector.begin()), vecEndIt(vector.end());

        for (; vecIt != vecEndIt; vecIt++) 
            if (*vecIt)
                (*vecIt)->Write(out, settings);
    }

    void TableBase::WriteTriggers (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDependentDDL("TRIGGER"));

            return;
        }

        set<string>::const_iterator it(m_setTriggers.begin()),
                                    end(m_setTriggers.end());
        for (it; it != end; it++)
            m_Dictionary.LookupTrigger(it->c_str()).Write(out, settings);
    }

    /// Table ////////////////////////////////////////////////////////////////

    Table::Table (Dictionary& dictionary) 
    : TableBase(dictionary)
    {
        m_TableType = ettNormal;
        m_TemporaryTableDuration = etttdUnknown;
        
        m_IOTOverflow_PCTThreshold  = 0;
        m_IOTOverflow_includeColumn = 0;
    }

    void Table::WriteDefinition (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            SetDBMS_MetaDataOption("CONSTRAINTS", false);
            SetDBMS_MetaDataOption("REF_CONSTRAINTS", false);

            out.Put(DBMS_MetaDataGetDDL("TABLE"));

            return;
        }

        out.PutIndent();
        out.Put("CREATE");
        if (settings.m_bSQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        if (m_TableType == ettTemporary)
            out.Put("GLOBAL TEMPORARY ");

        out.Put("TABLE ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
        out.Put(" (");
        if (!settings.m_bSQLPlusCompatibility
        && settings.m_bCommentsAfterColumn
        && !m_strComments.empty()) 
        {
            out.Put(" -- ");
            out.Put(m_strComments);
        }
        out.NewLine();

        out.MoveIndent(2);

        int 
            nMaxNameLen(0),     // max column name length
            nMaxTypeLen(0),     // max type-specification length
            nMaxOtherLen(0);    // max default-specification and null-specification length

        list<string> listTypeSpecs;
        int nSize = m_Columns.size();

        ColumnContainer::const_iterator
            it(m_Columns.begin()), end(m_Columns.end());

        for (int i(0); it != end; it++, i++) 
        {
            const TabColumn& column = *it->second.get();

            string strType;
            column.GetSpecString(strType);
            listTypeSpecs.push_back(strType);

            nMaxNameLen = max(nMaxNameLen, static_cast<int>(column.m_strColumnName.size()));
            nMaxTypeLen = max(nMaxTypeLen, static_cast<int>(strType.size()));

            size_t nLength = 0;
            if (!column.m_strDataDefault.empty())
                nLength += sizeof (" DEFAULT ") - 1
                        + column.m_strDataDefault.size();

// changed through oracle default clause error
            //if (!column.m_bNullable)
            //    nLength += sizeof (" NOT NULL") - 1;
            nLength += ((column.m_bNullable) ? sizeof (" NULL") : sizeof (" NOT NULL")) - 1;
// changed through oracle default clause error

            if (i < (nSize - 1)) 
                nLength++;      // add place for ',' 

            nMaxOtherLen = max(nMaxOtherLen, static_cast<int>(nLength));
        }

        list<string>::const_iterator 
            itTypes(listTypeSpecs.begin());

        it = m_Columns.begin();

        for (i = 0; it != end; it++, itTypes++, i++) 
        {
            const TabColumn& column = *it->second.get();

            out.PutIndent();
            int nPos = out.GetPosition();
            out.SafeWriteDBName(column.m_strColumnName);
            
            int j = column.m_strColumnName.size();
            do out.Put(" "); while (++j < nMaxNameLen + 1);

            out.Put(*itTypes);
            j = (*itTypes).size();
            if (!column.m_strDataDefault.empty()) 
            {
                while (j++ < nMaxTypeLen) out.Put(" "); 
                out.Put(" DEFAULT ");
                out.Put(column.m_strDataDefault);
            }

// changed through oracle default clause error
            //if (!column.m_bNullable) {
            //    while (j++ < nMaxTypeLen) out.Put(" "); 
            //    out.Put(" NOT NULL");
            //}
            while (j++ < nMaxTypeLen) out.Put(" "); 
            out.Put(column.m_bNullable ? " NULL" : " NOT NULL");
// changed through oracle default clause error

            if (i < (nSize - 1) || m_TableType == ettIOT) 
                out.Put(",");

            if (settings.m_bCommentsAfterColumn
            && !column.m_strComments.empty()) 
            {
                int j = out.GetPosition() - nPos;
                int nMaxLen = nMaxNameLen + nMaxTypeLen + nMaxOtherLen + 2;
                nMaxLen = max(nMaxLen, settings.m_nCommentsPos);

                while (j++ < nMaxLen)
                    out.Put(" ");

                out.Put("-- ");
                out.Put(column.m_strComments);
            }

            out.NewLine();
        }

        if (m_TableType == ettIOT)
            WritePrimaryKeyForIOT(out, settings);

        out.MoveIndent(-2);
        out.PutLine(")");

        out.MoveIndent(2);

        if (m_TableType == ettTemporary)
        {
            out.PutIndent();

            if (m_TemporaryTableDuration == ettdTransaction)
                out.Put("ON COMMIT DELETE ROWS");
            else if (m_TemporaryTableDuration == ettdSession)
                out.Put("ON COMMIT PRESERVE ROWS");
            else
                _CHECK_AND_THROW_(0, "Temporary table error:\ninvalid duration!");

            out.NewLine();
        }
        else if (m_TableType == ettIOT)
        {
            out.PutIndent();
            out.Put("ORGANIZATION INDEX");

            if (m_IOTOverflow_includeColumn > 0)
            {

                out.Put(" INCLUDING ");
                ColumnContainer::const_iterator 
                    it = m_Columns.find(m_IOTOverflow_includeColumn - 1);

                _CHECK_AND_THROW_(it != m_Columns.end(), "IO table error:\ninvalide include column!");

                out.SafeWriteDBName(it->second->m_strColumnName);

                out.NewLine();
                
                const Constraint* pk = FindPrimaryKey();
                if (pk)
                {
                    const Index& index = m_Dictionary.LookupIndex(pk->m_strOwner.c_str(), pk->m_strName.c_str());
                    out.MoveIndent(2);
                    index.StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, true, true, true);
                    out.MoveIndent(-2);
                }

                out.PutLine("OVERFLOW ");
                out.MoveIndent(2);
                m_IOTOverflow_Storage.Write(out, m_Dictionary, m_strOwner.c_str(), settings, false, true, true);
                out.MoveIndent(-2);
            }
                
            out.NewLine();
        }
        else if (m_strClusterName.empty())
        {
            StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, false, true, true);
        }
        else 
        {
            out.PutIndent();
            out.Put("CLUSTER ");
            out.SafeWriteDBName(m_strClusterName);
            out.Put(" (");
            out.NewLine();

            out.WriteColumns(m_clusterColumns, 2);

            out.PutLine(")");
        }

        if (m_bCache) out.PutLine("CACHE");

        out.MoveIndent(-2);

        out.PutLine("/");
        out.NewLine();
    }

    void Table::WriteComments (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDependentDDL("COMMENT"));

            return;
        }

        if (!m_strComments.empty()) 
        {
            out.PutIndent();
            out.Put("COMMENT ON TABLE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            out.Put(" IS ");
            out.WriteSQLString(m_strComments);
            out.Put(settings.m_EndOfShortStatement);
            out.NewLine();
            out.NewLine();
        }

        ColumnContainer::const_iterator
            it(m_Columns.begin()), end(m_Columns.end());

        for (; it != end; it++) 
        {
            const TabColumn& column = *it->second.get();
            if (!column.m_strComments.empty()) 
            {
                out.PutIndent();
                out.Put("COMMENT ON COLUMN ");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                out.Put(".");
                out.SafeWriteDBName(column.m_strColumnName);
                out.Put(" IS ");
                out.WriteSQLString(column.m_strComments);
                out.Put(settings.m_EndOfShortStatement);
                out.NewLine();
            }
        }
        if (m_Columns.size())
            out.NewLine();
    }

    int Table::Write (TextOutput& out, const WriteSettings& settings) const
    {
        //if (!m_strClusterName.empty())
        //    throw BasicException("Table write error:\nCluster tables not supported!");

        if (settings.m_bTableDefinition) 
            WriteDefinition(out, settings);
  
        if (settings.m_bIndexes) 
            WriteIndexes(out, settings);

        if (settings.m_bConstraints) 
        {
            if (UseDbms_MetaData())
            {
                WriteConstraints(out, settings, 'A');
            }
            else
            {
                WriteConstraints(out, settings, 'C');
                
                if (m_TableType != ettIOT)
                    WriteConstraints(out, settings, 'P');

                WriteConstraints(out, settings, 'U');
                WriteConstraints(out, settings, 'R');
            }
        }

        if (settings.m_bTriggers) 
            WriteTriggers(out, settings);
        
        if (settings.m_bComments) 
            WriteComments(out, settings);
/*
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return 0;
    }

    const Constraint* Table::FindPrimaryKey () const
    {
        set<string>::const_iterator it(m_setConstraints.begin()),
                                    end(m_setConstraints.end());
        for (it; it != end; it++) 
        {
            Constraint& constraint 
                = m_Dictionary.LookupConstraint(m_strOwner.c_str(), it->c_str());
            
            if (constraint.m_strType[0] == 'P')
            {
                return &constraint;
            }
        }

        return 0;
    }

    void Table::WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings& settings) const
    {
        const Constraint* pk = FindPrimaryKey();
        
        if (pk)
            pk->WritePrimaryKeyForIOT(out, settings);
    }

    /// Trigger ////////////////////////////////////////////////////////////////

        static // TODO: move sub_str into StrHelpers.h
        string sub_str (const string& text, int startLine, int startCol, int length = -1)
        {
            string::const_iterator it = text.begin();
            for (int line = 0, col = 0; it != text.end(); ++it, col++)
            {
                if (line == startLine && col == startCol)
                    return string(it, length != -1 ? it + length : text.end());
                else if (*it == '\n') 
                    { col = -1; line++; }
            }
            return string();
        }
        static // TODO: move sub_str into StrHelpers.h
        string sub_str (const string& text, int startLine, int startCol, int endLine, int endCol)
        {
            string buffer;
            string::const_iterator it = text.begin();
            for (int line = 0, col = 0; !(line == endLine && col == endCol) && it != text.end(); ++it, col++)
            {
                if ((line > startLine) || (line == startLine && col >= startCol))
                    buffer += *it;
                
                if (*it == '\n') 
                    { col = -1; line++; }
            }
            return buffer;
        }

    // 30.11.2004 bug fix, trigger reverse-engineering fails in OF clause if column name contains "ON_"/"_ON"/"_ON_"
    int Trigger::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDDL("TRIGGER"));

            return 1;
        }

        // TODO: move this class into a separated file
        class TriggerDescriptionAnalyzer : public SyntaxAnalyser
        {
        public:
            TriggerDescriptionAnalyzer () 
                : m_state(READING_TRIGGER_NAME) {}

            virtual void Clear () {}

            virtual void PutToken (const Token& tk)     
            { 
                // 12.04.2005 bug fix #1181324, (ak) trigger reverse-engineering fails if description contains comments (again)
                if (tk == etEOL || tk == etEOF || tk == etCOMMENT) return;

                switch (m_state)
                {
                case READING_TRIGGER_NAME:
                    switch (tk)
                    {
                    case etBEFORE:
                    case etAFTER:
                    case etINSTEAD:
                        m_state = LOOKING_FOR_ON;
                        break;
                    default:
                        m_triggerNameTokens.push_back(tk);
                    }
                    break;
                case LOOKING_FOR_ON:
                    if (tk == etON)
                        m_state = READING_TABLE_NAME;
                    break;
                case READING_TABLE_NAME:
                    switch (m_tableNameTokens.size())
                    {
                    case 0:
                        m_tableNameTokens.push_back(tk);
                        break;
                    case 1:
                        if (tk == etDOT)
                            m_tableNameTokens.push_back(tk); 
                        else
                            m_state = SKIPPING;
                        break;
                    case 2:
                        m_tableNameTokens.push_back(tk); 
                        m_state = SKIPPING;
                        break;
                    }
                    break;
                }
            }

            const std::vector<Token> GetTriggerNameTokens () const  { return m_triggerNameTokens; }
            const std::vector<Token> GetTableNameTokens () const    { return m_tableNameTokens; }

        private:
            enum { READING_TRIGGER_NAME, LOOKING_FOR_ON, READING_TABLE_NAME, SKIPPING } m_state;
            std::vector<Token> m_triggerNameTokens; // it might be from 1 to 3 tokens - "schema"."name"
            std::vector<Token> m_tableNameTokens;   // it might be from 1 to 3 tokens - "schema"."name"
        } 
        analyser;

        TokenMapPtr tokenMap(new TokenMap);
        tokenMap->insert(std::map<string, int>::value_type("'"      , etQUOTE              ));
        tokenMap->insert(std::map<string, int>::value_type("\""     , etDOUBLE_QUOTE       ));      
        tokenMap->insert(std::map<string, int>::value_type("("      , etLEFT_ROUND_BRACKET ));
        tokenMap->insert(std::map<string, int>::value_type(")"      , etRIGHT_ROUND_BRACKET));
        tokenMap->insert(std::map<string, int>::value_type("-"      , etMINUS              ));   
        tokenMap->insert(std::map<string, int>::value_type("/"      , etSLASH              ));   
        tokenMap->insert(std::map<string, int>::value_type("*"      , etSTAR               ));   
        tokenMap->insert(std::map<string, int>::value_type("ON"     , etON                 ));
        tokenMap->insert(std::map<string, int>::value_type("BEFORE" , etBEFORE             ));
        tokenMap->insert(std::map<string, int>::value_type("AFTER"  , etAFTER              ));
        tokenMap->insert(std::map<string, int>::value_type("INSTEAD", etINSTEAD            ));
        tokenMap->insert(std::map<string, int>::value_type("."      , etDOT                ));

        PlsSqlParser parser(&analyser, tokenMap);
        {
            string buffer;
            istringstream in(m_strDescription.c_str());

            for (int line = 0; getline(in, buffer); line++)
                parser.PutLine(line, buffer.c_str(), buffer.length());

            parser.PutEOF(line);
        }

        const std::vector<Token>& triggerNameTokens = analyser.GetTriggerNameTokens();
        _CHECK_AND_THROW_(triggerNameTokens.size() > 0,// && triggerNameTokens.size() <= 3, 
            "Cannot parse trigger description: cannot extract trigger name!");

        const std::vector<Token>& tableNameTokens  = analyser.GetTableNameTokens();
        _CHECK_AND_THROW_(tableNameTokens.size() > 0,// && tableNameTokens.size() <= 3, 
            "Cannot parse trigger description: cannot extract table name!");

        int nHeaderLines = 0;
        out.PutIndent();
        out.Put("CREATE OR REPLACE TRIGGER ");
        if (m_strTableName.empty() || m_strTableName == "")
        {
            if (settings.m_bShemaName || m_strOwner != m_strTableOwner) {
                out.SafeWriteDBName(m_strOwner);
                out.Put(".");
            }
        }
        else
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName || m_strOwner != m_strTableOwner);
        {
            string buffer;
            istringstream in(
                (m_strTableName.empty() || m_strTableName == "") ? 
                m_strDescription + ' ' :
                sub_str(m_strDescription, triggerNameTokens.rbegin()->line, triggerNameTokens.rbegin()->endCol(),
                        tableNameTokens.begin()->line, tableNameTokens.begin()->begCol())
                );

            for (int line = 0; getline(in, buffer); line++) 
            {
                if (line) 
                {
                    out.NewLine();
                    nHeaderLines++;
                }
                out.Put(buffer);
            }
        }
        if (! (m_strTableName.empty() || m_strTableName == ""))
        {
            out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_bShemaName);
            string buffer;
            istringstream in(
                sub_str(m_strDescription, tableNameTokens.rbegin()->line, tableNameTokens.rbegin()->endCol())
                );

            while (getline(in, buffer)) 
            {
                out.PutLine(buffer);
                nHeaderLines++;
            }
        }

        if (!m_strWhenClause.empty()) 
        {
            out.Put("WHEN (");
            out.Put(m_strWhenClause);
            out.PutLine(")");
            nHeaderLines += count_lines(m_strWhenClause.c_str()) + 1;
        }
        
        if (m_strActionType == "CALL") 
        {
            out.Put("CALL ");
        }

        write_text_block(out, m_strTriggerBody.c_str(), m_strTriggerBody.size(),
                         settings.m_bSQLPlusCompatibility, false);

        out.PutLine("/");
        out.NewLine();

        return nHeaderLines;
    }

    /// View ////////////////////////////////////////////////////////////////

    void View::WriteTriggers (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDependentDDL("TRIGGER"));

            return;
        }

        set<string>::const_iterator it(m_setTriggers.begin()),
                                    end(m_setTriggers.end());

        for (it; it != end; it++)
            m_Dictionary.LookupTrigger(it->c_str()).Write(out, settings);
    }

    void View::WriteComments (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDependentDDL("COMMENT"));

            return;
        }

        if (!m_strComments.empty()) 
        {
            out.PutIndent();
            out.Put("COMMENT ON TABLE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            out.Put(" IS ");
            out.WriteSQLString(m_strComments);
            out.Put(settings.m_EndOfShortStatement);
            out.NewLine();
            out.NewLine();
        }

        map<int,string>::const_iterator it(m_Columns.begin()),
                                        end(m_Columns.end());

        for (int i(0); it != end; it++, i++) 
        {
            map<int,string>::const_iterator comIt = m_mapColComments.find(i);

            if (comIt != m_mapColComments.end()) 
            {
                out.PutIndent();
                out.Put("COMMENT ON COLUMN ");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                out.Put(".");
                out.SafeWriteDBName(it->second);
                out.Put(" IS ");
                out.WriteSQLString((*comIt).second);
                out.Put(settings.m_EndOfShortStatement);
                out.NewLine();
            }
        }
        if (m_mapColComments.size())
            out.NewLine();
    }

    int View::Write (TextOutput& out, const WriteSettings& settings) const
    {
        int nHeaderLines = 0;

        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            if (settings.m_bViewWithForce)
                SetDBMS_MetaDataOption("FORCE", true);
            else
                SetDBMS_MetaDataOption("FORCE", false);

            out.Put(DBMS_MetaDataGetDDL("VIEW"));
        }
        else
        {
            if (!settings.m_bViewWithTriggers
            && m_setTriggers.size()) 
            {
                out.PutLine("PROMPT This view has instead triggers");
                nHeaderLines++;
            }
            
            out.PutIndent();
            out.Put("CREATE OR REPLACE");
            if (settings.m_bViewWithForce) out.Put(" FORCE");
            
            if (settings.m_bSQLPlusCompatibility) 
            {
                out.Put(" ");
            }
            else 
            {
                nHeaderLines++;
                out.NewLine();
                out.PutIndent();
            }
            
            out.Put("VIEW ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            out.Put(" (");

            if (settings.m_bCommentsAfterColumn
            && !m_strComments.empty()) 
            {
                out.Put(" -- ");
                out.Put(m_strComments);
            }
            out.NewLine();
            nHeaderLines++;

            int nMaxColLength = 0;
            int nSize = m_Columns.size();

            map<int,string>::const_iterator it(m_Columns.begin()),
                                            end(m_Columns.end());
            for (; it != end; it++)
                nMaxColLength = max(nMaxColLength, static_cast<int>(it->second.size()));

            nMaxColLength += 2;
            nMaxColLength = max(nMaxColLength, settings.m_nCommentsPos);

            out.MoveIndent(2);
            it = m_Columns.begin();
            for (int i = 0; it != end; it++, i++) 
            {
                out.PutIndent();
                int nPos = out.GetPosition();
                out.SafeWriteDBName(it->second);

                if (i < (nSize - 1)) out.Put(",");

                if (settings.m_bCommentsAfterColumn) 
                {
                    map<int,string>::const_iterator comIt = m_mapColComments.find(i);

                    if (comIt != m_mapColComments.end()) 
                    {
                        int j = out.GetPosition() - nPos;
                        while (j++ < nMaxColLength)
                            out.Put(" ");
                        out.Put("-- ");
                        out.Put((*comIt).second);
                    }
                }
                out.NewLine();
                nHeaderLines++;
            }

            out.MoveIndent(-2);
            out.PutLine(") AS");
            nHeaderLines++;

            write_text_block(out, m_strText.c_str(), m_strText.size(),
                             settings.m_bSQLPlusCompatibility, settings.m_bSQLPlusCompatibility);
    /*        
            if (!settings.m_bSQLPlusCompatibility)
                out.PutLine(m_strText.c_str(), skip_back_space(m_strText.c_str(), m_strText.size()));
            else {
                istrstream i(m_strText.c_str(), m_strText.size());
                string buffer;

                while (getline(i, buffer, '\n')) 
                {
                    int len = skip_back_space(buffer.c_str(), buffer.size());
                    if (len) out.PutLine(buffer.c_str(), len);
                }
            }
    */

            out.PutLine("/");
            out.NewLine();
        }

        if (settings.m_bViewWithTriggers) 
            WriteTriggers(out, settings);
        
        if (settings.m_bComments)         
            WriteComments(out, settings);
/*        
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return nHeaderLines;
    }

    /// Sequence ////////////////////////////////////////////////////////////////

    Sequence::Sequence (Dictionary& dictionary) 
    : DbObject(dictionary) 
    {
        m_bCycleFlag =
        m_bOrderFlag = false;
    }

    int Sequence::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDDL("SEQUENCE"));

            return 1;
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_bSQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SEQUENCE ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
        out.NewLine();

        out.MoveIndent(2);

        out.PutIndent();
        out.Put("MINVALUE ");
        out.Put(m_strMinValue);
        out.NewLine();

        out.PutIndent();
        out.Put("MAXVALUE ");
        out.Put(m_strMaxValue);
        out.NewLine();

        out.PutIndent();
        out.Put("INCREMENT BY ");
        out.Put(m_strIncrementBy);
        out.NewLine();

        if (settings.m_bSequnceWithStart) 
        {
            out.PutIndent();
            out.Put("START WITH ");
            out.Put(m_strLastNumber);
            out.NewLine();
        }

        out.PutIndent();
        out.Put(m_bCycleFlag ? "CYCLE" : "NOCYCLE");
        out.NewLine();

        out.PutIndent();
        out.Put(m_bOrderFlag ? "ORDER" : "NOORDER");
        out.NewLine();

        out.PutIndent();
        if (m_strCacheSize.compare("0")) 
        {
            out.Put("CACHE ");
            out.Put(m_strCacheSize);
        } else {
            out.Put("NOCACHE");
        }
        out.NewLine();

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();
/*
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return 0;
    }

    /// PlsCode ////////////////////////////////////////////////////////////////

    int PlsCode::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            string theType = m_strType;
            /*
            if (theType == "PACKAGE" || theType == "TYPE")
            {
                if (((SQLToolsSettings &) settings).m_bSpecWithBody)
                    SetDBMS_MetaDataOption("BODY", true);
                else
                    SetDBMS_MetaDataOption("BODY", false);

                SetDBMS_MetaDataOption("SPECIFICATION", true);
            }

            if (theType == "PACKAGE BODY" || theType == "TYPE BODY")
            {
                if (theType == "PACKAGE BODY")
                    theType = "PACKAGE";

                if (theType == "TYPE BODY")
                    theType = "TYPE";

                if (((SQLToolsSettings &) settings).m_bBodyWithSpec)
                    SetDBMS_MetaDataOption("SPECIFICATION", true);
                else
                    SetDBMS_MetaDataOption("SPECIFICATION", false);

                SetDBMS_MetaDataOption("BODY", true);
            }
            */
            if (theType == "PACKAGE")
                theType = "PACKAGE_SPEC";
            else if (theType == "TYPE")
                theType = "TYPE_SPEC";
            else if (theType == "PACKAGE BODY")
                theType = "PACKAGE_BODY";
            else if (theType == "TYPE BODY")
                theType = "TYPE_BODY";

            out.Put(DBMS_MetaDataGetDDL(theType.c_str()));

            return 1;
        }

        out.PutIndent();
        out.Put("CREATE OR REPLACE");

        if (settings.m_bSQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        int nLen;
        const char* pszText = m_strText.c_str();
        pszText += skip_space_and_comments(pszText);
        get_word_SP(pszText, pszText, nLen);

        if (strnicmp(m_strType.c_str(), pszText, m_strType.size()))
        {
            int nSecond;
            const char* pszSecond;
            get_word_SP(pszText + nLen, pszSecond, nSecond);

            if (!(m_strType == "PACKAGE BODY" || m_strType == "TYPE BODY")
            || !(!strnicmp("PACKAGE", pszText, sizeof("PACKAGE") - 1) 
                 || !strnicmp("TYPE", pszText, sizeof("TYPE") - 1))
            || strnicmp("BODY", pszSecond, sizeof("BODY") - 1))
            {
                _CHECK_AND_THROW_(0, "Loading Error: a body header is invalid!");
            }

            nLen = pszSecond + nSecond - pszText;
            out.Put(pszText, nLen);
            pszText += nLen;
        }
        else
        {
            out.Put(pszText, m_strType.size());
            pszText += m_strType.size();
        }

        out.Put(" ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
/*
        out.Put(" ");
*/
        pszText += skip_db_name(pszText);
/*
        out.PutLine(pszText, skip_back_space(pszText, strlen(pszText)));
*/
        write_text_block(out, pszText, strlen(pszText),
                         settings.m_bSQLPlusCompatibility, false);

        out.PutLine("/");
        out.NewLine();
/*
        if (settings.m_bGrants && stricmp(GetTypeStr(), "PACKAGE BODY"))
            WriteGrants(out, settings);
*/
        return settings.m_bSQLPlusCompatibility ? 0 : 1;
    }

    /// Type ///////////////////////////////////////////////////////////////////

    void Type::WriteAsIncopmlete (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDDL("TYPE"));

            return;
        }

        bool parseError = false;

        const char* pszBuff = m_strText.c_str();
        pszBuff += skip_space_and_comments(pszBuff);
        parseError |= !strnicmp(pszBuff, "TYPE", sizeof("TYPE")-1) ? false : true;
        pszBuff += sizeof("TYPE")-1;
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);
        pszBuff += skip_db_name(pszBuff);
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);
        parseError |= 
            (!strnicmp(pszBuff, "AS", sizeof("AS")-1) || !strnicmp(pszBuff, "IS", sizeof("IS")-1)) ? false : true;
        pszBuff += sizeof("AS")-1;
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);

        _CHECK_AND_THROW_(!parseError, "Type parsing error: invalid header!");

        if (!(!strnicmp(pszBuff, "TABLE", sizeof(pszBuff)-1) && isspace(pszBuff[sizeof("TABLE")-1])))
        {
            out.PutIndent();
            out.Put("CREATE OR REPLACE TYPE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            out.Put(";");
            out.NewLine();
            out.PutLine("/");
        }
    }

    /// Synonym ////////////////////////////////////////////////////////////////

    int Synonym::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDDL("SYNONYM"));

            return 1;
        }
        out.PutIndent();
        
        // 16.11.2003 bug fix, missing public keyword for public synonyms
        if (m_strOwner == "PUBLIC")
        {
            out.Put("CREATE PUBLIC SYNONYM ");
            out.SafeWriteDBName(m_strName);
        }
        else
        {
            out.Put("CREATE SYNONYM ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
        }

        out.Put(" FOR ");
        out.PutOwnerAndName(m_strRefOwner, m_strRefName, settings.m_bShemaName || m_strOwner != m_strRefOwner);
        if (!m_strRefDBLink.empty()) 
        {
            out.Put("@");
            out.SafeWriteDBName(m_strRefDBLink);
        }
        out.Put(settings.m_EndOfShortStatement);
        out.NewLine();

        return 0;
    }

    /// Grant ///////////////////////////////////////////////////////////////

    // 06.04.2003 bug fix, some oracle 9i privilege are not supported
    int Grant::Write (TextOutput& out, const WriteSettings& settings) const
    {
        // Write GRANTs with global privileges. Example:
        // GRANT UPDATE ON table TO user;
        for (int i(0); i < 2; i++) 
        {
            const set<string>& privileges = !i ? m_privileges : m_privilegesWithAdminOption;
            set<string>::const_iterator it(privileges.begin()), end(privileges.end());

            if (it != end) 
            {
                TextOutputInMEM out2(true, 512);
                out2.SetLike(out);

                bool commaInList = false;

                for (; it != end; ++it)
                {
                    if (commaInList) out2.Put(",");
                    commaInList = true;
                    out2.Put(*it);
                }

                if (out2.GetLength() > 0)
                {
                    out.Put("GRANT ");

                    out.Put(out2.GetData(), out2.GetLength());

                    out.Put(" ON ");
                    out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                    out.Put(" TO ");
                    out.SafeWriteDBName(m_strGrantee);

                    if(i) out.Put(" WITH GRANT OPTION");

                    out.Put(settings.m_EndOfShortStatement);
                    out.NewLine();
                }
            }
            
            if (privileges.find("ENQUEUE") != end)
            {
                out.Put("exec Dbms_Aqadm.Grant_Queue_Privilege(\'enqueue', \'");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                
                out.Put("\',\'");
                out.SafeWriteDBName(m_strGrantee);
                out.Put("\',\'");

                if(!i) out.Put("\', FALSE);");
                else   out.Put("\', TRUE);");
            }

            if (privileges.find("DEQUEUE") != end)
            {
                out.Put("exec Dbms_Aqadm.Grant_Queue_Privilege(\'dequeue', \'");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                out.Put("\',\'");
                out.SafeWriteDBName(m_strGrantee);

                if(!i) out.Put("\', FALSE);");
                else   out.Put("\', TRUE);");
                
                out.NewLine();
            }
        }

        // Write GRANTs with column privileges. Example:
        // GRANT UPDATE(colx) ON table TO user;
        for (i = 0; i < 2; i++) 
        {
            const std::map<string,set<string> >& colPrivileges = (!i) ? m_colPrivileges : m_colPrivilegesWithAdminOption;
            std::map<string,set<string> >::const_iterator it(colPrivileges.begin()), end(colPrivileges.end());

            if (it != end) 
            {
                TextOutputInMEM out2(true, 512);
                out2.SetLike(out);

                bool commaInList = false;

                for (; it != end; ++it)
                {
                    if (commaInList) out2.Put(", ");
                    commaInList = true;
                    out2.Put(it->first);
                    out2.Put("("); 

                    bool commaInColList = false;
                    std::set<string>::const_iterator colIt(it->second.begin()); 
                    for (; colIt != it->second.end(); ++colIt)
                    {
                        if (commaInColList) out2.Put(", ");
                        commaInColList = true;
                        out2.SafeWriteDBName(*colIt); 
                    }

                    out2.Put(")");
                }

                if (out2.GetLength() > 0)
                {
                    out.Put("GRANT ");

                    out.Put(out2.GetData(), out2.GetLength());

                    out.Put(" ON ");
                    out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
                    out.Put(" TO ");
                    out.SafeWriteDBName(m_strGrantee);

                    if(i) out.Put(" WITH GRANT OPTION");

                    out.Put(settings.m_EndOfShortStatement);
                    out.NewLine();
                }
            }
        }

        return 0;
    }

    /// GrantContainer //////////////////////////////////////////////////////
    // If it's grantor then the arguments are (grantee_name, object_name)
    // If it's grantee then the arguments are (grantor_name, object_name)

    Grant& GrantContainer::CreateGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];
    
        GrantPtr ptr(new Grant);
    
        if (m_mapGrants.find(make_key(key, grantxx, object)) == m_mapGrants.end())
            m_mapGrants.insert(GrantMap::value_type(key, ptr));
        else
            throw XAlreadyExists(key);

        return *(ptr.get());
    }
    
    
    Grant& GrantContainer::LookupGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];
    
        GrantMapConstIter it = m_mapGrants.find(make_key(key, grantxx, object));
    
        if (it == m_mapGrants.end())
            throw XNotFound(key);

        return *(it->second.get());
    }


    void GrantContainer::DestroyGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];

        if (m_mapGrants.erase(make_key(key, grantxx, object)))
            throw XNotFound(key);
    }


        inline int least (const Grant* g1, const Grant* g2)
        {
            return (g1->m_strGrantee < g2->m_strGrantee)
                   && (g1->m_strOwner < g2->m_strOwner) 
                   && (g1->m_strName < g2->m_strName);
        }

        typedef vector<const Grant*>  GrantVector;
        typedef GrantVector::iterator GrantVectorIter;

    int GrantContainer::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            if (m_mapGrants.begin() != m_mapGrants.end())
                out.Put(DBMS_MetaDataGetGrantedDDL("OBJECT_GRANT", (*m_mapGrants.begin()).second.get()->m_strGrantee.c_str()));

            return 1;
        }

        GrantVector vecObjGrants;

        GrantMapConstIter mapIt(m_mapGrants.begin()), 
                          mapEndIt(m_mapGrants.end());

        for (; mapIt != mapEndIt; mapIt++) 
            vecObjGrants.push_back(((*mapIt).second.get()));

        stable_sort(vecObjGrants.begin(), vecObjGrants.end(), least);

        GrantVectorIter vecIt(vecObjGrants.begin()),
                        vecEndIt(vecObjGrants.end());

        for (; vecIt != vecEndIt; vecIt++) 
            if (*vecIt)
                (*vecIt)->Write(out, settings);

        return 0;
    }
    
    void GrantContainer::EnumGrants (void (*pfnDo)(const Grant&, void*), void* param, bool sort) const
    {
        GrantMapConstIter mapIt(m_mapGrants.begin()), 
                          mapEndIt(m_mapGrants.end());

        if (!sort)
            for (; mapIt != mapEndIt; mapIt++) 
                pfnDo(*(*mapIt).second.get(), param);
        else {
            GrantVector vecObjGrants;

            for (; mapIt != mapEndIt; mapIt++) 
                vecObjGrants.push_back(((*mapIt).second.get()));

            stable_sort(vecObjGrants.begin(), vecObjGrants.end(), least);

            GrantVectorIter vecIt(vecObjGrants.begin()),
                            vecEndIt(vecObjGrants.end());

            for (; vecIt != vecEndIt; vecIt++) 
                if (*vecIt)
                    pfnDo(**vecIt, param);
        }
    }
    
        struct _WriteGrantContext
        {
            const char*          m_name;
            TextOutput&          m_out;
            const WriteSettings& m_settings;

            _WriteGrantContext (const char* name, TextOutput& out, const WriteSettings& settings)
            : m_name(name), m_out(out), m_settings(settings)
            {
            }
        };

        void write_object_grant (const Grant& grant, void* param)
        {
            _WriteGrantContext& context = *reinterpret_cast<_WriteGrantContext*>(param);
            if (grant.m_strName == context.m_name)
                grant.Write(context.m_out, context.m_settings);
        }

    void Grantor::WriteObjectGrants (const char* name, TextOutput& out, const WriteSettings& settings) const
    {
        // use EnumGrants for sorting
        _WriteGrantContext cxt(name, out, settings);
        EnumGrants(write_object_grant, &cxt, true);
        // ! out.NewLine();
    }

    /// Cluster /////////////////////////////////////////////////////////////

    Cluster::Cluster (Dictionary& dictionary) 
    : DbObject(dictionary) 
    {
        m_bIsHash =
        m_bCache  = false;
    }

    int Cluster::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            SetDBMS_MetaDataOption("CONSTRAINTS", false);
            SetDBMS_MetaDataOption("REF_CONSTRAINTS", false);

            out.Put(DBMS_MetaDataGetDDL("CLUSTER"));
        }
        else
        {
            out.PutIndent();
            out.Put("CREATE");

            if (settings.m_bSQLPlusCompatibility) 
            {
                out.Put(" ");
            }
            else 
            {
                out.NewLine();
                out.PutIndent();
            }

            out.Put("CLUSTER ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
            out.Put(" (");
            out.NewLine();

            out.MoveIndent(2);

            ColContainer::const_iterator 
                it(m_Columns.begin()), 
                end(m_Columns.end());

            string type;

            for (int i = m_Columns.size() - 1; it != end; it++, i--) 
            {
                const Column& column = *it->second.get();

                out.PutIndent();
                out.SafeWriteDBName(column.m_strColumnName);
                out.Put(" ");
                column.GetSpecString(type);
                out.Put(type);
                //out.Put(column.m_bNullable ? " NULL" : " NOT NULL");
                if (i) out.Put(",");
                out.NewLine();
            }
            out.MoveIndent(-2);
            out.PutLine(")");
            out.MoveIndent(2);
            
            if (!m_strSize.empty()) 
            {
                out.PutIndent();
                out.Put("SIZE ");
                out.Put(m_strSize);
                out.NewLine();
            }
            
            if (m_bIsHash) 
            {
                out.PutIndent();
                out.Put("HASHKEYS ");
                out.Put(m_strHashKeys);
                out.NewLine();

                if (!m_strHashExpression.empty()) 
                {
                    out.PutIndent();
                    out.Put("HASH IS ");
                    out.Put(m_strHashExpression);
                    out.NewLine();
                }
            }

            StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, false, true, false);
            if (m_bCache) out.PutLine("CACHE");

            out.MoveIndent(-2);
            out.PutLine("/");
            out.NewLine();
        }

        // write indexes
        {
            set<string>::const_iterator it(m_setIndexes.begin()),
                                        end(m_setIndexes.end());
            for (it; it != end; it++)
                m_Dictionary.LookupIndex(it->c_str()).Write(out, settings);
        }

        return UseDbms_MetaData() ? 1 : 0;
    }


    /// DATABASE LINK object ////////////////////////////////////////////////
    
    int DBLink::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            out.Put(DBMS_MetaDataGetDDL("DB_LINK"));

            return 1;
        }

        out.PutIndent();

        // 10.11.2003 bug fix, missing public keyword for public database links
        if (m_strOwner == "PUBLIC")
            out.Put("CREATE PUBLIC DATABASE LINK ");
        else
            out.Put("CREATE DATABASE LINK ");

        out.SafeWriteDBName(m_strName); // don't have owner

        if (!m_strUser.empty() || !m_strHost.empty()) 
        {
            out.NewLine();
            out.MoveIndent(2);
            out.PutIndent();
            if (!m_strUser.empty())
            {
                out.Put("CONNECT TO ");
                out.SafeWriteDBName(m_strUser);
                out.Put(" IDENTIFIED BY ");
                if (!m_strPassword.empty())
                    out.Put( m_strPassword);
                else
                    out.Put("\'NOT ACCESSIBLE\'");
            }
            if (!m_strHost.empty())
            {
                out.Put(" USING \'"); 
                out.Put(m_strHost);    
                out.Put("\'"); 
                out.MoveIndent(-2);
            }
        }
        out.Put(settings.m_EndOfShortStatement);
        out.NewLine();
        out.NewLine();

        return 0;
    }

    /// SNAPSHOT LOG object /////////////////////////////////////////////////

    int SnapshotLog::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            out.Put(DBMS_MetaDataGetDependentDDL("MATERIALIZED_VIEW_LOG"));

            return 1;
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_bSQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SNAPSHOT LOG ON ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);
        out.NewLine();

        out.MoveIndent(2);

        StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, false, false, true);

        out.MoveIndent(-2);

        out.PutLine("/");
        out.NewLine();

        return 0;
    }

    /// SNAPSHOT object /////////////////////////////////////////////////////

    int Snapshot::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (UseDbms_MetaData())
        {
            InitDBMS_MetaData();

            InitDBMS_MetaDataStorage(settings);

            out.Put(DBMS_MetaDataGetDDL("MATERIALIZED_VIEW"));

            return 1;
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_bSQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SNAPSHOT ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_bShemaName);

        if (settings.m_bCommentsAfterColumn
        && !m_strComments.empty()) 
        {
            out.Put(" -- ");
            out.Put(m_strComments);
        }
        out.NewLine();


        out.MoveIndent(2);

        if (m_strClusterName.empty())
            StorageExt::Write(out, m_Dictionary, m_strOwner.c_str(), settings, false, true, true);
        else 
        {
            out.PutIndent();
            out.Put("CLUSTER ");
            out.SafeWriteDBName(m_strClusterName);
            out.Put(" (");
            out.NewLine();

            out.WriteColumns(m_clusterColumns, 2);

            out.PutLine(")");
        }

        out.MoveIndent(-2);

#pragma message("   \"USING INDEX\" is not implemented!") 

        out.PutLine("REFRESH");
        
        out.MoveIndent(2);
        out.PutIndent();
        out.Put(m_strRefreshType);

        if (!m_strStartWith.empty())
        {
            out.Put(" START WITH ");
            out.Put(m_strStartWith);
        }

        if (!m_strNextTime.empty())
        {
            out.Put(" NEXT ");
            out.Put(m_strNextTime);
        }

        out.MoveIndent(-2);


        out.PutLine(" AS ");
        write_text_block(out, m_strQuery.c_str(), m_strQuery.size(),
                         settings.m_bSQLPlusCompatibility, settings.m_bSQLPlusCompatibility);

        out.PutLine("/");
        out.NewLine();

#pragma message("   \"WriteComments\" is not implemented!") 

        return 0;
    }

}// END namespace OraMetaDict
