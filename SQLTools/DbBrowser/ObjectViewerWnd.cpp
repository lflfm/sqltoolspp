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

// 25.12.2004 (ak) R1091240 - Find Object/Object Viewer improvements
// 09.01.2005 (ak) replaced CComboBoxEx with CComboBox because of unpredictable CComboBoxEx
// 20.03.2005 (ak) bug fix, an exception on "Find Object", if some name component is null
// 20.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

// This file is a subject for future changes - it has be changed
// after ObjectTreeBuilder redesign

#include "stdafx.h"
#include "SQLTools.h"
#include "COMMON\AppGlobal.h"
#include <COMMON/DlgDataExt.h>
#include "COMMON\StrHelpers.h"
#include "DbBrowser\ObjectViewerWnd.h"
#include "DbBrowser\ObjectTreeBuilder.h"
#include "SQLWorksheet/PlsSqlParser.h"
#include "COMMON/GUICommandDictionary.h"

////////////////////////////////////////////////////////////////////////////////
// CObjectViewerWnd
CObjectViewerWnd::CObjectViewerWnd()
: CDialog(IDD),
m_selChanged(false)
{
	m_accelTable = 0;
}

CObjectViewerWnd::~CObjectViewerWnd()
{
	if (m_accelTable)
		DestroyAcceleratorTable(m_accelTable);
}

BOOL CObjectViewerWnd::PreTranslateMessage(MSG* pMsg)
{
	if (m_accelTable)
		if (TranslateAccelerator(m_hWnd, m_accelTable, pMsg) == 0)
			return CDialog::PreTranslateMessage(pMsg);
		else
			return true;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

void CObjectViewerWnd::OnEditCopy()
{
    // ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsVisible(), FALSE);
    string theText = m_treeViewer.GetSelectedItemsAsText(true, false);
	Common::CopyTextToClipboard(theText);

	Global::SetStatusText("Copied to clipboard: " + theText);
}

void CObjectViewerWnd::OnEditCopyWithNewLines()
{
    // ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsVisible(), FALSE);
    string theText = m_treeViewer.GetSelectedItemsAsText(true, true);
	Common::CopyTextToClipboard(theText);

	Global::SetStatusText("Copied to clipboard: " + theText);
}

void CObjectViewerWnd::OnSqlObjViewer()
{
    // ShowControlBar(&m_wndObjectViewerFrame, !m_wndObjectViewerFrame.IsVisible(), FALSE);
	GetParentFrame()->SendMessage(WM_COMMAND, (1 << 16) | ID_SQL_OBJ_VIEWER, 0);
}

BOOL CObjectViewerWnd::Create (CWnd* pParentWnd)
{
	if (! m_accelTable)
	{
		CMenu menu;
		VERIFY(menu.LoadMenu(IDR_OBJECTVIEWER_DUMMY));
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		Common::GUICommandDictionary::AddAccelDescriptionToMenu(pPopup->m_hMenu);
		m_accelTable = Common::GUICommandDictionary::GetMenuAccelTable(pPopup->m_hMenu);
	}

	return CDialog::Create(IDD_OBJ_INFO_TREE, pParentWnd);
}

void CObjectViewerWnd::DoDataExchange (CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_CBString(pDX, IDC_OIT_INPUT, m_input);
    DDX_Control(pDX, IDC_OIT_INPUT, m_inputBox);
    DDX_Control(pDX, IDC_OIT_TREE, m_treeViewer);
}

void CObjectViewerWnd::ShowObject (const std::string& name)
{
    m_input = name;
    UpdateData(FALSE);
    OnOK();
}

BOOL CObjectViewerWnd::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_Images.Create(IDB_SQL_GENERAL_IMAGELIST, 16, 64, RGB(0,255,0));

    m_input = "%";
    setQuestionIcon();

    return TRUE;  // return TRUE unless you set the focus to a control
}

void CObjectViewerWnd::setQuestionIcon ()
{
    m_inputBox.ResetContent();
    int inx = m_inputBox.AddString(m_input.c_str());
    m_inputBox.SetItemData(inx, (DWORD)-1);
    m_inputBox.SetCurSel(0);
}

    class FindObjectException : public Common::AppException {
    public: FindObjectException (const std::string& what) : Common::AppException (what) {}
    };

    struct ObjectDescriptor { std::string owner, name, type; };
    void FindObjects (const std::string& name, std::vector<ObjectDescriptor>&);
    void PopulateComboBox (const std::vector<ObjectDescriptor>&, CComboBox&);
    bool FindTypeAndImageByStr (const string& typeStr, int& type, int& image);
    bool FindImageByType (int type, int& image);


