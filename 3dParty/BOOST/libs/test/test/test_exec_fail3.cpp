//  (C) Copyright Gennadiy Rozental 2001-2002.
//  (C) Copyright Beman Dawes 2000.
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
//  Description : test failures reported by BOOST_CHECK. Should fail during run.
// ***************************************************************************

// Boost.Test
#include <boost/test/test_tools.hpp>

int test_main( int, char *[] )  // note the name
{
    int v = 1;

    BOOST_CHECK( v == 2 );
    BOOST_CHECK( 2 == v );

    return 0;
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
//  25 Oct 01  Revisited version (Gennadiy Rozental)
//   7 Nov 00  Initial boost version (Beman Dawes)

// ***************************************************************************

// EOF
