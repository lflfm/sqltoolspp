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

// 13.12.2004 bug fix, '/' might be recognized as a statement separator at any position
// 13.03.2005 (ak) R1105003: Bind variables
// 20.03.2005 (ak) bug fix, unnecessary bug report on statement, like this "select * from dual;;"
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#include "stdafx.h"
#include <sstream>
#include <assert.h>
#include "PlSqlAnalyser.h"

    // tokens for pl/sql, probably it can be reduced
	static struct
    {
		EToken token;
		const char* keyword;
	}
    g_plsqlTokens[] =
    {
		etDECLARE,				"DECLARE",
		etFUNCTION,				"FUNCTION",
		etPROCEDURE,			"PROCEDURE",
		etPACKAGE,				"PACKAGE",
		etBODY,					"BODY",
		etBEGIN,				"BEGIN",
		etEXCEPTION,			"EXCEPTION",
		etEND,					"END",
		etIF,					"IF",
		etTHEN,					"THEN",
		etELSE,					"ELSE",
		etELSIF,				"ELSIF",
		etFOR,					"FOR",
		etWHILE,				"WHILE",
		etLOOP,					"LOOP",
		etEXIT,					"EXIT",
		etIS,					"IS",
		etAS,					"AS",
		etSEMICOLON,			";",
		etQUOTE,				"'",
		etDOUBLE_QUOTE,			"\"",
		etLEFT_ROUND_BRACKET,	"(",
		etRIGHT_ROUND_BRACKET,	")",
		etMINUS,				"-",
		etSLASH,				"/",
		etSTAR,					"*",
		etSELECT,				"SELECT",
		etINSERT,				"INSERT",
		etUPDATE,				"UPDATE",
		etDELETE,				"DELETE",
		etALTER,				"ALTER",
		etANALYZE,				"ANALYZE",
		etCREATE,				"CREATE",
		etDROP,					"DROP",
		etFROM,					"FROM",
		etWHERE,				"WHERE",
		etSET,					"SET",
		etOPEN,                 "OPEN",
        etLANGUAGE,             "LANGUAGE",
        etLESS,                 "<",
        etGREATER,              ">",
        etTRIGGER,              "TRIGGER",
        etTYPE,                 "TYPE",
        etOR,                   "OR",
        etREPLACE,              "REPLACE",
        etDOT,                  ".",
        etCOLON,			    ":",
        etEQUAL,                "="
	};


TokenMapPtr PlSqlAnalyser::getTokenMap ()
{
    TokenMapPtr map(new std::map<std::string, int>);
	for (int i(0); i < sizeof(g_plsqlTokens)/sizeof(g_plsqlTokens[0]); i++)
		map->insert(std::map<string, int>::value_type(g_plsqlTokens[i].keyword, g_plsqlTokens[i].token));
    return map;
}

    const int RECOGNITION_LENGTH = 6;

    struct PlSqlAnalyser::RecognitionSeq
    {
        PlSqlAnalyser::ESQLType sqlType;
        EToken tokens[RECOGNITION_LENGTH];
    }
    PlSqlAnalyser::g_recognitionSeq[] =
    {
        { BLOCK,        etLESS,    etLESS,      etNONE,    etNONE,      etNONE, etNONE },
        { BLOCK,        etDECLARE, etNONE,      etNONE,    etNONE,      etNONE, etNONE },
        { BLOCK,        etBEGIN,   etNONE,      etNONE,    etNONE,      etNONE, etNONE },
        { FUNCTION,     etCREATE,  etFUNCTION,  etNONE,    etNONE,      etNONE, etNONE },
        { PACKAGE_BODY, etCREATE,  etPACKAGE,   etBODY,    etNONE,      etNONE, etNONE },
        { PACKAGE,      etCREATE,  etPACKAGE,   etNONE,    etNONE,      etNONE, etNONE },
        { PROCEDURE,    etCREATE,  etPROCEDURE, etNONE,    etNONE,      etNONE, etNONE },
        { TRIGGER,      etCREATE,  etTRIGGER,   etNONE,    etNONE,      etNONE, etNONE },
        { TYPE_BODY,    etCREATE,  etTYPE,      etBODY,    etNONE,      etNONE, etNONE },
        { TYPE,         etCREATE,  etTYPE,      etNONE,    etNONE,      etNONE, etNONE },
        { FUNCTION,     etCREATE,  etOR,        etREPLACE, etFUNCTION,  etNONE, etNONE },
        { PACKAGE_BODY, etCREATE,  etOR,        etREPLACE, etPACKAGE,   etBODY, etNONE },
        { PACKAGE,      etCREATE,  etOR,        etREPLACE, etPACKAGE,   etNONE, etNONE },
        { PROCEDURE,    etCREATE,  etOR,        etREPLACE, etPROCEDURE, etNONE, etNONE },
        { TRIGGER,      etCREATE,  etOR,        etREPLACE, etTRIGGER,   etNONE, etNONE },
        { TYPE_BODY,    etCREATE,  etOR,        etREPLACE, etTYPE,      etBODY, etNONE },
        { TYPE,         etCREATE,  etOR,        etREPLACE, etTYPE,      etNONE, etNONE },
    };

