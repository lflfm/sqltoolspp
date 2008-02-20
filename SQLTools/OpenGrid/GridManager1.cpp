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

/***************************************************************************/
/*      Purpose: Grid manager implementation for grid component            */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

#include "stdafx.h"
#include "GridManager.h"
#include "SQLTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{

NavGridManager::NavGridManager (CWnd* client, AGridSource* source, bool shouldDeleteSource)
: CalcGridManager(client, source, shouldDeleteSource),
    m_ScrollMode(false)
{
}

void NavGridManager::EvClose ()
{
    CalcGridManager::EvClose();
    m_ScrollMode = false;
}

void NavGridManager::EvSize (const CSize& size, bool force)
{
    CalcGridManager::DoEvSize(size, !m_ScrollMode, force/*FALSE*//*TRUE*/);
}

bool NavGridManager::EvKeyDown (unsigned key, unsigned repeatCount, unsigned /*flags*/)
{
    endScrollMode();

    switch (key) 
    {
    case VK_TAB:
        if (!m_Options.m_Tabs) 
        {
            if (m_pClientWnd->GetParent()
                && GetWindowLong(*m_pClientWnd, GWL_STYLE) & WS_TABSTOP) 
            {
                CWnd* next = m_pClientWnd->GetParent()->GetNextDlgTabItem(m_pClientWnd, (0xFF00 & GetKeyState(VK_SHIFT)));
                if (next) next->SetFocus();
                break;
            } else
                return 0;
        }
        while(repeatCount--)
            if (0xFF00 & GetKeyState(VK_SHIFT))
            {
                if (!MoveToLeft(edHorz)
                    && (GetSQLToolsSettings().GetGridWraparound() && (!MoveToLeft(edVert) || !MoveToEnd(edHorz)))) break;
            }
            else
            {
                if (!MoveToRight(edHorz)
                    && (GetSQLToolsSettings().GetGridWraparound() && (!MoveToRight(edVert) || !MoveToHome(edHorz)))) break;
            }
        break;
    case VK_UP:
        while(repeatCount-- && MoveToLeft(edVert));
        break;
    case VK_DOWN:
        while(repeatCount-- && MoveToRight(edVert));
        break;
    case VK_LEFT:
        while(repeatCount--)
            if (!MoveToLeft(edHorz)
                && (GetSQLToolsSettings().GetGridWraparound() && (!MoveToLeft(edVert) || !MoveToEnd(edHorz)))) break;
        break;
    case VK_RETURN:
        if (!m_Options.m_MoveOnEnter) return 0;
    case VK_RIGHT:
        while(repeatCount--)
            if (!MoveToRight(edHorz)
                && (GetSQLToolsSettings().GetGridWraparound() && (!MoveToRight(edVert) || !MoveToHome(edHorz)))) break;
        break;
    case VK_HOME:
        if (0xFF00 & GetKeyState(VK_CONTROL))
            MoveToHome(edVert);
        else
            MoveToHome(edHorz);
        break;
    case VK_END:
        if (0xFF00 & GetKeyState(VK_CONTROL))
            MoveToEnd(edVert);
        else
            MoveToEnd(edHorz);
        break;
    case VK_PRIOR:
        while(repeatCount--) 
        {
            if (!MoveToPageUp(edVert)) break;
            m_pClientWnd->UpdateWindow();
        }
        break;
    case VK_NEXT:
        while(repeatCount--) 
        {
            if (!MoveToPageDown(edVert)) break;
            m_pClientWnd->UpdateWindow();
        }
        break;
    default:
        return 0;
    }
    return 1;
}

bool NavGridManager::MoveToLeft (eDirection dir)
{
    endScrollMode();
    return m_Rulers[dir].MoveToLeft();
}

bool NavGridManager::MoveToRight (eDirection dir)
{
    endScrollMode();
    return m_Rulers[dir].MoveToRight();
}

bool NavGridManager::MoveToHome (eDirection dir)
{
    endScrollMode();
    return m_Rulers[dir].MoveToHome();
}

bool NavGridManager::MoveToEnd (eDirection dir)
{
    endScrollMode();
    return m_Rulers[dir].MoveToEnd();
}

bool NavGridManager::MoveToPageUp (eDirection dir)
{
    bool retVal = false;

    endScrollMode();

    if (m_Rulers[dir].GetCurrent() > m_Rulers[dir].GetTopmost())  // the current is not the first
    {
        m_Rulers[dir].SetCurrent(m_Rulers[dir].GetTopmost());
        retVal = true;
    } 
    else // it's the first visible item 
    {    
        m_Rulers[dir].adjustTopmostByPos(m_Rulers[dir].GetCurrent()-1, true);
        m_Rulers[dir].SetCurrent(m_Rulers[dir].GetTopmost());
        retVal = true;
    }

    return retVal;
}

