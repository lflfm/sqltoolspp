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

// 05.06.2003 small improvement, thousand delemiters for cost,cardinality and bytes in Explain Plan viewer 
// 07.12.2003 bug fix, explain plan result may contain duplicated records if you press F9 too fast
// 07.12.2003 bug fix, explain plan table is not cleared after fetching result

#include "stdafx.h"
#include "SQLTools.h"
#include "SQLWorksheetDoc.h"
#include <SQLWorksheet\CommandParser.h>
//#include "ObjectTreeBuilder.h"
#include "MainFrm.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "SQLToolsSettings.h"
#include <OCI8/BCursor.h>
#include "ExplainPlanView.h"

#include "XPlanView.h"
#include "Booklet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc

const int ps_operation = 0;
const int ps_options   = 1;

const int ps_object_name     = 2;
const int ps_object_instance = 3;
const int ps_object_type     = 4;

const int ps_id          = 5;
const int ps_parent_id   = 6;

const int ps_optimizer   = 7;
const int ps_cost        = 8;
const int ps_cardinality = 9;
const int ps_bytes       = 10;

static const char expl_xplan[] = "select plan_table_output from table(dbms_xplan.display('<PLAN_TABLE>', :sttm_id, 'ALL'))";

static const char expl_xplan_display_cursor[] = "select plan_table_output from table(dbms_xplan.display_cursor(:sql_id, :child_number, 'ALLSTATS LAST'))";

LPSCSTR expl_sttm[2] = {
    "SELECT operation," //  0
            "options,"        //  1
            "object_name,"    //  2
            "object_instance,"//  3
            "object_type,"    //  4
            "id,"             //  5
            "parent_id "      //  6
            //"optimizer,"      //  7 only for 7.3
            //"cost,"           //  8 only for 7.3
            //"cardinality,"    //  9 only for 7.3
            //"bytes "          // 10 only for 7.3
        "FROM <PLAN_TABLE> "
            "WHERE statement_id = :sttm_id "
            "ORDER BY id, position",

        "SELECT operation," //  0
            "options,"        //  1
            "object_name,"    //  2
            "object_instance,"//  3
            "object_type,"    //  4
            "id,"             //  5
            "parent_id,"      //  6
            "optimizer,"      //  7 only for 7.3
            "to_char(cost,'FM999g999g999g999g999g999g999g999g999'),"           //  8 only for 7.3
            "to_char(cardinality,'FM999g999g999g999g999g999g999g999g999'),"    //  9 only for 7.3
            "to_char(bytes, 'FM999g999g999g999g999g999g999g999g999') "          // 10 only for 7.3
        "FROM <PLAN_TABLE> "
            "WHERE statement_id = :sttm_id "
            "ORDER BY id, position",
};

static const char expl_clear[] = "DELETE FROM <PLAN_TABLE> WHERE statement_id = :sttm_id";

struct CTypeMap {
  int m_Size;
  struct TData {
    int           m_ImageId;
    const char*   m_Key;
  } *m_Data;
};

CTypeMap::TData _typeMap [] = {
    { 20, "FILTER"           },
    { 10, "VIEW"             },
    {  2, "TABLE ACCESS"     },
    {  5, "INDEX"            },
    {  0, "SELECT STATEMENT" },
    {  0, "DELETE STATEMENT" },
    {  0, "UPDATE STATEMENT" },
    { 21, "NESTED LOOPS"     },
    { 22, "HASH JOIN"        },
    { 22, "MERGE JOIN"       },
    { 19, "SORT"             },
};

CTypeMap typeMap = { sizeof _typeMap / sizeof _typeMap[0], _typeMap };


void CPLSWorksheetDoc::DoSqlDbmsXPlanDisplayCursor()
{
	if (m_connect.GetVersion() >= OCI8::esvServer10X)
	{
		string text, buff;
		OciCursor explan_curs(m_connect, expl_xplan_display_cursor);
		explan_curs.Bind(":sql_id", m_connect.GetCurrentSqlID().c_str());
		explan_curs.Bind(":child_number", m_connect.GetCurrentSqlChildNumber().c_str());

		explan_curs.ExecuteShadow();
		while (explan_curs.Fetch()) 
		{
			explan_curs.GetString(0, buff);
			text += buff;
			text += "\r\n";
		}

        explan_curs.Close();

		m_connect.SetSession();

		m_pXPlan->SetWindowText(text.c_str());
		m_pXPlan->SetIsDisplayCursor(true);
	}
}

