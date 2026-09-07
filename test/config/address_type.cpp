/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"

using namespace config;

BOOST_AUTO_TEST_SUITE(address_type_tests)

// to_address_type

BOOST_AUTO_TEST_CASE(address_type__to_address_type__loopback_v6__ipv6)
{
    const auto type = to_address_type(messages::peer::loopback_ip_address);
    BOOST_REQUIRE(type == address_type::ipv6);
}

BOOST_AUTO_TEST_CASE(address_type__to_address_type__loopback_mapped__ipv4)
{
    constexpr asio::ipv6::bytes_type mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 127, 0, 0, 1
    };

    BOOST_REQUIRE(to_address_type(mapped) == address_type::ipv4);
}

BOOST_AUTO_TEST_SUITE_END()
