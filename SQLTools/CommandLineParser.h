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

// 03.01.2005 (ak) added static method CCommandLineParser::GetConnectionInfo

#pragma once

#include <string>
#include <vector>
#include <iostream>

class CCommandLineParser
{
public:
    CCommandLineParser () { Clear (); }

    void Clear ();
    bool Parse ();
    void Pack (std::string& data) const;
    void Unpack (const char*, int);
    void Process ();

    bool GetConnectOption () const { return m_connect; }
    bool GetNologOption   () const { return m_nolog;   }
    bool GetStartOption   () const { return m_start;   }
    bool GetReuseOption   () const { return m_reuse;   }
    bool GetHelpOption    () const { return m_help;    }

    const std::vector<std::string>& GetFiles () const   { return m_files; }

    static
    bool GetConnectionInfo (const std::string& connectStr, std::string& user, std::string& password, std::string& alias, std::string& port, std::string& sid, std::string& mode);
    bool GetConnectionInfo (std::string& user, std::string& password, std::string& alias, std::string& port, std::string& sid, std::string& mode) const;

    void SetStartingDefaults ();

private:
    bool check_option (const char* str, const char* option);
    bool check_option (const char* str, const char* option, std::string& arg);

    bool m_error;
    int  m_errorPos;
    std::string m_errorMessage;

    bool m_connect;
    bool m_nolog;
    bool m_start;
    bool m_reuse;
    bool m_help;

    std::string m_connectStr;
    std::vector<std::string> m_files;
};

inline
bool CCommandLineParser::GetConnectionInfo (std::string& user, std::string& password, std::string& alias, std::string& port, std::string& sid, std::string& mode) const
{
    return GetConnectionInfo(m_connectStr, user, password, alias, port, sid, mode);
}
