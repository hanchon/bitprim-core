/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/bitcoin/chainv2/input.hpp>

#include <sstream>
#include <bitcoin/bitcoin/constants.hpp>
#include <bitcoin/bitcoin/utility/container_sink.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>
#include <bitcoin/bitcoin/utility/ostream_writer.hpp>
#include <bitcoin/bitcoin/wallet/payment_address.hpp>

namespace libbitcoin { namespace chainv2 {

using namespace bc::wallet;

// Constructors.
//-----------------------------------------------------------------------------

input::input()
    : previous_output_{}, sequence_(0)
{}

input::input(input&& other) noexcept
    : input(std::move(other.previous_output_), std::move(other.script_), other.sequence_)
{}

input::input(const input& other)
    : input(other.previous_output_, other.script_, other.sequence_)
{}

input::input(output_point&& previous_output, chainv2::script&& script, uint32_t sequence)
  : previous_output_(std::move(previous_output)), script_(std::move(script)), sequence_(sequence)
{}

input::input(output_point const& previous_output, chainv2::script const& script, uint32_t sequence)
  : previous_output_(previous_output), script_(script), sequence_(sequence) 
{}

// Operators.
//-----------------------------------------------------------------------------

input& input::operator=(input&& other) noexcept {
    previous_output_ = std::move(other.previous_output_);
    script_ = std::move(other.script_);
    sequence_ = other.sequence_;
    return *this;
}

input& input::operator=(const input& other) {
    previous_output_ = other.previous_output_;
    script_ = other.script_;
    sequence_ = other.sequence_;
    return *this;
}

bool input::operator==(const input& other) const {
    return (sequence_ == other.sequence_)
        && (previous_output_ == other.previous_output_)
        && (script_ == other.script_);
}

bool input::operator!=(const input& other) const {
    return !(*this == other);
}

// input::operator chain::input() const {
//     return chain::input(static_cast<chain::output_point>(previous_output_), script_, sequence_);
// }


// Deserialization.
//-----------------------------------------------------------------------------

input input::factory_from_data(const data_chunk& data, bool wire) {
    input instance;
    instance.from_data(data, wire);
    return instance;
}

input input::factory_from_data(std::istream& stream, bool wire) {
    input instance;
    instance.from_data(stream, wire);
    return instance;
}

input input::factory_from_data(reader& source, bool wire) {
    input instance;
    instance.from_data(source, wire);
    return instance;
}

bool input::from_data(const data_chunk& data, bool wire) {
    data_source istream(data);
    return from_data(istream, wire);
}

bool input::from_data(std::istream& stream, bool wire) {
    istream_reader source(stream);
    return from_data(source, wire);
}

bool input::from_data(reader& source, bool wire) {
    reset();

    if (!previous_output_.from_data(source, wire))
        return false;

    script_.from_data(source, true);
    sequence_ = source.read_4_bytes_little_endian();

    if (!source)
        reset();

    return source;
}

void input::reset() {
    previous_output_.reset();
    script_.reset();
    sequence_ = 0;
}

// Since empty script and zero sequence are valid this relies on the prevout.
bool input::is_valid() const {
    return sequence_ != 0 || previous_output_.is_valid() || script_.is_valid();
}

// Serialization.
//-----------------------------------------------------------------------------

data_chunk input::to_data(bool wire) const {
    data_chunk data;
    auto const size = serialized_size(wire);
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream, wire);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void input::to_data(std::ostream& stream, bool wire) const {
    ostream_writer sink(stream);
    to_data(sink, wire);
}

void input::to_data(writer& sink, bool wire) const {
    previous_output_.to_data(sink, wire);
    script_.to_data(sink, true);
    sink.write_4_bytes_little_endian(sequence_);
}

// Size.
//-----------------------------------------------------------------------------

size_t input::serialized_size(bool wire) const {
    return previous_output_.serialized_size(wire) + script_.serialized_size(true) + sizeof(sequence_);
}

// Accessors.
//-----------------------------------------------------------------------------

output_point& input::previous_output() {
    return previous_output_;
}

output_point const& input::previous_output() const {
    return previous_output_;
}

void input::set_previous_output(output_point const& value) {
    previous_output_ = value;
}

void input::set_previous_output(output_point&& value) {
    previous_output_ = std::move(value);
}

chainv2::script& input::script() {
    return script_;
}

chainv2::script const& input::script() const {
    return script_;
}

void input::set_script(chainv2::script const& value) {
    script_ = value;
    invalidate_cache();
}

void input::set_script(chainv2::script&& value) {
    script_ = std::move(value);
    invalidate_cache();
}

uint32_t input::sequence() const {
    return sequence_;
}

void input::set_sequence(uint32_t value) {
    sequence_ = value;
}

// protected
void input::invalidate_cache() const {
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    mutex_.lock_upgrade();

    if (address_) {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        address_.reset();
        //---------------------------------------------------------------------
        mutex_.unlock_and_lock_upgrade();
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

payment_address input::address() const {
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    mutex_.lock_upgrade();

    if (!address_) {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();

        // TODO: limit this to input patterns.
        address_ = std::make_shared<payment_address>(payment_address::extract(script_));
        mutex_.unlock_and_lock_upgrade();
        //---------------------------------------------------------------------
    }

    auto const address = *address_;
    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    return address;
}

// Validation helpers.
//-----------------------------------------------------------------------------

bool input::is_final() const {
    return sequence_ == max_input_sequence;
}

bool input::is_locked(size_t block_height, uint32_t median_time_past) const {
    if ((sequence_ & relative_locktime_disabled) != 0)
        return false;

    // bip68: a minimum block-height constraint over the input's age.
    auto const minimum = (sequence_ & relative_locktime_mask);
    auto const& prevout = previous_output_.validation;

    if ((sequence_ & relative_locktime_time_locked) != 0) {
        // Median time past must be monotonically-increasing by block.
        BITCOIN_ASSERT(median_time_past >= prevout.median_time_past);
        auto const age_seconds = median_time_past - prevout.median_time_past;
        return age_seconds < (minimum << relative_locktime_seconds_shift);
    }

    BITCOIN_ASSERT(block_height >= prevout.height);
    auto const age_blocks = block_height - prevout.height;
    return age_blocks < minimum;
}

size_t input::signature_operations(bool bip16_active) const {
    auto sigops = script_.sigops(false);

    if (bip16_active) {
        // Breaks debug build unit testing.
        ////BITCOIN_ASSERT(previous_output_.is_valid());

        // This cannot overflow because each total is limited by max ops.
        auto const& cache = previous_output_.validation.cache.script();
        sigops += script_.embedded_sigops(cache);
    }

    return sigops;
}

}} // namespace libbitcoin::chainv2
