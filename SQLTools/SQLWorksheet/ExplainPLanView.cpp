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
#include <sstream>
#include "resource.h"
#include "ExplainPlanView.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppUtilities.h"
#include "COMMON/TempFilesManager.h"

using namespace std;

CExplainPlanView::CExplainPlanView()
{
}

CExplainPlanView::~CExplainPlanView()
{
}

BEGIN_MESSAGE_MAP(CExplainPlanView, CTreeView)
    ON_WM_CREATE()
    ON_COMMAND(ID_GRID_OPEN_WITH_IE, OnOpenWithIE)
    ON_UPDATE_COMMAND_UI(ID_GRID_OPEN_WITH_IE, OnUpdate_OpenWithIE)
END_MESSAGE_MAP()

// CExplainPlanView message handlers

int CExplainPlanView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CTreeView::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_Images.Create(IDB_SQL_GENERAL_IMAGELIST, 16, 64, RGB(0,255,0));
    GetTreeCtrl().SetImageList(&m_Images, TVSIL_NORMAL);

    return 0;
}

void CExplainPlanView::StartHtmlTree (const std::string& sqlStatement)        
{ 
    m_sqlStatement = sqlStatement; 
    Common::trim_symmetric(m_sqlStatement);
    m_htmlTree.clear(); 
    m_rootItem.clear(); 
}

void CExplainPlanView::AppendHtmlItem (int parentId, int itemId, const string& text)
{
    ostringstream out;

    if (parentId == 0) // root item
    {
        out << 'i' << itemId;
        m_rootItem = out.str();
        out.seekp(0);

        out << "var i" << itemId << " = new WebFXTree('" << text << "')" << endl;

        m_htmlTree = out.str();
    }
    else
    {
        out << "var i" << itemId << " = new WebFXTreeItem('"
            << text << "'); i" << itemId << ".open = true; i" << parentId 
            << ".add(i" << itemId << ");" << endl;

        m_htmlTree += out.str();
    }
}

void CExplainPlanView::EndHtmlTree ()
{
    m_htmlTree += "document.write(" + m_rootItem + ");\n";
}

void CExplainPlanView::OnOpenWithIE ()
{
    if (GetTreeCtrl().GetCount() > 0)
    {
        string xtreePath;
        Common::AppGetPath(xtreePath);
        xtreePath += "\\html\\xtree";
        string xtreePathForJScript = string("file:///") + xtreePath; // "file:///e:/..."
        string::iterator it = xtreePathForJScript.begin();
        for (; it != xtreePathForJScript.end(); ++it)
            if (*it == '\\') *it = '/';

        string text;
        ifstream in((xtreePath + "\\explain_plan.html-template").c_str());
        if (in && getline(in, text, '\0'))
        {
            Common::Substitutor subst;
            subst.AddPair("<SQL_STATEMENT>", m_sqlStatement);
            subst.AddPair("<PLAN_TREE>", m_htmlTree);
            subst.AddPair("<XTREE_FOLDER>", xtreePathForJScript); 
            subst << text.c_str();
            text = subst.GetResult();
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Cannot open explain plan template file.", MB_OK|MB_ICONSTOP);
            AfxThrowUserException();
        }

        string file = TempFilesManager::CreateFile("htm");
        if (!file.empty())
        {
            ofstream of(file.c_str());

            of << text;            

            of.close();

            Common::AppShellOpenFile(file.c_str());
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Cannot generate temporary file name for export.", MB_OK|MB_ICONSTOP);
            AfxThrowUserException();
        }
    }
}

void CExplainPlanView::OnUpdate_OpenWithIE (CCmdUI *pCmdUI)
{
    pCmdUI->Enable(GetTreeCtrl().GetCount() > 0 ? TRUE : FALSE);
}
