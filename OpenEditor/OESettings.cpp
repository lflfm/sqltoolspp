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

// 08.11.2003 bug fix, file extension is not recognized properly if it's shorter then 3 chars
// 17.01.2005 (ak) simplified settings class hierarchy
// 17.01.2005 (ak) changed exception handling for suppressing unnecessary bug report

#include "stdafx.h"
#include "OpenEditor/OESettings.h"
#include "OpenEditor/OEHelpers.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif//_AFX

namespace OpenEditor
{
    using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// 	BaseSettings

void BaseSettings::AddSubscriber (SettingsSubscriber* psubscriber) const
{
    if (psubscriber)
        m_subscribers.insert(psubscriber);
}


void BaseSettings::RemoveSubscriber (SettingsSubscriber* psubscriber) const
{
    if (psubscriber)
        m_subscribers.erase(psubscriber);
}


void BaseSettings::NotifySettingsChanged ()
{
    std::set<SettingsSubscriber*>::iterator
        it(m_subscribers.begin()), end(m_subscribers.end());

    for (; it != end; ++it)
        (*it)->OnSettingsChanged();
}


/////////////////////////////////////////////////////////////////////////////
// 	ClassSettings

GlobalSettings::GlobalSettings ()
{

    m_PrintHeader       = ""  ;
	m_PrintFooter       = "&P";
	m_PrintLeftMargin   = 20  ;
	m_PrintRightMargin  = 10  ;
	m_PrintTopMargin    = 10  ;
	m_PrintBottomMargin = 10  ;

    m_PrintMarginMeasurement = GetSysPrintMeasuremnt();

    m_DefFileExtension = "txt";
    m_UndoLimit      = 1000 ;
    m_UndoMemLimit   = 1000 ; // kb
    m_TruncateSpaces = true ;
    m_CursorBeyondEOL = false;
    m_CursorBeyondEOF = false;
    m_Ruler = false;
    m_TimestampFormat = "dd.mm.yyyy hh24:mi";
    m_EOFMark = true;


	m_BlockKeepMarking                = false;
    m_BlockKeepMarkingAfterDragAndDrop= true;
	m_BlockDelAndBSDelete             = true ;
	m_BlockTypingOverwrite            = true ;
	m_BlockTabIndent                  = true ;
	m_ColBlockDeleteSpaceAfterMove    = true ;
	m_ColBlockCursorToStartAfterPaste = true ;

    m_FileBackup  = true;
    m_FileLocking = true;

    m_FileAutoscrollAfterReload = false;
    m_Locale = "English";

	//m_FileSaveAddTimestamp = false;
    //m_FileSaveAddTimestampFormat = "ddmmyy";
}


int GlobalSettings::GetSysPrintMeasuremnt ()
{
    char buff[4];

    if (::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, buff, sizeof(buff)))
    {
        return (buff[0] == '1') ? 1 : 0;
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// 	ClassSettings

ClassSettings::ClassSettings (const ClassSettings& other)
{
    *this = other;
}


ClassSettings& ClassSettings::operator = (const ClassSettings& other)
{
    if (this != &other)
    {
    m_Name           = other.m_Name           ;
    m_Extensions     = other.m_Extensions     ;
    m_Language       = other.m_Language       ;
    m_Delimiters     = other.m_Delimiters     ;
    m_TabSpacing     = other.m_TabSpacing     ;
    m_IndentSpacing  = other.m_IndentSpacing  ;
    m_IndentType     = other.m_IndentType     ;
    m_TabExpand      = other.m_TabExpand      ;

    m_NormalizeKeywords = other.m_NormalizeKeywords;
    m_FileCreateAs   = other.m_FileCreateAs   ;

    m_ColumnMarkersSet = other.m_ColumnMarkersSet;
    m_VisualAttributesSet = other.m_VisualAttributesSet;

    NotifySettingsChanged();
    }
    return *this;
}


/////////////////////////////////////////////////////////////////////////////
// 	Settings
Settings::Settings (const SettingsManager& settingsManager)
    : m_globalSettings(0),
    m_classSettings(0),
    m_template(0),
    m_FileSaveAs(effDefault),
    m_overridenTabSpacing(false),
    m_overridenIndentSpacing(false),
    m_overridenIndentType(false),
    m_overridenTabExpand(false),
    m_overridenFileCreateAs(false)
{
    settingsManager.InitializeSettings(*this);
    ASSERT(m_globalSettings.get());
    m_globalSettings->AddSubscriber(this);
}

Settings::~Settings ()
{
    try { EXCEPTION_FRAME;

        if (m_globalSettings.get())
            m_globalSettings->RemoveSubscriber(this);
    }
    _DESTRUCTOR_HANDLER_;

    try { EXCEPTION_FRAME;

        if (m_classSettings.get())
            m_classSettings->RemoveSubscriber(this);
    }
    _DESTRUCTOR_HANDLER_;
}

Settings& Settings::operator = (const Settings& other)
{
    if (this != &other)
    {
        m_globalSettings->RemoveSubscriber(this);
        m_globalSettings = other.m_globalSettings;
        m_globalSettings->AddSubscriber(this);

        m_classSettings->RemoveSubscriber(this);
        m_classSettings = other.m_classSettings;
        m_classSettings->AddSubscriber(this);

        m_template = other.m_template;

        m_overridenTabSpacing    = other.m_overridenTabSpacing;
        m_overridenIndentSpacing = other.m_overridenIndentSpacing;
        m_overridenIndentType    = other.m_overridenIndentType;
        m_overridenTabExpand     = other.m_overridenTabExpand;
        m_overridenFileCreateAs  = other.m_overridenFileCreateAs;

        m_TabSpacing    = other.m_TabSpacing;
        m_IndentSpacing = other.m_IndentSpacing;
        m_IndentType    = other.m_IndentType;
        m_TabExpand     = other.m_TabExpand;
        m_FileCreateAs  = other.m_FileCreateAs;

        m_FileSaveAs    = other.m_FileSaveAs;

        NotifySettingsChanged();
    }
    return *this;
}

void Settings::SetClassSetting (ClassSettingsPtr settings, TemplatePtr templ, bool notify)
{
    if (m_classSettings.get())
        m_classSettings->RemoveSubscriber(this);

    m_classSettings = settings;

    if (m_classSettings.get())
        m_classSettings->AddSubscriber(this);

    m_template = templ;

    if (notify && settings.get())
        NotifySettingsChanged();
}

/////////////////////////////////////////////////////////////////////////////
// 	SettingsManager
SettingsManager::SettingsManager ()
: m_globalSettings(new GlobalSettings)
{
}

SettingsManager::SettingsManager (const SettingsManager& other)
: m_globalSettings(new GlobalSettings)
{
    *this = other;
}


SettingsManager::~SettingsManager ()
{
    m_classSettingsVector.clear(); // it must be call while m_globalSettings still exists
}


SettingsManager& SettingsManager::operator = (const SettingsManager& other)
{
    ASSERT_EXCEPTION_FRAME;

    *m_globalSettings = *other.m_globalSettings;
    m_templateCollection = other.m_templateCollection;

    ClassSettingsVector::const_iterator
        other_it(other.m_classSettingsVector.begin()),
        other_end(other.m_classSettingsVector.end());

    // it is a not initialized object
    if (m_classSettingsVector.size() == 0)
    {
        for (; other_it != other_end; ++other_it)
        {
            m_classSettingsVector.push_back(ClassSettingsPtr(new ClassSettings(**other_it)));
        }

    }
    // copy from a cloned one
    else if (m_classSettingsVector.size() == other.m_classSettingsVector.size())
    {
        ClassSettingsVector::iterator
            it(m_classSettingsVector.begin()),
            end(m_classSettingsVector.end());

        for (; it != end && other_it != other_end; ++it, ++other_it)
        {
            **it = **other_it;
        }
    }
    else
    {
        THROW_APP_EXCEPTION("SettingsManager: illegal assigment.");
    }

    return *this;
}


ClassSettings& SettingsManager::CreateClass (const std::string& name)
{
    ClassSettingsPtr cls;

    try {
        cls = FindByName(name);
    } 
    catch (const Common::AppException&) {}

    if (cls.get())
        THROW_APP_EXCEPTION("class with name " + name + " already exists.");

    cls = ClassSettingsPtr(new ClassSettings);

    m_classSettingsVector.push_back(cls);
    m_classSettingsVector.back()->SetName(name);

    return *cls;
}

ClassSettings& SettingsManager::GetByName (const std::string& name)
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            return **it;
        }

