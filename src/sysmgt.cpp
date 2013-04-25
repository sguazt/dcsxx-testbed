/**
 * \file rubis_mgmt.hpp
 *
 * \brief Driver for managing a RUBiS instance.
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

#include <boost/smart_ptr.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dcs/cli.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/data_estimators.hpp>
#include <dcs/testbed/data_smoothers.hpp>
#include <dcs/testbed/system_management.hpp>
#include <dcs/testbed/workload_category.hpp>
#include <dcs/testbed/workload_drivers.hpp>
#include <dcs/testbed/workload_generator_category.hpp>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>


namespace detail { namespace /*<unnamed>*/ {

//enum aggregation_category
//{
//	mean_aggregation
//};

enum data_smoother_category
{
	dummy_smoother,
	brown_single_exponential_smoother,
	brown_double_exponential_smoother,
	holt_winters_double_exponential_smoother
};

enum data_estimator_category
{
	mean_estimator,
	chen2000_ewma_quantile_estimator,
	chen2000_ewsa_quantile_estimator,
	jain1985_p2_algorithm_quantile_estimator,
	welsh2003_ewma_quantile_estimator,
	welsh2003_ewma_ext_quantile_estimator
};


const dcs::testbed::workload_category default_workload(dcs::testbed::olio_workload);
const dcs::testbed::workload_generator_category default_workload_driver(dcs::testbed::rain_workload_generator);
const ::std::string default_workload_driver_rain_path("/usr/local/opt/rain-workload-toolkit");
const ::std::string default_out_dat_file("./sysmgt-out.dat");
const double default_sampling_time(10);
const data_estimator_category default_data_estimator(mean_estimator);
const double default_quantile_prob(0.99);
const double default_chen2000_ewma_w(0.05);
const double default_chen2000_ewsa_w(0.05);
const double default_welsh2003_ewma_alpha(0.7);
const data_smoother_category default_data_smoother(brown_single_exponential_smoother);
const double default_brown_single_exponential_alpha(0.7);
const double default_brown_double_exponential_alpha(0.7);
const double default_holt_winters_double_exponential_alpha(0.8);
const double default_holt_winters_double_exponential_beta(0.3);
const double default_holt_winters_double_exponential_delta(0.7);


void usage(char const* progname)
{
	//DEPRECATED
	::std::cerr << "Usage: " << progname << " [options]" << ::std::endl
				<< " --help" << ::std::endl
				<< "   Show this message." << ::std::endl
				<< " --out-dat-file <file path>" << ::std::endl
				<< "   The path to the output data file." << ::std::endl
				<< "   [default: '" << default_out_dat_file << "']." << ::std::endl
				<< " --ts <time in secs>" << ::std::endl
				<< "   Sampling time (in seconds)." << ::std::endl
				<< "   [default: " << default_sampling_time << "]." << ::std::endl
				<< " --verbose" << ::std::endl
				<< "   Show verbose messages." << ::std::endl
				<< "   [default: disabled]." << ::std::endl
				<< " --wkl <name>" << ::std::endl
				<< "   The workload to generate. Possible values are: 'olio', 'rubis'." << ::std::endl
				<< "   [default: '" << default_workload << "']." << ::std::endl
				<< " --wkl-driver <name>" << ::std::endl
				<< "   The workload driver to use. Possible values are: 'rain'." << ::std::endl
				<< "   [default: '" << default_workload_driver << "']." << ::std::endl
				<< " --wkl-driver-rain-path <name>" << ::std::endl
				<< "   The full path to the RAIN workload driver." << ::std::endl
				<< "   [default: '" << default_workload_driver_rain_path << "']." << ::std::endl
				<< ::std::endl;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, data_smoother_category& cat)
{
    ::std::string s;
    is >> s;
    ::dcs::string::to_lower(s);

    if (!s.compare("brown_ses"))
    {
        cat = brown_single_exponential_smoother;
    }
    else if (!s.compare("dummy"))
    {
        cat = dummy_smoother;
    }
    else if (!s.compare("brown_des"))
    {
        cat = brown_double_exponential_smoother;
    }
    else if (!s.compare("holt_winters_des"))
    {
        cat = holt_winters_double_exponential_smoother;
    }
    else
    {
        DCS_EXCEPTION_THROW(::std::runtime_error,
                            "Unknown data smoother category");
    }

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_istream<CharT,CharTraitsT>& operator>>(::std::basic_istream<CharT,CharTraitsT>& is, data_estimator_category& cat)
{
    ::std::string s;
    is >> s;
    ::dcs::string::to_lower(s);

    if (!s.compare("mean"))
    {
        cat = mean_estimator;
    }
    else if (!s.compare("chen2000_ewma_quantile"))
    {
        cat = chen2000_ewma_quantile_estimator;
    }
    else if (!s.compare("chen2000_ewsa_quantile"))
    {
        cat = chen2000_ewsa_quantile_estimator;
    }
    else if (!s.compare("jain1985_p2_algorithm_quantile"))
    {
        cat = jain1985_p2_algorithm_quantile_estimator;
    }
    else if (!s.compare("welsh2003_ewma_quantile"))
    {
        cat = welsh2003_ewma_quantile_estimator;
    }
    else if (!s.compare("welsh2003_ewma_ext_quantile"))
    {
        cat = welsh2003_ewma_ext_quantile_estimator;
    }
    else
    {
        DCS_EXCEPTION_THROW(::std::runtime_error,
                            "Unknown data estimator category");
    }

	return is;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, data_smoother_category cat)
{
	switch (cat)
	{
		case brown_single_exponential_smoother:
			os << "brown_ses";
			break;
		case brown_double_exponential_smoother:
			os << "brown_des";
			break;
		case dummy_smoother:
			os << "dummy";
			break;
		case holt_winters_double_exponential_smoother:
			os << "holt_winters_des";
			break;
	}

	return os;
}

template <typename CharT, typename CharTraitsT>
inline
::std::basic_ostream<CharT,CharTraitsT>& operator<<(std::basic_ostream<CharT,CharTraitsT>& os, data_estimator_category cat)
{
	switch (cat)
	{
		case mean_estimator:
			os << "mean";
			break;
		case chen2000_ewma_quantile_estimator:
			os << "chen2000_ewma_quantile";
			break;
		case chen2000_ewsa_quantile_estimator:
			os << "chen2000_ewsa_quantile";
			break;
		case jain1985_p2_algorithm_quantile_estimator:
			os << "jain1985_p2_algorithm_quantile";
			break;
		case welsh2003_ewma_quantile_estimator:
			os << "welsh2003_ewma_quantile";
			break;
		case welsh2003_ewma_ext_quantile_estimator:
			os << "welsh2003_ewma_ext_quantile";
			break;
	}

	return os;
}

}} // Namespace detail::<unnamed>


