/**
 * \file dcs/testbed/rain_workload_driver.hpp
 *
 * \brief Workload driver based on the RAIN workload toolkit.
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
 * along with dcsxx-testbed. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
#define DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP


#include <boost/smart_ptr.hpp>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dcs/exception.hpp>
#include <dcs/logging.hpp>
#include <dcs/system/posix_process.hpp>
#include <dcs/system/process_status_category.hpp>
#include <dcs/testbed/base_workload_driver.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <fstream>
#include <istream>
#include <list>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>


namespace dcs { namespace testbed {

namespace detail {

namespace /*<unnamed>*/ {

inline
::std::string make_java_command(::std::string const& java_home)
{
	return java_home + "/bin/java";
}

inline
::std::string make_java_command()
{
	char* java_path = ::std::getenv("JAVA_HOME");
	if (java_path)
	{
		return make_java_command(::std::string(java_path));
	}

	java_path = ::std::getenv("JRE_HOME");
	if (java_path)
	{
		return make_java_command(::std::string(java_path));
	}

	return ::std::string("java");
}

/**
 * \brief Build arguments to pass to RAIN workload toolkit.
 *
 * The basic structure of the RAIN command is:
 * <pre>
 *  java [<java-arg1> ... <java-argN>] \
 *       -cp "rain.jar:<path to workload JAR>" \
 *       radlab.rain.Benchmark <path to Rain JSON configuration file>
 * </pre>
 */
template <typename FwdIterT>
inline
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat,
											  ::std::string const& rain_home,
											  FwdIterT arg_first,
											  FwdIterT arg_last)
{
	::std::vector< ::std::string > args(arg_first, arg_last);

	::std::string workload;
	switch (wkl_cat)
	{
		case olio_workload:
			workload = "olio";
			break;
		case rubis_workload:
			workload = "rubis";
			break;
		default:
		{
			::std::ostringstream oss;
			oss << "Workload '" << to_string(wkl_cat) << "' not handled";
			DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
		}
	}

	args.push_back("-cp");
	args.push_back(rain_home + "/rain.jar:" + rain_home + "/workloads/" + workload + ".jar");
	args.push_back("-Djava.util.logging.config.file=" + rain_home + "/config/logging.properties");
	args.push_back("radlab.rain.Benchmark");
	args.push_back(rain_home + "/config/rain.config." + workload + ".json");

	return args;
}

inline
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat,
											  ::std::string const& rain_home)
{
	::std::vector< ::std::string > java_args;
	java_args.push_back("-Xmx1g");
	java_args.push_back("-Xms256m");

	return make_rain_args(wkl_cat, rain_home, java_args.begin(), java_args.end());
}

inline
::std::vector< ::std::string > make_rain_args(workload_category wkl_cat)
{
	return make_rain_args(wkl_cat, ".");
}

inline
::std::string make_rain_metrics_file_path(workload_category wkl_cat, ::std::string const& path = ".", ::std::string const& suffix = "")
{
	::std::string workload;
	switch (wkl_cat)
	{
		case olio_workload:
			workload = "cloudstone";
			break;
		case rubis_workload:
			workload = "rubis";
			break;
		default:
		{
			::std::ostringstream oss;
			oss << "Workload '" << to_string(wkl_cat) << "' not handled";
			DCS_EXCEPTION_THROW(::std::invalid_argument, oss.str());
		}
	}

	return path + "/metrics-snapshots-" + workload + "-001-" + suffix + ".log";
}

template <typename T>
inline
T atomic_get(::pthread_mutex_t& mutex, T const& attr)
{
	T ret;

	::pthread_mutex_lock(&mutex);
		ret = attr;
	::pthread_mutex_unlock(&mutex);

	return ret;
}

template <typename T>
inline
void atomic_set(::pthread_mutex_t& mutex, T& attr, T val)
{
	::pthread_mutex_lock(&mutex);
		attr = val;
	::pthread_mutex_unlock(&mutex);
}

} // Namespace <unnamed>

/// Monitors the RAIN ramp-up (transient) phase
static void* thread_monitor_rain_rampup(void* arg);

/**
 * \brief Monitors the RAIN steady-state phase and parses the RAIN metrics
 *  snapshot file.
 */
static void* thread_monitor_rain_steady_state(void* arg);

/// Redirects the RAIN steady-state standard output to a log file.
static void* thread_log_rain_steady_state_out(void* arg);

} // Namespace detail


