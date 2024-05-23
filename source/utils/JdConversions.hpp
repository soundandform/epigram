//
//  JdConversions.hpp
//
//  Created by Steven Massey on 5/22/24.
//

#ifndef JdConversions_hpp
#define JdConversions_hpp

# include <algorithm>

template <typename T, typename Tmin, typename Tmax>
void ConstrainToRange (T & io_value, const Tmin & i_min, const Tmax & i_max)
{
	io_value = std::min (io_value, (T) i_max);
	io_value = std::max (io_value, (T) i_min);
}



#endif /* Conversions_hpp */
