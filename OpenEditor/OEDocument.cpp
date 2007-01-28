/* 
    Copyright (C) 2002 Aleksey Kochetov

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
    09.03.2003 bug fix, backup file problem if a destination is the current folder
    14.04.2003 bug fix, SetModifiedFlag clears an undo history
    06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db 
               if "save files when switching tasks" is activated.
    21.01.2004 bug fix, Lock/open/save file error handling chanded to avoid paranoic bug reporting
    10.06.2004 improvement, the program asks to continue w/o locking if it cannot lock a file
    30.06.2004 bug fix, unknown exception on "save as", if a user doesn't have permissions to rewrite the destination 
    17.01.2005 (ak) simplified settings class hierarchy 
    28.03.2005 bug fix, (ak) if file are locked, an unrecoverable error occurs on "Reload"
    30.03.2005 bug fix, a new bug introduced by the previous bug fix
*/

#include "stdafx.h"
#include "Shlwapi.h"
#include "COMMON/AppUtilities.h"
#include "COMMON/AppGlobal.h"
#include <COMMON/ExceptionHelper.h>
#include "COMMON\StrHelpers.h"
#include "COMMON/VisualAttributesPage.h"
#include "COMMON/PropertySheetMem.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/OEPrintPageSetup.h"
#include "OpenEditor/OEEditingPage.h"
#include "OpenEditor/OEBlockPage.h"
#include "OpenEditor/OEClassPage.h"
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OESettingsStreams.h"
#include <OpenEditor/OELanguageStreams.h>
#include "OpenEditor/OETemplateStreams.h"
#include "OpenEditor/OEFileInfoDlg.h"
#include "OpenEditor/OEFileInfoDlg.h"
#include "OpenEditor/OEGeneralPage.h"
#include "OpenEditor/OEFilePage.h"
#include "OpenEditor/OEBackupPage.h"
#include "OpenEditor/OEPropPage.h"
#include "OpenEditor/OETemplatesPage.h"
#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEOverwriteFileDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COEDocument

using namespace std;
using namespace Common;
using namespace OpenEditor;

#define CHECK_FILE_OPERATION(r) if(!(r)) file_exeption::check_last_error();

SettingsManager COEDocument::m_settingsManager;
Searcher        COEDocument::m_searcher(true);
bool            COEDocument::m_saveModified_silent = false;
bool            COEDocument::m_saveModified_skipNew = false;
bool            COEDocument::m_enableOpenUnexisting = false;

IMPLEMENT_DYNCREATE(COEDocument, CDocumentExt)

BEGIN_MESSAGE_MAP(COEDocument, CDocument)
	//{{AFX_MSG_MAP(COEDocument)
	ON_COMMAND(ID_EDIT_PRINT_PAGE_SETUP, OnEditPrintPageSetup)
	ON_COMMAND(ID_EDIT_FILE_SETTINGS, OnEditFileSettings)
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
    ON_UPDATE_COMMAND_UI(ID_FILE_RELOAD, OnUpdate_FileReload)
    ON_UPDATE_COMMAND_UI(ID_FILE_SYNC_LOCATION, OnUpdate_FileReload)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_TYPE,  OnUpdate_FileType)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_LINES, OnUpdate_FileLines)

    ON_COMMAND(ID_EDIT_VIEW_LINE_NUMBERS, OnEditViewLineNumbers)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_LINE_NUMBERS, OnUpdate_EditViewLineNumbers)
	ON_COMMAND(ID_EDIT_VIEW_WHITE_SPACE, OnEditViewWhiteSpace)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_WHITE_SPACE, OnUpdate_EditViewWhiteSpace)
    ON_COMMAND(ID_EDIT_VIEW_RULER, OnEditViewRuler)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_RULER,  OnUpdate_EditViewRuler)
    ON_COMMAND(ID_EDIT_VIEW_COLUMN_MARKERS, OnEditViewColumnMarkers)
	ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_COLUMN_MARKERS,  OnUpdate_EditViewColumnMarkers)
    ON_COMMAND(ID_EDIT_VIEW_INDENT_GUIDE, OnEditViewIndentGuide)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW_INDENT_GUIDE,  OnUpdate_EditViewIndentGuide)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COEDocument construction/destruction

