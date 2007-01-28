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
#include "ExtractDDLSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ExtractDDLSettings::ExtractDDLSettings ()
{
    // start of OraMetaDict::WriteSettings
    m_bComments =               TRUE;
    m_bGrants =                 TRUE; 
    m_bLowerNames =             TRUE; 
    m_bShemaName =              FALSE; 
    m_bNoStorageForConstraint = FALSE;
    m_nStorageClause =          1;
    m_bAlwaysPutTablespace =    FALSE;
    m_bCommentsAfterColumn =    FALSE;
    m_nCommentsPos =            48;
    m_bSequnceWithStart =       FALSE;
    m_bViewWithForce =          FALSE;
	m_bSQLPlusCompatibility =   TRUE;
    m_bConstraints =            FALSE;
    m_bIndexes =                FALSE; 
    m_bTableDefinition =        TRUE; 
    m_bTriggers =               FALSE; 
    m_bViewWithTriggers =       FALSE;
    // end of OraMetaDict::WriteSettings
    m_bGroupByDDL =             TRUE;
    
    m_bUseDbAlias             = FALSE;
    m_nUseDbAliasAs           = 0;
    m_bOptimalViewOrder       = TRUE;
	m_bExtractCode            = TRUE;
	m_bExtractTables          = TRUE;
	m_bExtractTriggers        = TRUE;
	m_bExtractViews           = TRUE;
	m_bExtractSequences       = TRUE;
	m_bExtractSynonyms        = TRUE;
	m_bExtractGrantsByGrantee = TRUE;
};


    const int TheSettingsVersion = 1001;

void ExtractDDLSettingsReader::operator >> (ExtractDDLSettings& settings)
{
    m_version = 0;
    m_in.read("Version", m_version);

    _CHECK_AND_THROW_(m_version >= 1001 && m_version <= TheSettingsVersion, "Unsupported settings version!");

    m_in.read("Comments",               settings.m_bComments              );
    m_in.read("Grants",                 settings.m_bGrants                );
    m_in.read("LowerNames",             settings.m_bLowerNames            );
    m_in.read("ShemaName",              settings.m_bShemaName             );
    m_in.read("NoStorageForConstraint", settings.m_bNoStorageForConstraint); 
    m_in.read("StorageClause",          settings.m_nStorageClause         );
    m_in.read("AlwaysPutTablespace",    settings.m_bAlwaysPutTablespace   );
    m_in.read("CommentsAfterColumn",    settings.m_bCommentsAfterColumn   );
    m_in.read("CommentsPos",            settings.m_nCommentsPos           );
    m_in.read("SequnceWithStart",       settings.m_bSequnceWithStart      );
    m_in.read("ViewWithForce",          settings.m_bViewWithForce         );
    m_in.read("SQLPlusCompatibility",   settings.m_bSQLPlusCompatibility  );
    m_in.read("Constraints",            settings.m_bConstraints           );
    m_in.read("Indexes",                settings.m_bIndexes               );
    m_in.read("TableDefinition",        settings.m_bTableDefinition       );
    m_in.read("Triggers",               settings.m_bTriggers              );
    m_in.read("ViewWithTriggers",       settings.m_bViewWithTriggers      );
    m_in.read("GroupByDDL",             settings.m_bGroupByDDL            );
    m_in.read("UseDbAliasAs",           settings.m_nUseDbAliasAs          ); 
    m_in.read("OptimalViewOrder",       settings.m_bOptimalViewOrder      ); 
    m_in.read("ExtractCode",            settings.m_bExtractCode           ); 
    m_in.read("ExtractTables",          settings.m_bExtractTables         ); 
    m_in.read("ExtractTriggers",        settings.m_bExtractTriggers       ); 
    m_in.read("ExtractViews",           settings.m_bExtractViews          ); 
    m_in.read("ExtractSequences",       settings.m_bExtractSequences      ); 
    m_in.read("ExtractSynonyms",        settings.m_bExtractSynonyms       ); 
    m_in.read("ExtractGrantsByGrantee", settings.m_bExtractGrantsByGrantee); 
}

void ExtractDDLSettingsWriter::operator << (const ExtractDDLSettings& settings)
{
    m_out.write("Version", TheSettingsVersion);

    m_out.write("Comments",               settings.m_bComments              );
    m_out.write("Grants",                 settings.m_bGrants                );
    m_out.write("LowerNames",             settings.m_bLowerNames            );
    m_out.write("ShemaName",              settings.m_bShemaName             );
    m_out.write("NoStorageForConstraint", settings.m_bNoStorageForConstraint); 
    m_out.write("StorageClause",          settings.m_nStorageClause         );
    m_out.write("AlwaysPutTablespace",    settings.m_bAlwaysPutTablespace   );
    m_out.write("CommentsAfterColumn",    settings.m_bCommentsAfterColumn   );
    m_out.write("CommentsPos",            settings.m_nCommentsPos           );
    m_out.write("SequnceWithStart",       settings.m_bSequnceWithStart      );
    m_out.write("ViewWithForce",          settings.m_bViewWithForce         );
    m_out.write("SQLPlusCompatibility",   settings.m_bSQLPlusCompatibility  );
    m_out.write("Constraints",            settings.m_bConstraints           );
    m_out.write("Indexes",                settings.m_bIndexes               );
    m_out.write("TableDefinition",        settings.m_bTableDefinition       );
    m_out.write("Triggers",               settings.m_bTriggers              );
    m_out.write("ViewWithTriggers",       settings.m_bViewWithTriggers      );
    m_out.write("GroupByDDL",             settings.m_bGroupByDDL            );
    m_out.write("UseDbAliasAs",           settings.m_nUseDbAliasAs          ); 
    m_out.write("OptimalViewOrder",       settings.m_bOptimalViewOrder      ); 
    m_out.write("ExtractCode",            settings.m_bExtractCode           ); 
    m_out.write("ExtractTables",          settings.m_bExtractTables         ); 
    m_out.write("ExtractTriggers",        settings.m_bExtractTriggers       ); 
    m_out.write("ExtractViews",           settings.m_bExtractViews          ); 
    m_out.write("ExtractSequences",       settings.m_bExtractSequences      ); 
    m_out.write("ExtractSynonyms",        settings.m_bExtractSynonyms       ); 
    m_out.write("ExtractGrantsByGrantee", settings.m_bExtractGrantsByGrantee); 
}