int main(int argc, char *argv[])
{
	typedef double real_type;
	typedef unsigned int uint_type;

	namespace testbed = ::dcs::testbed;

	bool help(false);
	std::string out_dat_file;
	detail::data_estimator_category data_estimator;
	real_type quantile_prob(0);
	real_type chen2000_ewma_w(0);
	real_type chen2000_ewsa_w(0);
	real_type welsh2003_ewma_alpha(0);
	detail::data_smoother_category data_smoother;
	real_type brown_single_exponential_alpha(0);
	real_type brown_double_exponential_alpha(0);
	real_type holt_winters_double_exponential_alpha(0);
	real_type holt_winters_double_exponential_beta(0);
	real_type holt_winters_double_exponential_delta(0);
	real_type ts(0);
	bool verbose(false);
	testbed::workload_category wkl;
	testbed::workload_generator_category wkl_driver;
	std::string wkl_driver_rain_path;

	// Parse command line options
	try
	{
		help = dcs::cli::simple::get_option(argv, argv+argc, "--help");
		out_dat_file = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--out-dat-file", detail::default_out_dat_file);
		data_estimator = dcs::cli::simple::get_option<detail::data_estimator_category>(argv, argv+argc, "--data-estimator", detail::default_data_estimator);
		quantile_prob = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--quantile-prob", detail::default_quantile_prob);
		chen2000_ewma_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewma-w", detail::default_chen2000_ewma_w);
		chen2000_ewsa_w = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--chen2000_ewsa-w", detail::default_chen2000_ewsa_w);
		welsh2003_ewma_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--welsh2003_ewma-alpha", detail::default_welsh2003_ewma_alpha);
		data_smoother = dcs::cli::simple::get_option<detail::data_smoother_category>(argv, argv+argc, "--data-smoother", detail::default_data_smoother);
		brown_single_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_ses-alpha", detail::default_brown_single_exponential_alpha);
		brown_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--brown_des-alpha", detail::default_brown_double_exponential_alpha);
		holt_winters_double_exponential_alpha = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-alpha", detail::default_holt_winters_double_exponential_alpha);
		holt_winters_double_exponential_beta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-beta", detail::default_holt_winters_double_exponential_beta);
		holt_winters_double_exponential_delta = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--holt_winters_des-delta", detail::default_holt_winters_double_exponential_delta);
		ts = dcs::cli::simple::get_option<real_type>(argv, argv+argc, "--ts", detail::default_sampling_time);
		verbose = dcs::cli::simple::get_option(argv, argv+argc, "--verbose");
		wkl = dcs::cli::simple::get_option<testbed::workload_category>(argv, argv+argc, "--wkl", detail::default_workload);
		wkl_driver = dcs::cli::simple::get_option<testbed::workload_generator_category>(argv, argv+argc, "--wkl-driver", detail::default_workload_driver);
		wkl_driver_rain_path = dcs::cli::simple::get_option<std::string>(argv, argv+argc, "--wkl-driver-rain-path", detail::default_workload_driver_rain_path);
	}
	catch (std::exception const& e)
	{
		std::ostringstream oss;
		oss << "Error while parsing command-line options: " << e.what();
		dcs::log_error(DCS_LOGGING_AT, oss.str());

		detail::usage(argv[0]);
		std::abort();
		return EXIT_FAILURE;
	}

	if (help)
	{
		detail::usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int ret(0);

	if (verbose)
	{
		std::ostringstream oss;

		oss << "Output data file: " << out_dat_file;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Data estimator: " << data_estimator;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Quantile estimator probability: " << quantile_prob;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen,2000)'s EWMA quantile estimator w: " << chen2000_ewma_w;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Chen,2000)'s EWSA quantile estimator w: " << chen2000_ewsa_w;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "(Welsh,2003)'s EWMA quantile estimator alpha: " << welsh2003_ewma_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Data smoother: " << data_smoother;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Brown's single exponential smoother alpha: " << brown_single_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Brown's double exponential smoother alpha: " << brown_double_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother alpha: " << holt_winters_double_exponential_alpha;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother beta: " << holt_winters_double_exponential_beta;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Holt-Winters' double exponential smoother delta: " << holt_winters_double_exponential_delta;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Sampling time: " << ts;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload: " << wkl;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver: " << wkl_driver;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");

		oss << "Workload driver RAIN path: " << wkl_driver_rain_path;
		dcs::log_info(DCS_LOGGING_AT, oss.str());
		oss.str("");
	}

//	typedef boost::shared_ptr< testbed::base_virtual_machine<real_type> > vm_pointer;

	try
	{
//		vm_pointer p_oliodb_vm(new testbed::libvirt_virtual_machine<real_type>(oliodb_uri, oliodb_name));
//		vm_pointer p_olioweb_vm(new testbed::libvirt_virtual_machine<real_type>(olioweb_uri, olioweb_name));

//		const std::size_t nt(2); // Number of tiers

//		std::vector<vm_pointer> vms(nt);
//		vms[0] = p_olioweb_vm;
//		vms[1] = p_oliodb_vm;

		boost::shared_ptr< testbed::base_workload_driver > p_driver;
		switch (wkl_driver)
		{
			case testbed::rain_workload_generator:
				p_driver = boost::make_shared<testbed::rain_workload_driver>(wkl, wkl_driver_rain_path);
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown workload driver");
		}

		boost::shared_ptr< testbed::base_estimator<real_type> > p_estimator;
		switch (data_estimator)
		{
			case detail::mean_estimator:
				p_estimator = boost::make_shared< testbed::mean_estimator<real_type> >();
				break;
			case detail::chen2000_ewma_quantile_estimator:
				p_estimator = boost::make_shared< testbed::chen2000_ewma_quantile_estimator<real_type> >(quantile_prob, chen2000_ewma_w);
				break;
			case detail::chen2000_ewsa_quantile_estimator:
				p_estimator = boost::make_shared< testbed::chen2000_ewsa_quantile_estimator<real_type> >(quantile_prob, chen2000_ewsa_w);
				break;
			case detail::jain1985_p2_algorithm_quantile_estimator:
				p_estimator = boost::make_shared< testbed::jain1985_p2_algorithm_quantile_estimator<real_type> >(quantile_prob);
				break;
			case detail::welsh2003_ewma_quantile_estimator:
				p_estimator = boost::make_shared< testbed::welsh2003_ewma_quantile_estimator<real_type> >(quantile_prob, welsh2003_ewma_alpha, false);
				break;
			case detail::welsh2003_ewma_ext_quantile_estimator:
				p_estimator = boost::make_shared< testbed::welsh2003_ewma_quantile_estimator<real_type> >(quantile_prob, welsh2003_ewma_alpha, true);
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data estimator");
		}

		boost::shared_ptr< testbed::base_smoother<real_type> > p_smoother;
		switch (data_smoother)
		{
			case detail::brown_single_exponential_smoother:
				p_smoother = boost::make_shared< testbed::brown_single_exponential_smoother<real_type> >(brown_single_exponential_alpha);
				break;
			case detail::brown_double_exponential_smoother:
				p_smoother = boost::make_shared< testbed::brown_double_exponential_smoother<real_type> >(brown_double_exponential_alpha);
				break;
			case detail::dummy_smoother:
				p_smoother = boost::make_shared< testbed::dummy_smoother<real_type> >();
				break;
			case detail::holt_winters_double_exponential_smoother:
				if (holt_winters_double_exponential_delta > 0)
				{
					p_smoother = boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(holt_winters_double_exponential_delta);
				}
				else
				{
					p_smoother = boost::make_shared< testbed::holt_winters_double_exponential_smoother<real_type> >(holt_winters_double_exponential_alpha, holt_winters_double_exponential_beta);
				}
				break;
			default:
				DCS_EXCEPTION_THROW(std::runtime_error, "Unknown data smoother");
		}

		testbed::system_management<real_type> sysmgt(p_driver);
		sysmgt.output_data_file(out_dat_file);
		sysmgt.sampling_time(ts);
		sysmgt.data_estimator(p_estimator);
		sysmgt.data_smoother(p_smoother);

		sysmgt.run();
	}
	catch (std::exception const& e)
	{
		ret = 1;
		dcs::log_error(DCS_LOGGING_AT, e.what());
	}
	catch (...)
	{
		ret = 1;
		dcs::log_error(DCS_LOGGING_AT, "Unknown error");
	}

	return ret;
}