void CObjectViewerWnd::OnOK ()
{
    try { EXCEPTION_FRAME;

        if (UpdateData())
        {
            m_treeViewer.DeleteAllItems();

            std::vector<ObjectDescriptor> result;
            FindObjects(m_input, result);

            if (result.size() > 0)
            {
                PopulateComboBox(result, m_inputBox);

                if (result.size() == 1)
                {
                    m_inputBox.SetCurSel(0);
                    OnInput_SelChange();
                }
                else
                {
                    m_inputBox.PostMessage(CB_SHOWDROPDOWN, TRUE);
                }
            }
            else
            {
                ::MessageBeep(MB_ICONSTOP);
                Global::SetStatusText('<' + m_input + "> not found!");
                m_inputBox.SetFocus();
                setQuestionIcon();
            }
        }
    }
    catch (const FindObjectException& x)
    {
        DEFAULT_HANDLER(x);
        m_inputBox.SetFocus();
    }
    _COMMON_DEFAULT_HANDLER_
}

void CObjectViewerWnd::OnCancel ()
{
    AfxGetMainWnd()->SetFocus();
}

BEGIN_MESSAGE_MAP(CObjectViewerWnd, CDialog)
    ON_WM_SIZE()
    ON_CBN_SELCHANGE(IDC_OIT_INPUT, OnInput_SelChange)
    ON_CBN_CLOSEUP(IDC_OIT_INPUT, OnInput_CloseUp)
	ON_COMMAND(ID_SQL_OBJ_VIEWER, OnSqlObjViewer)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_COPY_NEW_LINES, OnEditCopyWithNewLines)
END_MESSAGE_MAP()

// CObjectViewerWnd message handlers
void CObjectViewerWnd::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (cx > 0 && cy > 0
    && m_inputBox.m_hWnd
    && nType != SIZE_MAXHIDE && nType != SIZE_MINIMIZED)
    {
        CRect rect;
        GetClientRect(&rect);

        CRect comboRect;
        m_inputBox.GetWindowRect(&comboRect);
        int comboH = comboRect.Height() + 2;

        HDWP hdwp = ::BeginDeferWindowPos(10);
        ::DeferWindowPos(hdwp, m_inputBox, 0, rect.left, rect.top,
            rect.Width(), rect.Height()/2, SWP_NOZORDER);
        ::DeferWindowPos(hdwp, m_treeViewer, 0, rect.left,
            rect.top + comboH, rect.Width(), rect.Height() - comboH, SWP_NOZORDER);
        ::EndDeferWindowPos(hdwp);
    }
}

void CObjectViewerWnd::OnInput_CloseUp ()
{
    if (m_selChanged)
        OnInput_SelChange();
}

void CObjectViewerWnd::OnInput_SelChange ()
{
    try { EXCEPTION_FRAME;

        m_selChanged = true;

        if (m_inputBox.GetDroppedState())
            return;

        m_selChanged = false;

        int inx = m_inputBox.GetCurSel();
        if (inx != CB_ERR)
        {
            CString buffer;
            m_inputBox.GetLBText(inx, buffer);

            m_treeViewer.DeleteAllItems();

            int image, type = m_inputBox.GetItemData(inx);

            if (type != -1
            && FindImageByType(type, image))
            {
                TV_INSERTSTRUCT  tvstruct;
                memset(&tvstruct, 0, sizeof(tvstruct));
                tvstruct.hParent      = TVI_ROOT;
                tvstruct.hInsertAfter = TVI_LAST;
                tvstruct.item.mask    = TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
                tvstruct.item.pszText   = (LPSTR)(LPCSTR)buffer;
                tvstruct.item.lParam    = type;
                tvstruct.item.iImage    =
                tvstruct.item.iSelectedImage = image;
                tvstruct.item.cChildren = 1;

                if (HTREEITEM hItem = m_treeViewer.InsertItem(&tvstruct))
                {
                    m_treeViewer.Expand(hItem, TVE_EXPAND);

                    if (tvstruct.item.lParam == titSynonym)
                        m_treeViewer.Expand(m_treeViewer.GetChildItem(hItem), TVE_EXPAND);
                }
            }
        }
    }
    _COMMON_DEFAULT_HANDLER_
}

