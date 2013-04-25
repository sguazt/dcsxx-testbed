/**
 * \file dcs/testbed/detail/quantile.hpp
 *
 * \brief Sample quantile.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2013       Marco Guazzone (marco.guazzone@gmail.com)
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed (below referred to as "this program").
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_DETAIL_QUANTILE_HPP
#define DCS_TESTBED_DETAIL_QUANTILE_HPP


#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>


namespace dcs { namespace testbed { namespace detail {

enum quantile_category
{
    type1_quantile,
    type2_quantile,
    type3_quantile,
    type4_quantile,
    type5_quantile,
    type6_quantile,
    type7_quantile,
    type8_quantile,
    type9_quantile
};

template <typename T, typename IterT>
inline
T quantile(IterT first, IterT last, T prob, quantile_category cat = type7_quantile)
{
	T q(::std::numeric_limits<T>::quiet_NaN());

	::std::vector<T> data(first, last);
	::std::size_t n = data.size();

	if (n > 0)
	{
		if (cat == type7_quantile)
		{
			T index = 1+(n-1)*prob;
			::std::size_t lo = static_cast< ::std::size_t >(::std::floor(index));
			::std::size_t hi = static_cast< ::std::size_t >(::std::ceil(index));
			::std::partial_sort(data.begin(), data.begin()+hi, data.end());
			//::std::sort(data.begin(), data.end());
			q = data[lo-1];
			T h = index-lo; // h > 0, by construction
			q = (1-h)*q+h*data[hi-1];
		}
		else
		{
/*TODO
			if (cat == type1_quantile || cat == type2_quantile || cat == type3_quantile)
			{
				// Types 1, 2 and 3 are discontinuous sample quantiles
				T nppm = (cat == type3_quantile) ? (n*prob-.5) : (n*prob);
				::std::size_t j = static_cast< ::std::size_t >(::std::floor(nppm));
				::std::size_t h;
				switch (cat)
				{
					case type1_quantile:
						h = nppm > j;
						break;
					case type2_quantile:
						h = ((nppm > j)+1)/2;
						break;
					case type3_quantile:
						h = (nppm != j) | ((j % 2) == 1);
						break;
					default:
						break;
				}
			}
			else
			{
				// Types 4 through 9 are continuous sample quantiles
				T a;
				T b;

				switch (cat)
				{
					case type4_quantile:
						a = 0;
						b = 1;
						break;
					case type5_quantile:
						a = b = 0.5;
						break;
					case type6_quantile:
						a = b = 0;
						break;
					case type7_quantile:
						a = b = 1;
						break;
					case type8_quantile:
						a = b = 1/3.0;
						break;
					case type9_quantile:
						a = b = 3/8.0;
						break;
					default:
						break;
				}
				// need to watch for rounding errors here
				T fuzz = 4.0*::std::numeric_limits<T>::epsilon();
				nppm = a+prob*(n+1-a-b);
				j = ::std::floor(nppm+fuzz);
				h = nppm-j;
				if (::std::abs(h) < fuzz)
				{
					h = 0;
				}
			}
			::std::partial_sort(data.begin(), data.end());
			::std::vector<T> x;
			x.push_back(data.front());
			x.push_back(data.front());
			x.insert(x.end(), data.begin(), data.end());
			x.push_back(data.back());
			q = x[j+2];
			if (h == 1)
			{
				q = x[j+3];
			}
			other = (0<h) & (h<1);
			if (other)
			{
				q = ((1-h)*x[j+2] + h*x[j+3]);
			}
*/
		}
    }

    return q;
}

}}} // Namespace dcs::testbed::detail

#endif // DCS_TESTBED_DETAIL_QUANTILE_HPP
