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

// 07.01.2005 (Ken Clubok) R1092514: SQL*Plus CONNECT/DISCONNECT commands
// 09.01.2005 (ak) bug fix, "&var." does not work
// 09.01.2005 (ak) bug fix, cannot execute "exec null" w/o ending ';'
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 20.03.2005 (ak) sql*plus like behaviour for processing ';/'
// 28.09.2006 (ak) bXXXXXXX: EXEC does not support bind variables

#include "stdafx.h"
#include <assert.h>
#include "CommandParser.h"
#include "OpenEditor\OEHelpers.h"
#include "Common\StrHelpers.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

    std::map<std::string,std::string> CommandParser::m_substitutions;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
/** @brief Constructor for CommandParser.
 *
 * @arg performer: Command Performer - distinguishes between script execution and Explain Plan.
 * @arg substitution: Are substitution variables enabled?
 */
CommandParser::CommandParser (CommandPerformer& performer, bool substitution)
: m_performer(performer),
m_parser(&m_scriptAnalyser, m_scriptAnalyser.getTokenMap()),
m_sqlOnly(false),
m_substitution(substitution)
{
    Start(0, 0);
}

/** @brief Empty Destructor for CommandParser.
 */
CommandParser::~CommandParser ()
{
}

/** @brief Initializes CommandParser.
 *
 * @arg line: Line in editor to begin parsing.
 * @arg column: Column in editor to begin parsing.
 */
void CommandParser::Start (int line, int column)
{
    m_baseLine = line;
    m_baseColumn = column;

    m_lines = 0;
    m_baseErrorLine = 0;

    m_text.clear();
    m_textSubstituted =
    m_exeCommand = false;

    m_parser.Clear();
    m_parser.SetAnalyzer(&m_scriptAnalyser);
    m_parser.SetTokenMap(m_scriptAnalyser.getTokenMap());
}

/** @brief Parses and executes a single command.
 *
 * @arg _str: Contains command to parse.
 * @arg _len: Length of command string.
 */
void CommandParser::PutLine (const char* _str, int _len)
{
    const char* str = _str;
    int len = _len;
    string buffer;

    // make substitution as the first step
    // if a substitution variable is not defined, ask user to input it
    if (m_substitution)
        for (const char* ptr = str, *end = str + len; ptr != end; ptr++)
            if (*ptr == '&')
            {
                makeSubstitution(str, len, buffer);
                str = buffer.c_str();
                len = buffer.length();
                break;
            }

    int currLine = m_baseLine + m_lines++;
    m_parser.PutLine(currLine, str, len);

    if (m_parser.GetAnalyzer() == &m_scriptAnalyser)
    {
        if (m_scriptAnalyser.IsEmpty())
        {
            if (m_parser.IsEmpty())
                Restart();
            // else
                // parser may contain unclosed comments

            return;
        }

        if (m_scriptAnalyser.IsUnrecognized())
        {
            m_parser.Clear();
            m_parser.SetAnalyzer(&m_plsqlAnalyser);
            m_parser.SetTokenMap(m_plsqlAnalyser.getTokenMap());
            m_parser.PutLine(currLine, str, len);
            m_text.append(str, len);
        }
        else
        {
            if (m_sqlOnly)
                throw CommandParserException("Only SQL allowed in this mode!");

            // The parser supports only a single line command.
            // This is a limitation of the implemetation.
            const string text(str, len);
            const std::vector<Token> tokens = m_scriptAnalyser.GetTokens();

            if (m_scriptAnalyser.IsCompleted() && !tokens.empty())
            {
                switch (*tokens.begin())
                {
                case etPROMPT:      doPrompt(tokens, text);         break;
                case etREMARK:      doRemark(tokens, text);         break;
                case etDEFINE:      doDefine(tokens, text);         break;
                case etUNDEFINE:    doUndefine(tokens, text);       break;
                case etEXECUTE:     doExecute(tokens, text);        break;
                case etDESCRIBE:    doDescribe(tokens, text);       break;
				case etCONNECT:		doConnect(tokens, text);		break;
				case etDISCONNECT:	doDisconnect(tokens, text);		break;
				case etVARIABLE:	doVariable(tokens,text);		break;
                default:            doUnsupported(tokens, text);    break;
                }
            }
            else
                doIncompleted(tokens, text);
            return;
        }
    }
    else // emPLSQL
    {
        m_text += '\n';
        m_text.append(str, len);
    }

    if (m_plsqlAnalyser.IsCompleted())
        finalizeAndExecuteSql();
}