class rain_workload_driver: public base_workload_driver
{
	private: typedef base_workload_driver base_type;
	private: typedef ::dcs::system::posix_process sys_process_type;
	private: typedef ::boost::shared_ptr<sys_process_type> sys_process_pointer;
	public: typedef base_type::observation_type observation_type;


	friend void* detail::thread_monitor_rain_rampup(void*);
	friend void* detail::thread_monitor_rain_steady_state(void*);
	friend void* detail::thread_log_rain_steady_state_out(void*);


	// We need to use such variable to initialize mutexes until C++11.
	// Instead, since C++11, we can replace it with 'extended initializer
	// lists' in the constructor, that is:
	//   public: rain_workload_driver(...)
	//   : ...,
	//     ready_mutex_(PTHREAD_MUTEX_INITIALIZER),
	//     obs_mutex_(PTHREAD_MUTEX_INITIALIZER)
	//   {...}
	private: static const ::pthread_mutex_t mutex_init_val_;
//	private: static const ::pthread_cond_t cond_init_val_;


	public: rain_workload_driver(workload_category wkl_cat)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(wkl_cat)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_),
	  rampup_thread_mutex_(mutex_init_val_),
	  steady_thread_mutex_(mutex_init_val_),
	  logger_thread_mutex_(mutex_init_val_)
//	  steady_state_mutex_(mutex_init_val_),
//	  steady_state_cond_(cond_init_val_)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home)
	: cmd_(detail::make_java_command()),
	  args_(detail::make_rain_args(wkl_cat, rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_),
	  rampup_thread_mutex_(mutex_init_val_),
	  steady_thread_mutex_(mutex_init_val_),
	  logger_thread_mutex_(mutex_init_val_)
//	  steady_state_mutex_(mutex_init_val_),
//	  steady_state_cond_(cond_init_val_)
	{
	}

	public: rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(wkl_cat, rain_home)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_),
	  rampup_thread_mutex_(mutex_init_val_),
	  steady_thread_mutex_(mutex_init_val_),
	  logger_thread_mutex_(mutex_init_val_)
//	  steady_state_mutex_(mutex_init_val_),
//	  steady_state_cond_(cond_init_val_)
	{
	}

	public: template <typename FwdIterT>
			rain_workload_driver(workload_category wkl_cat,
								 ::std::string const& rain_home,
								 ::std::string const& java_home,
								 FwdIterT arg_first,
								 FwdIterT arg_last)
	: cmd_(detail::make_java_command(java_home)),
	  args_(detail::make_rain_args(wkl_cat, rain_home, arg_first, arg_last)),
	  metrics_path_(detail::make_rain_metrics_file_path(wkl_cat)),
	  ready_(false),
	  rampup_thread_active_(false),
	  steady_thread_active_(false),
	  logger_thread_active_(false),
	  ready_mutex_(mutex_init_val_),
	  obs_mutex_(mutex_init_val_),
	  rampup_thread_mutex_(mutex_init_val_),
	  steady_thread_mutex_(mutex_init_val_),
	  logger_thread_mutex_(mutex_init_val_)