///////////////////////////////////////////////////////////////////////////////
//
// struct ObjectDescriptor { std::string owner, name, type; };
//
// Valid input for FindObjects: name1[.name2]
//      Name can be either a sequence of A-z,0-9,_#$ or a quoted string
//      '%' and '_' are allowed for Oracle LIKE search, if name is not a quoted string,
//      but LIKE search is turned on if name contains '%' because '_' is too frequent in names
//      Currently name3 will be ignored because there is no support for package functions/procedures
//
///////////////////////////////////////////////////////////////////////////////
    void quetyObjectDescriptors (
        std::vector<std::pair<std::string, bool> > names,
        std::vector<ObjectDescriptor>& result);

void FindObjects (const std::string& input, std::vector<ObjectDescriptor>& result)
{
	// Check existence of an open DB connection.
	// B#1191428 - % in disconnected 'Object viewer'
	if (((CSQLToolsApp*)AfxGetApp())->GetConnect().IsOpen() == false) {
		throw Common::AppException("Find object: not connected to database!");
	}
	
	TokenMapPtr tokenMap(new TokenMap);
    tokenMap->insert(std::map<string, int>::value_type("\"", etDOUBLE_QUOTE));
    tokenMap->insert(std::map<string, int>::value_type("." , etDOT         ));
    tokenMap->insert(std::map<string, int>::value_type("@" , etAT_SIGN     ));
    tokenMap->insert(std::map<string, int>::value_type("%" , etPERCENT_SIGN));

    SimpleSyntaxAnalyser analyser;
    PlsSqlParser parser(&analyser, tokenMap);
    parser.PutLine(0, input.c_str(), input.length());
    parser.PutEOF(0);

    std::vector<std::pair<std::string, bool> > names;
    const std::vector<Token> tokens = analyser.GetTokens();
    std::vector<Token>::const_iterator it = tokens.begin();
    for (int i = 0; i < 2; ++i)
    {
        bool percent = false, dot = false, eof = false, quoted = false;
        int start = (it != tokens.end()) ? it->offset : 0;

        for (; it != tokens.end(); ++it)
        {
            switch (*it)
            {
            case etDOUBLE_QUOTED_STRING:
                quoted = true;
                break;
            case etPERCENT_SIGN:
                percent = true;
                break;
            case etDOT:
                dot = true;
                break;
            case etEOL:
            case etEOF:
                eof = true;
                break;
            case etAT_SIGN:
                throw Common::AppException("Find object: DB link not allowed!");
            }
            if (dot || eof) break;
        }

        int end = (it != tokens.end()) ? it->offset : input.length();

        string name(input.substr(start, end-start));
        Common::trim_symmetric(name);

        if (!quoted)
            Common::to_upper_str(name.c_str(), name);
        else
            name = name.substr(1, name.length()-2);

        // 20.03.2005 (ak) bug fix, an exception on "Find Object", if some name component is null
        if (!name.empty())
            names.push_back(make_pair(name, percent));

        TRACE("NAME: %s\n", name.c_str());

        if (dot) ++it;
        if (eof) break;
    }

    if (names.size() > 0)
        quetyObjectDescriptors(names, result);
}

#include <OCI8/BCursor.h>
#include "COMMON\StrHelpers.h"

static const char* cszSelectFromAllObjects =
    "SELECT /*+RULE*/ owner, object_name, object_type FROM sys.all_objects"
    " WHERE object_type in ("
    "'TABLE','VIEW','SYNONYM','SEQUENCE','PROCEDURE','FUNCTION',"
    "'PACKAGE','PACKAGE BODY','TRIGGER','TYPE','TYPE BODY')";//",'OPERATOR')"
static const char* cszOrderByClause =
    " ORDER BY owner, object_name, object_type";

