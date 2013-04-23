/**
 * \file dcs/testbed/data_smoothers.hpp
 *
 * \brief Classes to smooth data.
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

#ifndef DCS_TESTBED_DATA_SMOOTHERS_HPP
#define DCS_TESTBED_DATA_SMOOTHERS_HPP


#include <dcs/debug.hpp>
#include <dcs/macro.hpp>
#include <limits>
#include <vector>


namespace dcs { namespace testbed {

template <typename ValueT>
class base_smoother
{
	public: typedef ValueT value_type;
	protected: typedef ::std::vector<value_type> data_container;


	public: value_type smooth(value_type val)
	{
		return this->do_smooth(::std::vector<value_type>(1, val));
	}

	public: template <typename IterT>
			value_type smooth(IterT first, IterT last)
	{
		return this->do_smooth(::std::vector<value_type>(first, last));
	}

	public: value_type forecast(unsigned int t) const
	{
		return this->do_forecast(t);
	}

	public: void reset()
	{
		return this->do_reset();
	}

	private: virtual value_type do_smooth(data_container const& data) = 0; 
	private: virtual value_type do_forecast(unsigned int t) const = 0;
	private: virtual void do_reset() = 0;
};

template <typename ValueT>
class dummy_smoother: public base_smoother<ValueT>
{
	private: typedef base_smoother<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: dummy_smoother()
	: v_(::std::numeric_limits<value_type>::quiet_NaN())
	{
	}

	private: value_type do_smooth(data_container const& data)
	{
		v_ = data.back();

		return v_;
	}

	private: value_type do_forecast(unsigned int t) const
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING(t);

		return v_;
	}

	private: void do_reset()
	{
		v_ = ::std::numeric_limits<value_type>::quiet_NaN();
	}

	private: value_type v_;
}; // dummy_smoother

template <typename ValueT>
class brown_single_exponential_smoother: public base_smoother<ValueT>
{
	private: typedef base_smoother<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit brown_single_exponential_smoother(double alpha)
	: alpha_(alpha),
	  s_(0),
	  init_(true)
	{
	}

	private: value_type do_smooth(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			if (init_)
			{
				s_ = val;
				init_ = false;
			}
			else
			{
				s_ = alpha_*val+(1-alpha_)*s_;
			}
		}

		return s_;
	}

	private: value_type do_forecast(unsigned int t) const
	{
		DCS_MACRO_SUPPRESS_UNUSED_VARIABLE_WARNING(t);

		return s_;
	}

	private: void do_reset()
	{
		init_ = true;
		s_ = 0;
	}


	private: double alpha_;
	private: value_type s_;
	private: bool init_;
}; // brown_single_exponential_smoother

template <typename ValueT>
class holt_winters_double_exponential_smoother: public base_smoother<ValueT>
{
	private: typedef base_smoother<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit holt_winters_double_exponential_smoother(double delta)
	: alpha_(1-(1-delta)*(1-delta)),
	  beta_(delta*delta/alpha_),
	  s_(0),
	  b_(0),
	  init_s_(true),
	  init_b_(true)
	{
	}

	public: holt_winters_double_exponential_smoother(double alpha, double beta)
	: alpha_(alpha),
	  beta_(beta),
	  s_(0),
	  init_s_(true),
	  init_b_(true)
	{
	}

	private: value_type do_smooth(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			if (init_s_)
			{
				s_ = val;
				init_s_ = false;
			}
			else if (init_b_)
			{
				init_b_ = false;
				b_ = val - s_;
			}
			else
			{
				value_type old_s(s_);
				s_ = alpha_*val+(1-alpha_)*(old_s+b_);
				b_ = beta_*(s_-old_s)+(1-beta_)*b_;
			}
		}

		return s_;
	}

	private: value_type do_forecast(unsigned int t) const
	{
		return s_+t*b_;
	}

	private: void do_reset()
	{
		init_s_ = init_b_ = true;
		s_ = b_ = 0;
	}


	private: double alpha_;
	private: double beta_;
	private: value_type s_;
	private: value_type b_;
	private: bool init_s_;
	private: bool init_b_;
}; // holt_winters_double_exponential_smoother

template <typename ValueT>
class brown_double_exponential_smoother: public base_smoother<ValueT>
{
	private: typedef base_smoother<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit brown_double_exponential_smoother(double alpha)
	: alpha_(alpha),
	  s1_(0),
	  s2_(0),
	  a_(0),
	  b_(0),
	  init_s_(true)
	{
	}

	private: value_type do_smooth(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			if (init_s_)
			{
				s1_ = s2_ = val;
				init_s_ = false;
			}
			else
			{
				s1_ = alpha_*val+(1-alpha_)*s1_;
				s2_ = alpha_*s1_+(1-alpha_)*s2_;
			}
		}

		a_ = 2*s1_-s2_;
		b_ = (alpha_/(1-alpha_))*(s1_-s2_);

		return a_;
	}

	private: value_type do_forecast(unsigned int t) const
	{
		return a_+t*b_;
	}

	private: void do_reset()
	{
		init_s_ = true;
		s1_ = s2_ = a_ = b_ = 0;
	}


	private: double alpha_;
	private: value_type s1_;
	private: value_type s2_;
	private: value_type a_;
	private: value_type b_;
	private: bool init_s_;
}; // brown_double_exponential_smoother

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DATA_SMOOTHERS_HPP