//	  steady_state_mutex_(mutex_init_val_),
//	  steady_state_cond_(cond_init_val_)
	{
	}

	public: ~rain_workload_driver()
	{
		// Terminate process
		try
		{
			proc_.terminate();
		}
		catch (...)
		{
			// empty
		}
		// Remove threads
		if (this->rampup_thread_active())
		{
			::pthread_cancel(rampup_thread_);
			::pthread_join(rampup_thread_, 0);
		}
		if (this->steady_state_thread_active())
		{
			::pthread_cancel(steady_thread_);
			::pthread_join(steady_thread_, 0);
		}
		if (this->logger_thread_active())
		{
			::pthread_cancel(logger_thread_);
			::pthread_join(logger_thread_, 0);
		}
//		// Destroy condition variables
//		::pthread_cond_destroy(&steady_state_cond_);
		// Destroy mutexes
		::pthread_mutex_destroy(&ready_mutex_);
		::pthread_mutex_destroy(&obs_mutex_);
		::pthread_mutex_destroy(&rampup_thread_mutex_);
		::pthread_mutex_destroy(&steady_thread_mutex_);
		::pthread_mutex_destroy(&logger_thread_mutex_);
//		::pthread_mutex_destroy(&steady_state_mutex_);
	}

	public: ::std::string metrics_file_path() const
	{
		return metrics_path_;
	}

	private: void ready(bool val)
	{
		detail::atomic_set(ready_mutex_, ready_, val);
	}

	private: sys_process_type& process()
	{
		return proc_;
	}

	private: sys_process_type const& process() const
	{
		return proc_;
	}

	private: void add_observation(observation_type const& obs)
	{
		::pthread_mutex_lock(&obs_mutex_);
			obs_.push_back(obs);
		::pthread_mutex_unlock(&obs_mutex_);
	}

	private: void add_observation(::std::time_t ts,
								  ::std::string const& op,
								  real_type val)
	{
		this->add_observation(base_type::make_observation(ts, op, val));
	}

	private: ::pthread_t& rampup_thread()
	{
		return rampup_thread_;
	}

	private: ::pthread_t const& rampup_thread() const
	{
		return rampup_thread_;
	}

	private: void rampup_thread_active(bool val)
	{
		detail::atomic_set(rampup_thread_mutex_, rampup_thread_active_, val);
	}

	private: bool rampup_thread_active() const
	{
		return detail::atomic_get(rampup_thread_mutex_, rampup_thread_active_);
	}

	private: ::pthread_t& steady_state_thread()
	{
		return steady_thread_;
	}

	private: ::pthread_t const& steady_state_thread() const
	{
		return steady_thread_;
	}

	private: void steady_state_thread_active(bool val)
	{
		detail::atomic_set(steady_thread_mutex_, steady_thread_active_, val);
	}

	private: bool steady_state_thread_active() const
	{
		return detail::atomic_get(steady_thread_mutex_, steady_thread_active_);
	}

	private: ::pthread_t& logger_thread()
	{
		return logger_thread_;
	}

	private: ::pthread_t const& logger_thread() const
	{
		return logger_thread_;
	}

	private: void logger_thread_active(bool val)
	{
		detail::atomic_set(logger_thread_mutex_, logger_thread_active_, val);
	}

	private: bool logger_thread_active() const
	{
		return detail::atomic_get(logger_thread_mutex_, logger_thread_active_);
	}

	private: workload_generator_category do_category() const
	{
		return rain_workload_generator;
	}

	private: void do_start()
	{
		// Stop previously running process and thread (if any)
		if (proc_.alive())
		{
			proc_.terminate();
		}
		if (this->rampup_thread_active())
		{
			::pthread_cancel(rampup_thread_);
			::pthread_join(rampup_thread_, 0);
		}
		if (this->steady_state_thread_active())
		{
			::pthread_cancel(steady_thread_);
			::pthread_join(steady_thread_, 0);
		}
		if (this->logger_thread_active())
		{
			::pthread_cancel(logger_thread_);
			::pthread_join(logger_thread_, 0);
		}

		// Run a new process
		ready_ = false;
		this->rampup_thread_active(false);
		this->steady_state_thread_active(false);
		this->logger_thread_active(false);
		proc_.command(cmd_);
		proc_.asynch(true);
		proc_.run(args_.begin(), args_.end(), false, true);
		if (proc_.status() != ::dcs::system::running_process_status)
		{
			::std::ostringstream oss;
			oss << "Unable to start RAIN workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}

		// Run a thread to monitor RAIN ramp-up (transient) phase
		if (::pthread_create(&rampup_thread_, 0, &detail::thread_monitor_rain_rampup, this) != 0)
		{
			::std::ostringstream oss;
			oss << "Unable to start ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
		}
//		// Run a thread to monitor RAIN steady-state phase
//		if (::pthread_create(&steady_thread_, 0, &detail::thread_monitor_rain_steady_state, this) != 0)
//		{
//			::std::ostringstream oss;
//			oss << "Unable to start steady-state monitor thread for the RAIN workload driver: " << ::strerror(errno);
//
//			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
//		}
//		// Run a thread to log the RAIN standard output during the steady-state phase
//		if (::pthread_create(&steady_thread_, 0, &detail::thread_log_rain_steady_state_out, this) != 0)
//		{
//			::std::ostringstream oss;
//			oss << "Unable to start steady-state output logger thread for the RAIN workload driver: " << ::strerror(errno);
//
//			DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
//		}
	}

	private: void do_stop()
	{
		proc_.terminate();

		if (this->rampup_thread_active())
		{
			if (::pthread_cancel(rampup_thread_) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to cancel ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			if (::pthread_join(rampup_thread_, 0) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to join ramp-up phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->rampup_thread_active(false);
		}

		if (this->steady_state_thread_active())
		{
			if (::pthread_cancel(steady_thread_) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to cancel steady-state phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			if (::pthread_join(steady_thread_, 0) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to join steady-state phase monitor thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->steady_state_thread_active(false);
		}

		if (this->logger_thread_active())
		{
			if (::pthread_cancel(logger_thread_) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to cancel steady-state phase output logger thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			if (::pthread_join(logger_thread_, 0) != 0)
			{
				::std::ostringstream oss;
				oss << "Unable to join steady-state phase output logger thread for the RAIN workload driver: " << ::strerror(errno);

				DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
			}
			this->logger_thread_active(false);
		}
	}

	private: bool do_done() const
	{
//		bool ret(false);
//
//		::pthread_mutex_lock(&obs_mutex_);
//			ret = !proc_.alive();
//		::pthread_mutex_unlock(&obs_mutex_);
//
//		return ret;
		return !proc_.alive();
	}

	private: bool do_ready() const
	{
		return detail::atomic_get(ready_mutex_, ready_);
	}

	private: bool do_has_observation() const
	{
		bool ret(false);

		::pthread_mutex_lock(&obs_mutex_);
			ret = !obs_.empty();
		::pthread_mutex_unlock(&obs_mutex_);

		return ret;
	}

	private: ::std::vector<observation_type> do_observations() const
	{
		::std::vector<observation_type> obs;

		::pthread_mutex_lock(&obs_mutex_);
			while (!obs_.empty())
			{
				obs.push_back(obs_.front());
				obs_.pop_front();
			}
		::pthread_mutex_unlock(&obs_mutex_);

		return obs;
	}

/*
	private: real_type do_observation() const
	{
		//FIXME: parameterize the type of statistics the user want
		::boost::accumulators::accumulator_set< real_type, ::boost::accumulators::stats< ::boost::accumulators::tag::mean > > acc;

		::pthread_mutex_lock(&obs_mutex_);
			while (!obs_.empty())
			{
				acc(obs_.front());
				obs_.pop_front();
			}
		::pthread_mutex_unlock(&obs_mutex_);

		return ::boost::accumulators::mean(acc);
	}
*/


	private: ::std::string cmd_;
	private: ::std::vector< ::std::string > args_;
	private: ::std::string metrics_path_;
	private: bool ready_;
	private: bool rampup_thread_active_;
	private: bool steady_thread_active_;
	private: bool logger_thread_active_;
	private: sys_process_type proc_;
	private: mutable ::std::list<observation_type> obs_;
	private: ::pthread_t rampup_thread_;
	private: ::pthread_t steady_thread_;
	private: ::pthread_t logger_thread_;
	private: mutable ::pthread_mutex_t ready_mutex_;
	private: mutable ::pthread_mutex_t obs_mutex_;
	private: mutable ::pthread_mutex_t rampup_thread_mutex_;
	private: mutable ::pthread_mutex_t steady_thread_mutex_;
	private: mutable ::pthread_mutex_t logger_thread_mutex_;
//	private: mutable ::pthread_mutex_t steady_state_mutex_;
//	private: mutable ::pthread_cond_t steady_state_cond_;
}; // rain_workload_driver

const ::pthread_mutex_t rain_workload_driver::mutex_init_val_ = PTHREAD_MUTEX_INITIALIZER;

//const ::pthread_cond_t rain_workload_driver::cond_init_val_ = PTHREAD_COND_INITIALIZER;


namespace detail {

void* thread_monitor_rain_rampup(void* arg)
{
DCS_DEBUG_TRACE("RAMP-UP THREAD -- Entering");
	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	p_driver->rampup_thread_active(true);

//	::pthread_mutex_lock(&(p_driver->steady_state_mutex_));

	::std::istream& is = p_driver->process().output_stream();

	while (is.good())
	{
		::std::string line;

		::std::getline(is, line);

		// Look for "Ramp up finished!" string
		if (line.find("Ramp up finished") != ::std::string::npos)
		{
			p_driver->ready(true);
			break;
		}
	}

	p_driver->rampup_thread_active(false);

	// Run a thread to monitor RAIN steady-state phase
	if (::pthread_create(&(p_driver->steady_thread_), 0, &detail::thread_monitor_rain_steady_state, p_driver) != 0)
	{
		::std::ostringstream oss;
		oss << "Unable to start steady-state monitor thread for the RAIN workload driver: " << ::strerror(errno);

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
	// Run a thread to log the RAIN standard output during the steady-state phase
	if (::pthread_create(&(p_driver->steady_thread_), 0, &detail::thread_log_rain_steady_state_out, p_driver) != 0)
	{
		::std::ostringstream oss;
		oss << "Unable to start steady-state output logger thread for the RAIN workload driver: " << ::strerror(errno);

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}
//DCS_DEBUG_TRACE("RAMP-UP THREAD -- Broadcasting");
//	::pthread_cond_broadcast(&(p_driver->steady_state_cond_));
//DCS_DEBUG_TRACE("RAMP-UP THREAD -- Unocking");
//	::pthread_mutex_unlock(&(p_driver->steady_state_mutex_));

DCS_DEBUG_TRACE("RAMP-UP THREAD -- Leaving");

	return 0;
}

void* thread_monitor_rain_steady_state(void* arg)
{
	// Available fields in a row (each field is separated by one or more white-spaces):
	// - '[' <generated-during> ']'
	// - <timestamp>
	// - <operation name>
	// - <response time>
	// - '[' <operation request> ']'
	// - <total response time>
	// - <number of observations>

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Entering");
	const ::std::size_t timestamp_field(2);
	const ::std::size_t operation_field(3);
	const ::std::size_t response_time_field(4);
	const ::std::size_t max_open_trials(50);
	const unsigned int min_zzz_time(2);
	const unsigned int max_zzz_time(10);

	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	p_driver->steady_state_thread_active(true);

//DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Locking");
//	::pthread_mutex_lock(&(p_driver->steady_state_mutex_));
//	while (!this->ready())
//	{
//DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Waiting cond");
//		::pthread_cond_wait(&(p_driver->steady_state_cond_), &(p_driver->steady_state_mutex_));
//DCS_DEBUG_TRACE("STEADT-STATE THREAD -- Cond happened");
//	}
//	::pthread_mutex_unlock(&(p_driver->steady_state_mutex_));


	::std::size_t trial(0);
	unsigned int zzz_time(min_zzz_time);
	::std::ifstream ifs;
	do
	{
		++trial;

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Waiting... (Trial: " << trial << "/" << max_open_trials << ", Zzz: " << zzz_time << ")");
		::sleep(zzz_time);
		++zzz_time;

		ifs.open(p_driver->metrics_file_path().c_str());
	}
	while (trial < max_open_trials && !ifs.good());

	//if (!ifs.is_open() || !ifs.good())
	if (!ifs.good())
	{
		::std::ostringstream oss;
		oss << "Cannot open file '" << p_driver->metrics_file_path() << "'";

		DCS_EXCEPTION_THROW(::std::runtime_error, oss.str());
	}

	// Emulate the behavior of 'tail -f' command
	::std::ifstream::pos_type fpos(0);
	bool new_data(false);
	do
	{
		while (ifs.good())
		{
			fpos = ifs.tellg();

			::std::string line;

			::std::getline(ifs, line);

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- IFS STREAM -- LINE: " << line << " - POS: " << fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));

			const ::std::size_t n(line.size());

			if (!ifs.good() || n == 0)
			{
				continue;
			}

			::std::time_t obs_ts(0); // timestamp (in secs from Epoch)
			::std::string obs_op; // Operation label
			long obs_rtns(0); // response time (in ns)
			::std::size_t field(0);
			for (::std::size_t pos = 0; pos < n; ++pos)
			{
				// eat all heading space
				for (; pos < n && ::std::isspace(line[pos]); ++pos)
				{
					;
				}
				if (pos < n)
				{
					++field;

					switch (field)
					{
						case timestamp_field:
						{
							::std::size_t pos2(pos);
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_ts;
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Timestamp: " << obs_ts);
							pos = pos2;
							break;
						}
						case operation_field:
						{
							::std::size_t pos2(pos);
							//for (; pos2 < n && ::std::isalpha(line[pos2]); ++pos2)
							for (; pos2 < n && !::std::isspace(line[pos2]); ++pos2)
							{
								;
							}
							obs_op = line.substr(pos, pos2-pos);
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Operation: " << obs_op);
							pos = pos2;
							break;
						}
						case response_time_field:
						{
							::std::size_t pos2(pos);
							for (; pos2 < n && ::std::isdigit(line[pos2]); ++pos2)
							{
								;
							}
							::std::istringstream iss(line.substr(pos, pos2-pos));
							iss >> obs_rtns;
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Response Time (nsecs): " << obs_rtns);
							pos = pos2;
							break;
						}
						default:
							// skip these fields
							for (; pos < n && !::std::isspace(line[pos]); ++pos)
							{
								;
							}
							break;
					}
				}
			}

			// Select only a subset of operations
#ifdef DCSXX_TESTBED_EXP_FILTER_OPERATIONS
//			if (obs_op == "About-Me"
//				|| obs_op == "Bid"
//				|| obs_op == "Browse-Categories"
//				|| obs_op == "Browse-Regions"
//				|| obs_op == "Buy-Now-Item"
//				|| obs_op == "Sell")
			if (obs_op == "Browse-Regions")
			{
				p_driver->add_observation(obs_ts, obs_op, obs_rtns*1.0e-6);
			}
#else // DCSXX_TESTBED_EXP_FILTER_OPERATIONS
			p_driver->add_observation(obs_ts, obs_op, obs_rtns*1.0e-6);
#endif // DCSXX_TESTBED_EXP_FILTER_OPERATIONS
		}

		// Found EOF. Two possible reasons:
		// 1. There is no data to read
		// 2. There is new data but we need to refresh input buffers
		// Investigate...

		zzz_time = min_zzz_time;
		new_data = false;
		do
		{
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Zzz... (" << zzz_time << ")");
			// Sleep for a while, just to let new data arrive
			::sleep(zzz_time);
			zzz_time = ::std::max((zzz_time+1) % max_zzz_time, min_zzz_time);
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Cheking for new data");

			// Close to stream in order to avoid possible failures during the following call to open
			ifs.close();
			// Reopen the stream "at end"
			ifs.open(p_driver->metrics_file_path().c_str(), ::std::ios_base::ate);
			if (ifs.good())
			{
				ifs.sync();
				::std::ifstream::pos_type new_fpos(ifs.tellg());
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- REOPENED (good) -- OLD POS: " << fpos << " - NEW POS: " << new_fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs) << " - IN_AVAIL: " << ifs.rdbuf()->in_avail());
				if (fpos != new_fpos)
				{
					// The file has changed, we are in case #2

					// Restart to read file from the old position
					ifs.seekg(fpos);
					new_data = true;
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- SOUGHT IFS STREAM -- OLD POS: " << fpos << " - NEW POS: " << new_fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
				}
//				else
//				{
//					// We are here because
//					// - new_fpos==-1 -> Failed to get new position.
//					//   This may be due to a temporary situation.
//					//   So, instead of stop reading, try to reopen the file later.
//					// - fpos==new_fpos -> The file has not changed, maybe we are in case #1 (or simply we have to wait for other few secs)
//					new_data = false;
//				}
			}
//			else
//			{
//DCS_DEBUG_TRACE("STEADY-STATE THREAD -- REOPENED (failed) -- OLD POS: " << fpos << " - GOOD: " << ifs.good() << " - EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
//				new_data = false;
//			}
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Cheking for new data --> " << new_data);
		}
		//while (!p_driver->done() && !new_data);
		while (!new_data);
	}
	//while (!p_driver->done() && new_data);
	while (new_data);

DCS_DEBUG_TRACE("STEADY-STATE THREAD -- OUT-OF-LOOP IFS STREAM -- EOF: " << ifs.eof() << " - FAIL: " << ifs.fail() << " - BAD: " << ifs.bad() << " - !(): " << !static_cast<bool>(ifs));
	if (ifs.is_open())
	{
		ifs.close();
	}
DCS_DEBUG_TRACE("STEADY-STATE THREAD -- Leaving");

	p_driver->steady_state_thread_active(false);

	return 0;
}

void* thread_log_rain_steady_state_out(void* arg)
{
DCS_DEBUG_TRACE("LOGGER THREAD -- Entering");
	rain_workload_driver* p_driver = static_cast<rain_workload_driver*>(arg);

	p_driver->logger_thread_active(true);

//DCS_DEBUG_TRACE("LOGGER THREAD -- Locking");
//	::pthread_mutex_lock(&(p_driver->steady_state_mutex_));
//	while (!this->ready())
//	{
//DCS_DEBUG_TRACE("LOGGER THREAD -- Waiting cond");
//		::pthread_cond_wait(&(p_driver->steady_state_cond_), &(p_driver->steady_state_mutex_));
//DCS_DEBUG_TRACE("LOGGER THREAD -- Cond happened");
//	}
//	::pthread_mutex_unlock(&(p_driver->steady_state_mutex_));

	::std::istream& is = p_driver->process().output_stream();

	while (is.good())
	{
		::std::string line;

		::std::getline(is, line);

		::dcs::log_info(DCS_LOGGING_AT, line);
	}

	p_driver->logger_thread_active(false);

DCS_DEBUG_TRACE("LOGGER THREAD -- Leaving");
	return 0;
}

} // Namespace detail

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_RAIN_WORKLOAD_DRIVER_HPP
