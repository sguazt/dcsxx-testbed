/**
 * \file dcs/testbed/application_performance_category.hpp
 *
 * \brief Categories for performance metrics.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2012 Marco Guazzone (marco.guazzone@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DCS_TESTBED_APPLICATION_PERFORMANCE_CATEGORY_HPP
#define DCS_TESTBED_APPLICATION_PERFORMANCE_CATEGORY_HPP


namespace dcs { namespace testbed {

enum application_performance_category
{
	response_time_application_performance,
	throughput_application_performance
};

}} // Namespace dcs::testbed

#endif // DCS_TESTBED_APPLICATION_PERFORMANCE_CATEGORY_HPP
