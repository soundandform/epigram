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

	namespace detail // https://stackoverflow.com/questions/35293470/checking-if-a-type-is-a-map
	{
	  // Needed for some older versions of GCC
	  template<typename...>
		struct voider { using type = void; };

	  // std::void_t will be part of C++17, but until then define it ourselves:
	  template<typename... T>
		using void_t = typename voider<T...>::type;

	  template<typename T, typename U = void>
		struct is_mappish_impl : std::false_type { };

	  template<typename T>
		struct is_mappish_impl<T, void_t<typename T::key_type,
										 typename T::mapped_type,
										 decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>>
		: std::true_type { };
	}

	template<typename T>
	struct is_mappish : detail::is_mappish_impl<T>::type { };

}
