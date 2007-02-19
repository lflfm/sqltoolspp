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

// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 07.02.2005 (Ken Clubok) R1105003: Bind variables

#include "stdafx.h"
#include "SQLToolsSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


SQLToolsSettings::SQLToolsSettings ()
{
    m_DateFormat          = "dd.mm.yy";
    m_Autocommit          = false;
    m_CommitOnDisconnect  = 2; //confirm
    m_SavePassword        = true;
    m_OutputEnable        = true;
    m_OutputSize          = 100000;
    m_SessionStatistics   = false;
    m_ScanForSubstitution = false;
    m_PlanTable           = "PLAN_TABLE";
    m_CancelQueryDelay    = 1;
    m_TopmostCancelQuery  = true;
    m_DbmsXplanDisplayCursor = false;
                                 
    m_GridMaxColLength           = 16;
    m_GridMultilineCount         = 3;
    m_GridNullRepresentation     = "";
    m_GridAllowLessThanHeader    = false;
    m_GridAllowRememColWidth     = true;
    m_GridColumnFitType          = 0;
    m_GridExpFormat              = 0;
    m_GridExpWithHeader          = 0;
    m_GridExpCommaChar           = ",";
    m_GridExpQuoteChar           = "\"";
    m_GridExpQuoteEscapeChar     = "\"";
    m_GridExpPrefixChar          = "=";
    m_GridExpColumnNameAsAttr    = false;

    m_GridPrefetchLimit          = 100;
    m_GridMaxLobLength           = 2 * 1024;
    m_GridBlobHexRowLength       = 32;
    m_GridSkipLobs               = false;
    m_GridTimestampFormat        = "dd.mm.yy hh24:mi ss:ff3";

    m_HistoryEnabled             = true;
    m_HistoryAction              = Paste;
    m_HistoryKeepSelection       = true;
    m_HistoryMaxCout             = 100;
    m_HistoryMaxSize             = 64;
    m_HistoryValidOnly           = true;
    m_HistoryDistinct            = false;
    m_HistoryInherit             = false;
    m_HistoryPersitent           = false;
    m_HistoryGlobal              = false;

    // start of OraMetaDict::WriteSettings
    m_bComments =               TRUE;
    m_bGrants =                 TRUE; 
    m_bLowerNames =             TRUE; 
    m_bShemaName =              TRUE; 
    m_bNoStorageForConstraint = FALSE;
    m_nStorageClause =          1;
    m_bAlwaysPutTablespace =    FALSE;
    m_bCommentsAfterColumn =    FALSE;
    m_nCommentsPos =            48;
    m_bSequnceWithStart =       FALSE;
    m_bViewWithForce =          FALSE;
	m_bSQLPlusCompatibility =   FALSE;
    m_bConstraints =            TRUE;
    m_bIndexes =                TRUE; 
    m_bTableDefinition =        TRUE; 
    m_bTriggers =               TRUE; 
    m_bViewWithTriggers =       FALSE;
    // end of OraMetaDict::WriteSettings
    m_bGroupByDDL =             TRUE;
    m_bPreloadDictionary =      FALSE;
    m_bPreloadStartPercent =    7;
    m_bSpecWithBody =           FALSE;
    m_bBodyWithSpec =           FALSE;
    // hidden
    m_NoPrompts =               false;
    m_EndOfShortStatement =     ";";

    m_QueryVASet.SetName("Data Grid Window");
    m_QueryVASet.Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_QueryVASet.Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

    m_StatVASet.SetName("Statistics Window");
    m_StatVASet.Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_StatVASet.Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

    m_OutputVASet.SetName("Output Window");
    m_OutputVASet.Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_OutputVASet.Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));
    
    m_HistoryVASet.SetName("History Window");
    m_HistoryVASet.Add(VisualAttribute("Text", "MS Sans Serif", 8));
    //m_HistoryVASet.Add(VisualAttribute("Selected Text", "MS Sans Serif", 8));

	m_BindVASet.SetName("Bind Window");
    m_BindVASet.Add(VisualAttribute("Text", "MS Sans Serif", 8));

}