static
void makeQueryByNameInCurrSchema (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 0);
    Common::Substitutor subst;
    subst.AddPair("<EQL>", names.at(0).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << (cursor.GetConnect().GetVersion() < OCI8::esvServer81X
        ? " AND owner = USER and object_name <EQL> :name"
        : " AND owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA') AND object_name <EQL> :name");
    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":name", names.at(0).first.c_str());
    cursor.Execute();
}

static
void makeQueryByNameInPublic (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 0);
    Common::Substitutor subst;
    subst.AddPair("<EQL>", names.at(0).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << " AND owner = 'PUBLIC' AND object_name <EQL> :name";
    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":name", names.at(0).first.c_str());
    cursor.Execute();
}

static
void makeQueryBySchemaAndName (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 1);
    Common::Substitutor subst;
    subst.AddPair("<EQL1>", names.at(0).second  ? "like" : "=");
    subst.AddPair("<EQL2>", names.at(1).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << " AND owner <EQL1> :schema AND object_name <EQL2> :name";
    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":schema", names.at(0).first.c_str());
    cursor.Bind(":name", names.at(1).first.c_str());
    cursor.Execute();
}

static
void fetchObjectDescriptors (OciCursor& cursor, std::vector<ObjectDescriptor>& result)
{
    for (int rows = 0; cursor.Fetch(); rows++)
    {
        ObjectDescriptor desc;
        cursor.GetString(0, desc.owner);
        cursor.GetString(1, desc.name);
        cursor.GetString(2, desc.type);
        result.push_back(desc);

        TRACE("FOUND: %s.%s.%s\n", desc.owner.c_str(), desc.name.c_str(), desc.type.c_str());

        // problem with public.% :(
        if (rows > 300)
            throw FindObjectException("Find object: too many objects found!\nPlease use more precise criteria!");
    }
    cursor.Close();
}

///////////////////////////////////////////////////////////////////////////////
//    if it is a single name then there is no question.
//    if it is two comonent name, the question is
//    what the first component means - schema or package?
//        if it is a name with exact match (no '%' for like),
//        probably this name captured for the editor
//        so use oracle name resolving rule (double check it):
//            look at the current schema then public synomyns
//        if it is a name with widcard '%'
//        most likely it is an interactive input and the fist component is schema
//            look at this schema for objects then if nothing is found
//            thow the second component away and call the first case
///////////////////////////////////////////////////////////////////////////////
void quetyObjectDescriptors (
    std::vector<std::pair<std::string, bool> > names,
    std::vector<ObjectDescriptor>& result)
{
    OciCursor cursor(((CSQLToolsApp*)AfxGetApp())->GetConnect());

    if (names.size() > 1)
    {
        makeQueryBySchemaAndName(names, cursor);
        fetchObjectDescriptors(cursor, result);
    }

    if (/*names.size() == 1 ||*/ result.empty())
    {
        makeQueryByNameInCurrSchema(names, cursor);
        fetchObjectDescriptors(cursor, result);

        if (result.empty())
        {
            makeQueryByNameInPublic(names, cursor);
            fetchObjectDescriptors(cursor, result);
        }
    }
}

#include "ObjectTreeBuilder.h"
void PopulateComboBox (const std::vector<ObjectDescriptor>& result, CComboBox& list)
{
    list.ResetContent();
    std::vector<ObjectDescriptor>::const_iterator it = result.begin();
    for (int i = 0; it != result.end(); ++it, ++i)
    {
        std::string object(it->owner + '.' + it->name);

        int typeId, imageId;
        if (FindTypeAndImageByStr(it->type, typeId, imageId))
        {
            int inx = list.AddString(object.c_str());
            list.SetItemData(inx, typeId);
        }
    }
}

bool FindTypeAndImageByStr (const string& typeStr, int& type, int& image)
{
    for (int j = 0; j < _TypeMap.m_Size; j++)
    {
        if (typeStr == _TypeMap.m_Data[j].m_Key)
        {
            type = _TypeMap.m_Data[j].m_Type;
            image = _TypeMap.m_Data[j].m_ImageId;
            return true;
        }
    }
    return false;
}

bool FindImageByType (int type, int& image)
{
    for (int j = 0; j < _TypeMap.m_Size; j++)
    {
        if (type == _TypeMap.m_Data[j].m_Type)
        {
            image = _TypeMap.m_Data[j].m_ImageId;
            return true;
        }
    }
    return false;
}