    THROW_APP_EXCEPTION("class with name " + name + " not found.");
}

void SettingsManager::DestroyClass (const std::string& name)
{
    ClassSettingsVector::iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            m_classSettingsVector.erase(it);
            break;
        }
}

/** @fn inline bool is_ext_supported (const string& _list, const std::string& _ext)
 * @brief Checks if ext exists inside the list of accepted extensions.
 *
 * @arg _list List of the extensions (for example: "txt lst log").
 * @arg _ext Checked extension (for example: "sql").
 * @return true or false - depends on existence of the extension.
 */
inline
bool is_ext_supported (const string& _list, const std::string& _ext)
{
    if (!_ext.empty())
    {
        string::iterator it;
        string list = string(' ' + _list + ' ');
        for (it = list.begin(); it != list.end(); ++it)
            *it = toupper(*it);

        string ext = string(1, ' ') + _ext + ' '; // bug fix, file extension is not recognized properly if it's shorter then 3 chars
        for (it = ext.begin(); it != ext.end(); ++it)
            *it = toupper(*it);

        return strstr(list.c_str(), ext.c_str()) ? true : false;
    }

    return false;
}

/** @fn ClassSettingsPtr SettingsManager::FindByExt (const std::string& ext, bool _default) const
 * @brief Finds proper template class searching by the extension.
 *
 * @arg ext Searched extension (for example: "sql").
 * @arg _default ???
 * @return Found template class or default (?) class.
 */
