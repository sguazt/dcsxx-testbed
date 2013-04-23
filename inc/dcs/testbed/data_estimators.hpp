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


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <dcs/debug.hpp>
#include <dcs/macro.hpp>
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

template <typename ValueT>
class p2_quantile_estimator: public base_estimator<ValueT>
{
	private: typedef base_estimator<ValueT> base_type;
	public: typedef typename base_type::value_type value_type;
	private: typedef typename base_type::data_container data_container;
	private: typedef ::boost::accumulators::accumulator_set<value_type,
															::boost::accumulators::stats< ::boost::accumulators::tag::p_square_quantile > > accumulator_type;


	public: explicit p2_quantile_estimator(value_type prob)
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
}; // mean_estimator

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_DATA_ESTIMATORS_HPP
