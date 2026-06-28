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
#include "../test.hpp"

////#if defined(HAVE_SLOW_TESTS)

using namespace http;
using namespace network::http;

struct accessor
  : public body::writer
{
    using base = body::writer;
    using base::writer;
    using base::to_writer;
};

BOOST_AUTO_TEST_SUITE(http_body_writer_tests)

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__undefined__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    ///value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__empty__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__json__constructs_json_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = json_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<json_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__data__constructs_data_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = chunk_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<data_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__span__constructs_span_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = span_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<span_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__buffer__constructs_buffer_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = buffer_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<buffer_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__string__constructs_string_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = string_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<string_writer>(variant));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(variant_body_writer_file_body_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__file__constructs_file_writer)
{
    // In dubug builds boost asserts that the file is open.
    // BOOST_ASSERT(body_.file_.is_open());
    boost_code ec{};
    file_body::value_type file{};
    file.open((TEST_PATH).c_str(), boost::beast::file_mode::write, ec);
    BOOST_REQUIRE(!ec);

    message_header<false, fields> header{};
    body::value_type value{};
    value = std::move(file);
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<file_writer>(variant));
}

BOOST_AUTO_TEST_SUITE_END()

// http::body::size (content_length framing)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(http_body_size_tests)

// Drive the variant body writer exactly as beast does, totaling emitted bytes.
static size_t emitted_bytes(body::writer& writer)
{
    boost_code ec{};
    writer.init(ec);
    size_t total{};
    for (;;)
    {
        const auto out = writer.get(ec);
        if (ec || !out)
            break;

        total += boost::asio::buffer_size(out.get().first);
        if (!out.get().second)
            break;
    }

    return total;
}

BOOST_AUTO_TEST_CASE(http_body__size__empty__zero)
{
    body::value_type value{};
    value = empty_body::value_type{};
    BOOST_REQUIRE_EQUAL(body::size(value), 0u);
}

BOOST_AUTO_TEST_CASE(http_body__size__string__string_length)
{
    body::value_type value{};
    value = string_body::value_type{ "hello" };
    BOOST_REQUIRE_EQUAL(body::size(value), 5u);
}

BOOST_AUTO_TEST_CASE(http_body__size__json__matches_serialized_model)
{
    json_body::value_type json{};
    json.model = boost::json::parse(R"({"jsonrpc":"2.0","result":42,"id":1})");
    const auto expected = boost::json::serialize(json.model).size();

    body::value_type value{};
    value = std::move(json);
    BOOST_REQUIRE_EQUAL(body::size(value), expected);
}

// Non-circular: size() must equal the bytes the writer actually emits, so
// beast's content_length frame is correct.
BOOST_AUTO_TEST_CASE(http_body__size__json__matches_writer_output)
{
    json_body::value_type json{};
    json.model = boost::json::parse(R"({"jsonrpc":"2.0","result":42,"id":1})");

    body::value_type value{};
    value = std::move(json);
    const auto sized = body::size(value);

    message_header<false, fields> header{};
    body::writer writer{ header, value };
    BOOST_REQUIRE_EQUAL(sized, emitted_bytes(writer));
}

// The json-rpc writer appends a single '\n' terminator (the bitcoind RPC
// response path); size() must include it so content_length is exact.
BOOST_AUTO_TEST_CASE(http_body__size__rpc_response__matches_writer_output)
{
    network::rpc::response rpc_value{};
    rpc_value.message.jsonrpc = network::rpc::version::v2;

    body::value_type value{};
    value = std::move(rpc_value);
    const auto sized = body::size(value);

    message_header<false, fields> header{};
    body::writer writer{ header, value };
    const auto emitted = emitted_bytes(writer);
    BOOST_REQUIRE_GT(emitted, 0u);
    BOOST_REQUIRE_EQUAL(sized, emitted);
}

BOOST_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS

