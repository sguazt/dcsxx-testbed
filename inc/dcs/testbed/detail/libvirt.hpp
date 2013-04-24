/**
 * \file dcs/testbed/detail/libvirt.hpp
 *
 * \brief Utilities to communicate with libvirt.
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

#ifndef DCS_TESTBED_DETAIL_LIBVIRT_HPP
#define DCS_TESTBED_DETAIL_LIBVIRT_HPP


#include <cassert>
#include <cstdlib>
#include <cstring>
#ifndef NDEBUG
# include <iostream>
#endif // NDEBUG
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed { namespace detail { namespace libvirt {

::std::string to_string(virTypedParameter const& param)
{
	::std::ostringstream oss;

	switch (param.type)
	{
		case VIR_TYPED_PARAM_INT:
			oss << param.value.i;
			break;
		case VIR_TYPED_PARAM_UINT:
			oss << param.value.ui;
			break;
		case VIR_TYPED_PARAM_LLONG:
			oss << param.value.l;
			break;
		case VIR_TYPED_PARAM_ULLONG:
			oss << param.value.ul;
			break;
		case VIR_TYPED_PARAM_DOUBLE:
			oss << param.value.d;
			break;
		case VIR_TYPED_PARAM_BOOLEAN:
			oss << param.value.b;
			break;
#if LIBVIR_VERSION_NUMBER >= 10000
		case VIR_TYPED_PARAM_STRING:
			oss << param.value.s;
			break;
#endif // LIBVIR_VERSION_NUMBER
	}

	return oss.str();
}

::std::string last_error(virConnectPtr conn)
{
	assert( conn );

	::std::string err_str;
	virErrorPtr err = new virError();

	int ret = virConnCopyLastError(conn, err);
	switch (ret)
	{
		case 0:
			// No error found
			break;
		case -1:
			err_str = "Parameter error when attempting to get last error";
			break;
		default:
			err_str = err->message;
			break;
	}

	virResetError(err);
	delete err;

	return err_str;
}

virConnectPtr connect(::std::string const& uri)
{
	// Connect to libvirtd daemons
	//
	// virConnectOpenAuth is called here with all default parameters,
	// except, possibly, the URI of the hypervisor.
	//
	virConnectPtr conn = virConnectOpenAuth((!uri.empty() ? uri.c_str() : 0), virConnectAuthPtrDefault, 0);
	if (0 == conn)
	{
		::std::ostringstream oss;
		oss << "No connection to hypervisor with URI '" << uri << "': " << last_error(0);
		throw ::std::runtime_error(oss.str());
	}

	return conn;
}

void disconnect(virConnectPtr conn)
{
	assert( conn );

	if (0 != virConnectClose(conn))
	{
		throw ::std::runtime_error(last_error(conn));
	}
}

::std::string hypervisor_info(virConnectPtr conn)
{
	/* virConnectGetType returns a pointer to a static string, so no
	* allocation or freeing is necessary; it is possible for the call
	* to fail if, for example, there is no connection to a
	* hypervisor, so check what it returns. */
	const char *hvType(0);
	hvType = virConnectGetType(conn);
	if (0 == hvType)
	{
		::std::ostringstream oss;
		oss << "Failed to get hypervisor type: " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	unsigned long hvVer;

	if (0 != virConnectGetVersion(conn, &hvVer))
	{
		::std::ostringstream oss;
		oss << "Failed to get hypervisor version: " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	unsigned long major;
	unsigned long minor;
	unsigned long release;

	major = hvVer / 1000000;
	hvVer %= 1000000;
	minor = hvVer / 1000;
	release = hvVer % 1000;

	::std::ostringstream oss;

	oss << hvType << " (" << major << "." << minor << "." << release << ")";

	return oss.str();
}

virDomainPtr connect_domain(virConnectPtr conn, ::std::string const& name)
{
	assert( conn );

	virDomainPtr dom = virDomainLookupByName(conn, name.c_str());
	if (0 == dom)
	{
		::std::ostringstream oss;
		oss << "Failed to get Domain for \"" << name << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return dom;
}

void disconnect_domain(virConnectPtr conn, virDomainPtr dom)
{
	assert( conn );
	assert( dom );

	if (0 != virDomainFree(dom))
	{
		::std::ostringstream oss;
		oss << "Failed to free data for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}
}

template <typename ParamT>
struct sched_param_value;

template <>
struct sched_param_value<int>
{
	int& operator()(virTypedParameter& param)
	{
		return param.value.i;
	}

	int operator()(virTypedParameter const& param) const
	{
		return param.value.i;
	}
}; // sched_param_value

template <>
struct sched_param_value<unsigned int>
{
	unsigned int& operator()(virTypedParameter& param)
	{
		return param.value.ui;
	}

	unsigned int operator()(virTypedParameter const& param) const
	{
		return param.value.ui;
	}
}; // sched_param_value

template <>
struct sched_param_value<long long int>
{
	long long int& operator()(virTypedParameter& param)
	{
		return param.value.l;
	}

	long long int operator()(virTypedParameter const& param) const
	{
		return param.value.l;
	}
}; // sched_param_value

template <>
struct sched_param_value<unsigned long long int>
{
	unsigned long long int& operator()(virTypedParameter& param)
	{
		return param.value.ul;
	}

	unsigned long long int operator()(virTypedParameter const& param) const
	{
		return param.value.ul;
	}
}; // sched_param_value

template <>
struct sched_param_value<double>
{
	double& operator()(virTypedParameter& param)
	{
		return param.value.d;
	}

	double operator()(virTypedParameter const& param) const
	{
		return param.value.d;
	}
}; // sched_param_value

template <>
struct sched_param_value<char>
{
	char& operator()(virTypedParameter& param)
	{
		return param.value.b;
	}

	char operator()(virTypedParameter const& param) const
	{
		return param.value.b;
	}
}; // sched_param_value

#if LIBVIR_VERSION_NUMBER >= 10000
template <>
struct sched_param_value<char*>
{
	char*& operator()(virTypedParameter& param)
	{
		return param.value.s;
	}

	char* operator()(virTypedParameter const& param) const
	{
		return param.value.s;
	}
}; // sched_param_value
#endif // LIBVIR_VERSION_NUMBER

template <typename ParamT>
ParamT sched_param(virConnectPtr conn, virDomainPtr dom, ::std::string const& name, int flags)
{
	assert( conn );
	assert( dom );

	int ret(0);

	char* sched(0);
	int sched_nparams(0);

	sched = virDomainGetSchedulerType(dom, &sched_nparams);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler type for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}
#ifndef NDEBUG
	::std::clog << "Scheduler: " << ::std::string(sched) << ::std::endl;
#endif // NDEBUG
	::std::free(sched);

	virTypedParameterPtr sched_params = new virTypedParameter[sched_nparams];
	ret = virDomainGetSchedulerParametersFlags(dom, sched_params, &sched_nparams, flags);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}
#ifndef NDEBUG
	for (int i = 0; i < sched_nparams; ++i)
	{
		::std::clog << "Scheduler parameter #" << (i+1) << ": <" << sched_params[i].field << "," << to_string(sched_params[i]) << ">" << ::std::endl;
	}
#endif // NDEBUG

	ParamT value;
	bool found(false);
	for (int i = 0; i < sched_nparams && !found; ++i)
	{
		if (!::std::strncmp(name.c_str(), sched_params[i].field, VIR_TYPED_PARAM_FIELD_LENGTH))
		{
			value = sched_param_value<ParamT>()(sched_params[i]);
			found = true;
		}
	}

	delete[] sched_params;

	if (!found)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameter '" << name << "' for domain \"" << virDomainGetName(dom) << "\": Not Found";
		throw ::std::runtime_error(oss.str());
	}

	return value;
}

