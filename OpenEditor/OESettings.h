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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OESettings_h__
#define __OESettings_h__

/*
    28.06.02 new properties have been added
    01.10.02 new properties have been added
    10.01.03 new properties have been added
    29.06.2003 improvement, "Restrict cursor" has been replaced with "Cursor beyond EOL" and "Cursor beyond EOF"
    17.01.2005 (ak) simplified settings class hierarchy 
*/

#include <string>
#include <vector>
#include <set>
#include <arg_shared.h>
#include "COMMON/VisualAttributes.h"
#include "OpenEditor/OETemplates.h"


#define OES_DECLARE_PROPERTY(T,N) \
    protected: \
        T m_##N; \
    public: \
        const T& Get##N () const { return m_##N; }; \
        void  Set##N (const T& val, bool notify = true) \
            { m_##N = val; if (notify) NotifySettingsChanged(); }

#define OES_AGGREGATE_GLOBAL_PROPERTY(T,N) \
    public: \
        const T& Get##N () const {  _ASSERTE(m_globalSettings.get()); return m_globalSettings->Get##N(); }; \

#define OES_AGGREGATE_CLASS_PROPERTY(T,N) \
    public: \
        const T& Get##N () const {  _ASSERTE(m_classSettings.get()); return m_classSettings->Get##N(); }; \

#define OES_OVERRIDE_CLASS_PROPERTY(T,N) \
    protected: \
        T m_##N; \
        bool m_overriden##N; \
    public: \
        const T& Get##N () const \
            { _ASSERTE(m_classSettings.get()); \
              return (m_overriden##N) ? m_##N : m_classSettings->Get##N(); }; \
        void  Set##N (const T& val, bool notify = true) \
            { _ASSERTE(m_classSettings.get()); \
              m_##N = val; m_overriden##N = true; if (notify) NotifySettingsChanged(); }



namespace OpenEditor
{
    using std::string;
    using std::vector;
    using std::set;
    using arg::counted_ptr;

    using Common::VisualAttribute;
    using Common::VisualAttributesSet;
    using Common::VisualAttributeOption;

    class SyntaxAnalizer;
    class Highlighter;
    class Language;

    class SettingsSubscriber;
    class BaseSettings;
    class GlobalSettings;
    class ClassSettings;
    class Settings;
    class SettingsManager;
    class SettingsManagerReader;
    class SettingsManagerWriter;

    typedef counted_ptr<GlobalSettings> GlobalSettingsPtr;
    typedef counted_ptr<ClassSettings> ClassSettingsPtr;
    typedef std::vector<ClassSettingsPtr> ClassSettingsVector;

    enum EFileFormat { effDefault = -1, effDos = 0, effUnix = 1, effMac = 2 };
    enum EBackupMode { ebmNone = 0, ebmCurrentDirectory = 1, ebmBackupDirectory = 2 };

    ///////////////////////////////////////////////////////////////////////////
    // 	SettingsSubscriber
    class SettingsSubscriber
    {
        friend class BaseSettings;
    protected:
        virtual ~SettingsSubscriber () {};
    private:
        virtual void OnSettingsChanged () = 0;
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	BaseSettings
    class BaseSettings
    {
    public:
        virtual void AddSubscriber (SettingsSubscriber*) const;
        virtual void RemoveSubscriber (SettingsSubscriber*) const;
        virtual void NotifySettingsChanged ();

        // dont copy subscribers
        BaseSettings () {} 
        BaseSettings (const BaseSettings&) {}
        BaseSettings& operator = (const BaseSettings&) { return *this; }

        virtual ~BaseSettings () {};
    private:
        mutable set<SettingsSubscriber*> m_subscribers;
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	GlobalSettings is a global part of a settings implementation
    class GlobalSettings : public BaseSettings
    {
        friend class SettingsManagerReader;
        friend class SettingsManagerWriter;
    public:
        GlobalSettings ();
        static int GetSysPrintMeasuremnt ();

        OES_DECLARE_PROPERTY(bool,   PrintBlackAndWhite);
        OES_DECLARE_PROPERTY(string, PrintHeader);
        OES_DECLARE_PROPERTY(string, PrintFooter);
        OES_DECLARE_PROPERTY(int,    PrintMarginMeasurement);
        OES_DECLARE_PROPERTY(double, PrintLeftMargin);
        OES_DECLARE_PROPERTY(double, PrintRightMargin);
        OES_DECLARE_PROPERTY(double, PrintTopMargin);
        OES_DECLARE_PROPERTY(double, PrintBottomMargin);

        OES_DECLARE_PROPERTY(string, DefFileExtension);
        OES_DECLARE_PROPERTY(int,  UndoLimit);
        OES_DECLARE_PROPERTY(int,  UndoMemLimit);
        OES_DECLARE_PROPERTY(bool, UndoAfterSaving);
        OES_DECLARE_PROPERTY(bool, TruncateSpaces);
        OES_DECLARE_PROPERTY(bool, CursorBeyondEOL);
        OES_DECLARE_PROPERTY(bool, CursorBeyondEOF);
        OES_DECLARE_PROPERTY(bool, LineNumbers);
        OES_DECLARE_PROPERTY(bool, VisibleSpaces);
        OES_DECLARE_PROPERTY(bool, Ruler);
        OES_DECLARE_PROPERTY(string, TimestampFormat);
        OES_DECLARE_PROPERTY(bool, EOFMark);
        OES_DECLARE_PROPERTY(bool, HomeGotoFirstColumn);
        OES_DECLARE_PROPERTY(bool, ColumnMarkers);
        OES_DECLARE_PROPERTY(bool, IndentGuide);

        OES_DECLARE_PROPERTY(bool, BlockKeepMarking);
        OES_DECLARE_PROPERTY(bool, BlockKeepMarkingAfterDragAndDrop);
        OES_DECLARE_PROPERTY(bool, BlockDelAndBSDelete);
        OES_DECLARE_PROPERTY(bool, BlockTypingOverwrite);
        OES_DECLARE_PROPERTY(bool, BlockTabIndent);
        OES_DECLARE_PROPERTY(bool, ColBlockDeleteSpaceAfterMove);
        OES_DECLARE_PROPERTY(bool, ColBlockCursorToStartAfterPaste);
        OES_DECLARE_PROPERTY(bool, ColBlockEditMode);
        OES_DECLARE_PROPERTY(string, MouseSelectionDelimiters);

        OES_DECLARE_PROPERTY(bool,   FileLocking);
        OES_DECLARE_PROPERTY(bool,   FileMemMapForBig);
        OES_DECLARE_PROPERTY(int,    FileMemMapThreshold);
        OES_DECLARE_PROPERTY(bool,   FileSaveOnSwith);
        OES_DECLARE_PROPERTY(bool,   FileDetectChanges);
        OES_DECLARE_PROPERTY(bool,   FileReloadChanges);
        OES_DECLARE_PROPERTY(bool,   FileAutoscrollAfterReload);
        OES_DECLARE_PROPERTY(bool,   FileOverwriteReadonly);
        OES_DECLARE_PROPERTY(int,    FileBackup);
        OES_DECLARE_PROPERTY(string, FileBackupName);
        OES_DECLARE_PROPERTY(string, FileBackupDirectory);
	//	OES_DECLARE_PROPERTY(bool,   FileSaveAddTimestamp);
	//	OES_DECLARE_PROPERTY(string, FileSaveAddTimestampFormat);

        OES_DECLARE_PROPERTY(bool,   AllowMultipleInstances);
        OES_DECLARE_PROPERTY(bool,   NewDocOnStartup);
        OES_DECLARE_PROPERTY(bool,   MaximizeFirstDocument);
        OES_DECLARE_PROPERTY(bool,   WorkDirFollowsDocument);
        OES_DECLARE_PROPERTY(bool,   SaveCurPosAndBookmarks);
        OES_DECLARE_PROPERTY(bool,   SaveMainWinPosition);
        OES_DECLARE_PROPERTY(bool,   DoubleClickCloseTab);
		OES_DECLARE_PROPERTY(bool,   FindObjectMoveCursor);
        OES_DECLARE_PROPERTY(string, Locale);
        OES_DECLARE_PROPERTY(string, KeymapLayout);
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	ClassSettings is class document settings
    class ClassSettings : public BaseSettings
    {
        friend class SettingsManagerReader;
        friend class SettingsManagerWriter;
    public:
        // class properties
        OES_DECLARE_PROPERTY(string, Name);
        OES_DECLARE_PROPERTY(string, Extensions);
        OES_DECLARE_PROPERTY(string, Language);

        OES_DECLARE_PROPERTY(string, Delimiters);
        OES_DECLARE_PROPERTY(int,    TabSpacing);
        OES_DECLARE_PROPERTY(int,    IndentSpacing);
        OES_DECLARE_PROPERTY(int,    IndentType);
        OES_DECLARE_PROPERTY(bool,   TabExpand);
        OES_DECLARE_PROPERTY(bool,   NormalizeKeywords);
        OES_DECLARE_PROPERTY(vector<int>, ColumnMarkersSet);

        OES_DECLARE_PROPERTY(int,    FileCreateAs);

        OES_DECLARE_PROPERTY(VisualAttributesSet, VisualAttributesSet);

        VisualAttributesSet& GetVisualAttributesSet () { return m_VisualAttributesSet; }

    public:
        // constructors, destructor
        ClassSettings () {}
        ClassSettings (const ClassSettings&);
        // assign operator
        ClassSettings& operator = (const ClassSettings&);
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	Settings is text document settings
    class Settings : public SettingsSubscriber, public BaseSettings
    {
        friend class SettingsManager;
        friend class SettingsManagerReader;
        friend class SettingsManagerWriter;
    public:
        // class properties
        OES_AGGREGATE_CLASS_PROPERTY(string, Name);
        OES_AGGREGATE_CLASS_PROPERTY(string, Extensions);
        OES_AGGREGATE_CLASS_PROPERTY(string, Language);
        OES_AGGREGATE_CLASS_PROPERTY(string, Delimiters);
        OES_AGGREGATE_CLASS_PROPERTY(bool,   NormalizeKeywords);
        OES_AGGREGATE_CLASS_PROPERTY(vector<int>, ColumnMarkersSet);
        OES_AGGREGATE_CLASS_PROPERTY(VisualAttributesSet, VisualAttributesSet);

        // overridden class properties
        OES_OVERRIDE_CLASS_PROPERTY(int,     TabSpacing);
        OES_OVERRIDE_CLASS_PROPERTY(int,     IndentSpacing);
        OES_OVERRIDE_CLASS_PROPERTY(int,     IndentType);
        OES_OVERRIDE_CLASS_PROPERTY(bool,    TabExpand);
        OES_OVERRIDE_CLASS_PROPERTY(int,     FileCreateAs); // Dos, Unix, Mac
        
        // instance properties
        OES_DECLARE_PROPERTY(int, FileSaveAs); // Default, Dos, Unix, Mac

        // global properties
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   PrintBlackAndWhite);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, PrintHeader);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, PrintFooter);
        OES_AGGREGATE_GLOBAL_PROPERTY(int,    PrintMarginMeasurement);
        OES_AGGREGATE_GLOBAL_PROPERTY(double, PrintLeftMargin);
        OES_AGGREGATE_GLOBAL_PROPERTY(double, PrintRightMargin);
        OES_AGGREGATE_GLOBAL_PROPERTY(double, PrintTopMargin);
        OES_AGGREGATE_GLOBAL_PROPERTY(double, PrintBottomMargin);

        OES_AGGREGATE_GLOBAL_PROPERTY(string, DefFileExtension);
        OES_AGGREGATE_GLOBAL_PROPERTY(int,    UndoLimit);
        OES_AGGREGATE_GLOBAL_PROPERTY(int,    UndoMemLimit);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   UndoAfterSaving);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   TruncateSpaces);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   CursorBeyondEOL);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   CursorBeyondEOF);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   LineNumbers);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   VisibleSpaces);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   Ruler);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, TimestampFormat);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   EOFMark);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   HomeGotoFirstColumn);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   ColumnMarkers);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   IndentGuide);

        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   BlockKeepMarking);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   BlockKeepMarkingAfterDragAndDrop);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   BlockDelAndBSDelete);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   BlockTypingOverwrite);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   BlockTabIndent);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   ColBlockDeleteSpaceAfterMove);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   ColBlockCursorToStartAfterPaste);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   ColBlockEditMode);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, MouseSelectionDelimiters);

        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileLocking);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileMemMapForBig);
        OES_AGGREGATE_GLOBAL_PROPERTY(int,    FileMemMapThreshold);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileSaveOnSwith);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileDetectChanges);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileReloadChanges);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileAutoscrollAfterReload);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileOverwriteReadonly);
        OES_AGGREGATE_GLOBAL_PROPERTY(int,    FileBackup);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, FileBackupName);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, FileBackupDirectory);
	//	OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FileSaveAddTimestamp);
	//	OES_AGGREGATE_GLOBAL_PROPERTY(string, FileSaveAddTimestampFormat);

        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   AllowMultipleInstances);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   NewDocOnStartup);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   MaximizeFirstDocument);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   WorkDirFollowsDocument);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   SaveCurPosAndBookmarks);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   SaveMainWinPosition);
        OES_AGGREGATE_GLOBAL_PROPERTY(bool,   DoubleClickCloseTab);
		OES_AGGREGATE_GLOBAL_PROPERTY(bool,   FindObjectMoveCursor);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, Locale);
        OES_AGGREGATE_GLOBAL_PROPERTY(string, KeymapLayout);

        // class/language template support
        const TemplatePtr GetTemplate () const { return m_template; };
    public:
        // constructors, destructor
        Settings (const SettingsManager&);
        ~Settings ();

        // assign operator
        Settings& operator = (const Settings&);

    private:
        GlobalSettingsPtr m_globalSettings;
        ClassSettingsPtr  m_classSettings;
        TemplatePtr       m_template;

        // forwading notification from m_globalSettings and m_classSettings
        virtual void OnSettingsChanged () { NotifySettingsChanged(); };

        void SetClassSetting (ClassSettingsPtr, TemplatePtr, bool notify = true);

        Settings ();
        Settings (const Settings& other);
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	SettingsManager
    //
    //  A document has a pointer to a ClassSettings instance
    //  so we will support removing only if a reference counter is equal 0.
    //  For GUI edit operations we will create a copy of SettingsManager
    //  and copy state to an original object after a successful validation.
    //
    class SettingsManager
    {
        friend class SettingsManagerReader;
        friend class SettingsManagerWriter;
        friend class TemplateCollectionReader;
        friend class TemplateCollectionWriter;
    public:
        // constructors, destructor
        SettingsManager ();
        SettingsManager (const SettingsManager& other);
        ~SettingsManager ();

        // assign operator
        SettingsManager& operator = (const SettingsManager&);

        // create, destroy a class (for GUI)
        ClassSettings& CreateClass (const std::string& name);
        ClassSettings& GetByName (const std::string& name);
        void DestroyClass (const std::string& name);

        // global settings
        GlobalSettingsPtr GetGlobalSettings () const { return m_globalSettings; }

        // find methods
        ClassSettingsPtr FindByExt (const std::string&, bool _default) const;
        ClassSettingsPtr FindByName (const std::string&) const;
        ClassSettingsPtr GetDefaults () const;

        // enumeration
        int GetClassCount () const;
        ClassSettingsPtr GetClassByPos (int i) const;

        // class/language template support
        const TemplateCollection& GetTemplateCollection () const;

        void InitializeSettings (Settings&) const;
        // set class settings and template for settings by language name
        void SetClassSettingsByLang (const std::string& name, Settings&) const;
        void SetClassSettingsByExt (const std::string& ext, Settings&) const;

    private:
        GlobalSettingsPtr   m_globalSettings;
        ClassSettingsVector m_classSettingsVector;
        TemplateCollection m_templateCollection;
    };


    ///////////////////////////////////////////////////////////////////////////
    // 	SettingsManager
    inline
    int SettingsManager::GetClassCount () const
    {
        return m_classSettingsVector.size();
    }

    inline
    ClassSettingsPtr SettingsManager::GetDefaults () const
    {
        return FindByExt(GetGlobalSettings()->GetDefFileExtension(), true);
    }

    inline
    ClassSettingsPtr SettingsManager::GetClassByPos (int i) const
    {
        return m_classSettingsVector.at(i);
    }

    inline
    const TemplateCollection& SettingsManager::GetTemplateCollection () const
    {
        return m_templateCollection;
    }

};//namespace OpenEditor


#endif//__OESettings_h__
