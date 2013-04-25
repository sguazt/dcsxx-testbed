/**
 * \file dcs/testbed/data_estimators.hpp
 *
 * \brief Classes to estimate data.
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

#ifndef DCS_TESTBED_DATA_ESTIMATORS_HPP
#define DCS_TESTBED_DATA_ESTIMATORS_HPP


#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <cmath>
#include <cstddef>
#include <dcs/debug.hpp>
#include <dcs/macro.hpp>
#include <dcs/testbed/detail/quantile.hpp>
#include <vector>


namespace dcs { namespace testbed {

template <typename ValueT>
class base_estimator
{
	public: typedef ValueT value_type;
	protected: typedef ::std::vector<value_type> data_container;


	public: void collect(value_type val)
	{
		this->do_collect(::std::vector<value_type>(1, val));
	}

	public: template <typename IterT>
			void collect(IterT first, IterT last)
	{
		this->do_collect(::std::vector<value_type>(first, last));
	}

	public: value_type estimate() const
	{
		return this->do_estimate();
	}

	public: void reset()
	{
		return this->do_reset();
	}

	private: virtual void do_collect(data_container const& data) = 0; 
	private: virtual value_type do_estimate() const = 0;
	private: virtual void do_reset() = 0;
};

template <typename ValueT>
class mean_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::mean > > accumulator_type;


	public: explicit mean_estimator()
	: acc_()
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			acc_(val);
		}
	}

	private: value_type do_estimate() const
	{
		return ::boost::accumulators::mean(acc_);
	}

	private: void do_reset()
	{
		acc_ = accumulator_type();
	}


	private: accumulator_type acc_;
}; // mean_estimator

/**
 * The P^2 Algorithm for incremental quantile estimation.
 *
 * From:
 *  Raj Jain and Imrich Chlamtac,
 *  "The P^2 Algorithm for Dynamic Calculation of Quantiles and Histograms Without Storing Observations"
 *  Communications of the ACM, 28(10):1076-1085 1985
 */
template <typename ValueT>
class jain1985_p2_algorithm_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::p_square_quantile > > accumulator_type;


	public: explicit jain1985_p2_algorithm_quantile_estimator(value_type prob)
	: prob_(prob),
	  acc_(::boost::accumulators::quantile_probability = prob)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			acc_(val);
		}
	}

	private: value_type do_estimate() const
	{
		return ::boost::accumulators::p_square_quantile(acc_);
	}

	private: void do_reset()
	{
		acc_ = accumulator_type(::boost::accumulators::quantile_probability = prob_);
	}


	private: value_type prob_;
	private: accumulator_type acc_;
}; // jain1985_p2_algorithm_quantile_estimator

/**
 * The EWMA-based incremental quantile estimation method from (Welsh,2003).
 *
 * The extended version extends the original one, by computing the quantile with a linear interpolation.
 *
 * From:
 *  Matt Welsh and David Culler,
 *  "Adaptive Overload Control for Busy Internet Servers",
 *  In Proc. of the 4th Conference on USENIX Symposium on Internet Technologies and Systems (USITS'03), 2003
 */
template <typename ValueT>
class welsh2003_ewma_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: explicit welsh2003_ewma_quantile_estimator(value_type prob, value_type alpha, bool extended = false)
	: prob_(prob),
	  alpha_(alpha),
	  data_(),
	  ewma_(0),
	  init_(true),
	  ext_(extended)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			data_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		value_type q(0);

		if (ext_)
		{
			q = detail::quantile(data_.begin(), data_.end(), prob_);
		}
		else
		{
			::std::size_t np = ::std::min(static_cast< ::std::size_t >(::std::ceil(prob_*data_.size())), data_.size());
			::std::partial_sort(data_.begin(), data_.begin()+np, data_.end());
			q = data_[np-1];
		}

		if (init_)
		{
			ewma_ = q;
			init_ = false;
		}
		else
		{
			ewma_ = alpha_*ewma_+(1-alpha_)*q;
		}
		data_.clear();

		return ewma_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		ewma_ = 0;
	}


	private: value_type prob_;
	private: value_type alpha_;
	private: mutable data_container data_;
	private: mutable value_type ewma_;
	private: mutable bool init_;
	private: bool ext_;
}; // welsh2003_ewma_quantile_estimator


