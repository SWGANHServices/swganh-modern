// File: src/servers/login/swg_protocol.cpp - Test Version
#include "swg_protocol.hpp"
#include "../../core/logger.hpp"
#include "../../core/config.hpp"

namespace swganh {
namespace login {

LoginRequest SWGLoginProtocol::parse_login_request(const std::vector<u8>& data) {
    LoginRequest request;
    
    LOG_DEBUG("=== Parsing Login Request ===");
    LOG_DEBUG_F("Total packet size: {} bytes", data.size());
    
    size_t offset = 10; // Skip SOE header (4 bytes) + SWG header (6 bytes)
    
    LOG_DEBUG_F("Starting string parsing at offset: {}", offset);
    
    // Read username
    request.username = read_string(data, offset);
    LOG_DEBUG_F("Parsed username: '{}' (offset now: {})", request.username, offset);
    
    // Read password  
    request.password = read_string(data, offset);
    LOG_DEBUG_F("Parsed password: '{}' (offset now: {})", request.password, offset);
    
    // Read client version
    request.client_version = read_string(data, offset);
    LOG_DEBUG_F("Parsed client version: '{}' (offset now: {})", request.client_version, offset);
    
    return request;
}

std::vector<u8> SWGLoginProtocol::create_login_response(LoginResult result, u32 account_id) {
    std::vector<u8> response;
    
    LOG_INFO("=== Creating SIMPLE Error Response for Testing ===");
    
    // Try a very simple response format - just operand count and error code
    // This should at least get the client to react differently if our format is working
    
    // Write operand count (1 = just the result code)
    write_u16_le(response, 1);
    
    // Use a simple error opcode that might be recognized
    // Let's try using the same opcode as the request but with a simple modification
    write_u32_le(response, 0x411396C5); // Request was 0x411396C4, try +1
    
    // Write error result code (1 = failure)
    write_u32_le(response, 1); // Always send failure for now to test
    
    LOG_INFO_F("Created simple error response ({} bytes)", response.size());
    
    return response;
}

std::vector<u8> SWGLoginProtocol::create_server_list_response() {
    std::vector<u8> response;
    Config& config = Config::instance();
    
    // Operand count for server list: count + server_data
    write_u16_le(response, 2);
    
    // Server List Response opcode
    write_u32_le(response, static_cast<u32>(SWGOpcode::SERVER_LIST_RESPONSE));
    
    // Number of servers (1 for now)
    write_u32_le(response, 1);
    
    // Server info
    write_u32_le(response, 1); // Server ID
    write_string(response, config.get("server_name"));
    write_string(response, config.get("server_population"));
    write_u32_le(response, 100); // Current population
    write_u32_le(response, 3000); // Max population
    write_string(response, "127.0.0.1"); // Server address
    write_u16_le(response, 44464); // Zone server port
    
    return response;
}

std::vector<u8> SWGLoginProtocol::wrap_in_soe_data(const std::vector<u8>& swg_message, u16 sequence) {
    std::vector<u8> soe_packet;
    
    // SOE Data packet (0x0800) for responses
    soe_packet.push_back(0x00);
    soe_packet.push_back(0x08);
    
    // Sequence number (little-endian)
    soe_packet.push_back(sequence & 0xFF);
    soe_packet.push_back((sequence >> 8) & 0xFF);
    
    // Add SWG message data
    soe_packet.insert(soe_packet.end(), swg_message.begin(), swg_message.end());
    
    // Add CRC footer (2 bytes) - simplified for now
    soe_packet.push_back(0x00);
    soe_packet.push_back(0x00);
    
    LOG_INFO_F("Wrapped SWG message in SOE packet ({} bytes total)", soe_packet.size());
    
    return soe_packet;
}

std::string SWGLoginProtocol::read_string(const std::vector<u8>& data, size_t& offset) {
    if (offset + 2 > data.size()) {
        LOG_WARNING_F("Cannot read string length at offset {} (data size: {})", offset, data.size());
        return "";
    }
    
    // Read length (little-endian)
    u16 length = data[offset] | (data[offset + 1] << 8);
    LOG_DEBUG_F("Reading string at offset {}: length = {}", offset, length);
    offset += 2;
    
    if (length == 0) {
        LOG_DEBUG("String length is 0, returning empty string");
        return "";
    }
    
    if (length > 1000) {  // Sanity check
        LOG_WARNING_F("String length {} seems unreasonable, returning empty string", length);
        return "";
    }
    
    if (offset + length > data.size()) {
        LOG_WARNING_F("String length {} extends beyond data size at offset {} (data size: {})", 
                     length, offset, data.size());
        return "";
    }
    
    // Read string data
    std::string result(data.begin() + offset, data.begin() + offset + length);
    offset += length;
    
    LOG_DEBUG_F("Read string: '{}' (new offset: {})", result, offset);
    return result;
}

void SWGLoginProtocol::write_u32_le(std::vector<u8>& data, u32 value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

void SWGLoginProtocol::write_u16_le(std::vector<u8>& data, u16 value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
}

void SWGLoginProtocol::write_string(std::vector<u8>& data, const std::string& str) {
    // Write length first (little-endian)
    write_u16_le(data, static_cast<u16>(str.length()));
    
    // Write string data
    data.insert(data.end(), str.begin(), str.end());
}

} // namespace login
} // namespace swganh