void CommandParser::PutEOF ()
{
    if (m_parser.GetAnalyzer() == &m_plsqlAnalyser)
    {
        m_parser.PutEOF(m_baseLine + m_lines);
        finalizeAndExecuteSql();
    }
}

void CommandParser::finalizeAndExecuteSql ()
{
    // 20.03.2005 (ak) sql*plus like behaviour for processing ';/'
    OpenEditor::DelimitersMap delim(" \t/");

    while (!m_text.empty() && delim[*m_text.rbegin()])
        m_text.resize(m_text.size()-1);

    if (!m_plsqlAnalyser.mustHaveClosingSemicolon() 
    && !m_text.empty() && *m_text.rbegin() == ';')
            m_text.resize(m_text.size()-1);

    m_performer.DoExecuteSql(*this, m_text, getBindVariables(m_text, m_plsqlAnalyser.GetBindVarTokens()));
}

void CommandParser::makeSubstitution (const char* str, int len, string& buffer)
{
    const char* ptr = str, *end = str + len;

    while (ptr != end)
    {
        if (*ptr == '&')
        {
            // SQLTools prompts you for a value each time an "&" variable is found,
            // and the first time an "&&" variable is found.
            bool save = false;

            if (*++ptr == '&')
            {
                ptr++; // &&
                save = true;
            }

            string var, val;

            while (ptr != end && (isalnum(*ptr) || *ptr == '_'))
                var += *ptr++;

            if (*ptr == '.') ptr++;

            Common::to_upper_str(var.c_str(), var);

            std::map<std::string,std::string>::const_iterator
                it = m_substitutions.find(var);

            if (it != m_substitutions.end())
                val = it->second;
            else
            {
                m_performer.GoTo(*this, m_baseLine + m_lines);

                if (!m_performer.InputValue(*this, "Enter value for \"" + var + "\":", val))
                    throw CommandParserException("SQLTools: user requested cancel of current operation");

                if (save)
                    m_substitutions[var] = val;
            }

            buffer += val;
            m_textSubstituted = true;
        }
        else
        {
            buffer += *ptr++;
        }
    }
}

    static // TODO: move sub_str into StrHelpers.h
    string sub_str (const string& text, int _line, int _col, int len = -1)
    {
        string::const_iterator it = text.begin();
        for (int line = 0, col = 0; it != text.end(); ++it, col++)
        {
            if (line == _line && col == _col)
                return string(it, len != -1 ? it + len : text.end());
            else if (*it == '\n')
                { col = -1; line++; }
        }
        return string();
    }
    static
    string get_str_by_token (const string& text, const Token& token, int baseLine, int baseColumn)
    {
        int line = token.line - baseLine;
        int column  = line ? token.offset : token.offset + baseColumn;

        string str = sub_str(text, line, column, token.length);

        if (!str.empty() && *str.begin() == '"')
            str = str.substr(1, str.length()-2);
        else
            Common::to_upper_str(str.c_str(), str);

        return str;
    }

void CommandParser::GetCodeName (std::string& owner, std::string& name, std::string& type) const
{
    if (m_parser.GetAnalyzer() == &m_plsqlAnalyser
    && m_plsqlAnalyser.mayHaveCompilationErrors())
    {
        Token ownerToken, nameToken;
        VERIFY(m_plsqlAnalyser.GetNameTokens(ownerToken, nameToken));

        if (ownerToken != etNONE) {
            owner = get_str_by_token(m_text, ownerToken, m_baseLine, m_baseColumn);
           _CHECK_AND_THROW_(!owner.empty(), "CommandParser::GetCodeName: cannot extract object owner.");
        }

        name = get_str_by_token(m_text, nameToken, m_baseLine, m_baseColumn);
        _CHECK_AND_THROW_(!name.empty(), "CommandParser::GetCodeName: cannot extract object name.");

        type = m_plsqlAnalyser.GetSqlType();
    }
}

int CommandParser::GetErrorBaseLine () const
{
    if (m_parser.GetAnalyzer() == &m_plsqlAnalyser)
        return m_plsqlAnalyser.GetCompilationErrorBaseToken().line;
    return m_baseLine;
}

void CommandParser::GetSelectErrorPos (int offset, int& line, int& column) const
{
    line   = 0;
    column = m_baseColumn;
    // Sometimes OCI_ATTR_PARSE_ERROR_OFFSET returns position greater than total length of line.
    // For example this returns position 76 (it should return 40)
    // "BEGIN
    //    SELECT 1 INTO 1 FROM dual WHERE 1=1 
    //  END;"
    // Is this bug of the OCI?
    if(offset > (int)m_text.length())
        offset = m_text.length()-1;// -1 fix -> SQLPlus compatibility

    for (int i = 0; i < offset && m_text.at(i); i++)
    {
        if (m_text.at(i) == '\n') {
            column = 0;
            line++;
        } else
            column++;
    }

    if (line && m_exeCommand) // skip generated "BEGIN\n" for EXECUTE ...
        line--;

    line += m_baseLine;
}

std::vector<std::string> CommandParser::getBindVariables (const string& text, const std::vector<Token>& bindVarTokens) const
{
    std::vector<string> bindVars;

    std::vector<Token>::const_iterator it = bindVarTokens.begin();

    for (; it != bindVarTokens.end(); ++it)
        bindVars.push_back(get_str_by_token(text, *it, m_baseLine, m_baseColumn));

    return bindVars;
}

void CommandParser::doPrompt (const std::vector<Token>&, const string& text)
{
    m_performer.DoMessage(*this, text);
}

void CommandParser::doRemark (const std::vector<Token>&, const string& text)
{
    m_performer.DoMessage(*this, text);
}

void CommandParser::doDefine (const std::vector<Token>& tokens, const string& text)
{
    switch (int size = tokens.size())
    {
    case 0:
        assert(0);
        break;
    case 1:
        // TODO: print a list of all substitution variables including embedded
        //DEFINE _DATE                  = "22-NOV-04" (CHAR)
        //DEFINE _CONNECT_IDENTIFIER    = "" (CHAR)
        //DEFINE _USER                  = "" (CHAR)
        //DEFINE _PRIVILEGE             = "" (CHAR)
        //DEFINE _SQLPLUS_RELEASE       = "1001000200" (CHAR)
        m_performer.DoMessage(*this, "List of all DEFINEs is not implemented yet");
        break;
    case 2:
        {
            string var = sub_str(text, 0, tokens.at(1).offset, tokens.at(1).length);
            Common::to_upper_str(var.c_str(), var);
            std::map<std::string,std::string>::const_iterator it = m_substitutions.find(var);
            if (it != m_substitutions.end())
                m_performer.DoMessage(*this, "DEFINE " + var + "= \"" + it->second + "\"");
            else
                m_performer.DoMessage(*this, "Symbol " + var + " is UNDEFINED");
        }
        break;
    default:
        // this is pretty much siplified because of a signle line command
        {
            string var = sub_str(text, 0, tokens.at(1).offset, tokens.at(1).length);
            Common::to_upper_str(var.c_str(), var);
            string val = sub_str(text, 0, tokens.at(2).offset, tokens.at(2).length);

            if (val != "=")
                throw CommandParserException("DEFINE requires an equal sign (=)");
            if (size < 4)
                throw CommandParserException("DEFINE requires a value following equal sign");

            switch (tokens.at(3))
            {
            case etQUOTED_STRING:
            case etDOUBLE_QUOTED_STRING:
                val = sub_str(text, 0, tokens[3].offset + 1, tokens[3].length -2);
                break;
            default:
                val = sub_str(text, 0, tokens[3].offset, tokens[3].length);
                break;
            }
            m_substitutions[var] = val;
            m_performer.DoMessage(*this, "DEFINE " + var + "= \"" + val + "\"");
        }
        break;
    }
    m_performer.DoNothing(*this);
}

void CommandParser::doUndefine (const std::vector<Token>& tokens, const string& text)
{
    switch (int size = tokens.size())
    {
    case 0: assert(0); break;
    case 1: break; // do nothing
    default:
        {
            string var = sub_str(text, 0, tokens.at(1).offset, tokens.at(1).length);
            Common::to_upper_str(var.c_str(), var);
            std::map<std::string,std::string>::iterator it = m_substitutions.find(var);
            if (it != m_substitutions.end())
                m_substitutions.erase(it);
            m_performer.DoMessage(*this, "UNDEFINE " + var);
        }
        break;
    }
    m_performer.DoNothing(*this);
}

void CommandParser::doExecute (const std::vector<Token>& tokens, const string& text)
{
    m_exeCommand = true;
    m_text = "begin\n" + string(tokens.at(1).offset, ' ') + sub_str(text, 0, tokens.at(1).offset);
    string::size_type pos = m_text.find_last_not_of(" \t\r\n");
    if (pos != string::npos && m_text.at(pos) != ';')
        m_text += ';';
    m_text += "\nend;";
    m_performer.DoExecuteSql(*this, m_text, getBindVariables(text, m_scriptAnalyser.GetBindVarTokens()));
}