template <typename ParamT>
void sched_param(virConnectPtr conn, virDomainPtr dom, ::std::string const& name, ParamT value, int flags)
{
	assert( conn );
	assert( dom );

	int ret(0);

	char* sched(0);
	int sched_nparams(0);

	sched = virDomainGetSchedulerType(dom, &sched_nparams);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler type for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}
#ifndef NDEBUG
	::std::clog << "Scheduler: " << ::std::string(sched) << ::std::endl;
#endif // NDEBUG
	::std::free(sched);

	virTypedParameterPtr sched_params = new virTypedParameter[sched_nparams];
	ret = virDomainGetSchedulerParametersFlags(dom, sched_params, &sched_nparams, flags);
	if (0 != ret)
	{
		::std::ostringstream oss;
		oss << "Failed to get scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}
#ifndef NDEBUG
	for (int i = 0; i < sched_nparams; ++i)
	{
		::std::clog << "Scheduler parameter #" << (i+1) << ": <" << sched_params[i].field << "," << to_string(sched_params[i]) << ">" << ::std::endl;
	}
#endif // NDEBUG

	bool found(false);
	for (int i = 0; i < sched_nparams && !found; ++i)
	{
		if (!::std::strncmp(name.c_str(), sched_params[i].field, VIR_TYPED_PARAM_FIELD_LENGTH))
		{
			sched_param_value<ParamT>()(sched_params[i]) = value;
			found = true;
		}
	}

	if (found)
	{
		ret = virDomainSetSchedulerParametersFlags(dom, sched_params, sched_nparams, flags);
		if (0 != ret)
		{
			::std::ostringstream oss;
			oss << "Failed to set scheduler parameters for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
			throw ::std::runtime_error(oss.str());
		}
	}

	delete[] sched_params;

	if (!found)
	{
		::std::ostringstream oss;
		oss << "Failed to set scheduler parameter '" << name << "' for domain \"" << virDomainGetName(dom) << "\": Not Found";
		throw ::std::runtime_error(oss.str());
	}
}

int num_vcpus(virConnectPtr conn, virDomainPtr dom, int flags)
{
	assert( conn );
	assert( dom );

	int ret(0);

	ret = virDomainGetVcpusFlags(dom, flags);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the number of vCPUs for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return ret;
}

unsigned int id(virConnectPtr conn, virDomainPtr dom)
{
	assert( conn );
	assert( dom );

	int ret(0);

	ret = virDomainGetID(dom);
	if (0 > ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the ID for domain \"" << virDomainGetName(dom) << "\": " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return ret;
}

::std::string name(virConnectPtr conn, virDomainPtr dom)
{
	assert( conn );
	assert( dom );

	char const* ret(0);

	ret = virDomainGetName(dom);
	if (0 == ret)
	{
		::std::ostringstream oss;
		oss << "Failed to query the name for domain: " << last_error(conn);
		throw ::std::runtime_error(oss.str());
	}

	return ret;
}

}}}} // Namespace dcs::testbed::detail::libvirt

#endif // DCS_TESTBED_DETAIL_LIBVIRT_HPP
