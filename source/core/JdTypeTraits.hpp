//
//  JdTypeTraits.hpp
//
//

#include <vector>
#include <set>
#include <list>
#include <deque>
#include <type_traits>

namespace jd
{
	template <typename T>
	struct has_iterator : std::false_type { };
	
	template <typename... Ts> struct has_iterator <std::list	<Ts...>> : std::true_type { };
	template <typename... Ts> struct has_iterator <std::vector	<Ts...>> : std::true_type { };
	template <typename... Ts> struct has_iterator <std::deque	<Ts...>> : std::true_type { };
	template <typename... Ts> struct has_iterator <std::set		<Ts...>> : std::true_type { };
	
}
