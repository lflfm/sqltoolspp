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

// 28.09.2006 (ak) bXXXXXXX: poor fetch performance because prefetch rows disabled

#pragma once
#pragma warning ( disable : 4430 )
#include <oci.h>

#ifdef XMLTYPE_SUPPORT
extern "C" {
#include <xmlproc.h>
#include <oraxml.h>
}
#endif//XMLTYPE_SUPPORT

#include <set>
#include <string>
#include <COMMON/Handler.h>
#include <MetaDict/MetaDictionary.h>

namespace OCI8
{
    using std::set;
    using std::string;

#ifdef XMLTYPE_SUPPORT
    typedef xmlctx XMLCtx;
    typedef xmlnode XMLNode;
    typedef xmlerr XMLErr;
#endif//XMLTYPE_SUPPORT

    class Object;
    class ConnectBase;

///////////////////////////////////////////////////////////////////////////////
// OciException
///////////////////////////////////////////////////////////////////////////////
class Exception : public std::exception
{
    int m_errcode;
public:
    Exception ();

    Exception (int code, const char* msg)
        : std::exception(msg), m_errcode(code) {}

    Exception (const Exception& other)
        : std::exception(other), m_errcode(other.m_errcode) {}

    operator const char* () const   { return what(); }
    operator int () const           { return m_errcode; }

    static void CHECK (ConnectBase*, sword status);
};

class UserCancel : public Exception
{
public:
    UserCancel (int code, const char* msg)
        : Exception(code, msg) {}
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::Object
///////////////////////////////////////////////////////////////////////////////
class Object
{
public:
    virtual void Close (bool purge = false) = 0;

protected:
    Object (ConnectBase&);
    virtual ~Object ();

    ConnectBase& m_connect;
private:
    // prohibited
    Object (const Object&);
    operator = (const Object&);
};

///////////////////////////////////////////////////////////////////////////////
// ConnectBase
///////////////////////////////////////////////////////////////////////////////

    enum EConnectionMode
    {
        ecmDefault = OCI_DEFAULT,
        ecmSysDba  = OCI_SYSDBA,
        ecmSysOper = OCI_SYSOPER
    };

    enum ESafety
    {
        esNone,
        esWarnings,
        esReadOnly,
    };

    enum EServerVersion
    {
        esvServerUnknown,
        esvServer7X,
        esvServer73X,
        esvServer80X,
        esvServer81X,
        esvServer9X,
        esvServer10X,
        esvServer11X
    };

    enum EClentVersion
    {
        ecvClientUnknown,
        ecvClient80X,
        ecvClient81X,
        ecvClient9X,
        ecvClient10X,
        ecvClient11X
    };


class ConnectBase
{
    friend class Exception;
protected:
    ConnectBase  (unsigned mode = OCI_THREADED|OCI_DEFAULT|OCI_OBJECT);
    virtual ~ConnectBase ();

    virtual void Open  (const char* uid, const char* pswd, const char* alias, EConnectionMode mode, ESafety safety);
    virtual void Close (bool purge = false);
    virtual void Reconnect();

public:
    virtual void Commit (bool guaranteedSafe = false);
    virtual void Rollback ();
    virtual void Break (bool purge);

	void CheckShadowSession(const bool bForceConnectShadow = false);
	void SetSession();
	void SetShadowSession();
    bool IsClosing() { return m_IsClosing; }

    OraMetaDict::ObjectLookupCache & GetObjectLookupCache() {return m_ObjectLookupCache;};

    OCIEnv*    GetOCIEnv ()     { return m_envhp; }
    OCISvcCtx* GetOCISvcCtx ()  { return m_svchp; }
    OCIError*  GetOCIError ()   { return m_errhp; }

#ifdef XMLTYPE_SUPPORT
    OCIType*       GetXMLType ()        { return m_xmltype; }
    const OCIType* GetXMLType () const  { return m_xmltype; }
#endif//XMLTYPE_SUPPORT

    bool IsOpen () const        { return m_open; }
    bool IsOpenShadow () const        { return m_openShadow; }

    const char* GetUID () const         { return m_uid.c_str(); }
    const char* GetPassword () const    { return m_password.c_str(); }
    const char* GetAlias () const       { return m_alias.c_str(); }

    EConnectionMode GetMode () const    { return m_mode; }
    ESafety         GetSafety () const  { return m_safety; }

    void CHECK_ALLOC (sword status) { ASSERT_EXCEPTION_FRAME; if (status != OCI_SUCCESS) check_alloc(status); }
    void CHECK (sword status)       { ASSERT_EXCEPTION_FRAME; if (status != OCI_SUCCESS) check(status); }
    void CHECK_INTERRUPT ()         { ASSERT_EXCEPTION_FRAME; if (m_interrupted) check_interrupt(); }

    void Attach (Object*);
    void Detach (Object*);

    static void MakeTNSString (std::string& str, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid = false);
    static EClentVersion GetClientVersion() { return m_clientVersoon; }

    static bool IsTimestampSupported ();
    static bool IsIntervalSupported ();
#ifdef XMLTYPE_SUPPORT
    static bool IsXMLTypeSupported ();
#endif//XMLTYPE_SUPPORT

    // the following methods are available for OCI 8.1.X and later
    void DateTimeToText (const OCIDateTime* date, const char* ftm, size_t fmt_length, int fsprec,
                         const char* lang_name, size_t lang_length, char* buf, size_t* buf_size);
    void IntervalToText (const OCIInterval* inter, int lfprec, int fsprec,
                         char* buf,  size_t buf_size, size_t* result_len);

#ifdef XMLTYPE_SUPPORT
    // the following method are available for OCI 10.X
    size_t XmlSaveDom (XMLNode*, char* buffer, size_t buffer_size);
#endif//XMLTYPE_SUPPORT

private:
    virtual void check_alloc (sword status);
    virtual void check (sword status);
    virtual void check_interrupt ();
    virtual void DoOpen();

    static EClentVersion m_clientVersoon;

    OCIEnv*     m_envhp;
    OCIServer*  m_srvhp;
    OCISession* m_authp;
    OCISession* m_auth_shadowp;
    OCISvcCtx*	m_svchp;
    OCIError*	m_errhp;
#ifdef XMLTYPE_SUPPORT
    XMLCtx*     m_xctx;
    OCIType*    m_xmltype;
#endif//XMLTYPE_SUPPORT


    bool m_open, m_interrupted, m_openShadow, m_ext_auth, m_IsClosing;
    string m_uid, m_password, m_alias;
    EConnectionMode m_mode;
    ESafety m_safety;
    OraMetaDict::ObjectLookupCache m_ObjectLookupCache;

    set<Object*> m_dependencies;

private:
    // copy-constraction & assign-operation is not supported
    ConnectBase (const ConnectBase&);
    operator = (const ConnectBase&);
};


///////////////////////////////////////////////////////////////////////////////
// Connect
///////////////////////////////////////////////////////////////////////////////
class Connect : public ConnectBase
{
public:
    Common::Handler<Connect> 
        m_evBeforeOpen, m_evAfterOpen,
        m_evBeforeClose, m_evAfterClose;

    Connect ();
    ~Connect ();

    void Open  (const char* uid, const char* pswd, const char* alias, EConnectionMode mode, ESafety safety);
    void Open  (const char* uid, const char* pswd, const char* host, const char* port, const char* sid, bool serviceInsteadOfSid, EConnectionMode mode, ESafety safety);
    void Close (bool purge = false);
    void Reconnect();

    void ExecuteStatement (const char*, bool guaranteedSafe = false);
    void ExecuteShadowStatement (const char*, bool guaranteedSafe = false);

    void EnableOutput (bool enable = true, unsigned long size = ULONG_MAX, bool = false);
    void ResetOutput ();
    bool IsOutputEnabled () const;

    const char* GetConnectString () const { return GetAlias(); }
    std::string GetDisplayString (bool mode = false) const;

    const char* GetGlobalName ();               // throw Exception;
    const char* GetVersionStr ();               // throw Exception;
    const char* GetSessionSid ();               // throw Exception;
    EServerVersion GetVersion ();               // throw Exception;

	void RetrieveCurrentSqlInfo();
	const string& GetCurrentSqlAddress() const {return m_CurrentSqlAddress;}
	const string& GetCurrentSqlHashValue() const {return m_CurrentSqlHashValue;}
	const string& GetCurrentSqlChildNumber() const {return m_CurrentSqlChildNumber;}
	const string& GetCurrentSqlID() const {return m_CurrentSqlID;}
    // NLS parameters
    void LoadSessionNlsParameters ();
protected:
    bool m_OutputEnable;
    bool m_UnlimitedOutputSize;
    unsigned long m_OutputSize;
    string m_strVersion, m_strGlobalName;
    string m_strHost, m_strPort, m_strSid;
    bool m_bypassTns;
	string m_sessionSid;
	string m_CurrentSqlAddress;
	string m_CurrentSqlHashValue;
	string m_CurrentSqlChildNumber;
	string m_CurrentSqlID;
    bool m_GetSIDFailed;


#define NLS_PARAMETER(param) \
    protected: \
    string m_##param, m_db##param; \
    public: \
    const string& Get##param () const   { return !m_##param.empty() ? m_##param : m_db##param; } \
    void Set##param (const string& val) { m_##param = val; } \
    void Set##param (const char* val)   { m_##param = val; }

    NLS_PARAMETER(NlsLanguage);             //NLS_LANGUAGE
    NLS_PARAMETER(NlsNumericCharacters);    //NLS_NUMERIC_CHARACTERS
    NLS_PARAMETER(NlsDateFormat);           //NLS_DATE_FORMAT
    NLS_PARAMETER(NlsDateLanguage);         //NLS_DATE_LANGUAGE
    NLS_PARAMETER(NlsTimeFormat);           //NLS_TIME_FORMAT
    NLS_PARAMETER(NlsTimestampFormat);      //NLS_TIMESTAMP_FORMAT
    NLS_PARAMETER(NlsTimeTzFormat);         //NLS_TIME_TZ_FORMAT
    NLS_PARAMETER(NlsTimestampTzFormat);    //NLS_TIMESTAMP_TZ_FORMAT

public:
    void AlterSessionNlsParams (); // changes session NLS_DATE_FORMAT only
	void CheckShadowSession(const bool bForceConnectShadow = false);
	void SetShadowClientInfo();

private:
    // copy-constraction & assign-operation is not supported
    Connect (const Connect&);
    operator = (const Connect&);
};

//////////////////////////////////////////////////////////////////////////

inline
bool Connect::IsOutputEnabled () const
{
    return m_OutputEnable;
}

inline
void Connect::ResetOutput ()
{
    EnableOutput(m_OutputEnable);
}

};