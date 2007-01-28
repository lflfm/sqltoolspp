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

/*
    16.03.2003 bug fix, plsql match sometimes fails after some edit operations
*/

#include "stdafx.h"
#include <map>
#include "OpenEditor/OEHelpers.h"
#include <COMMON/ExceptionHelper.h>
#include "SQLWorksheet/PlsSqlParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


    using std::map;
	OpenEditor::DelimitersMap m_Delimiters(" \t\'\"\\()[]{}+-*/.,!?;:=><%|@&^");

PlsSqlParser::PlsSqlParser (SyntaxAnalyser* analyzer, TokenMapPtr tokenMap)
	: m_analyzer(analyzer),
    m_tokenMap(tokenMap),
    m_sequenceOfLength(0)
{
    m_sequenceOf = eNone;
    SetTokenMap(m_tokenMap);
}

// 16.03.2003 bug fix, plsql match sometimes fails after some edit operations
void PlsSqlParser::Clear ()
{
    m_sequenceOf = eNone;
}

void PlsSqlParser::SetTokenMap (TokenMapPtr tokenMap)
{
    m_tokenMap = tokenMap;
    m_fastmap.erase();
    std::map<string, int>::const_iterator it = m_tokenMap->begin();
    for (; it != m_tokenMap->end(); it++)
		m_fastmap[it->first[0]] = true;
}

void PlsSqlParser::PutEOF (int line)
{
	Token token;
	token = etEOF;
    token.line = line;
    token.offset = 0;
	token.length = 0;
	m_analyzer->PutToken(token);
}

bool PlsSqlParser::PutLine (int line, const char* str, int length)
{
    //TRACE("PutLine: %4d:%s\n", line+1, string(str, length).c_str());

    Token token;

	for (int offset = 0; offset < length; )
    {
        token = etUNKNOWN;
        token.line = line;
        token.offset = offset;
		token.length = 0;

        string buffer;

        if (m_sequenceOf == eNone)
        {
            // skip white space
            for (; offset < length && isspace(str[offset]); offset++)
                {}

            // check EOL
            if (!(offset < length))
                break;

			token.offset = offset;
            // read string token
            if (m_Delimiters[str[offset]])
                buffer += str[offset++];
            else
                for (; offset < length && !m_Delimiters[str[offset]]; offset++)
                    buffer += toupper(str[offset]);

			token.length = buffer.length();
        }
        else
        {
            switch (m_sequenceOf)
            {
            case eEndLineComment:
                m_sequenceOfLength += length - offset;
                offset = length;
                m_sequenceOf = eNone;
				token = m_sequenceToken;
                break;
            case eQuotedString:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    if (str[offset] == '\'')
                    {
                        offset++;
                        m_sequenceOf = eNone;
						token = m_sequenceToken;
                        break;
                    }
                }
                break;
            case eDblQuotedString:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    if (str[offset] == '\"')
                    {
                        offset++;
                        m_sequenceOf = eNone;
						token = m_sequenceToken;
                        break;
                    }
                }
                break;
            case eComment:
                for (; offset < length; offset++)
                {
                    m_sequenceOfLength++;
                    if (offset && str[offset-1] == '*' && str[offset] == '/')
                    {
                        offset++;
                        m_sequenceOf = eNone;
						token = m_sequenceToken;
                        break;
                    }
                }
                break;
            }

            // sequence end
            if (m_sequenceOf == eNone)
            {
                token.length = m_sequenceOfLength;
                m_sequenceOfLength = 0;
            }
        }


        if (token == etUNKNOWN && !buffer.empty() && m_fastmap[buffer.at(0)])
        {
            std::map<string, int>::const_iterator it = m_tokenMap->find(buffer);

            if (it != m_tokenMap->end())
            {
				token = (EToken)it->second;

                switch (token)
                {
                case etQUOTE:
                    m_sequenceOf = eQuotedString; // start string accumulation
					m_sequenceToken = etQUOTED_STRING;
					m_sequenceToken.line = token.line;
					m_sequenceToken.offset = token.offset;
					m_sequenceToken.length = token.length;
                    m_sequenceOfLength = 1;
                    break;
                case etDOUBLE_QUOTE:
                    m_sequenceOf = eDblQuotedString; // start string accumulation
					m_sequenceToken = etDOUBLE_QUOTED_STRING;
					m_sequenceToken.line = token.line;
					m_sequenceToken.offset = token.offset;
					m_sequenceToken.length = token.length;
                    m_sequenceOfLength = 1;
                    break;
                case etMINUS:
                    if (str[offset] == '-')
                    {
                        m_sequenceOf = eEndLineComment; // skip line remainer
						m_sequenceToken = etCOMMENT;
						m_sequenceToken.line = token.line;
						m_sequenceToken.offset = token.offset;
						m_sequenceToken.length = token.length;
                        m_sequenceOfLength = 2;
                        offset++;
                    }
                    break;
                case etSLASH:
                    if (str[offset] == '*')
                    {
                        m_sequenceOf = eComment; // skip comment
						m_sequenceToken = etCOMMENT;
						m_sequenceToken.line = token.line;
						m_sequenceToken.offset = token.offset;
						m_sequenceToken.length = token.length;
                        m_sequenceOfLength = 2;
                        offset++;
                    }
                    break;
                }
            }
        }

        if (m_sequenceOf == eNone)
        {
//			TRACE("PutToken %s(%s)\n", get_token_keyword(token), buffer.c_str());
			m_analyzer->PutToken(token);
        }
    }

    if (m_sequenceOf == eEndLineComment)
        m_sequenceOf = eNone;
	
    if (m_sequenceOf == eNone)
    {
        token = etEOL;
	    token.line = line;
	    token.offset = length;
	    m_sequenceToken.length = 0;
	    m_analyzer->PutToken(token);
    }

    return true;
}