PlSqlAnalyser::PlSqlAnalyser ()
: m_state(TYPE_RECOGNITION), m_sqlType(SQL)
{
}

void PlSqlAnalyser::Clear ()
{
    m_sqlType = SQL;
    m_state = TYPE_RECOGNITION;
    m_tokens.clear();
    m_bindVarTokens.clear();
    m_nameTokens[0] = Token();
    m_nameTokens[1] = Token();
    m_lastToken = Token();
}

const char* PlSqlAnalyser::GetSqlType () const
{
    switch (m_sqlType)
    {
    case SQL:          return "SQL";
    case BLOCK:        return "BLOCK";
    case FUNCTION:     return "FUNCTION";
    case PROCEDURE:    return "PROCEDURE";
    case PACKAGE:      return "PACKAGE";
    case PACKAGE_BODY: return "PACKAGE BODY";
    case TRIGGER:      return "TRIGGER";
    case TYPE:         return "TYPE";
    case TYPE_BODY:    return "TYPE BODY";
    }
    assert(0);
    return "SQL";
}

bool PlSqlAnalyser::mustHaveClosingSemicolon () const
{
    switch (m_sqlType)
    {
    case SQL:
        return false;
    case BLOCK:
    case FUNCTION:
    case PROCEDURE:
    case PACKAGE:
    case PACKAGE_BODY:
    case TRIGGER:
    case TYPE:
    case TYPE_BODY:
        return true;
    }
    assert(0);
    return false;
}

bool PlSqlAnalyser::mayHaveCompilationErrors () const
{
    switch (m_sqlType)
    {
    case SQL:
    case BLOCK:
        return false;
    case FUNCTION:
    case PROCEDURE:
    case PACKAGE:
    case PACKAGE_BODY:
    case TRIGGER:
    case TYPE:
    case TYPE_BODY:
        return true;
    }
    assert(0);
    return false;
}

bool PlSqlAnalyser::GetNameTokens (Token& owner, Token& name) const
{
    if (m_nameTokens[1] != etNONE)
    {
        owner = m_nameTokens[0];
        name  = m_nameTokens[1];
        return true;
    }
    else if (m_nameTokens[0] != etNONE)
    {
        name  = m_nameTokens[0];
        return true;
    }
    return false;
}

Token PlSqlAnalyser::GetCompilationErrorBaseToken () const
{
    std::vector<Token>::const_reverse_iterator it = m_tokens.rbegin();

    if (it != m_tokens.rend())
    {
        if (*it == etBODY)
            ++it;

        if (it != m_tokens.rend())
            return *it;
    }

    return Token();
}