ClassSettingsPtr SettingsManager::FindByExt (const std::string& ext, bool _default) const
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if (is_ext_supported((**it).GetExtensions(), ext))
        {
            return *it;
        }

    if (!_default)
        THROW_APP_EXCEPTION("class for extension " + ext + " not found.");

    return m_classSettingsVector.at(0);
}


ClassSettingsPtr SettingsManager::FindByName (const std::string& name) const
{
    ASSERT_EXCEPTION_FRAME;

    ClassSettingsVector::const_iterator
        it(m_classSettingsVector.begin()),
        end(m_classSettingsVector.end());

    for (; it != end; ++it)
        if ((**it).GetName() == name)
        {
            return *it;
        }

    THROW_APP_EXCEPTION("class with name " + name + " not found.");
}

void SettingsManager::InitializeSettings (Settings& settings) const
{
    settings.m_globalSettings = m_globalSettings;
    ClassSettingsPtr classSettings = GetDefaults();

    TemplatePtr templ;
    try {
        templ = GetTemplateCollection().Find(classSettings->GetName());
    }
    catch (const Common::AppException&) {}

    settings.SetClassSetting(classSettings, templ, false);
}

void SettingsManager::SetClassSettingsByLang (const std::string& name, Settings& settings) const
{
    ClassSettingsPtr classSettings = FindByName(name);

    TemplatePtr templ;
    try {
        templ = GetTemplateCollection().Find(classSettings->GetName());
    }
    catch (const Common::AppException&) {}

    settings.SetClassSetting(classSettings, templ, false);
}

void SettingsManager::SetClassSettingsByExt (const std::string& ext, Settings& settings) const
{
    ClassSettingsPtr classSettings = FindByExt(ext, true);

    TemplatePtr templ;
    try {
        templ = GetTemplateCollection().Find(classSettings->GetName());
        }
    catch (const Common::AppException&) {}

    settings.SetClassSetting(classSettings, templ, false);
}

};//namespace OpenEditor