const VisualAttributesSet& SQLToolsSettings::GetVASet (const string& name) const
{
    if (m_QueryVASet.GetName() == name)
        return m_QueryVASet;
    else if (m_StatVASet.GetName() == name)
        return m_StatVASet;
    else if (m_OutputVASet.GetName() == name)
        return m_OutputVASet;
    else if (m_HistoryVASet.GetName() == name)
        return m_HistoryVASet;
	else if (m_BindVASet.GetName() == name)
        return m_BindVASet;

    _CHECK_AND_THROW_(0, (string("VisualAttributesSet \"") + name + "\" not found!").c_str());
}



#define OESMS_WRITE_MEMBER(o,p) m_out.write(#p, o.m_##p);
#define OESMS_READ_MEMBER(o,p)  m_in.read(#p, o.m_##p);
#define OESMS_VER_READ_MEMBER(ver,o,p,def)  \
    if (m_version >= ver) m_in.read(#p, o.m_##p); \
    else o.m_##p = def;

#define OESMS_VER_READ_MEMBER_DEFAULT(ver,o,p,def)  \
    if (m_version >= ver) m_in.read_with_default(#p, o.m_##p, def); \
    else o.m_##p = def;

    using std::hex;
    using std::dec;
    using std::endl;
    using std::getline;

    const int TheSettingsVersion = 1016;

void SQLToolsSettingsReader::operator >> (SQLToolsSettings& settings)
{
    m_version = 0;
    m_in.read("Version", m_version);

    _CHECK_AND_THROW_(m_version >= 1001 && m_version <= TheSettingsVersion, "Unsupported settings version!");

    OESMS_READ_MEMBER(settings, DateFormat         );
    OESMS_READ_MEMBER(settings, Autocommit         );
    OESMS_VER_READ_MEMBER(1014, settings, CommitOnDisconnect, 2); // confirm
    OESMS_READ_MEMBER(settings, SavePassword       );
    OESMS_READ_MEMBER(settings, OutputEnable       );
    OESMS_READ_MEMBER(settings, OutputSize         );
    OESMS_READ_MEMBER(settings, SessionStatistics  );
    OESMS_READ_MEMBER(settings, ScanForSubstitution);
    OESMS_READ_MEMBER(settings, PlanTable          );
    OESMS_VER_READ_MEMBER(1009, settings, CancelQueryDelay, 1);
    OESMS_VER_READ_MEMBER(1010, settings, TopmostCancelQuery, true);
    OESMS_VER_READ_MEMBER_DEFAULT(1016, settings, DbmsXplanDisplayCursor, false);
                                
    OESMS_READ_MEMBER(settings, GridMaxColLength          );
    OESMS_READ_MEMBER(settings, GridMultilineCount        );
    OESMS_READ_MEMBER(settings, GridNullRepresentation    );
    OESMS_READ_MEMBER(settings, GridDateFormat            );
    OESMS_READ_MEMBER(settings, GridAllowLessThanHeader   );
    OESMS_READ_MEMBER(settings, GridAllowRememColWidth    );
    OESMS_READ_MEMBER(settings, GridColumnFitType         );

    OESMS_VER_READ_MEMBER(1016, settings, GridNlsNumberFormat,      "");
    OESMS_VER_READ_MEMBER(1016, settings, GridNlsDateFormat,        "");
    OESMS_VER_READ_MEMBER(1016, settings, GridNlsTimeFormat,        "");
    OESMS_VER_READ_MEMBER(1016, settings, GridNlsTimestampFormat,   "");
    OESMS_VER_READ_MEMBER(1016, settings, GridNlsTimestampTzFormat, "");

    OESMS_VER_READ_MEMBER(1003, settings, GridExpFormat,          0);
    OESMS_VER_READ_MEMBER(1003, settings, GridExpWithHeader,      0);
    OESMS_VER_READ_MEMBER(1003, settings, GridExpCommaChar,       ",");
    OESMS_VER_READ_MEMBER(1003, settings, GridExpQuoteChar,       "\"");
    OESMS_VER_READ_MEMBER(1003, settings, GridExpQuoteEscapeChar, "\"");
    OESMS_VER_READ_MEMBER(1015, settings, GridExpPrefixChar,      "=");
    OESMS_VER_READ_MEMBER(1005, settings, GridExpColumnNameAsAttr, false);

    OESMS_VER_READ_MEMBER(1004, settings, GridPrefetchLimit,      100);
    OESMS_VER_READ_MEMBER(1004, settings, GridMaxLobLength,       2 * 1024);
    OESMS_VER_READ_MEMBER(1004, settings, GridBlobHexRowLength,   32);
    OESMS_VER_READ_MEMBER(1006, settings, GridSkipLobs,           false);
    OESMS_VER_READ_MEMBER(1006, settings, GridTimestampFormat,    "");

    OESMS_VER_READ_MEMBER(1006, settings, HistoryEnabled,         true);
    OESMS_VER_READ_MEMBER(1008, settings, HistoryAction,          SQLToolsSettings::HistoryAction::Paste);
    OESMS_VER_READ_MEMBER(1008, settings, HistoryKeepSelection,   true);
    OESMS_VER_READ_MEMBER(1006, settings, HistoryMaxCout,         100);
    OESMS_VER_READ_MEMBER(1006, settings, HistoryMaxSize,         64);
    OESMS_VER_READ_MEMBER(1007, settings, HistoryValidOnly,       true);
    OESMS_VER_READ_MEMBER(1006, settings, HistoryDistinct,        false);
    OESMS_VER_READ_MEMBER(1006, settings, HistoryInherit,         false);
    OESMS_VER_READ_MEMBER(1006, settings, HistoryPersitent,       false);
    OESMS_VER_READ_MEMBER(1012, settings, HistoryGlobal,          false);

    {
        Stream::Section sect(m_in, "DDL");
        // start of OraMetaDict::WriteSettings
        m_in.read("Comments",               settings.m_bComments);
        m_in.read("Grants",                 settings.m_bGrants);
        m_in.read("LowerNames",             settings.m_bLowerNames);
        m_in.read("ShemaName",              settings.m_bShemaName);
        m_in.read("SQLPlusCompatibility",   settings.m_bSQLPlusCompatibility);
        m_in.read("CommentsAfterColumn",    settings.m_bCommentsAfterColumn);
        m_in.read("CommentsPos",            settings.m_nCommentsPos);
        m_in.read("Constraints",            settings.m_bConstraints);
        m_in.read("Indexes",                settings.m_bIndexes);
        m_in.read("NoStorageForConstraint", settings.m_bNoStorageForConstraint);
        m_in.read("StorageClause",          settings.m_nStorageClause);
        m_in.read("AlwaysPutTablespace",    settings.m_bAlwaysPutTablespace);
        m_in.read("TableDefinition",        settings.m_bTableDefinition);
        m_in.read("Triggers",               settings.m_bTriggers);
        m_in.read("SequnceWithStart",       settings.m_bSequnceWithStart);
        m_in.read("ViewWithTriggers",       settings.m_bViewWithTriggers);
        m_in.read("ViewWithForce",          settings.m_bViewWithForce);
        // end of OraMetaDict::WriteSettings
        m_in.read("GroupByDDL",             settings.m_bGroupByDDL);
        m_in.read("PreloadDictionary",      settings.m_bPreloadDictionary);
        m_in.read("PreloadStartPercent",    settings.m_bPreloadStartPercent);
        m_in.read("BodyWithSpec",           settings.m_bBodyWithSpec);
        m_in.read("SpecWithBody",           settings.m_bSpecWithBody);

        OESMS_VER_READ_MEMBER(1011, settings, NoPrompts,         false);
        OESMS_VER_READ_MEMBER(1011, settings, EndOfShortStatement, ";");
    }

    read(settings.m_QueryVASet );
    read(settings.m_StatVASet  );
    read(settings.m_OutputVASet);

    if (m_version >= 1013) 
        read(settings.m_HistoryVASet);
}

void SQLToolsSettingsWriter::operator << (const SQLToolsSettings& settings)
{
    m_out.write("Version", TheSettingsVersion);

    OESMS_WRITE_MEMBER(settings, DateFormat         );
    OESMS_WRITE_MEMBER(settings, Autocommit         );
    OESMS_WRITE_MEMBER(settings, CommitOnDisconnect );
    OESMS_WRITE_MEMBER(settings, SavePassword       );
    OESMS_WRITE_MEMBER(settings, OutputEnable       );
    OESMS_WRITE_MEMBER(settings, OutputSize         );
    OESMS_WRITE_MEMBER(settings, SessionStatistics  );
    OESMS_WRITE_MEMBER(settings, ScanForSubstitution);
    OESMS_WRITE_MEMBER(settings, PlanTable          );
    OESMS_WRITE_MEMBER(settings, CancelQueryDelay   );
    OESMS_WRITE_MEMBER(settings, TopmostCancelQuery );
    OESMS_WRITE_MEMBER(settings, DbmsXplanDisplayCursor );
                                
    OESMS_WRITE_MEMBER(settings, GridMaxColLength          );
    OESMS_WRITE_MEMBER(settings, GridMultilineCount        );
    OESMS_WRITE_MEMBER(settings, GridNullRepresentation    );
    OESMS_WRITE_MEMBER(settings, GridDateFormat            ); // obsolete
    OESMS_WRITE_MEMBER(settings, GridAllowLessThanHeader   );
    OESMS_WRITE_MEMBER(settings, GridAllowRememColWidth    );
    OESMS_WRITE_MEMBER(settings, GridColumnFitType         );

    OESMS_WRITE_MEMBER(settings, GridNlsNumberFormat       );
    OESMS_WRITE_MEMBER(settings, GridNlsDateFormat         );
    OESMS_WRITE_MEMBER(settings, GridNlsTimeFormat         );
    OESMS_WRITE_MEMBER(settings, GridNlsTimestampFormat    );
    OESMS_WRITE_MEMBER(settings, GridNlsTimestampTzFormat  );

    OESMS_WRITE_MEMBER(settings, GridExpFormat             );
    OESMS_WRITE_MEMBER(settings, GridExpWithHeader         );
    OESMS_WRITE_MEMBER(settings, GridExpCommaChar          );
    OESMS_WRITE_MEMBER(settings, GridExpQuoteChar          );
    OESMS_WRITE_MEMBER(settings, GridExpQuoteEscapeChar    );
    OESMS_WRITE_MEMBER(settings, GridExpPrefixChar    );
    OESMS_WRITE_MEMBER(settings, GridExpColumnNameAsAttr   );

    OESMS_WRITE_MEMBER(settings, GridPrefetchLimit         );
    OESMS_WRITE_MEMBER(settings, GridMaxLobLength          );
    OESMS_WRITE_MEMBER(settings, GridBlobHexRowLength      );
    OESMS_WRITE_MEMBER(settings, GridSkipLobs              );
    OESMS_WRITE_MEMBER(settings, GridTimestampFormat       );

    OESMS_WRITE_MEMBER(settings, HistoryEnabled            );
    OESMS_WRITE_MEMBER(settings, HistoryAction             );
    OESMS_WRITE_MEMBER(settings, HistoryKeepSelection      );
    OESMS_WRITE_MEMBER(settings, HistoryMaxCout            );
    OESMS_WRITE_MEMBER(settings, HistoryMaxSize            );
    OESMS_WRITE_MEMBER(settings, HistoryValidOnly          );
    OESMS_WRITE_MEMBER(settings, HistoryDistinct           );
    OESMS_WRITE_MEMBER(settings, HistoryInherit            );
    OESMS_WRITE_MEMBER(settings, HistoryPersitent          );
    OESMS_WRITE_MEMBER(settings, HistoryGlobal             );

    {
        Stream::Section sect(m_out, "DDL");
        // start of OraMetaDict::WriteSettings
        m_out.write("Comments",               settings.m_bComments);
        m_out.write("Grants",                 settings.m_bGrants);
        m_out.write("LowerNames",             settings.m_bLowerNames);
        m_out.write("ShemaName",              settings.m_bShemaName);
        m_out.write("SQLPlusCompatibility",   settings.m_bSQLPlusCompatibility);
        m_out.write("CommentsAfterColumn",    settings.m_bCommentsAfterColumn);
        m_out.write("CommentsPos",            settings.m_nCommentsPos);
        m_out.write("Constraints",            settings.m_bConstraints);
        m_out.write("Indexes",                settings.m_bIndexes);
        m_out.write("NoStorageForConstraint", settings.m_bNoStorageForConstraint);
        m_out.write("StorageClause",          settings.m_nStorageClause);
        m_out.write("AlwaysPutTablespace",    settings.m_bAlwaysPutTablespace);
        m_out.write("TableDefinition",        settings.m_bTableDefinition);
        m_out.write("Triggers",               settings.m_bTriggers);
        m_out.write("SequnceWithStart",       settings.m_bSequnceWithStart);
        m_out.write("ViewWithTriggers",       settings.m_bViewWithTriggers);
        m_out.write("ViewWithForce",          settings.m_bViewWithForce);
        // end of OraMetaDict::WriteSettings
        m_out.write("GroupByDDL",             settings.m_bGroupByDDL);
        m_out.write("PreloadDictionary",      settings.m_bPreloadDictionary);
        m_out.write("PreloadStartPercent",    settings.m_bPreloadStartPercent);
        m_out.write("BodyWithSpec",           settings.m_bBodyWithSpec);
        m_out.write("SpecWithBody",           settings.m_bSpecWithBody);

        OESMS_WRITE_MEMBER(settings, NoPrompts);
        OESMS_WRITE_MEMBER(settings, EndOfShortStatement);
    }

    write(settings.m_QueryVASet );
    write(settings.m_StatVASet  );
    write(settings.m_OutputVASet);
    write(settings.m_HistoryVASet);
}

void SQLToolsSettingsWriter::write (const VisualAttributesSet& set_)
{
    Stream::Section sect(m_out, "VA");
    m_out.write("Name", set_.GetName());

    int count = set_.GetCount();
    m_out.write("Count", count);

    for (int i(0); i < count; i++)
    {
        Stream::Section sect(m_out, i);
        write(set_[i]);
    }
}


void SQLToolsSettingsReader::read  (VisualAttributesSet& set_)
{
    Stream::Section sect(m_in, "VA");

    string setName;
    m_in.read("Name", setName);
    set_.SetName(setName);

    int count = 0;
    m_in.read("Count", count);
    set_.Clear();

    for (int i(0); i < count; i++)
    {
        VisualAttribute attr;
        Stream::Section sect(m_in, i);
        read(attr);
        set_.Add(attr);

        if (m_version <= 1003 && attr.m_Name == "Selected Text")
        {
            attr.m_Name = "Current Line";
            attr.m_Mask = Common::vaoBackground;
            attr.m_Background = RGB(255,255,127);
            set_.Add(attr);
        }
    }
}


void SQLToolsSettingsWriter::write (const VisualAttribute& attr)
{
    OESMS_WRITE_MEMBER(attr, Name);
    OESMS_WRITE_MEMBER(attr, FontName);
    OESMS_WRITE_MEMBER(attr, FontSize);
    OESMS_WRITE_MEMBER(attr, FontBold);
    OESMS_WRITE_MEMBER(attr, FontItalic);
    OESMS_WRITE_MEMBER(attr, FontUnderline);
    OESMS_WRITE_MEMBER(attr, Foreground);
    OESMS_WRITE_MEMBER(attr, Background);
    OESMS_WRITE_MEMBER(attr, Mask);
}


void SQLToolsSettingsReader::read  (VisualAttribute& attr)
{
    OESMS_READ_MEMBER(attr, Name);
    OESMS_READ_MEMBER(attr, FontName);
    OESMS_READ_MEMBER(attr, FontSize);
    OESMS_READ_MEMBER(attr, FontBold);
    OESMS_READ_MEMBER(attr, FontItalic);
    OESMS_READ_MEMBER(attr, FontUnderline);
    OESMS_READ_MEMBER(attr, Foreground);
    OESMS_READ_MEMBER(attr, Background);
    OESMS_READ_MEMBER(attr, Mask);
}

