//  (C) Copyright Gennadiy Rozental 2001-2002.
//  (C) Copyright Beman Dawes 2001. 
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile$
//
//  Version     : $Id$
//
//  Description : tests an ability of Program Execution Monitor to catch 
//  system exceptions. Should fail during run.
// ***************************************************************************

int cpp_main( int, char *[] )  // note the name
{
  int div = 0;
  return 10 / div;
}

//____________________________________________________________________________//

// ***************************************************************************
//  Revision History :
//  
//  $Log$
//  Revision 1.1  2007/01/28 23:07:34  randolf
//  Initial revision
//
//  Revision 1.1.1.1  2004/12/13 06:41:06  akochetov
//  no message
//
//  Revision 1.1  2004/05/19 23:12:06  master
//  start
//
//  Revision 1.4  2002/08/26 09:08:06  rogeeff
//  cvs kw added
//

// ***************************************************************************

// EOF
