/**
 * \file dcs/testbed/libvirt/virtual_machine.hpp
 *
 * \brief Manages VMs by means of libvirt toolkit.
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

#ifndef DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP
#define DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP


#include <dcs/assert.hpp>
#include <dcs/debug.hpp>
#include <dcs/logging.hpp>
#include <dcs/testbed/base_virtual_machine.hpp>
#include <dcs/testbed/libvirt/detail/utility.hpp>
#include <dcs/testbed/libvirt/sensors.hpp>
#include <dcs/testbed/virtual_machine_performance_category.hpp>
#include <dcs/uri.hpp>
#include <iostream>
#include <libvirt/libvirt.h>
#include <sstream>
#include <stdexcept>
#include <string>


namespace dcs { namespace testbed { namespace libvirt {

// Forward declarations
template <typename TraitsT>
class virtual_machine_manager;


template <typename TraitsT>
class virtual_machine: public base_virtual_machine<TraitsT>
{
	private: typedef base_virtual_machine<TraitsT> base_type;
	public: typedef typename base_type::traits_type traits_type;
	public: typedef typename base_type::real_type real_type;
	public: typedef typename base_type::uint_type uint_type;
	public: typedef typename base_type::identifier_type identifier_type;
	public: typedef typename base_type::vmm_pointer vmm_pointer;
	public: typedef virtual_machine_manager<traits_type>* vmm_impl_pointer;
	public: typedef base_sensor<traits_type> sensor_type;
	public: typedef ::boost::shared_ptr<sensor_type> sensor_pointer;


	public: virtual_machine(::std::string const& uri)
	: name_(detail::vm_name(uri)),
	  p_vmm_(0),
	  p_dom_(0)
	{
	}

	public: virtual_machine(vmm_impl_pointer p_vmm, ::std::string const& name)
	: name_(detail::vm_name(name)),
	  p_vmm_(p_vmm),
	  p_dom_(0)
	{
		init();
	}

	public: ~virtual_machine()
	{
		// According to http://www.stlport.org/doc/exception_safety.html we avoid to throw any exception inside the destructor
		try
		{
			detail::disconnect_domain(p_vmm_->connection(), p_dom_);
		}
		catch (::std::exception const& e)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << p_vmm_->id() << "': " << e.what();
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
		catch (...)
		{
			::std::ostringstream oss;
			oss << "Failed to disconnect from hypervisor '" << p_vmm_->id() << "': Unknown error";
			dcs::log_error(DCS_LOGGING_AT, oss.str());
		}
	}

	public: unsigned long raw_id() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return detail::domain_id(p_vmm_->connection(), p_dom_);
	}

	public: ::virDomainPtr domain() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		return p_dom_;
	}

	public: ::virDomainPtr domain()
	{
		return p_dom_;
	}

	private: void init()
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));

		// Connect to libvirtd daemon
		p_dom_ = detail::connect_domain(p_vmm_->connection(), name_);
		p_cpu_sens_ = ::boost::make_shared< cpu_utilization_sensor<traits_type> >(p_vmm_->connection(), p_dom_);
	}

	private: ::std::string do_name() const
	{
		return name_;
	}

	private: identifier_type do_id() const
	{
		::std::ostringstream oss;
		if (p_vmm_)
		{
			oss << p_vmm_->id();
		}
		else
		{
			oss << "<None>";
		}
		oss << ":" << this->name();
		return oss.str();
	}

	private: vmm_pointer do_vmm()
	{
		return p_vmm_;
	}

	private: vmm_pointer do_vmm() const
	{
		return p_vmm_;
	}

	private: uint_type do_max_num_vcpus() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int nvcpus = detail::num_vcpus(p_vmm_->connection(), p_dom_, VIR_DOMAIN_VCPU_MAXIMUM);

		return static_cast<uint_type>(nvcpus);
	}

	private: uint_type do_num_vcpus() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int nvcpus = detail::num_vcpus(p_vmm_->connection(), p_dom_, VIR_DOMAIN_AFFECT_CURRENT);

		return static_cast<uint_type>(nvcpus);
	}

	private: void do_cpu_share(real_type share) {
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int nvcpus(this->max_num_vcpus());

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		int cap(share < 1.0 ? share*nvcpus*100 : 0); //Note: cap == 0 ==> No upper cap
		detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", cap, VIR_DOMAIN_AFFECT_CURRENT);
	}

	private: real_type do_cpu_share() const
	{
		// pre: p_vmm_ != null
		DCS_ASSERT(p_vmm_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not connected to VMM"));
		// pre: p_dom_ != null
		DCS_ASSERT(p_dom_,
				   DCS_EXCEPTION_THROW(::std::logic_error,
									   "Not attached to a domain"));

		int cap(0);
		cap = detail::sched_param<int>(p_vmm_->connection(), p_dom_, "cap", VIR_DOMAIN_AFFECT_CURRENT);

		int nvcpus(this->max_num_vcpus());

		//FIXME: This is a Xen-related stuff. What for other hypervisors?
		//FIXME: Actually we assume that weight is 256 (its default value)
		real_type share(cap/(nvcpus*100.0));

		return share > 0 ? share : 1; //Note: cap == 0 ==> No upper cap
	}

	private: sensor_pointer do_sensor(virtual_machine_performance_category cat)
	{
		switch (cat)
		{
			case cpu_util_virtual_machine_performance:
				return p_cpu_sens_;
				break;
		}

		DCS_EXCEPTION_THROW(::std::runtime_error, "Sensor not available");
	}

	private: sensor_pointer do_sensor(virtual_machine_performance_category cat) const
	{
		switch (cat)
		{
			case cpu_util_virtual_machine_performance:
				return p_cpu_sens_;
				break;
		}

		DCS_EXCEPTION_THROW(::std::runtime_error, "Sensor not available");
	}


	private: ::std::string name_;
	private: vmm_impl_pointer p_vmm_;
	private: ::virDomainPtr p_dom_;
	private: sensor_pointer p_cpu_sens_;
}; // virtual_machine

}}} // Namespace dcs::testbed::libvirt

#endif // DCS_TESTBED_LIBVIRT_VIRTUAL_MACHINE_HPP