void CPLSWorksheetDoc::DoSqlExplainPlan (const string& text)
{
    CTreeCtrl& tree = m_pExplainPlan->GetTreeCtrl();
    tree.DeleteAllItems();

	m_pXPlan->SetWindowText("");

    //try { EXCEPTION_FRAME;

        int ver = (m_connect.GetVersion() < OCI8::esvServer80X) ? 0 : 1;
        srand((UINT)time(NULL));
        
        char sttm_id[80];
        _snprintf(sttm_id, sizeof(sttm_id), "SQLTOOLS.%d", rand());
        sttm_id[sizeof(sttm_id)-1] = 0;

        Common::Substitutor subst;
        subst.AddPair("<PLAN_TABLE>", GetSQLToolsSettings().GetPlanTable());
        subst.AddPair("<STTM_ID>", sttm_id);
        subst << "EXPLAIN PLAN SET STATEMENT_ID = \'<STTM_ID>\' INTO <PLAN_TABLE> FOR ";

        OCI8::Statement sttm(GetApp()->GetConnect());
        sttm.Prepare((std::string(subst.GetResult()) + text).c_str());
        sttm.Execute(1);

        subst.ResetContent();
        subst.ResetResult();
        subst.AddPair("<PLAN_TABLE>", GetSQLToolsSettings().GetPlanTable());
        subst << expl_sttm[ver];

        OciCursor explan_curs(GetApp()->GetConnect(), subst.GetResult());
        explan_curs.Bind(":sttm_id", sttm_id);

        void* val;
        string buff, line;
        CMapPtrToPtr map;

        TV_INSERTSTRUCT	tvstruct;
        memset(&tvstruct, 0, sizeof tvstruct);
        tvstruct.hInsertAfter    = TVI_LAST;
        tvstruct.item.mask       = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
        tvstruct.item.state      = TVIS_EXPANDED;
        tvstruct.item.stateMask  = TVIS_EXPANDED;
        tvstruct.item.cChildren  = 0;

        m_pExplainPlan->StartHtmlTree(text);
        explan_curs.Execute();
        while (explan_curs.Fetch()) 
        {
            if (!(!explan_curs.IsNull(ps_parent_id)
                && map.Lookup((void*)explan_curs.ToInt(ps_parent_id), val)))
                val = 0;

            explan_curs.GetString(ps_operation, line);
            explan_curs.GetString(ps_options, buff);
            if (!buff.empty())
                line += " (" + buff + ")";

            explan_curs.GetString(ps_object_name, buff);
            if (!buff.empty()) 
            {
                line += " of \"" + buff + "\"";
                explan_curs.GetString(ps_object_instance, buff);
                if (!buff.empty())
                    line += " #" + buff;
            }

            explan_curs.GetString(ps_object_type, buff);
            if (!buff.empty())
                line += " " + buff;

            if (ver) // only for 73
            {
                explan_curs.GetString(ps_optimizer, buff);
                if (!buff.empty())
                    line += " Optimizer=" + buff;

                explan_curs.GetString(ps_cost, buff);
                if (!buff.empty()) 
                {
                    line += " (Cost=" + buff;
                    explan_curs.GetString(ps_cardinality, buff);
                    line += " Card=" + buff;
                    explan_curs.GetString(ps_bytes, buff);
                    line += " bytes=" + buff + ")";
                }
            }

            tvstruct.hParent         = (HTREEITEM)val;
            tvstruct.item.pszText    = (char*)line.c_str();
            tvstruct.item.cchTextMax = line.length();

            tvstruct.item.iImage    =
                tvstruct.item.iSelectedImage = 0;

            for (int j = 0; j < typeMap.m_Size; j++) 
            {
                if (!strcmp(typeMap.m_Data[j].m_Key, explan_curs.ToString(ps_operation).c_str())) 
                {
                    tvstruct.item.iImage         =
                    tvstruct.item.iSelectedImage = typeMap.m_Data[j].m_ImageId;
                    break;
                }
            }
            val = (void*)tree.InsertItem(&tvstruct);

            m_pExplainPlan->AppendHtmlItem((int)tvstruct.hParent, (int)val, line);

            map.SetAt((void*)explan_curs.ToInt(ps_id), val);
        }
        m_pExplainPlan->EndHtmlTree();


        explan_curs.Close();

		if (m_connect.GetVersion() >= OCI8::esvServer9X)
		{
			string text;

			subst.ResetContent();
			subst.ResetResult();
			subst.AddPair("<PLAN_TABLE>", GetSQLToolsSettings().GetPlanTable());
			subst << expl_xplan;

			explan_curs.Prepare(subst.GetResult());
			explan_curs.Bind(":sttm_id", sttm_id);

			explan_curs.Execute();
			while (explan_curs.Fetch()) 
			{
				explan_curs.GetString(0, buff);
				text += buff;
				text += "\r\n";
			}

	        explan_curs.Close();

			m_pXPlan->SetWindowText(text.c_str());
			m_pXPlan->SetIsDisplayCursor(false);
		}

        subst.ResetContent();
        subst.ResetResult();
        subst.AddPair("<PLAN_TABLE>", GetSQLToolsSettings().GetPlanTable());
        subst << expl_clear;

        OciStatement clear_sttm(GetApp()->GetConnect());
        clear_sttm.Prepare(subst.GetResult());
        OciStringVar var_sttm_id(80+1); 
        var_sttm_id.Assign(sttm_id);
        clear_sttm.Bind(":sttm_id", var_sttm_id);
        clear_sttm.Execute(1);
  
// TODO: review the following changes
//        clear_sttm.Execute(1, true/*guaranteedSafe*/);
//        if (m_connect.GetSafety() == OCI8::esReadOnly)
//            m_connect.Commit(true/*guaranteedSafe*/);

        tree.Invalidate();
        tree.SetFocus();

		ActivateTab(m_pExplainPlan);

		if (!m_pBooklet->IsTabPinned())
			if (m_pXPlan->GetWindowTextLength() > 0)
			{
				m_pBooklet->getActiveView()->ShowWindow(SW_HIDE);
				m_pXPlan->ShowWindow(SW_SHOW);
				m_pXPlan->SetFocus();
			}
    //} 
    //_DEFAULT_HANDLER_
}
