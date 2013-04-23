/**
 * \file dcs/testbed/system_management.hpp
 *
 * \brief Performs system management experiments.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright (C) 2012       Marco Guazzone
 *                          [Distributed Computing System (DCS) Group,
 *                           Computer Science Institute,
 *                           Department of Science and Technological Innovation,
 *                           University of Piemonte Orientale,
 *                           Alessandria (Italy)]
 *
 * This file is part of dcsxx-testbed.
 *
 * dcsxx-testbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dcsxx-testbed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dcsxx-testbed.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_SYSTEM_MANAGEMENT_HPP
#define DCS_TESTBED_SYSTEM_MANAGEMENT_HPP

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#if defined(DCSXX_TESTBED_EXP_USE_MEAN_ESTIMATOR)
# include <boost/accumulators/statistics/mean.hpp>
#elif defined(DCSXX_TESTBED_EXP_USE_QUANTILE_ESTIMATOR)
# include <boost/accumulators/statistics/p_square_quantile.hpp>
#endif // DCSXX_TESTBED_EXP_USE_*_ESTIMATOR
#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <ctime>
#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/exception.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#ifdef DCS_DEBUG
# include <boost/numeric/ublas/io.hpp>
#endif // DCS_DEBUG


namespace dcs { namespace testbed {

template <typename RealT>
class system_management
{
	public: typedef RealT real_type;
//	public: typedef base_virtual_machine<real_type> vm_type;
//	public: typedef ::boost::shared_ptr<vm_type> vm_pointer;
	public: typedef base_workload_driver workload_driver_type;
	public: typedef ::boost::shared_ptr<workload_driver_type> workload_driver_pointer;
//	private: typedef ::std::vector<vm_pointer> vm_container;
	public: typedef base_estimator<real_type> estimator_type;
	public: typedef ::boost::shared_ptr<estimator_type> estimator_pointer;
	public: typedef base_smoother<real_type> smoother_type;
	public: typedef ::boost::shared_ptr<smoother_type> smoother_pointer;


	private: static const unsigned int default_sampling_time = 10;
	private: static const ::std::string default_output_data_file_path;


	/// Default constructor.
	public: system_management()
	: ts_(default_sampling_time),
	  out_dat_file_(default_output_data_file_path)
	{
	}

	/// A constructor.
	public: //template <typename FwdIterT>
			system_management(/*FwdIterT vm_first, FwdIterT vm_last, */workload_driver_pointer const& p_wkl_driver)
	: /*vms_(vm_first, vm_last),*/
	  p_wkl_driver_(p_wkl_driver),
	  ts_(default_sampling_time),
	  out_dat_file_(default_output_data_file_path)
	{
	}

	/// Set the path of the output data file.
	public: void output_data_file(::std::string const& s)
	{
		// pre: s != ""
		DCS_ASSERT(!s.empty(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Cannot use empty string as output data file name"));

		out_dat_file_ = s;
	}

	/// Set the sampling time.
	public: void sampling_time(real_type t)
	{
		// pre: t > 0
		DCS_ASSERT(t > 0,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Sampling time must be positive"));
		// pre: t <= max value
		DCS_ASSERT(t <= ::std::numeric_limits<unsigned int>::max(),
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Sampling time too large"));

		ts_ = static_cast<unsigned int>(t);
	}

	/// Set the data estimator
	public: void data_estimator(estimator_pointer const& p_estimator)
	{
		// pre: 0 <= v <= 1
		DCS_ASSERT(p_estimator,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid pointer to data estimator"));

		p_estimator_ = p_estimator;
	}

	/// Set the data smoother
	public: void data_smoother(smoother_pointer const& p_smoother)
	{
		// pre: 0 <= v <= 1
		DCS_ASSERT(p_smoother,
				   DCS_EXCEPTION_THROW(::std::invalid_argument,
									   "Invalid pointer to data smoother"));

		p_smoother_ = p_smoother;
	}

//	/**
//	 * \brief Perform system management by using as initial shares the
//	 *  100% of resource.
//	 */
//	public: void run()
//	{
//		::std::vector<real_type> init_shares(vms_.size(), 1);
//
//		this->run(init_shares.begin(), init_shares.end());
//	}

	/**
	 * \brief Perform system management with the given initial shares.
	 */
	public: /*template <typename FwdIterT>*/
			void run(/*FwdIterT share_first, FwdIterT share_end*/)
	{
//		// distance(share_first,share_end) == size(vms_)
//		DCS_ASSERT(static_cast< ::std::size_t >(::std::distance(share_first, share_end)) == vms_.size(),
//				   DCS_EXCEPTION_THROW(::std::invalid_argument,
//									   "Share container size does not match"));
//
//		typedef typename vm_container::const_iterator vm_iterator;
//		typedef typename vm_type::identifier_type vm_identifier_type;

		DCS_DEBUG_TRACE( "BEGIN Execution of System Management" );

/*
		vm_iterator vm_end_it(vms_.end());
		vm_iterator vm_beg_it(vms_.begin());

		if (vm_beg_it == vm_end_it)
		{
			// No VMs -> don't run anything
			return;
		}
*/

/*
		// Open output data file
		::std::ofstream ofs(out_dat_file_.c_str());
		if (!ofs.good())
		{
			::std::ostringstream oss;
			oss << "Cannot open output data file '" << out_dat_file_ << "'";

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
*/

/*
		// Write first part of header to output file
		ofs << "\"Sampling Time\"";

		// Get current shares in order to restore them at the end of execution
		share_container old_shares;
		for (vm_iterator vm_it = vm_beg_it;
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			old_shares.push_back(p_vm->cpu_share());
		}

		// Set initial shares
		for (vm_iterator vm_it = vm_beg_it;
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			// Set share
			p_vm->cpu_share(*share_first);

			// Write VM-related stuff of header to output file
			ofs << ",\"" << p_vm->name() << " CPU Share\"";

			++share_first;
		}

		// Write last part of header to output file
		ofs << ",\"Operation Time\",\"Operation Name\",\"Performance Index\",\"Entry Type\"" << ::std::endl;
*/

		// Start the workload driver
		p_wkl_driver_->start();

		// Set shares according to the given signal
		::std::time_t t0(-1);
		::std::time_t t1(-1);
		::std::time(&t1);
		while (!p_wkl_driver_->done())
		{
			DCS_DEBUG_TRACE( "   Driver is alive" );
			if (p_wkl_driver_->ready() && p_wkl_driver_->has_observation())
			{
				// Stringstream used to hold common output info
				::std::ostringstream oss;

				// Compute the elapsed time
				::std::time(&t1);
				if (t0 == -1)
				{
					// Set t0 to the start of observation collection phase
					t0 = t1;
				}
				double dt = ::std::difftime(t1, t0);

				DCS_DEBUG_TRACE( "-- Time " << dt );

				oss << dt;

/*
				// Generate new shares
				share_container share((*p_sig_gen_)());

				// check: consistency
				DCS_DEBUG_ASSERT( share.size() == vms_.size() );

				DCS_DEBUG_TRACE( "   Generated shares: " << dcs::debug::to_string(share.begin(), share.end()) );

				// Set new shares to every VM
				::std::size_t ix(0);
				for (vm_iterator vm_it = vm_beg_it;
					 vm_it != vm_end_it;
					 ++vm_it)
				{
					vm_pointer p_vm(*vm_it);

					// check: not null
					DCS_DEBUG_ASSERT( p_vm );

					DCS_DEBUG_TRACE( "   VM '" << p_vm->name() << "' :: Old CPU share: " << p_vm->cpu_share() << " :: New CPU share: " << share[ix] );

					oss << "," << p_vm->cpu_share();

					p_vm->cpu_share(share[ix]);

					++ix;
				}
*/
#ifdef DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
				::std::ofstream tmp_ofs("./smoothed_data_dump.log", ::std::ios_base::out | ::std::ios_base::app);
				if (!tmp_ofs.good())
				{
					::std::ostringstream oss;
					oss << "Cannot open output data file 'smoothed_data_dump.log'";

					DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
				}
#endif // DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA

				// Get collected observations
				typedef typename workload_driver_type::observation_type observation_type;
				typedef ::std::vector<observation_type> obs_container;
				typedef typename obs_container::const_iterator obs_iterator;
				obs_container obs = p_wkl_driver_->observations();
				obs_iterator obs_end_it(obs.end());
#ifdef DCSXX_TESTBED_EXP_RESET_ESTIMATION_EVERY_INTERVAL
				p_estimator_->reset();
#endif // DCSXX_TESTBED_EXP_RESET_ESTIMATION_EVERY_INTERVAL
				for (obs_iterator obs_it = obs.begin();
					 obs_it != obs_end_it;
					 ++obs_it)
				{
					real_type val(obs_it->value());
#ifdef DCSXX_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION
					p_smoother_->smooth(val);
# ifdef DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
					tmp_ofs << "UPDATE - VAL: " << val << " - SMOOTHED: " << p_smoother_->forecast(0) << ::std::endl;
# endif // DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
#else // DCSXX_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION
					p_estimator_->collect(val);
#endif // DCSXX_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION
				}

#ifndef DCSXX_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION
				// Compute a summary statistics of collected observation
				real_type summary_obs = p_estimator_->estimate();

				DCS_DEBUG_TRACE( "   Current (summary) observation: " << summary_obs );

				p_smoother_->smooth(summary_obs);
				::std::cerr << "UPDATE - VAL: " << summary_obs << " - SMOOTHED: " << p_smoother_->forecast(0) << ::std::endl;
# ifdef DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
				tmp_ofs << "UPDATE - VAL: " << summary_obs << " - SMOOTHED: " << p_smoother_->forecast(0) << ::std::endl;
# endif // DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
#endif // DCSXX_TESTBED_EXP_SMOOTHING_ON_SINGLE_OBSERVATION

				DCS_DEBUG_TRACE( "   Current SMOOTHED (summary) observation: " << p_smoother_->forecast(0) );

				// Open output data file
				::std::ofstream ofs(out_dat_file_.c_str());
				if (!ofs.good())
				{
					::std::ostringstream oss;
					oss << "Cannot open output data file '" << out_dat_file_ << "'";

					DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
				}

				// Write current observation (and overwrite old ones)
				ofs << p_smoother_->forecast(0) << ::std::endl;

#ifdef DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
				// Write current observation (and overwrite old ones)
				tmp_ofs << "FINAL SMOOTHED: " << p_smoother_->forecast(0) << ::std::endl;
#endif // DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
				// Close output data file
				ofs.close();
#ifdef DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
				// Close output data file
				tmp_ofs.close();
#endif // DCSXX_TESTBED_EXP_DUMP_SMOOTHED_DATA
			}

			// Wait until the next sampling time
			DCS_DEBUG_TRACE( "   Zzz... (: " << ts_ << ")" );
			::sleep(ts_);
		}

		// Stop the workload driver
		p_wkl_driver_->stop();

//		// Close output data file
//		ofs.close();

/*
		// Reset VM shares to values that VMs had just before running the driver
		::std::size_t ix(0);
		for (vm_iterator vm_it = vm_beg_it;
			 vm_it != vm_end_it;
			 ++vm_it)
		{
			vm_pointer p_vm(*vm_it);

			p_vm->cpu_share(old_shares[ix]);
			++ix;
		}
*/

		DCS_DEBUG_TRACE( "END Execution of System Management" );
	}


//	private: vm_container vms_; ///< VMs container
	private: workload_driver_pointer p_wkl_driver_; ///< Ptr to workload driver
//	private: signal_generator_pointer p_sig_gen_; ///< Ptr to signal generator used to excite VMs
	private: unsigned int ts_; ///< The sampling time
	private: ::std::string out_dat_file_; ///< The path to the output data file
	private: estimator_pointer p_estimator_;
	private: smoother_pointer p_smoother_;
}; // system_management

template <typename RealT>
const ::std::string system_management<RealT>::default_output_data_file_path("./sysmgnt_out.dat");

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_SYSTEM_MANAGEMENT_HPP