bool NavGridManager::MoveToPageDown (eDirection dir)
{
    bool retVal = false;

    endScrollMode();

    int last = m_Rulers[dir].GetLastFullVisiblePos();

    if (m_Rulers[dir].GetCurrent() < last) // the current is not the last
    { 
        m_Rulers[dir].SetCurrent(last);
        retVal = true;
    } 
    else 
    {
        int last = m_pSource->GetCount(dir) - 1;
        if (m_Rulers[dir].GetCurrent() < last)
        {
            m_Rulers[dir].SetTopmost(m_Rulers[dir].GetCurrent()+1);
            m_Rulers[dir].SetCurrent(
                m_Rulers[dir].GetDataItemCount() > 1 
                    ? m_Rulers[dir].GetLastFullVisiblePos() : m_Rulers[dir].GetCurrent()+1);
            retVal = true;
        }
    }

    return retVal;
}

void NavGridManager::EvScroll (eDirection dir, unsigned scrollCode, unsigned thumbPos)
 {
    bool enabled[2] = { m_Options.m_HorzScroller, m_Options.m_VertScroller };

    if (enabled[dir])
    {
        ScrollController ctrl(*this, dir);

        switch (scrollCode) 
        {
        case SB_LINELEFT:
            ScrollToLeft(dir);
            break;
        case SB_LINERIGHT:
            ScrollToRight(dir);
            break;
        case SB_PAGEUP:
            m_Rulers[dir].adjustTopmostByPos(m_Rulers[dir].GetTopmost()-1, true);
            break;
        case SB_PAGEDOWN:
            m_Rulers[dir].SetTopmost(
                m_Rulers[dir].GetDataItemCount() > 1 
                    ? m_Rulers[dir].GetLastVisiblePos() : m_Rulers[dir].GetTopmost()+1);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            if (!thumbPos)
                ScrollToHome(dir);
            else if (static_cast<int>(thumbPos) + m_Rulers[dir].GetDataItemCount() >= m_pSource->GetCount(dir))
                ScrollToEnd(dir);
            else
                m_Rulers[dir].SetTopmost(thumbPos);
            break;
        }
    }
 }

void NavGridManager::setScrollMode (bool scroll)
{
    if (scroll) 
    {
        if (!m_ScrollMode) 
        {
            m_ScrollMode = TRUE;
            m_Rulers[edHorz].SetSavedTopmost(m_Rulers[edHorz].GetTopmost());
            m_Rulers[edVert].SetSavedTopmost(m_Rulers[edVert].GetTopmost());
        }
    } else {
        if (m_ScrollMode) 
        {
            m_ScrollMode = FALSE;
            if (!IsCurrentFullVisible())
            {
                for (int i(0); i < 2; i++)
                    if (!m_Rulers[i].IsCurrentFullVisible()) 
                    {
                        m_Rulers[i].SetTopmost(m_Rulers[i].GetSavedTopmost());
                        RecalcRuller((eDirection)i, true/*adjust*/, true/*invalidate*/);
                    }
            }
        }
    }
}

bool NavGridManager::ScrollToLeft (eDirection dir)
{
    if (m_Rulers[dir].GetTopmost() > 0) 
    {
        ScrollController ctrl(*this, dir);

        m_Rulers[dir].DecTopmost();

        return true;
   }

    return false;
}

bool NavGridManager::ScrollToRight (eDirection dir)
{
    if (m_Rulers[dir].GetTopmost() < m_pSource->GetCount(dir) - 1
    && !m_Rulers[dir].GetUnusedCount()) // scroll untill the first unused line appears
    {
        ScrollController ctrl(*this, dir);

        m_Rulers[dir].IncTopmost();

        return true;
    }

    return false;
}

bool NavGridManager::ScrollToHome (eDirection dir)
{
    if (m_Rulers[dir].GetTopmost()) 
    {
        ScrollController ctrl(*this, dir);

        m_Rulers[dir].SetTopmost(0);

        return true;
    }

    return false;
}

bool NavGridManager::ScrollToEnd (eDirection dir)
{
    ScrollController ctrl(*this, dir);

    int topmost = m_Rulers[dir].GetTopmost();
    m_Rulers[dir].adjustTopmostByPos(m_pSource->GetCount(dir) - 1, true);

    return topmost == m_Rulers[dir].GetTopmost() ? false : true;
}

void NavGridManager::EvLButtonDown (unsigned /*modKeys*/, const CPoint& point)
{
    if (point.x >= 0 && point.x < m_Rulers[edHorz].GetClientSize()
    && point.y >= 0 && point.y < m_Rulers[edVert].GetClientSize())
    {
        for (int dir = 0; dir < 2; dir++) 
        {
            int inx = m_Rulers[dir].PointToItem(!dir ? point.x : point.y) 
                    - m_Rulers[dir].GetFirstFixCount();
            if (inx >= 0) 
            {
                int pos = inx + m_Rulers[dir].GetTopmost();
                if (m_Rulers[dir].GetCurrent() != pos
                && pos < m_pSource->GetCount((eDirection)dir)) 
                {
                    m_ScrollMode = false;
                    m_Rulers[dir].SetCurrent(pos);
                }
            }
        }
    }
}

void NavGridManager::Refresh ()
{
    endScrollMode();
    CalcGridManager::Refresh();
}

}//namespace OG2