COEDocument::COEDocument()
: m_settings(m_settingsManager)
{
    try { EXCEPTION_FRAME;

        m_vmdata = NULL;
        m_orgModified = false;
        m_newOrSaveAs = false;
        m_extensionInitialized = false;
        m_storage.SetSettings(&m_settings);
        m_storage.SetSearcher(&m_searcher);
    }
    _OE_DEFAULT_HANDLER_;
}


COEDocument::~COEDocument()
{
    CWaitCursor wait;

    try { EXCEPTION_FRAME;

        if (m_vmdata) VirtualFree(m_vmdata, 0, MEM_RELEASE);
        m_storage.Clear();
        m_mmfile.Close();
    }
    _DESTRUCTOR_HANDLER_;

    // for safe delete
    //m_storage.SetSettings(&m_settingsManager.GetDefaults());
}


BOOL COEDocument::IsModified()
{
    return m_storage.IsModified() ? TRUE : FALSE;
}


BOOL COEDocument::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// COEDocument othes methods

void COEDocument::SetPathName (LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	CDocument::SetPathName(lpszPathName, bAddToMRU);

    std::string extension;
    const char* ext = strrchr(lpszPathName, '.');
    
    if (ext) 
        extension = ++ext;

    if (!m_extensionInitialized || extension != m_extension)
    {
        m_extension = extension;
        if (!m_extensionInitialized
        || AfxMessageBox("Filename extension changed. Auto-setup for this extension?", MB_YESNO|MB_ICONQUESTION) == IDYES)
            m_settingsManager.SetClassSettingsByExt(m_extension, m_settings);
        m_extensionInitialized = true;
    }
}


void COEDocument::SetTitle (LPCTSTR lpszTitle)
{
    m_orgTitle = lpszTitle;
    CDocument::SetTitle((m_orgModified ? m_orgTitle + " *" : m_orgTitle).c_str());
}


void COEDocument::OnIdle ()
{
    if (m_orgModified != m_storage.IsModified())
    {
        m_orgModified = m_storage.IsModified();
        CDocument::SetTitle((m_orgModified ? m_orgTitle + " *" : m_orgTitle).c_str());
    }
}

BOOL COEDocument::OnIdle (LONG)
{
    return m_storage.OnIdle() ? TRUE : FALSE;
}

BOOL COEDocument::DoFileSave ()
{
    DWORD dwAttrib = m_strPathName.IsEmpty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributes(m_strPathName);
    
    if (dwAttrib != INVALID_FILE_ATTRIBUTES 
    && dwAttrib & FILE_ATTRIBUTE_READONLY)
    {
        if (m_settings.GetFileOverwriteReadonly())
        {
            SetFileAttributes(m_strPathName, dwAttrib & (~FILE_ATTRIBUTE_READONLY));
        }
        else
        {
            switch (OEOverwriteFileDlg(m_strPathName).DoModal())
            {
            case IDCANCEL:  // cancel saving
                return FALSE;
            case IDNO:      // remove write-protection here
                SetFileAttributes(m_strPathName, dwAttrib & (~FILE_ATTRIBUTE_READONLY));
            case IDYES:     // try "Save As"
                ;
            }
        }
    }

    if (CDocument::DoFileSave())
    {
        if (!m_settings.GetUndoAfterSaving())
            m_storage.ClearUndo();

        return TRUE;
    }

    return FALSE;
}

BOOL COEDocument::DoSave (LPCTSTR lpszPathName, BOOL bReplace)
{
    try
    {
        if (!lpszPathName) m_newOrSaveAs = true;
        BOOL retVal = doSave(lpszPathName, bReplace);
        m_newOrSaveAs = false;
        return retVal;
    }
    catch (...)
    {
        m_newOrSaveAs = false;
        throw;
    }
}


