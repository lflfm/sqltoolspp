
// (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
// Hinnant & John Maddock 2000.  Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_TT_IS_CONST_HPP_INCLUDED
#define BOOST_TT_IS_CONST_HPP_INCLUDED

#include "boost/config.hpp"

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#   include "boost/type_traits/detail/cv_traits_impl.hpp"
#   ifdef __GNUC__
#       include <boost/type_traits/is_reference.hpp>
#   endif
#else
#   include "boost/type_traits/is_reference.hpp"
#   include "boost/type_traits/is_array.hpp"
#   include "boost/type_traits/detail/yes_no_type.hpp"
#   include "boost/type_traits/detail/false_result.hpp"
#endif

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

namespace boost {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

//* is a type T  declared const - is_const<T>
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_const,T,::boost::detail::cv_traits_imp<T*>::is_const)
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T&,false)

#if defined(__BORLANDC__)
// these are illegal specialisations; cv-qualifies applied to
// references have no effect according to [8.3.2p1],
// C++ Builder requires them though as it treats cv-qualified
// references as distinct types...
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T& const,false)
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T& volatile,false)
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T& const volatile,false)
#endif

#ifdef __GNUC__
// special case for gcc where illegally cv-qualified reference types can be
// generated in some corner cases:
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T const,!(::boost::is_reference<T>::value))
BOOST_TT_AUX_BOOL_TRAIT_PARTIAL_SPEC1_1(typename T,is_const,T volatile const,!(::boost::is_reference<T>::value))
#endif

#else

namespace detail {

using ::boost::type_traits::yes_type;
using ::boost::type_traits::no_type;

yes_type is_const_tester(const volatile void*);
no_type is_const_tester(volatile void *);

template <bool is_ref, bool array>
struct is_const_helper
    : ::boost::type_traits::false_result
{
};

template <>
struct is_const_helper<false,false>
{
    template <typename T> struct result_
    {
        static T* t;
        BOOST_STATIC_CONSTANT(bool, value = (
            sizeof(detail::yes_type) == sizeof(detail::is_const_tester(t))
            ));
    };      
};

template <>
struct is_const_helper<false,true>
{
    template <typename T> struct result_
    {
        static T t;
        BOOST_STATIC_CONSTANT(bool, value = (
            sizeof(detail::yes_type) == sizeof(detail::is_const_tester(&t))
            ));
    };      
};

template <typename T>
struct is_const_impl
    : is_const_helper<
          is_reference<T>::value
        , is_array<T>::value
        >::template result_<T>
{ 
};

} // namespace detail

//* is a type T  declared const - is_const<T>
BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_const,T,::boost::detail::is_const_impl<T>::value)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(is_const,void,false)
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(is_const,void const,true)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(is_const,void volatile,false)
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(is_const,void const volatile,true)
#endif

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_CONST_HPP_INCLUDED
