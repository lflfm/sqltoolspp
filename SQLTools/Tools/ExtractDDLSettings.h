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

#pragma once
#ifndef __ExtractDDLSettings_H__
#define __ExtractDDLSettings_H__

#include <string>
#include <MetaDict/MetaObjects.h>
#include <OpenEditor/OEStreams.h>

    using std::string;
    using OpenEditor::Stream;
    using OpenEditor::InStream;
    using OpenEditor::OutStream;

class ExtractDDLSettings : public OraMetaDict::WriteSettings
{
public:
    ExtractDDLSettings ();

	bool m_bOptimalViewOrder;
    bool m_bGroupByDDL;

	bool m_bExtractCode;
	bool m_bExtractTables;
	bool m_bExtractTriggers;
	bool m_bExtractViews;
	bool m_bExtractSequences;
	bool m_bExtractSynonyms;
	bool m_bExtractGrantsByGrantee;

    bool m_bUseDbAlias;
    int  m_nUseDbAliasAs;

    std::string	
        m_strSchema,
	    m_strFolder,
        m_strDbAlias,
        m_strTableTablespace,
        m_strIndexTablespace;
};


class ExtractDDLSettingsReader
{
public:
    ExtractDDLSettingsReader (InStream& in) : m_in(in) {}
    virtual ~ExtractDDLSettingsReader () {}

    void operator >> (ExtractDDLSettings&);

protected:
    InStream& m_in;
    int       m_version;
};


class ExtractDDLSettingsWriter
{
public:
    ExtractDDLSettingsWriter (OutStream& out) : m_out(out) {}
    virtual ~ExtractDDLSettingsWriter () {}

    void operator << (const ExtractDDLSettings&);

protected:
    OutStream& m_out;
};

#endif//__ExtractDDLSettings_H__