// 28.03.2005 bug fix, (ak) if file are locked, an unrecoverable error occurs on "Reload"
void COEDocument::loadFile (const char* path, bool reload, bool external)
{
    ASSERT_EXCEPTION_FRAME;

    CWaitCursor wait;

    bool locking = m_settings.GetFileLocking();

    // reload if file is locked
    // 30.03.2005 bug fix, a new bug introduced by the previous bug fix
    if (reload && locking && !m_vmdata && m_mmfile.IsOpen()) 
    {
        unsigned long length = m_mmfile.GetDataLength();
        m_storage.SetText((const char*)m_mmfile.GetData(), length, true, reload, external);
        m_storage.SetSavedUndoPos();
        return;
    }

        if (!locking && m_settings.GetFileMemMapForBig())
        {
            DWORD attrs;
            __int64 fileSize;
            if (Common::AppGetFileAttrs(path, &attrs, &fileSize) 
            && fileSize > __int64(m_settings.GetFileMemMapThreshold()) * (1024 * 1024))
            {
                locking = true;
            }
        }

    MemoryMappedFile mmfile;
    try // open a file 
        {
            unsigned options = locking ? emmoRead|emmoWrite|emmoShareRead : emmoRead|emmoShareRead|emmoShareWrite;
            mmfile.Open(path, options, 0);
        }
        catch (const file_exeption& x)
        {
        if (locking 
        && (x.GetErrCode() == ERROR_ACCESS_DENIED || x.GetErrCode() == ERROR_SHARING_VIOLATION))
            {
                const char* text = m_settings.GetFileLocking()
                    ? "Cannot lock the file \"%s\".\nWould you like to continue without locking?"
                    : "Cannot lock the file \"%s\".\nMemory mapped file access is disabled.\nWould you like to continue?";

                char buffer[10 * MAX_PATH];
                _snprintf(buffer, sizeof(buffer), text, path);
                buffer[sizeof(buffer)-1] = 0;

                if (AfxMessageBox(buffer, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                {
                    locking = false;
                    unsigned options = emmoRead|emmoShareRead|emmoShareWrite;
                    mmfile.Open(path, options, 0);
                }
                else
                    AfxThrowUserException(); // it's the most silent exception
            }
            else
            THROW_APP_EXCEPTION(std::string("Cannot open file \"") + path + "\".\n" + x.what());
        }

    LPVOID vmdata = 0;
    bool irrecoverable = false; // if it's true and exception occurs, cleanup is required

    try // ok, read it
    {
        unsigned long length = mmfile.GetDataLength();

        if (locking) // it's equal to using of mm
        {
            irrecoverable = true;
            MemoryMappedFile::Swap(mmfile, m_mmfile);
            m_storage.SetText((const char*)m_mmfile.GetData(), length, true, reload, external);
            m_storage.SetSavedUndoPos();
        }
        else
        {
            if (length > 0)
            {
                vmdata = VirtualAlloc(NULL, length, MEM_COMMIT, PAGE_READWRITE);
                memcpy(vmdata, mmfile.GetData(), length);
                irrecoverable = true;
                m_storage.SetText((const char*)vmdata, length, true, reload, external);
                swap(vmdata, m_vmdata);
            }
            else
            {
                m_storage.SetText("", 0, false, reload, external);
            }

            m_storage.SetSavedUndoPos();

            if (vmdata) VirtualFree(vmdata, 0, MEM_RELEASE);
            vmdata = 0;
        }
    }
    catch (...)
    { 
        try { 
            if (vmdata) VirtualFree(vmdata, 0, MEM_RELEASE);
            vmdata = 0;
        } catch (...) {}
        
        if (irrecoverable)
    { 
            try { m_storage.SetText("", 0, false, false, false); } catch (...) {}
            try { if (m_vmdata) VirtualFree(m_vmdata, 0, MEM_RELEASE); m_vmdata = 0; } catch (...) {}
        try { m_mmfile.Close(); } catch (...) {}
        }
        throw;
    }
}


BOOL COEDocument::OnOpenDocument (LPCTSTR lpszPathName) 
{
    try { EXCEPTION_FRAME;

        _ASSERTE(!m_mmfile.IsOpen() && !m_vmdata);

        DWORD dummy;
        if (m_enableOpenUnexisting 
        && !AppGetFileAttrs(lpszPathName, &dummy))
        {
            if (COEDocument::OnNewDocument())
            {
	            SetPathName(lpszPathName);
                return TRUE;
            }
            return FALSE;
        }

        loadFile(lpszPathName);
    }
    catch (const CUserException*)
    {
        // silent termination because user was notified
        return FALSE;
    }
    catch (const std::exception& x)   
    { 
        DEFAULT_HANDLER(x); 
        return FALSE;
    }
    catch (...)   
    { 
        DEFAULT_HANDLER_ALL; 
        return FALSE;
    }

    CFileWatchClient::StartWatching(lpszPathName);

	return TRUE;
}

BOOL COEDocument::OnSaveDocument (LPCTSTR lpszPathName)
{
    bool storage_invalidated = false;
	char tmpname[_MAX_PATH];

    try { EXCEPTION_FRAME;

        backupFile(lpszPathName);       // create backup file
        m_storage.TruncateSpaces();     // truncate spaces before size allocation
        unsigned long length = m_storage.GetTextLength(); 
        
        // create mm file and copy data
        MemoryMappedFile outfile;

        // if we using mm file then we have to use an intermediate file for file saving
        if (m_mmfile.IsOpen())
        {
	        char *name, path[_MAX_PATH];
	        // get the directory for the intermediate file
            CHECK_FILE_OPERATION(::GetFullPathName(lpszPathName, sizeof(path), path, &name));
	        *name = 0; // cut file name
	        // get a temporary name for the intermediate file
            CHECK_FILE_OPERATION(::GetTempFileName(path, "OE~", 0, tmpname));
            // open output mm file
            outfile.Open(tmpname, emmoCreateAlways|emmoWrite|emmoShareRead, length);
        }
        else
        {
            // suspend file watching
            CFileWatchClient::StopWatching();
            // open output mm file
            outfile.Open(lpszPathName, emmoCreateAlways|emmoWrite|emmoShareRead, length);
        }

        // copy text to output mm file
        m_storage.GetText((char*)outfile.GetData(), length);
        outfile.Close();

        if (m_mmfile.IsOpen())
        {
            storage_invalidated = true;
            // suspend file watching
            CFileWatchClient::StopWatching();
            m_mmfile.Close();
            // delete the original file we have to ignore failure for SaveAs operation
            ::DeleteFile(lpszPathName);
            // rename tmp file to the original
            CHECK_FILE_OPERATION(!::rename(tmpname, lpszPathName));
        }

        // open a new copy and reload a file
        loadFile(lpszPathName, true/*reload*/);
        
        // resume file watching
        CFileWatchClient::StartWatching(lpszPathName);
    }
    catch (const file_exeption& x)   
    { 
        if (storage_invalidated) m_storage.Clear();
        string message = "Cannot save file \"";
        message += lpszPathName;
        message += "\".\n";
        message += x.what();
        AfxMessageBox(message.c_str(), MB_OK|MB_ICONSTOP);
        return FALSE;
    }
    catch (const std::exception& x)   
    { 
        if (storage_invalidated) m_storage.Clear();
        string message = "Cannot save file \"";
        message += lpszPathName;
        message += "\".\n";
        message += x.what();
        DEFAULT_HANDLER(std::exception(message.c_str()));
        return FALSE;
    }
    catch (...) 
    { 
        if (storage_invalidated) m_storage.Clear();
        DEFAULT_HANDLER_ALL;
        string message = "Cannot save file \"";
        message += lpszPathName;
        message += "\".\n";
        AfxMessageBox(message.c_str(), MB_OK|MB_ICONSTOP);
        return FALSE;
    }

    return TRUE;
}


/** @fn BOOL COEDocument::doSave (LPCTSTR lpszPathName, BOOL bReplace)
 * @brief Setup 'Save as...' dialog and call saving.
 *
 * @arg lpszPathName Path name where to save document file.
 * if lpszPathName is NULL then the user will be prompted (SaveAs)
 * note: lpszPathName can be different than 'm_strPathName'
 * if 'bReplace' is TRUE will change file name if successful (SaveAs)
 * if 'bReplace' is FALSE will not change path name (SaveCopyAs)
 *
 * @return True On successful writing the file.
 */
BOOL COEDocument::doSave (LPCTSTR lpszPathName, BOOL bReplace)
{
	CString newName = lpszPathName;
	
    if (newName.IsEmpty())
	{
        // 06.18.2003 bug fix, SaveFile dialog occurs for pl/sql files which are loaded from db 
        //            if "save files when switching takss" is activated.
		//newName = m_strPathName;
        GetNewPathName(newName);

		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			// check for dubious filename
			int iBad = newName.FindOneOf(_T(":/\\* "));
			if (iBad != -1)
            {
                newName.GetBuffer(iBad);
				newName.ReleaseBuffer(iBad);
            }

			// append the default suffix if there is one
			int iStart = 0;
            CString strExt = m_settings.GetExtensions().c_str();
#if _MFC_VER > 0x0600
			newName += '.' + strExt.Tokenize(_T(" ;,"), iStart);
#else
            int iEnd = strExt.FindOneOf(_T(" ;,"));
			newName += '.' + ((iEnd != -1) ? strExt.Left(iEnd) : strExt);
#endif
		}

		// add the timestamp before the extension
		// if there is an old timestamp - replace it
		/*if(GetSettings().GetFileSaveAddTimestamp() == TRUE) {
			string format=GetSettings().GetFileSaveAddTimestampFormat();
			Common::date_oracle_to_c(format.c_str(), format);
			if (!format.empty()) {
				time_t t = time(0);
				if (const tm* lt = localtime(&t)) {
					char buff[80];
					int buff_len;
					strftime(buff, sizeof(buff), format.c_str(), lt);
					buff[sizeof(buff)-1] = 0;
					buff_len=strlen(buff);

					// check for the existing timestamp & place new one
					if (buff_len > 0) {
						BOOL flag_overwrite=FALSE;
						int dot_pos=newName.Find("."), i, pos_start;

						if(dot_pos < 0) {
							pos_start=newName.GetLength()-buff_len; // check last len(buff) chars
						} else {
							pos_start = dot_pos-buff_len;// timestamp is before "."
						}
						if(pos_start >= 0) {
							for(i=pos_start; i < pos_start+buff_len; i++) {
								char curr_char=newName[i], dest_char=buff[i-pos_start];
								// if dest char is num/alpha/other => src char (current filename) also should be
								if(isdigit(dest_char)) {
									if(!isdigit(curr_char)) {
										break;
									}
								} else if(isalpha(dest_char)) {
									if(!isalpha(curr_char)) {
										break;
									}
								} else {
									if(dest_char != curr_char) {
										break;
									}
								}
							}
							// each character in the old timestamp has a type equal to the new timestamp
							if(i == pos_start+buff_len) {
								flag_overwrite=TRUE;
							}
						}

						if(flag_overwrite == TRUE) {
							// replace existing timestamp
							for(i=pos_start; i < pos_start+(int)strlen(buff); i++)
								newName.SetAt(i,buff[i-pos_start]);
						} else {
							// there isn't an old timestamp, so insert the new one before "." or at the end
							if(dot_pos >= 0) {
								newName.Insert(dot_pos, buff);
							} else {
								newName += buff;
							}
						}
					}
				} else
					AfxMessageBox("Datetime stamp failed. Unknown reason.", MB_OK|MB_ICONSTOP);
			} else
				AfxMessageBox("Datetime stamp failed. Empty format string.", MB_OK|MB_ICONSTOP);
		}*/

		if (!AfxGetApp()->DoPromptFileName(newName, 
		        bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, NULL/*pTemplate*/)
            )
			return FALSE; // don't even attempt to save
	}

	CWaitCursor wait;

	if (!OnSaveDocument(newName))
    {
        // 30.06.2004 bug fix, unknown exception on "save as", if a user doesn't have permissions to rewrite the destination 
		return FALSE;
    }

	// reset the title and change the document name
	if (bReplace)
		SetPathName(newName);

	return TRUE;        // success
}

void COEDocument::backupFile (LPCTSTR lpszPathName)
{
    EBackupMode backupMode = (EBackupMode)m_settings.GetFileBackup();

    if (!m_newOrSaveAs &&  backupMode != ebmNone)
    {
        int length = strlen(lpszPathName);

        if (lpszPathName && length)
        {
            std::string backupPath;

            switch (backupMode)
            {
            case ebmCurrentDirectory:
                {
                    std::string path, name, ext;
                    
                    CString buff(lpszPathName);
                    ::PathRemoveFileSpec(buff.LockBuffer()); buff.UnlockBuffer();
                    path = buff;

                    buff = ::PathFindExtension(lpszPathName);
                    ext = (!buff.IsEmpty() && buff[0] == '.') ? (LPCSTR)buff + 1 : buff;

                    buff = ::PathFindFileName(lpszPathName);
                    ::PathRemoveExtension(buff.LockBuffer()); buff.UnlockBuffer();
                    name = buff;

                    Common::Substitutor substr;
                    substr.AddPair("<NAME>", name);
                    substr.AddPair("<EXT>",  ext);
                    substr << m_settings.GetFileBackupName().c_str();

                    backupPath = path + '\\' + substr.GetResult();
                }
                break; // 09.03.2003 bug fix, backup file problem if a destination is the current folder
            case ebmBackupDirectory:
                {
                    std::string path, name;
                    
                    path = m_settings.GetFileBackupDirectory();

                    if (path.rbegin() != path.rend() && *path.rbegin() != '\\')
                        path += '\\';

                    name = ::PathFindFileName(lpszPathName);

                    backupPath = path + name;
                }
                break;
            }

// TODO: replace _CHECK_AND_THROW_ with THROW_APP_EXCEPTION and say why cannot create a backup file
            if (m_lastBackupPath != backupPath              // only for the first attemption
            && stricmp(GetPathName(), backupPath.c_str()))  // avoiding copy the file into itself
            {
                _CHECK_AND_THROW_(
                    CopyFile(lpszPathName, backupPath.c_str(), FALSE) != 0,
                    "Cannot create backup file!"
                    );

                m_lastBackupPath = backupPath;
            }
        }
    }
}

BOOL COEDocument::SaveModified ()
{
    if (m_saveModified_skipNew && m_strPathName.IsEmpty())
        return TRUE;
    else if (m_saveModified_silent && !m_strPathName.IsEmpty())
        return DoFileSave() ? TRUE : FALSE;
    else
        return CDocument::SaveModified();
}

void COEDocument::SetModifiedFlag (BOOL bModified)
{
    if (!bModified)
        m_storage.SetSavedUndoPos(); // 14.04.2003 bug fix, SetModifiedFlag clears an undo history
    else
        _ASSERTE(0);
}

void COEDocument::OnEditPrintPageSetup()
{
    SettingsManager mgr(m_settingsManager);

    if (COEPrintPageSetup(mgr).DoModal() == IDOK)
    {
        m_settingsManager = mgr;
        SaveSettingsManager();
    }
}

void COEDocument::SetText (LPVOID vmdata, unsigned long length)
{
    m_storage.Clear();
    m_mmfile.Close();
    if (m_vmdata) VirtualFree(m_vmdata, 0, MEM_RELEASE);

    m_vmdata = vmdata;
    m_storage.SetText((const char*)m_vmdata, length, true/*use_buffer*/, false/*reload*/, false/*external*/);
}

void COEDocument::SetText (const char* text, unsigned long length)
{
    LPVOID vmdata = VirtualAlloc(NULL, length, MEM_COMMIT, PAGE_READWRITE);
    
    try { EXCEPTION_FRAME;

        memcpy(vmdata, text, length);
        SetText(vmdata, length);
    }
    catch (...)
    {
        if (vmdata != m_vmdata)
            VirtualFree(vmdata, 0, MEM_RELEASE);
    }
}

void COEDocument::LoadSettingsManager ()
{
    std::string path;
    Global::GetSettingsPath(path);

    // Load language support
    LanguagesCollectionReader(
            FileInStream((path + "\\languages.dat").c_str())
        ) >> LanguagesCollection();

    TemplateCollectionReader(
            FileInStream((path + "\\templates.dat").c_str())
        ) >> m_settingsManager;

    // Load editor settings
    SettingsManagerReader(
            FileInStream((path + "\\settings.dat").c_str())
        ) >> m_settingsManager;

}

    static bool g_isBackupDone = false;

void COEDocument::SaveSettingsManager ()
{
    std::string path;
    Global::GetSettingsPath(path);

    if (!g_isBackupDone)
    {
        // old settings backup
        _CHECK_AND_THROW_(
                CopyFile((path + "\\templates.dat").c_str(), 
                    (path + "\\templates.dat.old").c_str(), FALSE) != 0
                && CopyFile((path + "\\settings.dat").c_str(), 
                    (path + "\\settings.dat.old").c_str(), FALSE) != 0,
                "Settings backup file creation error!"
            );
        g_isBackupDone = true;
    }

    // save new seddings
    TemplateCollectionWriter(
            FileOutStream((path + "\\templates.dat").c_str())
        ) << m_settingsManager;

    SettingsManagerWriter(
            FileOutStream((path + "\\settings.dat").c_str())
        ) << m_settingsManager;
}

void COEDocument::ShowSettingsDialog (SettingsDialogCallback* callback)
{
    SettingsManager mgr(m_settingsManager);

    static UINT gStartPage = 0;
	Common::CPropertySheetMem sheet("Settings", gStartPage);
	sheet.SetTreeViewMode(/*bTreeViewMode =*/TRUE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
	sheet.SetTreeWidth(160);
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;

    COEGeneralPage   gp(mgr);
    COEFilePage      fp(mgr);
    COEBackupPage    ap(mgr);
    COEEditingPage   ep(mgr);
    COEBlockPage     bp(mgr);
    COEClassPage     cp(mgr);
    COETemplatesPage tp(mgr);

    std::vector<VisualAttributesSet*> vasets;
    for (int i = 0, ilimit = mgr.GetClassCount() ; i < ilimit; i++)
    {
        ClassSettingsPtr settings = mgr.GetClassByPos(i);
        VisualAttributesSet& vaset = settings->GetVisualAttributesSet();
        vaset.SetName(settings->GetName());
        vasets.push_back(&vaset);
    }

    CVisualAttributesPage vp(vasets);
    
    if (callback) 
        callback->BeforePageAdding(sheet);

    sheet.AddPage(&gp);
    sheet.AddPage(&fp);
    sheet.AddPage(&ap);
    sheet.AddPage(&ep);
    sheet.AddPage(&bp);
    sheet.AddPage(&cp);
    sheet.AddPage(&tp);
    sheet.AddPage(&vp);

    if (callback) 
        callback->AfterPageAdding(sheet);

    if (sheet.DoModal() == IDOK)
    {
        m_settingsManager = mgr;
        SaveSettingsManager();
        if (callback) 
            callback->OnOk();
    }
    else
        if (callback) 
            callback->OnCancel();
}

    static void filetime_to_string (FILETIME& filetime, CString& str)
    {
        SYSTEMTIME systemTime;
        const int buffLen = 40;
        char* buff = str.GetBuffer(buffLen);

        FileTimeToLocalFileTime(&filetime, &filetime);
        FileTimeToSystemTime(&filetime, &systemTime);

        int dateLen = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, NULL, buff, buffLen);
        buff[dateLen - 1] = ' ';
        int timeLen = GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systemTime, NULL, buff + dateLen, buffLen - dateLen);

        str.ReleaseBuffer(dateLen + timeLen);
    }


