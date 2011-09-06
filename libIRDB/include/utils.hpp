


#include <sstream>
#include <map>
#include <algorithm>
#include <set>

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
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
