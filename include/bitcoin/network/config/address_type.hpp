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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ADDRESS_TYPE_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ADDRESS_TYPE_HPP

#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Address types of the BIP155 address space (dense, for indexed counters).
enum class address_type : uint8_t { ipv4, ipv6, onion, i2p, cjdns };
constexpr size_t address_types = 5;

/// Count of pooled addresses for each address type.
typedef std::array<size_t, address_types> address_counts;

/// Only ip addresses are currently representable (v4 as v6-mapped).
constexpr address_type to_address_type(
    const messages::peer::ip_address& ip) NOEXCEPT
{
    return is_v4(ip) ? address_type::ipv4 : address_type::ipv6;
}

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