void COEDocument::OnEditFileSettings()
{
    COEFileInfoDlg infoPage;

    unsigned usage, allocated, undo;
    m_storage.GetMemoryUsage(usage, allocated, undo);

    infoPage.m_Lines.Format("%d lines", m_storage.GetLineCount());
    infoPage.m_MemoryUsage.Format("%d bytes", usage);
    infoPage.m_MemoryAllocated.Format("%d bytes", allocated);
    infoPage.m_UndoMemoryUsage.Format("%d bytes", undo);
    infoPage.m_Path = GetPathName();

    if (!infoPage.m_Path.IsEmpty())
    {
        HANDLE hFile = CreateFile(
            infoPage.m_Path,        // file name
            0,                      // access mode
            FILE_SHARE_READ,        // share mode
            NULL,                   // SD
            OPEN_EXISTING,          // how to create
            FILE_ATTRIBUTE_NORMAL,  // file attributes
            NULL                    // handle to template file
        );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            BY_HANDLE_FILE_INFORMATION fileInformation;

            if (GetFileInformationByHandle(hFile, &fileInformation))
            {
                if (!fileInformation.nFileSizeHigh)
                    infoPage.m_DriveUsage.Format("%u bytes", fileInformation.nFileSizeLow);

                filetime_to_string (fileInformation.ftCreationTime,  infoPage.m_Created);
                filetime_to_string (fileInformation.ftLastWriteTime, infoPage.m_LastModified);
            }

            CloseHandle(hFile);
        }
    }

    Settings settings(m_settingsManager);
    settings = m_settings;
    COEPropPage propPage(m_settingsManager, settings);

    static UINT gStartPage = 0;
    POSITION pos = GetFirstViewPosition();
    Common::CPropertySheetMem sheet("File Properties...", gStartPage, GetNextView(pos));
    sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;

    sheet.AddPage(&propPage);
    sheet.AddPage(&infoPage);

    if (sheet.DoModal() == IDOK)
    {
        m_settings = settings;
    }
}

