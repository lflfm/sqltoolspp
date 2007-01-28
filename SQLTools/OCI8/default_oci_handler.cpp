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
#include <COMMON/ExceptionHelper.h>

void default_oci_handler (const OciException& x, const char* file, int line)
{
    switch (x)
    {
    // data is not up-to-date
    case 235:   // controlfile fixed table inconsistent due to concurrent update
    case 1555:  // snapshot too old: rollback segment number string with name "string" too small
    case 22924: // snapshot too old
    // user cancelation
    case 0:
    case 1013:
    // connection problem
    case 1017:
    case 12154:
    case 12505:
    case 12541:
    case 12545:
    case 12571:
        AfxMessageBox(x.what(), MB_OK|MB_ICONEXCLAMATION);
        break;
    // connection lost
    case 28: 
    case 1012: 
    case 3113: 
    case 3114:
        MessageBeep(MB_ICONSTOP);
        AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
        break;
    default:
        //Common::DefaultHandler(x, file, line);
        TRACE("OCI exception: %s at %s.%d", x.what(), file, line);
        MessageBeep(MB_ICONSTOP);
        AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
    }
}
