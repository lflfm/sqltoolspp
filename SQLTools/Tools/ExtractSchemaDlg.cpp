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
#include <COMMON\AppGlobal.h>
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "ExtractSchemaDlg.h"
#include "ExtractSchemaHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


std::string CExtractSchemaDlg::GetFullPath (ExtractDDLSettings& settings)
{
    return GetRootFolder(settings) + '\\' + GetSubdirName(settings);
}

std::string CExtractSchemaDlg::GetRootFolder (ExtractDDLSettings& settings)
{
    std::string folder = settings.m_strFolder;
        Common::trim_symmetric(folder);

    if (folder.size() && (*(folder.rbegin()) == '\\'))
        folder.resize(folder.size() - 1);

    if (settings.m_bUseDbAlias 
    && settings.m_nUseDbAliasAs == 2 
    && !settings.m_strDbAlias.empty())
        folder += '\\' + settings.m_strDbAlias;

    return folder;
}

std::string CExtractSchemaDlg::GetSubdirName (ExtractDDLSettings& settings)
{
    std::string name;
    
    if (settings.m_bUseDbAlias && !settings.m_strDbAlias.empty())
    {
        switch (settings.m_nUseDbAliasAs)
        {
        case 0: name = settings.m_strDbAlias + '_' + settings.m_strSchema; break;
        case 1: name = settings.m_strSchema + '_' + settings.m_strDbAlias; break;
        case 2: name = settings.m_strSchema;                               break;
        }
    }
    else
        name = settings.m_strSchema;

    return name;
}

/////////////////////////////////////////////////////////////////////////////
// CExtractSchemaDlg

CExtractSchemaDlg::CExtractSchemaDlg(OciConnect& connect, CWnd* pParentWnd)
: CPropertySheet("Extract Schema DDL", pParentWnd),
  m_connect(connect),
  m_MainPage  (m_DDLSettings, connect),
  m_FilterPage(m_DDLSettings),
  m_OptionPage(m_DDLSettings)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_psh.dwFlags &= ~PSH_HASHELP;

    string path;
    Global::GetSettingsPath(path);
    
    ExtractDDLSettingsReader(OpenEditor::FileInStream((path + "\\schemaddl.dat").c_str())) >> m_DDLSettings;

    m_DDLSettings.m_strDbAlias = connect.GetGlobalName();

    m_MainPage.m_psp.dwFlags &= ~PSP_HASHELP;
    m_FilterPage.m_psp.dwFlags &= ~PSP_HASHELP;
    m_OptionPage.m_psp.dwFlags &= ~PSP_HASHELP;

    AddPage(&m_MainPage);
    AddPage(&m_FilterPage);
    AddPage(&m_OptionPage);
}