void COEDocument::OnWatchedFileChanged ()
{
    try { EXCEPTION_FRAME;

        // ignore notification if it's a disabled feature
        if (m_settings.GetFileDetectChanges())
        {
            CString prompt = GetPathName();
            prompt += "\n\nThis file has been modified outside of the editor."
                      "\nDo you want to reload it";
            prompt += !IsModified() ? "?" : " and lose the changes made in the editor?";

            if ((m_settings.GetFileReloadChanges() && !IsModified())
            || AfxMessageBox(prompt, 
                !IsModified() ? MB_YESNO|MB_ICONQUESTION : MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
            {
                vector<COEditorView*> viewsToScroll;
                if (m_settings.GetFileAutoscrollAfterReload())
                {
                    POSITION pos = GetFirstViewPosition();
                    while (pos != NULL) 
                    {
                        CView* pView = GetNextView(pos);
                        if (pView && (pView->IsKindOf(RUNTIME_CLASS(COEditorView))))
                        {
                            Position pos = ((COEditorView*)pView)->GetPosition();
                            if (pos.line >= m_storage.GetLineCount()-1)
                                viewsToScroll.push_back(((COEditorView*)pView));
                        }
                    }
                }
                Global::SetStatusText(CString("Reloading file \"") + GetPathName() + "\" ...");
                CFileWatchClient::UpdateWatchInfo();
                loadFile(GetPathName(), true/*reload*/, true/*external*/);
                Global::SetStatusText(CString("File \"") + GetPathName() + "\" reloaded.");

                if (viewsToScroll.size())
                {
                    
                    for (std::vector<COEditorView*>::iterator it = viewsToScroll.begin();
                        it != viewsToScroll.end(); ++it)
                        (*it)->MoveToBottom();
                }

            }
        }
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnFileReload ()
{
    try { EXCEPTION_FRAME;

        if (!GetPathName().IsEmpty())
        {
            CString prompt = "Do you want to reload \n\n" + GetPathName() 
                    + "\n\nand lose the changes made in the editor?";

            if (!IsModified() || AfxMessageBox(prompt, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
            {
                Global::SetStatusText(CString("Reloading file \"") + GetPathName() + "\" ...");
                loadFile(GetPathName(), true/*reload*/, true/*external*/);
                Global::SetStatusText(CString("File \"") + GetPathName() + "\" reloaded.");
            }
        }
    }
    _OE_DEFAULT_HANDLER_
}

void COEDocument::OnUpdate_FileReload (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty());
}

void COEDocument::OnUpdate_FileType (CCmdUI* pCmdUI)
{
    pCmdUI->SetText(m_storage.GetFileFormatName());
    pCmdUI->Enable(TRUE);
}

void COEDocument::OnUpdate_FileLines (CCmdUI* pCmdUI)
{
    char buff[80];
    sprintf(buff, " Lines: %d ", m_storage.GetLineCount());
    pCmdUI->SetText(buff);
    pCmdUI->Enable(TRUE);
}

void COEDocument::OnEditViewWhiteSpace()
{
    GlobalSettingsPtr settings = m_settingsManager.GetGlobalSettings();
    settings->SetVisibleSpaces(!settings->GetVisibleSpaces());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewWhiteSpace(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetVisibleSpaces() ? TRUE : FALSE);
}

void COEDocument::OnEditViewLineNumbers ()
{
    GlobalSettingsPtr settings = m_settingsManager.GetGlobalSettings();
    settings->SetLineNumbers(!settings->GetLineNumbers());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewLineNumbers (CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetLineNumbers() ? TRUE : FALSE);
}

void COEDocument::OnEditViewRuler ()
{
    GlobalSettingsPtr settings = m_settingsManager.GetGlobalSettings();
    settings->SetRuler(!settings->GetRuler());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewRuler (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetRuler() ? TRUE : FALSE);
}

void COEDocument::OnEditViewColumnMarkers ()
{
    GlobalSettingsPtr settings = m_settingsManager.GetGlobalSettings();
    settings->SetColumnMarkers(!settings->GetColumnMarkers());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewColumnMarkers (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetColumnMarkers() ? TRUE : FALSE);
}

void COEDocument::OnEditViewIndentGuide ()
{
    GlobalSettingsPtr settings = m_settingsManager.GetGlobalSettings();
    settings->SetIndentGuide(!settings->GetIndentGuide());
    SaveSettingsManager();
}

void COEDocument::OnUpdate_EditViewIndentGuide (CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_settings.GetIndentGuide() ? TRUE : FALSE);
}

void COEDocument::OnFileOpen ()
{
    CString path = GetPathName();
    if (m_settings.GetWorkDirFollowsDocument() && !path.IsEmpty())
    {
        PathRemoveFileSpec(path.LockBuffer());
        path.UnlockBuffer();
    }
    CDocManagerExt::SetFileOpenPath(path);
    AfxGetApp()->OnCmdMsg(ID_FILE_OPEN, 0, 0, 0);
    CDocManagerExt::SetFileOpenPath("");
}

