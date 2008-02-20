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

#ifndef __SQLToolsSettings_h__
#define __SQLToolsSettings_h__

#include <string>
#include <MetaDict/MetaObjects.h>
#include <OpenEditor/OESettings.h>
#include "OpenEditor/OEStreams.h"

    using std::string;
    using OpenEditor::Stream;
    using OpenEditor::InStream;
    using OpenEditor::OutStream;
    using Common::VisualAttribute;
    using Common::VisualAttributesSet;
    using OpenEditor::SettingsSubscriber;

class SQLToolsSettings : public OpenEditor::BaseSettings, public OraMetaDict::WriteSettings
{
    friend class SQLToolsSettingsReader;
    friend class SQLToolsSettingsWriter;
    friend class CPropServerPage;
    friend class CPropGridPage1;
    friend class CPropGridPage2;
    friend class CPropGridOutputPage;
    friend class CPropHistoryPage;
    friend class CDBSCommonPage;
    friend class CPlusPlusPage;
    friend class CExtractSchemaOptionPage;

public:
    enum HistoryAction {
        Copy = 0,
        Paste = 1,
        ReplaceAll = 2
    };

    SQLToolsSettings ();

    OES_DECLARE_PROPERTY(string, DateFormat         );
    OES_DECLARE_PROPERTY(bool,   Autocommit         );
    OES_DECLARE_PROPERTY(int,    CommitOnDisconnect ); // 0-rollback,1-commit,2-confirm
    OES_DECLARE_PROPERTY(bool,   SavePassword       );
    OES_DECLARE_PROPERTY(bool,   OutputEnable       );
    OES_DECLARE_PROPERTY(int,    OutputSize         );
    OES_DECLARE_PROPERTY(bool,   SessionStatistics  );
    OES_DECLARE_PROPERTY(string, SessionStatisticsMode );
    OES_DECLARE_PROPERTY(bool,   ScanForSubstitution);
    OES_DECLARE_PROPERTY(string, PlanTable          );
    OES_DECLARE_PROPERTY(int,    CancelQueryDelay   );
    OES_DECLARE_PROPERTY(bool,   TopmostCancelQuery );
    OES_DECLARE_PROPERTY(bool,   DbmsXplanDisplayCursor);
    OES_DECLARE_PROPERTY(bool,   WhitespaceLineDelim);
    OES_DECLARE_PROPERTY(bool,   EmptyLineDelim);
    OES_DECLARE_PROPERTY(bool,   UnlimitedOutputSize);
    OES_DECLARE_PROPERTY(string, ExternalToolCommand);
    OES_DECLARE_PROPERTY(string, ExternalToolParameters);
    OES_DECLARE_PROPERTY(bool,   HaltOnErrors);
    OES_DECLARE_PROPERTY(bool,   UseDbmsMetaData);
    OES_DECLARE_PROPERTY(bool,   SaveFilesBeforeExecute);
    OES_DECLARE_PROPERTY(bool,   ColumnOrderByName);
    OES_DECLARE_PROPERTY(bool,   EnhancedVisuals);
    OES_DECLARE_PROPERTY(int,    MaxIdentLength);

    OES_DECLARE_PROPERTY(int,    GridMaxColLength        );
    OES_DECLARE_PROPERTY(int,    GridMultilineCount      );
    OES_DECLARE_PROPERTY(string, GridNullRepresentation  );
    OES_DECLARE_PROPERTY(string, GridDateFormat          ); // obsolete
    OES_DECLARE_PROPERTY(bool,   GridAllowLessThanHeader );
    OES_DECLARE_PROPERTY(bool,   GridAllowRememColWidth  );
    OES_DECLARE_PROPERTY(bool,   GridAutoResizeColWidthFetch  );
    OES_DECLARE_PROPERTY(bool,   GridWraparound          );
    OES_DECLARE_PROPERTY(int,    GridColumnFitType       );

