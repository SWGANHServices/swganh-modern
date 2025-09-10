// src/core/types.hpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <chrono>

namespace swganh::core {

// Basic type definitions
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using byte_vector = std::vector<u8>;
using string = std::string;

// Time types
using clock = std::chrono::steady_clock;
using time_point = clock::time_point;
using duration = clock::duration;

// SWG-specific types
using object_id = u64;
using account_id = u32;
using character_id = u64;
using galaxy_id = u32;

// Network types
using session_id = u32;
using sequence_number = u16;
using crc_value = u32;

// SOE Protocol constants
constexpr u16 SOE_CRC_LENGTH = 2;
constexpr u16 SOE_OPCODE_LENGTH = 2;
constexpr u16 SOE_SEQUENCE_LENGTH = 2;
constexpr u16 SOE_MAX_PACKET_SIZE = 496;

// SWG Protocol constants
constexpr u32 SWG_PACKET_MAX_SIZE = 496;
constexpr u16 SWG_CRC_SEED = 0x0000;

// Position and orientation
struct Vector3 {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};
    
    Vector3() = default;
    Vector3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
};

struct Quaternion {
    f32 x{0.0f};
    f32 y{0.0f};
    f32 z{0.0f};
    f32 w{1.0f};
    
    Quaternion() = default;
    Quaternion(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}
};

// Network address information
struct NetworkAddress {
    string ip;
    u16 port{0};
    
    NetworkAddress() = default;
    NetworkAddress(const string& ip_, u16 port_) : ip(ip_), port(port_) {}
    
    string to_string() const {
        return ip + ":" + std::to_string(port);
    }
};

// Result type for operations that can fail
template<typename T>
class Result {
public:
    Result(T&& value) : value_(std::move(value)), has_value_(true) {}
    Result(const string& error) : error_(error), has_value_(false) {}
    
    bool has_value() const { return has_value_; }
    bool has_error() const { return !has_value_; }
    
    const T& value() const { return value_; }
    T& value() { return value_; }
    
    const string& error() const { return error_; }
    
    operator bool() const { return has_value_; }
    
private:
    T value_{};
    string error_;
    bool has_value_{false};
};

// Helper for creating results
template<typename T>
Result<T> make_result(T&& value) {
    return Result<T>(std::forward<T>(value));
}

template<typename T>
Result<T> make_error(const string& error) {
    return Result<T>(error);
}

} // namespace swganh::core