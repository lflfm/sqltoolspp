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
#include "arg_shared.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\MetaDictionary.h"
#include "ExtractSchemaHelpers.h"
#include <OCI8/BCursor.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;
using namespace Common;
using namespace OraMetaDict;
using arg::counted_ptr;


/// helper class ////////////////////////////////////////////////////////////

    class DpdBuilder
    {
    public:
        DpdBuilder (const Dictionary&, vector<const DbObject*>&);

        void AddDependency (const char* owner, const char* name,
                            const char* ref_owner, const char* ref_name);

        void Unwind ();

    private:
        struct DpdNode
        {
            DbObject& m_object;                   // dictionary object reference
            int     m_refCount;                   // number of dependent objects
            std::vector<DpdNode*> m_dependencies; // used "OWNER.NAME" format
            bool    m_lock;                       // algorithm checking support

            DpdNode (DbObject& object);
            void AddDependency (DpdNode& node);
        };

        typedef counted_ptr<DpdNode>       DpdNodePtr;
        typedef map<string, DpdNodePtr>    DpdNodeMap;
        typedef DpdNodeMap::iterator       DpdNodeMapIter;
        typedef DpdNodeMap::const_iterator DpdNodeMapConstIter;

        void unwind (DpdNode& node);

        int m_Cycle, m_MaxCycle;
        DpdNodeMap m_mapNodes;
        vector<const DbObject*>& m_vecResult;
    };

/////////////////////////////////////////////////////////////////////////////

    inline
    DpdBuilder::
    DpdNode::DpdNode (DbObject& object)
    : m_object(object),
      m_refCount(0),
      m_lock(false) {}

    inline
    void DpdBuilder::
    DpdNode::AddDependency (DpdNode& node)
    {
        m_dependencies.push_back(&node);
        node.m_refCount++;
    }

    DpdBuilder::DpdBuilder (const Dictionary& dict, vector<const DbObject*>& result)
    : m_Cycle(0),
    m_MaxCycle(10000),
    m_vecResult(result)
    {
        ViewMapConstIter it, end;
        dict.InitIterators (it, end);

        for (; it != end; it++) {
            counted_ptr<DpdNode> ptr(new DpdNode(*(*it).second.get()));
            m_mapNodes.insert(DpdNodeMap::value_type(((*it).first), ptr));
        }
    }

    void DpdBuilder::AddDependency (const char* owner, const char* name,
                                    const char* ref_owner, const char* ref_name)
    {
        DpdNodeMapIter it;
        char key[cnDbKeySize];

        it = m_mapNodes.find(make_key(key, owner, name));
        if (it == m_mapNodes.end()) throw XNotFound(key);
        DpdNode& node1 = *(*it).second.get();

        it = m_mapNodes.find(make_key(key, ref_owner, ref_name));

        if (it != m_mapNodes.end()) // ref_obj can be not accessible
        {
            DpdNode& node2 = *(*it).second.get();
            node1.AddDependency(node2);
        }
    }

    void DpdBuilder::unwind (DpdNode& node)
    {
        _CHECK_AND_THROW_(m_Cycle++ < m_MaxCycle, "Unwind dependencies algorithm error!");

        std::vector<DpdNode*>::iterator
            it(node.m_dependencies.begin()),
            end(node.m_dependencies.end());

        for (; it != end; it++)
            unwind(**it);

        if (node.m_lock) return;
        m_vecResult.push_back(&node.m_object);
        node.m_lock = true;
    }

    void DpdBuilder::Unwind ()
    {
        DpdNodeMapIter it(m_mapNodes.begin()), end(m_mapNodes.end());
        for (; it != end; it++)
            if ((*it).second.get()->m_refCount == 0)
                unwind(*(*it).second.get());
    }


/// optimal view creation order builder /////////////////////////////////////

    LPSCSTR cszDpdSttm =
        "SELECT owner, name, referenced_owner, referenced_name FROM sys.all_dependencies"
          " WHERE owner = :p_owner"
            " AND referenced_owner = :p_owner"
            " AND type = 'VIEW'"
            " AND referenced_type = 'VIEW'";

    const int cn_owner      = 0;
    const int cn_name       = 1;
    const int cn_ref_owner  = 2;
    const int cn_ref_name   = 3;

void build_optimal_view_order (const Dictionary& dict, const char* schema,
                               OciConnect& connect, vector<const DbObject*>& result)
{
    DpdBuilder builder(dict, result);

    OciCursor cursor(connect, cszDpdSttm, 256);
    cursor.Bind(":p_owner", schema);

    cursor.Execute();
    while (cursor.Fetch())
        builder.AddDependency(
            cursor.ToString(cn_owner).c_str(),
            cursor.ToString(cn_name).c_str(),
            cursor.ToString(cn_ref_owner).c_str(),
            cursor.ToString(cn_ref_name).c_str());

    builder.Unwind();
}
