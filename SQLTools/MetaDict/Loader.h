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

#ifndef __LOADER_H__
#define __LOADER_H__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )

//
// Purpose: Class for loading oracle db objects in meta-dictionary.
// Note:    Reload the same object is not supported.
//          Copy-constraction & assign-operation is not supported.
//

#include <OCI8/Connect.h>


namespace OraMetaDict 
{
    class Dictionary;
    class WriteSettings;
    using namespace std;

    class Loader : OCI8::Object
    {
    public:
        Loader (OCI8::Connect&, Dictionary&);

        // init private data & load shared objects (tablespaces & users) into the dictionary
        void Init ();
        // reset state on disconnect
        virtual void Close (bool purge = false);

        // load procedures
        void LoadClusters         (const char* owner, const char* name, bool useLike);
        void LoadTables           (const char* owner, const char* name, bool useLike);
        void LoadRefFkConstraints (const char* owner, const char* name, bool useLike);
        void LoadTriggers         (const char* owner, const char* name, bool byTables, bool useLike);
        void LoadViews            (const char* owner, const char* name, bool useLike);
        void LoadSequences        (const char* owner, const char* name, bool useLike);
        void LoadSynonyms         (const char* owner, const char* name, bool useLike);
        void LoadFunctions        (const char* owner, const char* name, bool useLike);
        void LoadProcedures       (const char* owner, const char* name, bool useLike);
        void LoadPackages         (const char* owner, const char* name, bool useLike);
        void LoadPackageBodies    (const char* owner, const char* name, bool useLike);
        void LoadTypes            (const char* owner, const char* name, bool useLike);
        void LoadTypeBodies       (const char* owner, const char* name, bool useLike);
        
        void LoadObjects (const char* owner, const char* name, const char* type, const WriteSettings&, 
                          bool specWithBody, bool bodyWithSpec, bool useLike);
        
        void LoadGrantByGrantors  (const char* owner, const char* name, bool useLike);
        void LoadGrantByGranteies (const char* owner, const char* grantee, bool useLike);

        void LoadDBLinks          (const char* owner, const char* name, bool useLike);
        void LoadSnapshotLogs     (const char* owner, const char* name, bool useLike);
        void LoadSnapshots        (const char* owner, const char* name, bool useLike);

        void LoadIndexes          (const char* owner, const char* name, bool useLike);
        void LoadConstraints      (const char* owner, const char* name, bool useLike);

        static bool IsYes (const char*);
        static bool IsYes (const string&);
    private:
        void loadPlsCodes    (const char* owner, const char* name, const char* type, bool useLike);
        void loadIndexes     (const char* owner, const char* name, bool byTable, bool isCluster, bool useLike);
        void loadConstraints (const char* owner, const char* name, bool byTable, bool useLike);

        Dictionary& m_dictionary;
        OCI8::Connect& m_connect;
        
        bool m_isOpen; 
        std::string m_strCurSchema;

        // copy-constraction & assign-operation is not supported
        Loader (const Loader&);
        operator = (const Loader&);
    };

    /////////////////////////////////////////////////////////////////////////////

    inline
    void Loader::LoadFunctions (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "FUNCTION", useLike); }

    inline
    void Loader::LoadProcedures (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "PROCEDURE", useLike); }

    inline
    void Loader::LoadPackages   (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "PACKAGE", useLike); }

    inline
    void Loader::LoadPackageBodies (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "PACKAGE BODY", useLike); }

    inline
    void Loader::LoadTypes   (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "TYPE", useLike); }

    inline
    void Loader::LoadTypeBodies (const char* owner, const char* name, bool useLike)
        { loadPlsCodes(owner, name, "TYPE BODY", useLike); }

}  // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__LOADER_H__
