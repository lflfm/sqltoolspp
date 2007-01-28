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

// 03.03.2004 bug fix, export DDL fails if an object name contains on of \/:*?"<>|

#include "stdafx.h"
#include "Common\StrHelpers.h"
#include "Common\ExceptionHelper.h"
#include "ExtractSchemaHelpers.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CHECK_OPEN_ERROR (bool x)  { _CHECK_AND_THROW_(x, "File open error"); }
void CHECK_OPER_ERROR (bool x)  { _CHECK_AND_THROW_(x, "File operation error"); }
void CHECK_WRITE_ERROR (bool x) { _CHECK_AND_THROW_(x, "File writing error"); }

using namespace OraMetaDict;


/// FileList is a produced files list ///

FileList::FileList (const char* strFileName)
{
    m_pFile = fopen(strFileName, "wt");
    CHECK_OPEN_ERROR(m_pFile ? true : false);
}

FileList::~FileList ()
{
    try { EXCEPTION_FRAME;

        if (m_pFile)
        {
            write();
            CHECK_OPER_ERROR(fclose(m_pFile) != EOF);
        }
    }
    _DESTRUCTOR_HANDLER_;
}

void FileList::Cleanup ()
{
    fclose(m_pFile);
    m_pFile = 0;
}

void FileList::Push (const std::string& str, EStage stage)
{
    m_lists[stage].push_back(str);
}

void FileList::write ()
{
    _ASSERTE(m_pFile);
    
    CHECK_WRITE_ERROR(fputs("PROMPT ====================================================\n",   m_pFile) != EOF);
    CHECK_WRITE_ERROR(fputs("PROMPT      <<< SQLTools extract DDL utility V1.1 >>>      \n", m_pFile) != EOF);
    CHECK_WRITE_ERROR(fputs("PROMPT    ATTENTION: The utility is under construction.    \n",   m_pFile) != EOF);
    CHECK_WRITE_ERROR(fputs("PROMPT      USAGE: Run SQL Plus and input \"start DO.sql\"   \n", m_pFile) != EOF);
    CHECK_WRITE_ERROR(fputs("PROMPT ====================================================\n",   m_pFile) != EOF);
    CHECK_WRITE_ERROR(fputs("\nspool DO.log\n\n",   m_pFile) != EOF);

    std::list<std::string>::const_iterator it, beg, end;

    for (int i(0); i < 2; i++) {
        it = beg = m_lists[i].begin();
        end = m_lists[i].end();

        for (; it != end; it++) {
            CHECK_WRITE_ERROR(fputs("@@", m_pFile) != EOF);
            CHECK_WRITE_ERROR(fputs((*it).c_str(), m_pFile) != EOF);
            CHECK_WRITE_ERROR(fputc('\n', m_pFile) != EOF);
        }
    }

    CHECK_WRITE_ERROR(fputs("\nspool off\n",   m_pFile) != EOF);
}


/// FastFileOpen is a last opened file cash

FILE* FastFileOpen::Open (const std::string& name, const char* mode)
{
    if (m_strFileName != name) {
        Close();
        m_strFileName = name;
        m_pFile = fopen(m_strFileName.c_str(), mode);
        CHECK_OPEN_ERROR(m_pFile ? true : false);
    }
    return m_pFile;
}

void FastFileOpen::Close ()
{
    if (m_pFile)
        CHECK_OPER_ERROR(fclose(m_pFile) != EOF);

    m_strFileName.erase();
    
    m_pFile = 0;
}

void FastFileOpen::Cleanup ()
{
    if (m_pFile)
        fclose(m_pFile);

    m_strFileName.erase();
    
    m_pFile = 0;
}


/// WriteContext is a common data for file writing iterations ///
const std::string WriteContext::strNull;

void WriteContext::Cleanup ()
{
    m_fileList.Cleanup();
    m_fastFileOpen.Cleanup();
}

// don't close a file after use
FILE* WriteContext::OpenFile (const std::string& _name, FileList::EStage stage, const char* mode)
{
    if (!m_bSingleStream) {
        std::string path, name;
        // 03.03.2004 bug fix, export DDL fails if an object name contains on of \/:*?"<>|
        Common::make_safe_filename(_name.c_str(), name);

        if (m_strSubDir.size()) path += m_strSubDir + '\\';
        path += name + m_strSuffix + '.' + m_strDefExt;
        
        std::string fullPath = m_strPath + '\\' + path;

        bool idem = m_fastFileOpen.Idem(fullPath);
        /*FILE* pFile =*/ m_fastFileOpen.Open(fullPath, mode);

        if (!idem)
            m_fileList.Push(path, stage);

    }
    return m_fastFileOpen.GetStream();
}