void PlSqlAnalyser::PutToken (const Token& token)
{
    if (token == etCOMMENT) return;

    switch (m_state)
    {
    case TYPE_RECOGNITION:
        switch (m_sqlType)
        {
        case SQL: // unknown sql type
            if (token != etEOL) // skip EOL
            {
                m_tokens.push_back(token);
                ESQLType sqlType;
                switch (findMatch(sqlType))
                {
                case INCOMPLETE_MATCH:  // incomplete math
                case EXACT_MATCH:       // math, but we'll try again because a longer sequence may exist
                    break;              // do nothing;
                case NO_MATCH:          // no match but check the previous sequence
                    m_tokens.pop_back();

                    if (findMatch(sqlType) == EXACT_MATCH)
                    {
                        m_sqlType = sqlType;
                        m_state = NAME_RECOGNITION;
                    }
                    else
                        m_state = PENDING;
                }
            }
        }
        if (m_state != NAME_RECOGNITION)
            break;
    case NAME_RECOGNITION:
        {
            if (m_nameTokens[0] == etNONE)
                m_nameTokens[0] = token;
            else if (token != etDOT && m_lastToken == etDOT)
                m_nameTokens[1] = token;
            else if (token != etDOT)
                m_state = PENDING;
        }
        break;
    case PENDING:
        switch (m_sqlType)
        {
        case SQL:
            if (token == etSEMICOLON
            // 13.12.2004 bug fix, '/' might be recognized as a statement separator at any position
            || (token == etSLASH && !token.offset)
			/*|| (token == etEOL && token.offset == 0)*/)
                m_state = COMPLETED;
            break;
        case TRIGGER:
            if (*m_tokens.rbegin() == etTRIGGER
            && (token == etDECLARE || token == etBEGIN))
                m_tokens.push_back(token);
        default:
            if (token == etEOF
            // 13.12.2004 bug fix, '/' might be recognized as a statement separator at any position
            || (token == etSLASH && !token.offset))
                m_state = COMPLETED;
        }
        break;
    
    // 20.03.2005 (ak) bug fix, unnecessary bug report on statement, like this "select * from dual;;"
    // The following section was commented out because std::exception is handled as a critical application error
    // PlSqlAnalyser and CommandParser cannot process this problem as an input error so will pass such statement 
    // to a server and receive oracle error responce (sql*plus behaviour)

    //case COMPLETED:
    //    if (token != etEOL && token != etEOF) // skip EOL and etEOF
    //    {
    //        std::ostringstream out;
    //        out << "Unexpected token at (" << token.line << ',' << token.offset << ')';
    //        ASSERT_EXCEPTION_FRAME;
    //        throw std::exception(out.str().c_str());
    //    }
    };

    if (m_lastToken == etCOLON && token != etEQUAL)
        m_bindVarTokens.push_back(token);

    m_lastToken = token;
}

PlSqlAnalyser::EMatch
PlSqlAnalyser::findMatch (ESQLType& sqlType) const
{
    EToken tokens[RECOGNITION_LENGTH] = { etNONE, etNONE, etNONE, etNONE, etNONE, etNONE };
    std::vector<Token>::const_iterator it = m_tokens.begin();
    for (int i = 0; i < RECOGNITION_LENGTH && it != m_tokens.end(); ++it, i++)
        tokens[i] = *it;

    EMatch retCode = NO_MATCH;
    for (int i = 0; i < sizeof(g_recognitionSeq)/sizeof(g_recognitionSeq[0]); i++)
    {
        bool exact_match = true;
        bool match = true;

        for (int j = 0; j <  RECOGNITION_LENGTH; j++)
        {
            if (g_recognitionSeq[i].tokens[j] != tokens[j]) {
                exact_match = false;

                if (tokens[j] == etNONE) // it's not complete match
                    break;

                match = false;
            }
        }

        if (exact_match) {
            sqlType = g_recognitionSeq[i].sqlType;
            retCode = EXACT_MATCH;
            break;
        }

        if (match)
            retCode = INCOMPLETE_MATCH;
    }
    return retCode;
}

