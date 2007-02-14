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
#include "OCI8/Connect.h"
#include "OCI8/BCursor.h"
#include <COMMON/ExceptionHelper.h>
#include <COMMON/OsInfo.h>
#include "COMMON/AppGlobal.h"
#include "SQLTools.h"

// 23.06.2003 bug fix, 8.0.5 compatibility
// 26.10.2003 bug fix, any sql statement which is executed after cancelation will be also canceled
// 15.07.2004 bug fix, sqltools crashes if a connection was broken
// 20.11.2004 bug fix, some servers may return very long strings due to oracle bug
// 21.11.2004 bug fix, external authentication does not work
// 09.01.2005 bug fix, memory violation in ~Connect
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 28.09.2006 (ak) bXXXXXXX: poor fetch performance because prefetch rows disabled
// 28.09.2006 (ak) bXXXXXXX: confusing message if password is empty

#pragma comment ( lib, "oci" )

#ifdef _AFX
#   define SHOW_WAIT CWaitCursor wait
#else
#   define SHOW_WAIT
#endif//_AFX

#ifdef OCI_TRACE_STACK_ON_THROW
#define OCI_RAISE(x)	 do { Common::TraceStackOnThrow(); throw (x); } while(false)
#else
#define OCI_RAISE(x)	 do { throw (x); } while(false)
#endif


namespace OCI8
{
///////////////////////////////////////////////////////////////////////////////
// Exception
///////////////////////////////////////////////////////////////////////////////

Exception::Exception ()
: std::exception("OCI8: Cannot allocate OCI handle."),
  m_errcode(0)
{
}

void Exception::CHECK (ConnectBase* conn, sword status)
{
    ASSERT_EXCEPTION_FRAME;

    if (status != OCI_SUCCESS)
    {
#ifdef OCI_TRACE_STACK_ON_THROW
        Common::TraceStackOnThrow();
#endif
        int errcode;
        oratext message[512];
        memset(message, 0, sizeof message);
        OCIErrorGet(conn->GetOCIError(), 1, 0, &errcode, message, sizeof message, OCI_HTYPE_ERROR);

        switch (errcode)
        {
        case 28:
        case 1012:
        case 3113:
        case 3114:
            if (conn->IsOpen())
                conn->Close(true); // connect losed, not logged on
        }

        switch (errcode)
        {
        default:
            throw Exception(errcode, (const char*)message);
        case 1013:
            // 26.10.2003 bug fix, any sql statement which is executed after cancelation will be also canceled
            conn->m_interrupted = false;
            throw UserCancel(errcode, (const char*)message);
        case 1406: // ORA-01406: fetched column value was truncated
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// OCI8::Object
///////////////////////////////////////////////////////////////////////////////
Object::Object (ConnectBase& connect)
: m_connect(connect)
{
    m_connect.Attach(this);
}

Object::~Object ()
{
    try { EXCEPTION_FRAME;

        m_connect.Detach(this);
    }
    _DESTRUCTOR_HANDLER_;
}
///////////////////////////////////////////////////////////////////////////////
// ConnectBase
///////////////////////////////////////////////////////////////////////////////

    #define OCI_DECL_PROC(func) static func##_t g_##func = NULL;
    #define OCI_INIT_PROC(func) if (!g_##func) g_##func = (func##_t)GetProcAddress(g_ociDll, #func);
    #define XML_DECL_PROC(func) static func##_t g_##func = NULL;
    #define XML_INIT_PROC(func) if (!g_##func) g_##func = (func##_t)GetProcAddress(g_xmlDll, #func);

    typedef sword (*OCIEnvCreate_t) (OCIEnv **envp, ub4 mode, dvoid *ctxp,
                    dvoid *(*malocfp)(dvoid *ctxp, size_t size),
                    dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
                    void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
                    size_t xtramem_sz, dvoid **usrmempp);

    typedef sword (*OCIDateTimeToText_t) (dvoid *hndl, OCIError *err, CONST OCIDateTime *date,
                    CONST OraText *fmt, ub1 fmt_length, ub1 fsprec,
                    CONST OraText *lang_name, size_t lang_length,
                    ub4 *buf_size, OraText *buf);

    typedef sword (*OCIIntervalToText_t) (dvoid *hndl, OCIError *err, CONST OCIInterval *inter,
                    ub1 lfprec, ub1 fsprec, OraText *buffer, size_t buflen, size_t *resultlen);

    typedef xmlctx* (*OCIXmlDbInitXmlCtx_t) (OCIEnv*, OCISvcCtx*, OCIError*, ocixmldbparam*, int);
    typedef void   (*OCIXmlDbFreeXmlCtx_t) (xmlctx *xctx);

#ifdef XMLTYPE_SUPPORT
    typedef ubig_ora (*XmlSaveDom_t) (XMLCtx* xctx, XMLErr* err, XMLNode* root, ...);
#endif//XMLTYPE_SUPPORT

    static HINSTANCE g_ociDll = NULL, g_xmlDll = NULL;

    OCI_DECL_PROC(OCIEnvCreate)
#ifdef XMLTYPE_SUPPORT
    OCI_DECL_PROC(OCIXmlDbInitXmlCtx)
    OCI_DECL_PROC(OCIXmlDbFreeXmlCtx)
    XML_DECL_PROC(XmlSaveDom)
#endif//XMLTYPE_SUPPORT
    OCI_DECL_PROC(OCIDateTimeToText)
    OCI_DECL_PROC(OCIIntervalToText)

    EClentVersion ConnectBase::m_clientVersoon = ecvClientUnknown;

bool ConnectBase::IsTimestampSupported ()
{
    return g_OCIDateTimeToText ? true : false;
}

bool ConnectBase::IsIntervalSupported ()
{
    return g_OCIIntervalToText ? true : false;
}

#ifdef XMLTYPE_SUPPORT
bool ConnectBase::IsXMLTypeSupported ()
{
    return (g_OCIXmlDbInitXmlCtx && g_OCIXmlDbFreeXmlCtx && g_XmlSaveDom) ? true : false;
}
#endif//XMLTYPE_SUPPORT

ConnectBase::ConnectBase (unsigned mode)
{
    m_open  = false;
    m_interrupted = false;

    m_envhp = 0;
    m_srvhp = 0;
    m_authp = 0;
    m_svchp = 0;
    m_errhp = 0;
#ifdef XMLTYPE_SUPPORT
    m_xctx  = 0;
    m_xmltype = 0;
#endif//XMLTYPE_SUPPORT

    // 23.06.2003 bug fix, 8.0.5 compatibility

    if (!g_ociDll)
    {
        g_ociDll = LoadLibrary("oci.dll");

        if (g_ociDll)
        {
            OCI_INIT_PROC(OCIEnvCreate);
            OCI_INIT_PROC(OCIDateTimeToText)
#ifdef XMLTYPE_SUPPORT
            OCI_INIT_PROC(OCIXmlDbInitXmlCtx);
            OCI_INIT_PROC(OCIXmlDbFreeXmlCtx);
#endif//XMLTYPE_SUPPORT
            OCI_INIT_PROC(OCIIntervalToText);
            
            string version = Common::getDllVersionProperty("OCI.DLL", "FileVersion");

            if (!version.empty())
            {
                if (!strncmp(version.c_str(),"10", sizeof("10")-1))        m_clientVersoon = ecvClient10X;
                else if (!strncmp(version.c_str(),"9", sizeof("9")-1))     m_clientVersoon = ecvClient9X;
                else if (!strncmp(version.c_str(),"8.1", sizeof("8.1")-1)) m_clientVersoon = ecvClient81X;
                else if (!strncmp(version.c_str(),"8.0", sizeof("8.0")-1)) m_clientVersoon = ecvClient80X;
            }
            
        }
    }

#ifdef XMLTYPE_SUPPORT
    if (g_OCIXmlDbInitXmlCtx && !g_xmlDll)
    {
        g_xmlDll = LoadLibrary("oraxml10.dll");

        XML_INIT_PROC(XmlSaveDom);
    }
#endif//XMLTYPE_SUPPORT

    if (g_OCIEnvCreate)
    {
        CHECK_ALLOC(g_OCIEnvCreate((OCIEnv**)&m_envhp, mode, 0, 0, 0, 0, 0, 0));
    }
    else
    {
        static bool g_ociInitialized = false;
        if (!g_ociInitialized)
        {
            OCIInitialize(mode, 0, 0, 0, 0);
            g_ociInitialized = true;
        }
        CHECK_ALLOC(OCIEnvInit((OCIEnv**)&m_envhp, OCI_DEFAULT, 0, 0));
    }
}

ConnectBase::~ConnectBase ()
{
    try { EXCEPTION_FRAME;

        if (m_open) Close();

        OCIHandleFree(m_envhp, OCI_HTYPE_ENV);
    }
    _DESTRUCTOR_HANDLER_;
}

void ConnectBase::Open (const char* uid, const char* pswd, const char* alias, EConnectionMode mode, ESafety safety)
{
    SHOW_WAIT;

    m_uid.erase();
    m_password.erase();
    m_alias.erase();

    m_mode = mode;
    m_safety = safety;

    m_ext_auth = false;

    if ((!uid || !strlen(uid)) && (!pswd || !strlen(pswd)) && (alias && strlen(alias)))
    {
        m_alias = alias;
        m_ext_auth = true;
    }
    else if ((!pswd || !strlen(pswd)) || (!alias || !strlen(alias)))
    {
        const char *ptr = uid;

        for ( ; *ptr && *ptr != '/' && *ptr != '@'; ptr++)
            m_uid += *ptr;

        if (*ptr == '/')
        {
            ++ptr;
            for ( ; *ptr && *ptr != '@'; ptr++)
                m_password += *ptr;
        }

        if (*ptr == '@')
        {
            ++ptr;
            for ( ; *ptr && *ptr != '@'; ptr++)
                m_alias += *ptr;
        }

        if (alias || strlen(alias))
        {
            if (!m_alias.empty())
                OCI_RAISE(Exception(0, "Connect::Open: alias cannot be defined twice!"));
            m_alias = alias;
        }
    }
    else
    {
        if (uid)   m_uid = uid;
        if (pswd)  m_password = pswd;
        if (alias) m_alias = alias;
     }

    CHECK_ALLOC(OCIHandleAlloc(m_envhp, (dvoid**)&m_errhp, OCI_HTYPE_ERROR, 0, 0));
    CHECK_ALLOC(OCIHandleAlloc(m_envhp, (dvoid**)&m_srvhp, OCI_HTYPE_SERVER, 0, 0));
    CHECK(OCIServerAttach(m_srvhp, m_errhp, (OraText*)m_alias.c_str(), m_alias.length(), 0));

    CHECK_ALLOC(OCIHandleAlloc(m_envhp, (dvoid**)&m_authp, (ub4)OCI_HTYPE_SESSION, 0, 0));
    CHECK_ALLOC(OCIHandleAlloc(m_envhp, (dvoid**)&m_auth_shadowp, (ub4)OCI_HTYPE_SESSION, 0, 0));

    if (!m_uid.empty())
        CHECK(OCIAttrSet(m_authp, OCI_HTYPE_SESSION,
            (OraText*)m_uid.c_str(), m_uid.length(), OCI_ATTR_USERNAME, m_errhp));
        CHECK(OCIAttrSet(m_auth_shadowp, OCI_HTYPE_SESSION,
            (OraText*)m_uid.c_str(), m_uid.length(), OCI_ATTR_USERNAME, m_errhp));
    if (!m_password.empty())
        CHECK(OCIAttrSet(m_authp, OCI_HTYPE_SESSION,
            (OraText*)m_password.c_str(), m_password.length(), OCI_ATTR_PASSWORD, m_errhp));
        CHECK(OCIAttrSet(m_auth_shadowp, OCI_HTYPE_SESSION,
            (OraText*)m_password.c_str(), m_password.length(), OCI_ATTR_PASSWORD, m_errhp));

    CHECK_ALLOC(OCIHandleAlloc(m_envhp, (dvoid **)&m_svchp, OCI_HTYPE_SVCCTX, 0, 0));
    CHECK(OCIAttrSet(m_svchp, OCI_HTYPE_SVCCTX, m_srvhp, 0, OCI_ATTR_SERVER, m_errhp));
    CHECK(OCISessionBegin(m_svchp, m_errhp, m_authp, m_ext_auth ? OCI_CRED_EXT : OCI_CRED_RDBMS, m_mode));
	if (GetSQLToolsSettings().GetDbmsXplanDisplayCursor() || GetSQLToolsSettings().GetSessionStatistics())
	{
		CHECK(OCISessionBegin(m_svchp, m_errhp, m_auth_shadowp, m_ext_auth ? OCI_CRED_EXT : OCI_CRED_RDBMS, m_mode));
		m_openShadow = true;
    }
	else
		m_openShadow = false;
    CHECK(OCIAttrSet(m_svchp, OCI_HTYPE_SVCCTX, m_authp, 0, OCI_ATTR_SESSION, m_errhp));

#ifdef XMLTYPE_SUPPORT
    if (IsXMLTypeSupported())
    {
        OCIDuration dur = OCI_DURATION_SESSION;
        ocixmldbparam params[2];
        params[0].name_ocixmldbparam = XCTXINIT_OCIDUR;
        params[0].value_ocixmldbparam = &dur;
        m_xctx = g_OCIXmlDbInitXmlCtx(m_envhp, m_svchp, m_errhp, params, 1);

        CHECK(
            OCITypeByName(m_envhp, m_errhp, m_svchp,
                (const OraText*)"SYS", sizeof("SYS")-1, (const OraText*)"XMLTYPE", sizeof("XMLTYPE")-1,
                (const OraText*)0, 0, OCI_DURATION_SESSION, OCI_TYPEGET_HEADER, &m_xmltype));
    }
#endif//XMLTYPE_SUPPORT

	m_open = true;
}

void ConnectBase::CheckShadowSession()
{
	if (GetSQLToolsSettings().GetDbmsXplanDisplayCursor() || GetSQLToolsSettings().GetSessionStatistics())
	{
		if (! m_openShadow)
		{
			CHECK(OCISessionBegin(m_svchp, m_errhp, m_auth_shadowp, m_ext_auth ? OCI_CRED_EXT : OCI_CRED_RDBMS, m_mode));
			m_openShadow = true;
	    }
    }
	else
	{
		if (m_openShadow)
		{
			CHECK(OCISessionEnd(m_svchp, m_errhp, m_auth_shadowp, OCI_DEFAULT));
			m_openShadow = false;
		}
	}
}

void ConnectBase::SetSession()
{
    CHECK(OCIAttrSet(m_svchp, OCI_HTYPE_SVCCTX, m_authp, 0, OCI_ATTR_SESSION, m_errhp));
}

void ConnectBase::SetShadowSession()
{
    CHECK(OCIAttrSet(m_svchp, OCI_HTYPE_SVCCTX, m_auth_shadowp, 0, OCI_ATTR_SESSION, m_errhp));
}

void ConnectBase::Close (bool purge)
{
    SHOW_WAIT;

    STACK_OVERFLOW_GUARD(3);

	if (m_open)
    {
#ifdef XMLTYPE_SUPPORT
        if (m_xctx && IsXMLTypeSupported())
        {
            g_OCIXmlDbFreeXmlCtx(m_xctx);
            m_xctx = 0;
            try { CHECK(OCIObjectUnpin(m_envhp, m_errhp, m_xmltype)); }
            catch (const Exception&) { if (!purge) throw; }
            m_xmltype = 0;
        }
#endif//XMLTYPE_SUPPORT

        set<Object*>::iterator i = m_dependencies.begin();
        while (i != m_dependencies.end())
            (*i++)->Close(purge);

        try { CHECK(OCISessionEnd(m_svchp, m_errhp, m_authp, OCI_DEFAULT)); }
        catch (const Exception&) { if (!purge) throw; }
		if (m_openShadow)
		{
			try { CHECK(OCISessionEnd(m_svchp, m_errhp, m_auth_shadowp, OCI_DEFAULT)); }
			catch (const Exception&) { if (!purge) throw; }
			m_openShadow = false;
	    }
        try { CHECK(OCIServerDetach(m_srvhp, m_errhp, OCI_DEFAULT)); }
        catch (const Exception&) { if (!purge) throw; }

        OCIHandleFree(m_svchp, OCI_HTYPE_SVCCTX);
        OCIHandleFree(m_authp, OCI_HTYPE_SESSION);
        OCIHandleFree(m_auth_shadowp, OCI_HTYPE_SESSION);
        OCIHandleFree(m_srvhp, OCI_HTYPE_SERVER);
        OCIHandleFree(m_errhp, OCI_HTYPE_ERROR);

        m_open = false;
    }
}

void ConnectBase::Commit (bool guaranteedSafe)
{
    ASSERT_EXCEPTION_FRAME;

    if (!guaranteedSafe && GetSafety() == esReadOnly)
        OCI_RAISE(Exception(0, "ConnectBase::Commit: the operation is not allowed for Read-Only connection!"));

    SHOW_WAIT;
    CHECK_INTERRUPT();
    CHECK(OCITransCommit(m_svchp, m_errhp, (ub4) 0));
}

void ConnectBase::Rollback ()
{
    ASSERT_EXCEPTION_FRAME;

    //if (GetSafety() == esReadOnly)
    //    OCI_RAISE(Exception(0, "ConnectBase::Rollback: the operation is allowed for Read-Only connection!"));

    SHOW_WAIT;
    CHECK_INTERRUPT();
    CHECK(OCITransRollback(m_svchp, m_errhp, (ub4) 0));
}

void ConnectBase::Break (bool purge)
{
    if (IsOpen())
    {
        m_interrupted = true;

        if (purge)
            OCIBreak(m_svchp, m_errhp);
        else
            CHECK(OCIBreak(m_svchp, m_errhp));
    }
}

void ConnectBase::Attach (Object* obj)
{
    m_dependencies.insert(obj);
}

void ConnectBase::Detach (Object* obj)
{
    m_dependencies.erase(obj);
}

// the following methods are available for OCI 8.1.X and later
void ConnectBase::DateTimeToText (const OCIDateTime* date, const char* fmt, size_t fmt_length, int fsprec,
                              const char* lang_name, size_t lang_length, char* buf, size_t* buf_size)
{
    ASSERT_EXCEPTION_FRAME;

    if (!g_OCIDateTimeToText)
        OCI_RAISE(Exception(0, "ConnectBase::DateTimeToText: OCIDateTimeToText is not available!"));

    CHECK(g_OCIDateTimeToText(m_envhp, m_errhp, date, (const OraText*)fmt, (ub1)fmt_length,
        (ub1)fsprec, (const OraText*)lang_name, lang_length, buf_size, (OraText*)buf));
}

void ConnectBase::IntervalToText (const OCIInterval* inter, int lfprec, int fsprec, char* buf, size_t buf_size, size_t* result_len)
{
    ASSERT_EXCEPTION_FRAME;

    if (!g_OCIIntervalToText)
        OCI_RAISE(Exception(0, "ConnectBase::IntervalToText: OCIIntervalToText is not available!"));

    CHECK(g_OCIIntervalToText(m_envhp, m_errhp, inter, (ub1)lfprec, (ub1)fsprec, (OraText*)buf, buf_size, result_len));
}

#ifdef XMLTYPE_SUPPORT
// the following method are available for OCI 10.X
size_t ConnectBase::XmlSaveDom (XMLNode* node, char* buffer, size_t buffer_size)
{
    ASSERT_EXCEPTION_FRAME;

    if (!g_XmlSaveDom)
        OCI_RAISE(Exception(0, "ConnectBase::XmlSaveDom: XmlSaveDom is not available!"));

    XMLErr err;
    // ubig_ora len = currently always 0 because of oracle bug
    g_XmlSaveDom(m_xctx, &err, node, "buffer", buffer, "buffer_length", buffer_size, NULL);

    if (err != XMLERR_OK)
    {
        if (err == XMLERR_SAVE_OVERFLOW)
            strncpy(buffer, "XMLERR_SAVE_OVERFLOW", buffer_size);
        else if (err == XMLERR_NULL_PTR)
            strncpy(buffer, "XMLERR_NULL_PTR", buffer_size);
        else
            strncpy(buffer, "XMLERR_UNKNOWN", buffer_size);

        buffer[buffer_size-1] = 0;
    }

    return strlen(buffer);
}
#endif//XMLTYPE_SUPPORT

// private methods
void ConnectBase::check_alloc (sword status)
{
    ASSERT_EXCEPTION_FRAME;

    if (status != OCI_SUCCESS)
        OCI_RAISE(Exception());
}

void ConnectBase::check (sword status)
{
    Exception::CHECK(this, status);
}

void ConnectBase::check_interrupt ()
{
    ASSERT_EXCEPTION_FRAME;

    if (m_interrupted)
    {
        m_interrupted = false;
        OCI_RAISE(UserCancel(0, "SQLTools: user requested cancel of current operation"));
    }
}

void ConnectBase::MakeTNSString (std::string& str, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid)
{
    str = std::string("(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(Host=") + host + ")(Port=" + port + ")))"
        "(CONNECT_DATA=(" + (serviceInsteadOfSid ? "SERVICE_NAME" : "SID") + "=" + sid +")))";
}

///////////////////////////////////////////////////////////////////////////////
// Connect
///////////////////////////////////////////////////////////////////////////////

Connect::Connect ()
{
    m_OutputEnable = false;
    m_bypassTns = false;
    m_OutputSize = 20000L;
}

Connect::~Connect ()
{
}

void Connect::Open (const char* uid, const char* pswd, const char* alias, EConnectionMode mode, ESafety safety)
{
    m_evBeforeOpen.Handle(*this);

    m_bypassTns = false;
    m_strHost.clear();
    m_strPort.clear();
    m_strSid.clear();

    m_strGlobalName.erase();
    m_strVersion.erase();
	m_sessionSid.clear();

    ConnectBase::Open(uid, pswd, alias, mode, safety);

    LoadSessionNlsParameters();
    AlterSessionNlsParams();
    EnableOutput(m_OutputEnable, m_OutputSize, true);
	GetSessionSid();
	if (IsOpenShadow())
		SetShadowClientInfo();

	// AfxMessageBox((string("SID: ") + string(m_sessionSid)).c_str());

    m_evAfterOpen.Handle(*this);
}

void Connect::Open (const char* uid, const char* pswd, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid, EConnectionMode mode, ESafety safety)
{
    m_evBeforeOpen.Handle(*this);

    m_bypassTns = true;
    m_strHost = host;
    m_strPort = port;
    m_strSid  = sid;

    m_strGlobalName.erase();
    m_strVersion.erase();
	m_sessionSid.clear();

    string alias;
    MakeTNSString(alias, host, port, sid, serviceInsteadOfSid);
    ConnectBase::Open(uid, pswd, alias.c_str(), mode, safety);

    LoadSessionNlsParameters();
    AlterSessionNlsParams();
    EnableOutput(m_OutputEnable, m_OutputSize, true);
	GetSessionSid();
	if (IsOpenShadow())
		SetShadowClientInfo();

    m_evAfterOpen.Handle(*this);
}

void Connect::Close (bool purge)
{
	if (IsOpen())
    {
        // 15.07.2004 bug fix, sqltools crashes if a connection was broken
        if (!purge)
            m_evBeforeClose.Handle(*this);

        ConnectBase::Close(purge);

        m_strGlobalName.erase();
        m_strVersion.erase();
		m_sessionSid.erase();

        m_evAfterClose.Handle(*this);
    }
}

std::string Connect::GetDisplayString (bool mode) const
{
    std::string displayStr = GetUID();
    displayStr += '@';

    if (!m_bypassTns)
    {
        displayStr += GetAlias();
    }
    else
    {
        displayStr += m_strSid;
        displayStr += ':';
        displayStr += m_strHost;
    }

    if (mode)
    {
        switch (GetMode())
        {
        case OCI8::ecmSysDba:  displayStr += " as sysdba"; break;
        case OCI8::ecmSysOper: displayStr += " as sysoper"; break;
        }
    }

    return displayStr;
}

void Connect::ExecuteStatement (const char* sttm, bool guaranteedSafe)
{
    SHOW_WAIT;
    Statement cursor(*this);
	cursor.Prepare(sttm);
	cursor.Execute(1, guaranteedSafe);
}

void Connect::ExecuteShadowStatement (const char* sttm, bool guaranteedSafe)
{
    SHOW_WAIT;
    Statement cursor(*this);
	cursor.Prepare(sttm);
	cursor.ExecuteShadow(1, guaranteedSafe);
}

void Connect::EnableOutput (bool enable, unsigned long size, bool connectInit)
{
    // 22.03.2003 small improvement, removed redundant server calls
    if (m_OutputEnable != enable || m_OutputEnable && connectInit || m_OutputSize != size)
    {
        if (size != ULONG_MAX)
            m_OutputSize = size;

	    if (IsOpen())
        {
            if (enable)
            {
                char buffer[80];
                sprintf(buffer, "BEGIN dbms_output.enable(%ld); END;", m_OutputSize);
                ExecuteStatement(buffer, true);
            }
            else
            {
                if (!connectInit) // avoid server call because it's default
                    ExecuteStatement("BEGIN dbms_output.disable; END;", true);
            }
        }

        m_OutputEnable = enable;
    }
}

const char* Connect::GetGlobalName () // throw Exception
{
    if (!m_strGlobalName.size())
    {
        BuffCursor cursor(*this);
        cursor.Prepare("SELECT global_name FROM sys.global_name");
        cursor.Execute();
	    cursor.Fetch();
        cursor.GetString(0, m_strGlobalName);
    }
    return m_strGlobalName.c_str();
}

const char* Connect::GetSessionSid () // throw Exception
{
	if (! m_sessionSid.size())
	{
        try
        {
			BuffCursor cursor(*this);
			cursor.Prepare("SELECT sid FROM v$mystat where rownum <= 1");
			cursor.Execute();
			cursor.Fetch();
			cursor.GetString(0, m_sessionSid);
        }
        catch (const Exception& x)
        {
            MessageBeep(MB_ICONHAND);
			AfxMessageBox((string("Error: ") + x.what() + string("reading sid from v$mystat.")).c_str());
		}
	}

	return m_sessionSid.c_str();
}

const char* Connect::GetVersionStr () // throw Exception
{
    if (!m_strVersion.size())
    {
        try
        {
            Statement cursor(*this);
            StringVar version(1024), compatibility(1024); // 20.11.2004 bug fix, some servers may return very long strings due to oracle bug
	        cursor.Prepare("BEGIN dbms_utility.db_version(:version, :compatibility); END;");
            cursor.Bind(":version", version);
            cursor.Bind(":compatibility", compatibility);
	        cursor.Execute(1, true);
            version.GetString(m_strVersion);
        }
        catch (const Exception& x)
        {
            if (x == 6550) // if server is 7 then use v$version view
            {
                BuffCursor cursor(*this);
                cursor.Prepare("SELECT SubStr(banner, InStr(banner,'.')-1,"
                                            "InStr(banner,'.',1,3)-InStr(banner,'.')+1) version "
                                   "FROM v$version WHERE Upper(banner) LIKE '%ORACLE%'");
                cursor.Execute();
	            cursor.Fetch();
                cursor.GetString(0, m_strVersion);
            }
            else
                throw;
        }
    }
    return m_strVersion.c_str();
}

EServerVersion Connect::GetVersion ()
{
    EServerVersion version = esvServerUnknown;

    if (!m_strVersion.empty())
    {
        if (!strncmp(m_strVersion.c_str(),"10", sizeof("10")-1))        version = esvServer10X;
        else if (!strncmp(m_strVersion.c_str(),"9", sizeof("9")-1))     version = esvServer9X;
        else if (!strncmp(m_strVersion.c_str(),"8.1", sizeof("8.1")-1)) version = esvServer81X;
        else if (!strncmp(m_strVersion.c_str(),"8.0", sizeof("8.0")-1)) version = esvServer80X;
        else if (!strncmp(m_strVersion.c_str(),"7.3", sizeof("7.3")-1)) version = esvServer73X;
        else if (!strncmp(m_strVersion.c_str(),"7", sizeof("7")-1))     version = esvServer7X;
    }

    return version;
}

void Connect::LoadSessionNlsParameters ()
{
    m_dbNlsLanguage.erase();
    m_dbNlsNumericCharacters.erase();
    m_dbNlsDateFormat.erase();
    m_dbNlsDateLanguage.erase();
    m_dbNlsTimeFormat.erase();
    m_dbNlsTimestampFormat.erase();
    m_dbNlsTimeTzFormat.erase();
    m_dbNlsTimestampTzFormat.erase();

    BuffCursor cursor(*this);
    cursor.Prepare("SELECT parameter, value FROM sys.nls_session_parameters");
    cursor.Execute();

    while (cursor.Fetch())
    {
        string parameter, value;
        cursor.GetString(0, parameter);
        cursor.GetString(1, value);

        if (parameter == "NLS_LANGUAGE")
            m_dbNlsLanguage = value;
        else if (parameter == "NLS_NUMERIC_CHARACTERS")
            m_dbNlsNumericCharacters = value;
        else if (parameter == "NLS_DATE_FORMAT")
            m_dbNlsDateFormat = value;
        else if (parameter == "NLS_DATE_LANGUAGE")
            m_dbNlsDateLanguage = value;
        else if (parameter == "NLS_TIME_FORMAT")
            m_dbNlsTimeFormat = value;
        else if (parameter == "NLS_TIMESTAMP_FORMAT")
            m_dbNlsTimestampFormat = value;
        else if (parameter == "NLS_TIME_TZ_FORMAT")
            m_dbNlsTimeTzFormat = value;
        else if (parameter == "NLS_TIMESTAMP_TZ_FORMAT")
            m_dbNlsTimestampTzFormat = value;
    }
}

void Connect::RetrieveCurrentSqlInfo()
{
	m_CurrentSqlAddress.clear();
	m_CurrentSqlHashValue.clear();
	m_CurrentSqlChildNumber.clear();
	m_CurrentSqlID.clear();

	try
	{
		BuffCursor cursor(*this);
		if (GetVersion() >= esvServer10X)
			cursor.Prepare("SELECT rawtohex(sql_address) as sql_address, sql_hash_value, rawtohex(prev_sql_addr) as prev_sql_addr, prev_hash_value, sql_id, prev_sql_id, sql_child_number, prev_child_number from v$session where sid = :sid");
		else
			cursor.Prepare("SELECT rawtohex(sql_address) as sql_address, sql_hash_value, rawtohex(prev_sql_addr) as prev_sql_addr, prev_hash_value from v$session where sid = :sid");
		cursor.Bind(":sid", GetSessionSid());
		cursor.ExecuteShadow();

		while (cursor.Fetch())
		{
			string sql_address, sql_hash_value, prev_sql_addr, prev_hash_value;
			string sql_id, prev_sql_id, sql_child_number, prev_child_number;
			cursor.GetString(0, sql_address);
			cursor.GetString(1, sql_hash_value);
			cursor.GetString(2, prev_sql_addr);
			cursor.GetString(3, prev_hash_value);
			if (GetVersion() >= esvServer10X)
			{
				cursor.GetString(4, sql_id);
				cursor.GetString(5, prev_sql_id);
				cursor.GetString(6, sql_child_number);
				cursor.GetString(7, prev_child_number);
			}

			if (sql_address == "00")
			{
				m_CurrentSqlAddress = prev_sql_addr;
				m_CurrentSqlHashValue = prev_hash_value;
				if (GetVersion() >= esvServer10X)
				{
					m_CurrentSqlID = prev_sql_id;
					m_CurrentSqlChildNumber = prev_child_number;
				}
			}
			else
			{
				m_CurrentSqlAddress = sql_address;
				m_CurrentSqlHashValue = sql_hash_value;
				if (GetVersion() >= esvServer10X)
				{
					m_CurrentSqlID = sql_id;
					m_CurrentSqlChildNumber = sql_child_number;
				}
			}
		}

		cursor.Close();
    }
    catch (const Exception& x)
    {
		Global::SetStatusText(string("Error: ") + x.what() + string(" retrieving current sql address from v$session..."), TRUE);
    }

	SetSession();
}

void Connect::AlterSessionNlsParams ()
{
    if (IsOpen()
    && !m_NlsDateFormat.empty()
    && m_NlsDateFormat != m_dbNlsDateFormat)
    {
        string buff = "ALTER SESSION SET nls_date_format = \'";
        buff += m_NlsDateFormat;
        buff += '\'';

        ExecuteStatement(buff.c_str(), true);

        m_dbNlsDateFormat = m_NlsDateFormat;
    }
}

void Connect::CheckShadowSession()
{
	ConnectBase::CheckShadowSession();

	if (IsOpenShadow())
		SetShadowClientInfo();
}

void Connect::SetShadowClientInfo ()
{
    if (IsOpen())
    {
        string buff = "BEGIN DBMS_APPLICATION_INFO.SET_CLIENT_INFO('SQLTools_pp background session'); END;";

        ExecuteShadowStatement(buff.c_str(), true);
    }
}

};