    OES_DECLARE_PROPERTY(string, GridNlsNumberFormat     );
    OES_DECLARE_PROPERTY(string, GridNlsDateFormat       );
    OES_DECLARE_PROPERTY(string, GridNlsTimeFormat       );
    OES_DECLARE_PROPERTY(string, GridNlsTimestampFormat  );
    OES_DECLARE_PROPERTY(string, GridNlsTimestampTzFormat);

    OES_DECLARE_PROPERTY(int,    GridExpFormat           );
    OES_DECLARE_PROPERTY(bool,   GridExpWithHeader       );
    OES_DECLARE_PROPERTY(string, GridExpCommaChar        );
    OES_DECLARE_PROPERTY(string, GridExpQuoteChar        );
    OES_DECLARE_PROPERTY(string, GridExpQuoteEscapeChar  );
    OES_DECLARE_PROPERTY(string, GridExpPrefixChar  );
    OES_DECLARE_PROPERTY(bool,   GridExpColumnNameAsAttr );

    OES_DECLARE_PROPERTY(int,    GridPrefetchLimit       );
    OES_DECLARE_PROPERTY(int,    GridMaxLobLength        );
    OES_DECLARE_PROPERTY(int,    GridBlobHexRowLength    );
    OES_DECLARE_PROPERTY(bool,   GridSkipLobs            );
    OES_DECLARE_PROPERTY(string, GridTimestampFormat     );

    OES_DECLARE_PROPERTY(bool,   HistoryEnabled          );
    OES_DECLARE_PROPERTY(int,    HistoryAction           );
    OES_DECLARE_PROPERTY(bool,   HistoryKeepSelection    );
    OES_DECLARE_PROPERTY(int,    HistoryMaxCout          );
    OES_DECLARE_PROPERTY(int,    HistoryMaxSize          );
    OES_DECLARE_PROPERTY(bool,   HistoryValidOnly        );
    OES_DECLARE_PROPERTY(bool,   HistoryDistinct         );
    OES_DECLARE_PROPERTY(bool,   HistoryInherit          );
    OES_DECLARE_PROPERTY(bool,   HistoryPersitent        );
    OES_DECLARE_PROPERTY(bool,   HistoryGlobal           );

    // DDL settings
    bool m_bGroupByDDL;
    bool m_bPreloadDictionary;
    int  m_bPreloadStartPercent;
    bool m_bBodyWithSpec;           // package body with specification
    bool m_bSpecWithBody;           // specification with package body
    // hidden
    OES_DECLARE_PROPERTY(bool, NoPrompts);  // do not generate PROMPTs

    VisualAttributesSet m_QueryVASet;
    VisualAttributesSet m_StatVASet;
    VisualAttributesSet m_OutputVASet;
    VisualAttributesSet m_HistoryVASet;
    VisualAttributesSet m_BindVASet;


    VisualAttributesSet& GetQueryVASet  () { return m_QueryVASet ; }
    VisualAttributesSet& GetStatVASet   () { return m_StatVASet  ; }
    VisualAttributesSet& GetOutputVASet () { return m_OutputVASet; }
    VisualAttributesSet& GetHistoryVASet () { return m_HistoryVASet; }
    VisualAttributesSet& GetBindVASet () { return m_HistoryVASet; }

    const VisualAttributesSet& GetVASet (const string&) const;
};

class SQLToolsSettingsReader
{
public:
    SQLToolsSettingsReader (InStream& in) : m_in(in) {}
    virtual ~SQLToolsSettingsReader () {}

    void operator >> (SQLToolsSettings&);

protected:
    void read (VisualAttributesSet&);
    void read (VisualAttribute&);

    InStream& m_in;
    int       m_version;
};


class SQLToolsSettingsWriter
{
public:
    SQLToolsSettingsWriter (OutStream& out) : m_out(out) {}
    virtual ~SQLToolsSettingsWriter () {}

    void operator << (const SQLToolsSettings&);

protected:
    void write (const VisualAttributesSet&);
    void write (const VisualAttribute&);

    OutStream& m_out;
};

#endif//__SQLToolsSettings_h__