// File: src/servers/login/swg_protocol.hpp
#pragma once

#include <vector>
#include <string>
#include "../../core/types.hpp"
#include "../../core/account_manager.hpp"

namespace swganh {
namespace login {

// SWG Login Protocol Message IDs (corrected based on SWGANH research)
enum class SWGOpcode : u32 {
    // These are commonly seen opcodes from SWG protocol research
    LOGIN_REQUEST     = 0x41139C04,  // Observed in packet: 04 00 96 1F 13 41 = 0x411396C4
    LOGIN_RESPONSE    = 0x3436AEB6,  // Standard login response (needs verification)
    SERVER_LIST_REQUEST = 0x04778A75,
    SERVER_LIST_RESPONSE = 0x3C2EF2F4,
    
    // Alternative opcodes based on wiki patterns
    LOGIN_RESPONSE_ALT = 0xB6AE3634,  // Byte-swapped version
    ERROR_MESSAGE      = 0x00000000   // Generic error
};

struct LoginRequest {
    std::string username;
    std::string password;
    std::string client_version;
};

struct ServerInfo {
    u32 server_id;
    std::string name;
    std::string status;
    u32 population;
    u32 max_population;
    std::string address;
    u16 port;
};

class SWGLoginProtocol {
public:
    // Parse login request from data fragment
    static LoginRequest parse_login_request(const std::vector<u8>& data);
    
    // Create login response packet (wiki-compliant format)
    static std::vector<u8> create_login_response(LoginResult result, u32 account_id = 0);
    
    // Create server list response
    static std::vector<u8> create_server_list_response();
    
    // Wrap SWG message in SOE data packet (with CRC)
    static std::vector<u8> wrap_in_soe_data(const std::vector<u8>& swg_message, u16 sequence = 0);

private:
    // Helper functions for reading/writing data
    static std::string read_string(const std::vector<u8>& data, size_t& offset);
    static void write_u32_le(std::vector<u8>& data, u32 value);
    static void write_u16_le(std::vector<u8>& data, u16 value);
    static void write_string(std::vector<u8>& data, const std::string& str);
};

} // namespace login
} // namespace swganh