BEGIN_MESSAGE_MAP(CExtractSchemaDlg, CPropertySheet)
        ON_COMMAND(IDOK, OnOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtractSchemaDlg message handlers

BOOL CExtractSchemaDlg::OnInitDialog() 
{
    BOOL bResult = CPropertySheet::OnInitDialog();
        
    HWND hItem = ::GetDlgItem(m_hWnd, IDOK); 
    _ASSERTE(hItem);
    ::SetWindowText(hItem, "Load");

    hItem = ::GetDlgItem(m_hWnd, IDCANCEL);
    _ASSERTE(hItem);
    ::SetWindowText(hItem, "Close");
        
    return bResult;
}

using namespace std;
using namespace Common;
using namespace OraMetaDict;

    LPSCSTR szSubDir [] = 
    {
        "Tables", 
        "Triggers", 
        "Views", 
        "Sequences", 
        "Types", 
        "TypeBodies",
        "Functions", 
        "Procedures", 
        "Packages", 
        "PackageBodies",
        "Grants"
    };

inline
void CExtractSchemaDlg::MakeFolders (std::string& path)
{
    string folder = GetRootFolder(m_DDLSettings);

    if (folder.size())
    {
        list<string> listDir;
        // check existent path part
        while (!folder.empty() && *(folder.rbegin()) != ':'
        && GetFileAttributes(folder.c_str()) == 0xFFFFFFFF) {
            listDir.push_front(folder);

            size_t len = folder.find_last_of('\\');
            if (len != std::string::npos)
                folder.resize(len);
            else
                break;
        }

        // create non-existent path part
        list<string>::const_iterator it(listDir.begin()), end(listDir.end());
        for (; it != end; it++)
            _CHECK_AND_THROW_(CreateDirectory((*it).c_str(), NULL), "Can't create folder!");
    }

    path = GetFullPath(m_DDLSettings);

    if (GetFileAttributes(path.c_str()) != 0xFFFFFFFF) {
        string strMessage;
        strMessage = "Folder \"" + path + "\" already exists.\nDo you want to remove it?";

        if (AfxMessageBox(strMessage.c_str(), MB_YESNO) == IDNO)
            AfxThrowUserException();

        m_MainPage.SetStatusText("Removing folders...");
        _CHECK_AND_THROW_(AppDeleteDirectory(path.c_str()), "Can't remove folder!");
    }

    _CHECK_AND_THROW_(CreateDirectory(path.c_str(), NULL), "Can't create folder!");

    for (int i(0); i < sizeof szSubDir / sizeof szSubDir [0]; i++)
        _CHECK_AND_THROW_(CreateDirectory((path + "\\" + szSubDir[i]).c_str(), NULL), "Can't create folder!");
}

inline
void CExtractSchemaDlg::LoadSchema (OraMetaDict::Dictionary& dict, CAbortController& abortCtrl)
{
    Loader loader(m_connect, dict);
    loader.Init();

    if (m_DDLSettings.m_bExtractTables) {
        NextAction(abortCtrl, "Loading tables...");
        loader.LoadTables(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractViews) {
        NextAction(abortCtrl, "Loading views...");
        loader.LoadViews(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractTriggers) {
        NextAction(abortCtrl, "Loading triggers...");
        loader.LoadTriggers(m_DDLSettings.m_strSchema.c_str(), "%", false/*byTables*/, true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractSequences) {
        NextAction(abortCtrl, "Loading sequences...");
        loader.LoadSequences(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractCode) {
        NextAction(abortCtrl, "Loading types,functions,procedures && packages...");
        loader.LoadTypes        (m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadTypeBodies   (m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadFunctions    (m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadProcedures   (m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadPackages     (m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadPackageBodies(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractSynonyms) {
        NextAction(abortCtrl, "Loading Synonyms...");
        loader.LoadSynonyms(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (m_DDLSettings.m_bExtractGrantsByGrantee) {
        NextAction(abortCtrl, "Loading Grants...");
        loader.LoadGrantByGrantors(m_DDLSettings.m_strSchema.c_str(), "%", true/*useLike*/);
    }
}

inline
void CExtractSchemaDlg::OverrideTablespace (OraMetaDict::Dictionary& dict)
{
    if (!m_DDLSettings.m_strTableTablespace.empty())
    {
        m_DDLSettings.m_bAlwaysPutTablespace = TRUE;
        {
            TableMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName = 
                    m_DDLSettings.m_strTableTablespace;
        }
        {
            ClusterMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName = 
                    m_DDLSettings.m_strTableTablespace;
        }
    }
    if (!m_DDLSettings.m_strIndexTablespace.empty())
    {
        m_DDLSettings.m_bAlwaysPutTablespace = TRUE;
        {
            IndexMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName = 
                    m_DDLSettings.m_strIndexTablespace;
        }
    }
}

inline
void CExtractSchemaDlg::WriteSchema (OraMetaDict::Dictionary& dict, CAbortController& abortCtrl, std::string& path)
{
    WriteContext context(path, "DO.sql", m_DDLSettings);

    try 
    {
        NextAction(abortCtrl, "Write data to disk...");

        if (m_DDLSettings.m_bExtractSynonyms) {
            NextAction(abortCtrl, "Writing synonyms...");
            context.SetDefExt("sql");
            context.SetSubDir("");
            context.BeginSingleStream("Synonyms");
            dict.EnumSynonyms(write_object, &context);
            context.EndSingleStream();
        }

        if (m_DDLSettings.m_bExtractCode) {
            NextAction(abortCtrl, "Writing types...");

            context.SetDefExt("sql");
            context.SetSubDir("");
            context.BeginSingleStream("Types");
            dict.EnumTypes(write_incopmlete_type, &context);
            context.EndSingleStream();

            context.SetDefExt("pls");
            context.SetSubDir("Types");
            dict.EnumTypes(write_object, &context);
        }

        if (m_DDLSettings.m_bExtractTables) {
            NextAction(abortCtrl, "Writing tables...");
            context.SetDefExt("sql");
            context.SetSubDir("Tables");
            if (m_DDLSettings.m_bGroupByDDL) {
                dict.EnumTables(write_table_definition,     &context);
                dict.EnumTables(write_table_indexes,        &context);
                //bool bUseDBMS_MetaData = GetSQLToolsSettings().GetUseDbmsMetaData();

                //if (bUseDBMS_MetaData && 
                //    ((CSQLToolsApp*)AfxGetApp())->GetConnect().GetVersion() < OCI8::esvServer9X)
                //    bUseDBMS_MetaData = false;
                if (DbObject::UseDbms_MetaData())
                {
                    dict.EnumTables(write_table_nonref_constraint, &context);
                    dict.EnumTables(write_table_ref_constraint,  &context);
                }
                else
                {
                    dict.EnumTables(write_table_chk_constraint, &context);
                    dict.EnumTables(write_table_unq_constraint, &context);
                    dict.EnumTables(write_table_fk_constraint,  &context);
                }
            } else {
                m_DDLSettings.m_bConstraints = TRUE;
                m_DDLSettings.m_bIndexes     = TRUE;
                dict.EnumTables(write_object, &context);
            }
        }

        if (m_DDLSettings.m_bExtractSequences) {
            NextAction(abortCtrl, "Writing sequences...");
            context.SetDefExt("sql");
            context.SetSubDir("Sequences");
            dict.EnumSequences(write_object, &context);
        }

        if (m_DDLSettings.m_bExtractCode) {

            NextAction(abortCtrl, "Writing functions...");
            context.SetDefExt("plb");
            context.SetSubDir("Functions");
            dict.EnumFunctions(write_object, &context);

            NextAction(abortCtrl, "Writing procedures...");
            context.SetDefExt("plb");
            context.SetSubDir("Procedures");
            dict.EnumProcedures(write_object, &context);

            NextAction(abortCtrl, "Writing packages...");
            context.SetDefExt("pls");
            context.SetSubDir("Packages");
            dict.EnumPackages(write_object, &context);

        }

        if (m_DDLSettings.m_bExtractViews) {
            NextAction(abortCtrl, "Writing views...");
            context.SetDefExt("sql");
            context.SetSubDir("Views");

            if (m_DDLSettings.m_bOptimalViewOrder) {
                vector<const DbObject*> result;
                build_optimal_view_order (dict, m_DDLSettings.m_strSchema.c_str(), m_connect, result);

                vector<const DbObject*>::const_iterator
                    it(result.begin()), end(result.end());

                for (; it != end; it++)
                    write_object((DbObject&)**it, &context);
            } else
                dict.EnumViews(write_object, &context);

        }

        if (m_DDLSettings.m_bExtractCode) {
            NextAction(abortCtrl, "Writing types...");
            context.SetDefExt("plb");
            context.SetSubDir("TypeBodies");
            dict.EnumTypeBodies(write_object, &context);
        }

        if (m_DDLSettings.m_bExtractCode) {
            NextAction(abortCtrl, "Writing packages...");
            context.SetDefExt("plb");
            context.SetSubDir("PackageBodies");
            dict.EnumPackageBodies(write_object, &context);
        }

        if (m_DDLSettings.m_bExtractTriggers) {
            NextAction(abortCtrl, "Writing triggers...");
            context.SetDefExt("trg");
            context.SetSubDir("Triggers");
            dict.EnumTriggers(write_object, &context);
        }

        if (m_DDLSettings.m_bExtractGrantsByGrantee) {
            NextAction(abortCtrl, "Writing grants...");
            context.SetDefExt("sql");
            context.SetSubDir("Grants");
            dict.EnumGrantors(write_grantee, &context);
        }
    } 
    catch (...)
    {
        context.Cleanup();
        throw;
    }
}

void CExtractSchemaDlg::NextAction (CAbortController& abortCtrl, const char* text)
{
    m_connect.CHECK_INTERRUPT();
    m_MainPage.SetStatusText(text);
    abortCtrl.SetActionText(text);
}

    static bool g_isBackupDone = true;

void CExtractSchemaDlg::OnOK()
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        if (m_MainPage.m_hWnd)   m_MainPage.UpdateDataAndSaveHistory();
        if (m_FilterPage.m_hWnd) m_FilterPage.UpdateData();
        if (m_OptionPage.m_hWnd) m_OptionPage.UpdateData();

        string path;
        Global::GetSettingsPath(path);

        if (!g_isBackupDone)
        {
            _CHECK_AND_THROW_(
                    CopyFile((path + "\\schemaddl.dat").c_str(), 
                        (path + "\\schemaddl.dat.old").c_str(), FALSE) != 0,
                    "Settings backup file creation error!"
                );
            g_isBackupDone = true;
        }

        ExtractDDLSettingsWriter(OpenEditor::FileOutStream((path + "\\schemaddl.dat").c_str())) << m_DDLSettings;

        if (GetActiveIndex())
            SetActivePage(0);

        MakeFolders(path);

        CAbortController abortCtrl(*GetAbortThread(), &m_connect);

        Dictionary dict;
        LoadSchema(dict, abortCtrl);
        OverrideTablespace(dict);
        WriteSchema(dict, abortCtrl, path);
 
        NextAction(abortCtrl, "Process completed.");
    } 
    catch (const OciUserCancel& x)
    {
        AfxMessageBox(x.what(), MB_OK|MB_ICONEXCLAMATION);
        m_MainPage.SetStatusText("User terminated loading.");
    }
    catch (const OciException& x)
    {
        DEFAULT_OCI_HANDLER(x);
        m_MainPage.SetStatusText("Abnormal termination.");
    }
    catch (const std::exception& x)
    {
        DEFAULT_HANDLER(x);
        m_MainPage.SetStatusText("Abnormal termination.");
    }
}