/**
 * The EWMA-based incremental quantile estimation method from (Welsh,2003).
 *
 *
// From:
// //  Fei Chen and Diane Lambert and José C. Pinheiro
// //  "Incremental Quantile Estimation for Massive Tracking",
// //  In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
// //
//
 */
template <typename ValueT>
class chen2000_ewma_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: chen2000_ewma_quantile_estimator(value_type prob, value_type w)
	: prob_(prob),
	  w_(w),
	  data_(),
	  ewma_(0),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			data_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		value_type q = detail::quantile(data_.begin(), data_.end(), prob_);

		if (init_)
		{
			ewma_ = q;
			init_ = false;
		}
		else
		{
			ewma_ = (1-w_)*ewma_+w_*q;
		}
		data_.clear();

		return ewma_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		ewma_ = 0;
	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable value_type ewma_;
	private: mutable bool init_;
}; // chen2000_ewma_quantile_estimator

//
//// From:
////  Fei Chen and Diane Lambert and José C. Pinheiro
////  "Incremental Quantile Estimation for Massive Tracking",
////  In Proc. of the 6th ACM SIGKDD International Conference on Knowledge Discovery and Data Mining (KDD'00), 2000
////
template <typename ValueT>
class chen2000_ewsa_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;


	public: chen2000_ewsa_quantile_estimator(value_type prob, value_type w)
	: prob_(prob),
	  w_(w),
	  data_(),
	  s_(::std::numeric_limits<value_type>::quiet_NaN()),
	  f_(::std::numeric_limits<value_type>::quiet_NaN()),
	  r_(::std::numeric_limits<value_type>::quiet_NaN()),
	  c_(::std::numeric_limits<value_type>::quiet_NaN()),
	  init_(true)
	{
	}

	private: void do_collect(data_container const& data)
	{
		typedef typename data_container::const_iterator data_iterator;

		data_iterator data_end_it(data.end());
		for (data_iterator data_it = data.begin(); data_it != data_end_it; ++data_it)
		{
			value_type val(*data_it);

			data_.push_back(val);
		}
	}

	private: value_type do_estimate() const
	{
		const ::std::size_t m(data_.size());

		if (m > 0)
		{
			const value_type q25 = detail::quantile(data_.begin(), data_.end(), 0.25);
			const value_type q75 = detail::quantile(data_.begin(), data_.end(), 0.75);

			if (init_)
			{
				s_ = detail::quantile(data_.begin(), data_.end(), prob_);
				r_ = q75-q25;
				value_type c(1);
				for (::std::size_t i = 2; i <= m; ++i)
				{
					c += 1.0/::std::sqrt(i);
				}
				c_ = r_*c/m;
				::std::size_t cnt(0);
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (::std::abs(data_[i]-s_) <= c_)
					{
						++cnt;
					}
				}
				f_ = 1.0/(2*c_*m)*::std::max(cnt,::std::size_t(1));
				init_ = false;
			}
			else
			{
				::std::size_t scnt(0);
				::std::size_t fcnt(0);
				for (::std::size_t i = 0; i < m; ++i)
				{
					if (data_[i] <= s_)
					{
						++scnt;
					}
					if (::std::abs(data_[i]-s_) <= c_)
					{
						++fcnt;
					}
				}
				s_ = s_ + (w_/f_)*(prob_-scnt/static_cast<value_type>(m));
				f_ = (1-w_)*f_ + (w_/(2*c_*m))*fcnt;
				r_ = q75-q25;
				value_type c(0);
				const ::std::size_t m2(2*m);
				for (::std::size_t i = m+1; i <= m2; ++i)
				{
					c += 1.0/::std::sqrt(i);
				}
				c_ = r_*c/m;
			}
			data_.clear();
		}

		return s_;
	}

	private: void do_reset()
	{
		init_ = true;
		data_.clear();
		s_ = f_ = r_ = c_ = ::std::numeric_limits<value_type>::quiet_NaN();
	}


	private: value_type prob_;
	private: value_type w_;
	private: mutable data_container data_;
	private: mutable value_type s_;
	private: mutable value_type f_;
	private: mutable value_type r_;
	private: mutable value_type c_;
	private: mutable bool init_;
}; // chen2000_ewsa_quantile_estimator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DATA_ESTIMATORS_HPP
