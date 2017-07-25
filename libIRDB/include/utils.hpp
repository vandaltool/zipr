/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */


#ifndef _IRDB_UTIL_
#define _IRDB_UTIL_

#include <sstream>
#include <map>
#include <algorithm>
#include <set>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <functional>
#include <utility>
#include <stdint.h>

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

template <class T>
inline std::string to_hex_string (const T& t)
{
        std::stringstream ss;
        ss << std::hex << t;
        return ss.str();
}

 

/*
 * is_in_container - a handle template function returning whether key S is contained in container T.
 */
template <class T, class S>
inline bool is_in_container(const T& container, const S& key)
{
	bool is_in=container.find(key) != container.end();
	return is_in;
}

template <class S>
inline bool is_in_set(const std::set<S>& container, const S& key)
{
	return std::find(container.begin(), container.end(), key) != container.end();
}

/* 
 * find_map_object - without modifying the object, return the element 
 */
template <class T, class S> 
inline S const& find_map_object( const std::map< T , S > &a_map, const T& key)
{
	typename std::map< T , S >::const_iterator it;

	it=a_map.find(key);

	assert(it!=a_map.end());

	return (*it).second;
}

typedef uintptr_t virtual_offset_t;

template<class T> inline T strtoint(std::string s)
{
        std::stringstream str(s);
        T off;

        str >> off;

        return off;
}

// nb: relies on srand() being previously set 
template <typename IterType, typename Funct>
inline Funct for_randomOrder_each(const IterType &b, const IterType & e, const Funct &callback)
{
       std::map<int,const typename std::iterator_traits<IterType>::value_type *> m;
       for_each(b,e, [&](const typename std::iterator_traits<IterType>::value_type & o)
       {
               while(true)
               {
                       int rn=rand();
                       if(m.find(rn)==m.end())
                       {
                               m[rn]=&o;
                               break;
                       }
               }
       });
       for_each(m.begin(), m.end(), [&](const std::pair<int,const typename std::iterator_traits<IterType>::value_type *> &p)
       {
               callback(*p.second);
       });
}


#endif