// TODO: implement CommandParser::doDescribe
void CommandParser::doDescribe (const std::vector<Token>& tokens, const string& text)
{
    doUnsupported(tokens, text);
}

/** @brief Handles CONNECT command.
 *
 * If a connect string is given, we use that.  Otherwise, display the connect dialog box.
 *
 * @arg tokens: Token vector created by ScriptAnalyzer.
 * @arg text: Command string.
 */
void CommandParser::doConnect (const std::vector<Token>& tokens, const string& text)
{
    if (tokens.size() > 5)
		m_performer.DoConnect(*this, sub_str(text, 0, tokens.at(1).offset));
	else
		m_performer.DoConnect(*this, "");
}

/** @brief Handles DISCONNECT command.
 *
 * @arg tokens: Unused.
 * @arg text: Unused.
 */
void CommandParser::doDisconnect (const std::vector<Token>& /*tokens*/, const string& /*text*/)
{
    m_performer.DoDisconnect(*this);
}

/** @brief Handles VARIABLE command.
 *
 * @arg tokens: Token vector created by ScriptAnalyzer.
 * @arg text: Command string.
 */
#pragma warning (disable : 4706)
void CommandParser::doVariable (const std::vector<Token>& tokens, const string& text)
{
	ub2 i_size = (ub2)-1;
	string var;
    EBindVariableTypes type = EBVT_UNKNOWN;

	switch (int size = tokens.size())
    {
    case 0:
        assert(0);
        break;
    case 1:
		/* Should list all variables, with datatypes */
		break;
	case 2:
		/* Should show variable definition, with datatype */
        break;
    default:
        var = sub_str(text, 0, tokens.at(1).offset, tokens.at(1).length);
        Common::to_upper_str(var.c_str(), var);

		string s_type = sub_str(text, 0, tokens.at(2).offset, tokens.at(2).length);
		Common::to_upper_str(s_type.c_str(), s_type);

// TODO: write a helper class/function for convenient matching of vector<Token> and a sequence of EToken
// somethink like if (match(tokens, (etVARIABLE, etUNKNOWN, etNUMBER))) ...

        if (s_type == "VARCHAR2")
            type = EBVT_VARCHAR2;
		else if (s_type == "NUMBER")
            type = EBVT_NUMBER;
        else if (s_type == "CLOB")
            type = EBVT_CLOB;
        else
			throw CommandParserException(s_type + " - Unrecognized type.");

        // that variable size is legal only for CHAR and VARCHAR2
        if (type == EBVT_VARCHAR2)
        {
            switch (size) // VAR[IABLE] name TYPE (SIZE)
            {
            case 6:
		        if (tokens.at(3) != etLEFT_ROUND_BRACKET && tokens.at(5) != etRIGHT_ROUND_BRACKET)
			        throw CommandParserException("Variable size must be in parentheses.");

		        if (!(i_size = (ub2)atoi(sub_str(text, 0, tokens.at(4).offset, tokens.at(4).length).c_str())))
			        throw CommandParserException("Variable size must be non-zero number.");
                break;
            case 3:
		        throw CommandParserException(("Size is required for \"" + s_type + '\"'));
            default: // > 3
                throw CommandParserException("Invalid variable parameters : \"" + text.substr(tokens.at(3).offset) + '\"');
            }
        }
        else
        {
            if (size > 3)
                throw CommandParserException("Unexpected variable parameters : \"" + text.substr(tokens.at(3).offset) + '\"');
        }

        // TODO: consider increasing the limit to 32767 because it's valid for pl/sql
        if (type == EBVT_VARCHAR2 && i_size > 4000U) 
			throw CommandParserException("VARCHAR2 bind variable length cannot exceed 4000.");
	}

	if (type != EBVT_UNKNOWN)
		m_performer.AddBind(*this, var, type, i_size);
	else
		m_performer.DoNothing(*this);
}
#pragma warning (default : 4706)

void CommandParser::doUnsupported (const std::vector<Token>& tokens, const string& text)
{
    string command = sub_str(text, 0, tokens.at(0).offset, tokens.at(0).length);
    Common::to_upper_str(command.c_str(), command);
    m_performer.DoMessage(*this, command + " is unsupported. Skipped.");
}

void CommandParser::doIncompleted (const std::vector<Token>& tokens, const string& text)
{
    string command = sub_str(text, 0, tokens.at(0).offset, tokens.at(0).length);
    Common::to_upper_str(command.c_str(), command);
    throw CommandParserException((command + " is incomplete").c_str());
}
