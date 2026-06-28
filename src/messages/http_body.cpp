/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/http_body.hpp>

#include <cstdint>
#include <variant>
#include <boost/json.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace network::error;
    
// http::body::reader
// ----------------------------------------------------------------------------

void body::reader::init(const length_type& length, boost_code& ec) NOEXCEPT
{
    // Header is unread at construct, so this must be deferred until init.
    assign_reader(header_, value_);

    std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            ec = to_http_code(http_error_t::end_of_stream);
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                read.init(length, ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, reader_);
}

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            ec = to_http_code(http_error_t::end_of_stream);
            return size_t{};
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                return read.put(buffer, ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
                return size_t{};
            }
        }
    }, reader_);
}

void body::reader::finish(boost_code& ec) NOEXCEPT
{
    std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            // Called at beast finish_header and must succeed.
            ec = {};
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                read.finish(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, reader_);
}

// http::body::writer
// ----------------------------------------------------------------------------
    
void body::writer::init(boost_code& ec) NOEXCEPT
{
    std::visit(overload
    {
        [&] (std::monostate&) NOEXCEPT
        {
            ec = {};
        },
        [&](auto& write) NOEXCEPT
        {
            try
            {
                write.init(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, writer_);
}

body::writer::out_buffer body::writer::get(boost_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&] (std::monostate&) NOEXCEPT
        {
            ec = {};
            return out_buffer{};
        },
        [&](empty_writer&) NOEXCEPT
        {
            ec = {};

            // Socket body writer requires non-empty buffer to write empty.
            return out_buffer{ std::make_pair(asio::const_buffer{}, false) };
        },
        [&](auto& write) NOEXCEPT
        {
            try
            {
                return write.get(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
                return out_buffer{};
            }
        }
    }, writer_);
}

// http::body::size
// ----------------------------------------------------------------------------
// Measuring json/json-rpc requires a serialization pass; the writer then
// streams the body as usual. This forgoes streaming-only measurement but
// yields the content_length that http clients require (chunked is rejected
// by the rust jsonrpc crate and bitcoin-cli).

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

namespace
{
    inline uint64_t json_size(const boost::json::value& model) NOEXCEPT
    {
        try
        {
            return boost::json::serialize(model).size();
        }
        catch (...)
        {
            return {};
        }
    }

    template <typename Message>
    inline uint64_t rpc_size(const Message& message) NOEXCEPT
    {
        try
        {
            // The json-rpc writer appends a single '\n' terminator.
            const auto model = boost::json::value_from(message);
            return boost::json::serialize(model).size() + 1u;
        }
        catch (...)
        {
            return {};
        }
    }
}

uint64_t body::size(const value_type& value) NOEXCEPT
{
    if (!value.has_value())
        return {};

    return std::visit(overload
    {
        [](const std::monostate&) NOEXCEPT -> uint64_t { return {}; },
        [](const empty_value& body) NOEXCEPT -> uint64_t
        {
            return empty_body::size(body);
        },
        [](const data_value& body) NOEXCEPT -> uint64_t
        {
            return chunk_body::size(body);
        },
        [](const file_value& body) NOEXCEPT -> uint64_t
        {
            return file_body::size(body);
        },
        [](const span_value& body) NOEXCEPT -> uint64_t
        {
            return span_body::size(body);
        },
        [](const buffer_value&) NOEXCEPT -> uint64_t
        {
            // buffer_body is never assigned as a response body.
            return {};
        },
        [](const string_value& body) NOEXCEPT -> uint64_t
        {
            return string_body::size(body);
        },
        [](const json_value& body) NOEXCEPT -> uint64_t
        {
            return json_size(body.model);
        },
        [](const rpc::request& body) NOEXCEPT -> uint64_t
        {
            return rpc_size(body.message);
        },
        [](const rpc::response& body) NOEXCEPT -> uint64_t
        {
            return rpc_size(body.message);
        }
    }, value.value());
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