void WriteContext::BeginSingleStream (const std::string& name, FileList::EStage stage, const char* mode)
{
    _ASSERTE(!m_bSingleStream);
    m_fastFileOpen.Close();

    std::string path;
    if (m_strSubDir.size()) path += m_strSubDir + '\\';
    path += name + m_strSuffix + '.' + m_strDefExt;

    m_fastFileOpen.Open(m_strPath + '\\' + path, mode);
    m_fileList.Push(path, stage);
    m_bSingleStream = true;
}

void WriteContext::EndSingleStream ()
{
    _ASSERTE(m_bSingleStream);
    m_fastFileOpen.Close();
    m_bSingleStream = false;
}

/// dictionary iteration function ///////////////////////////////////////////

void write_object (DbObject& object, void* param)
{
    _ASSERTE(param);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    FILE* pFile = context.OpenFile(object.m_strName);
    TextOutputInFILE out(*pFile, context.GetDDLSettings().m_bLowerNames ? true : false);
    object.Write(out, context.GetDDLSettings());
} 

void write_table_definition (DbObject& object, void* param)
{
    _ASSERTE(param);
    Table& table = static_cast<Table&>(object);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    FILE* pFile = context.OpenFile(object.m_strName);
    TextOutputInFILE out(*pFile, context.GetDDLSettings().m_bLowerNames ? true : false);
    table.WriteDefinition(out, context.GetDDLSettings());
    table.WriteComments(out, context.GetDDLSettings());
} 

void write_table_indexes (DbObject& object, void* param)
{
    _ASSERTE(param);
    Table& table = static_cast<Table&>(object);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    TextOutputInMEM out(context.GetDDLSettings().m_bLowerNames ? true : false, 2*1024);
    table.WriteIndexes(out, context.GetDDLSettings());

    if (out.GetLength() > 0) 
    {
        FILE* pFile = context.OpenFile(object.m_strName + "~INX");
        CHECK_WRITE_ERROR(static_cast<int>(fwrite(out.GetData(), 1, out.GetLength(), pFile)) >= out.GetLength());
    }
} 

void write_table_chk_constraint (DbObject& object, void* param)
{
    _ASSERTE(param);
    Table& table = static_cast<Table&>(object);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    TextOutputInMEM out(context.GetDDLSettings().m_bLowerNames ? true : false, 2*1024);
    table.WriteConstraints(out, context.GetDDLSettings(), 'C');

    if (out.GetLength() > 0) 
    {
        FILE* pFile = context.OpenFile(object.m_strName + "~CHK");
        CHECK_WRITE_ERROR(static_cast<int>(fwrite(out.GetData(), 1, out.GetLength(), pFile)) >= out.GetLength());
    }
} 

void write_table_unq_constraint (DbObject& object, void* param)
{
    _ASSERTE(param);
    Table& table = static_cast<Table&>(object);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    TextOutputInMEM out(context.GetDDLSettings().m_bLowerNames ? true : false, 2*1024);
    
    if (table.m_TableType != ettIOT)
        table.WriteConstraints(out, context.GetDDLSettings(), 'P');

    table.WriteConstraints(out, context.GetDDLSettings(), 'U');

    if (out.GetLength() > 0) 
    {
        FILE* pFile = context.OpenFile(object.m_strName + "~UNQ");
        CHECK_WRITE_ERROR(static_cast<int>(fwrite(out.GetData(), 1, out.GetLength(), pFile)) >= out.GetLength());
    }
} 

void write_table_fk_constraint (DbObject& object, void* param)
{
    _ASSERTE(param);
    Table& table = static_cast<Table&>(object);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    TextOutputInMEM out(context.GetDDLSettings().m_bLowerNames ? true : false, 2*1024);
    table.WriteConstraints(out, context.GetDDLSettings(), 'R');

    if (out.GetLength() > 0) 
    {
        FILE* pFile = context.OpenFile(object.m_strName + "~FK");
        CHECK_WRITE_ERROR(static_cast<int>(fwrite(out.GetData(), 1, out.GetLength(), pFile)) >= out.GetLength());
    }
} 

void write_grant (const Grant& object, void* param)
{
    _ASSERTE(param);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    FILE* pFile = context.OpenFile(object.m_strGrantee);
    TextOutputInFILE out(*pFile, context.GetDDLSettings().m_bLowerNames ? true : false);
    object.Write(out, context.GetDDLSettings());
} 

void write_grantee (DbObject& object, void* param)
{
    _ASSERTE(param);
    static_cast<Grantee&>(object).EnumGrants(write_grant, param);
} 

void write_incopmlete_type (OraMetaDict::DbObject& object, void* param)
{
    _ASSERTE(param);
    WriteContext& context = *reinterpret_cast<WriteContext*>(param);

    FILE* pFile = context.OpenFile(object.m_strName);
    TextOutputInFILE out(*pFile, context.GetDDLSettings().m_bLowerNames ? true : false);
    static_cast<Type&>(object).WriteAsIncopmlete(out, context.GetDDLSettings());
}

/////////////////////////////////////////////////////////////////////////////
