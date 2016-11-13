/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_LOG_FEATURES_TIMER_IPP
#define LIBBITCOIN_LOG_FEATURES_TIMER_IPP

#include <bitcoin/bitcoin/log/features/timer.hpp>
#include <boost/log/attributes.hpp>
#include <boost/scope_exit.hpp>

namespace libbitcoin {
namespace log {
namespace features {

template<typename BaseType>
timer_feature<BaseType>::timer_feature()
{
}

template<typename BaseType>
timer_feature<BaseType>::timer_feature(timer_feature const& that)
  : BaseType(static_cast<BaseType const&>(that))
{
}

template<typename BaseType>
template<typename ArgsT>
timer_feature<BaseType>::timer_feature(ArgsT const& args)
  : BaseType(args)
{
}

template<typename BaseType>
template<typename ArgsT>
boost::log::record timer_feature<BaseType>::open_record_unlocked(
    ArgsT const& args)
{
    boost::log::attribute_set& attrs = BaseType::attributes();
    boost::log::attribute_set::iterator tag = add_timer_unlocked(
        attrs, args[keywords::timer | boost::parameter::void_()]);

    BOOST_SCOPE_EXIT_TPL((&tag)(&attrs))
    {
        if (tag != attrs.end())
            attrs.erase(tag);
    }
    BOOST_SCOPE_EXIT_END

    return BaseType::open_record_unlocked(args);
}

template<typename BaseType>
template<typename T>
boost::log::attribute_set::iterator
    timer_feature<BaseType>::add_timer_unlocked(
        boost::log::attribute_set& attrs, T const& value)
{
    boost::log::attribute_set::iterator tag = attrs.end();

    std::pair<boost::log::attribute_set::iterator, bool> res =
        BaseType::add_attribute_unlocked(attributes::timer.get_name(),
            boost::log::attributes::constant<std::chrono::milliseconds>(value));

    if (res.second)
        tag = res.first;

    return tag;
}

template<typename BaseType>
boost::log::attribute_set::iterator
    timer_feature<BaseType>::add_timer_unlocked(
        boost::log::attribute_set& attrs, boost::parameter::void_)
{
    return attrs.end();
}

} // namespace features
} // namespace log
} // namespace libbitcoin

#endif
