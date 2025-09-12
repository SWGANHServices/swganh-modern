// File: src/servers/login/main.cpp
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <csignal>

#include "../../core/logger.hpp"
#include "../../core/config.hpp"
#include "../../core/account_manager.hpp"
#include "../../network/udp_server.hpp"
#include "swg_protocol.hpp"

using namespace swganh;

volatile bool running = true;

void signal_handler(int) {
    LOG_INFO("Received shutdown signal");
    running = false;
}

// Create SOE Session Response packet
std::vector<u8> create_session_response(uint32_t connection_id) {
    std::vector<u8> response;
    
    response.push_back(0x00);
    response.push_back(0x02);
    
    response.push_back(connection_id & 0xFF);
    response.push_back((connection_id >> 8) & 0xFF);
    response.push_back((connection_id >> 16) & 0xFF);
    response.push_back((connection_id >> 24) & 0xFF);
    
    uint32_t crc_seed = 0x12345678;
    response.push_back(crc_seed & 0xFF);
    response.push_back((crc_seed >> 8) & 0xFF);
    response.push_back((crc_seed >> 16) & 0xFF);
    response.push_back((crc_seed >> 24) & 0xFF);
    
    response.push_back(0x02);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);
    
    uint32_t server_udp_size = 496;
    response.push_back(server_udp_size & 0xFF);
    response.push_back((server_udp_size >> 8) & 0xFF);
    response.push_back((server_udp_size >> 16) & 0xFF);
    response.push_back((server_udp_size >> 24) & 0xFF);
    
    uint32_t protocol = 3;
    response.push_back(protocol & 0xFF);
    response.push_back((protocol >> 8) & 0xFF);
    response.push_back((protocol >> 16) & 0xFF);
    response.push_back((protocol >> 24) & 0xFF);
    
    return response;
}

// Simple hex dump function
std::string hex_dump(const std::vector<u8>& data) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    
    for (size_t i = 0; i < data.size(); i += 16) {
        oss << "\n    " << std::setw(8) << i << ": ";
        
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < data.size()) {
                oss << std::setw(2) << static_cast<unsigned int>(data[i + j]) << " ";
            } else {
                oss << "   ";
            }
        }
        
        oss << " |";
        
        for (size_t j = 0; j < 16 && i + j < data.size(); ++j) {
            char c = static_cast<char>(data[i + j]);
            oss << (std::isprint(c) ? c : '.');
        }
        oss << "|";
    }
    
    return oss.str();
}

// Manual packet analysis with fixed formatting
void debug_login_packet(const std::vector<u8>& data) {
    LOG_INFO("=== MANUAL PACKET ANALYSIS ===");
    
    if (data.size() >= 4) {
        std::ostringstream soe_header;
        soe_header << "SOE Header: " << std::hex << std::uppercase << std::setfill('0');
        soe_header << std::setw(2) << static_cast<unsigned int>(data[0]) << " ";
        soe_header << std::setw(2) << static_cast<unsigned int>(data[1]) << " ";
        soe_header << std::setw(2) << static_cast<unsigned int>(data[2]) << " ";
        soe_header << std::setw(2) << static_cast<unsigned int>(data[3]);
        LOG_INFO(soe_header.str());
    }
    
    if (data.size() >= 10) {
        std::ostringstream swg_header;
        swg_header << "SWG Header: " << std::hex << std::uppercase << std::setfill('0');
        for (size_t i = 4; i < 10; ++i) {
            swg_header << std::setw(2) << static_cast<unsigned int>(data[i]) << " ";
        }
        LOG_INFO(swg_header.str());
    }
    
    // Now let's manually parse the strings starting at offset 10
    size_t offset = 10;
    
    for (int i = 0; i < 3 && offset + 2 < data.size(); ++i) {
        u16 len = data[offset] | (data[offset + 1] << 8);
        
        std::ostringstream str_info;
        str_info << "String " << i << ": length = " << len << " at offset " << offset;
        LOG_INFO(str_info.str());
        
        if (len > 0 && len < 1000 && offset + 2 + len <= data.size()) {
            std::string str(data.begin() + offset + 2, data.begin() + offset + 2 + len);
            std::ostringstream str_content;
            str_content << "String " << i << ": '" << str << "'";
            LOG_INFO(str_content.str());
            offset += 2 + len;
        } else {
            std::ostringstream str_error;
            str_error << "String " << i << " invalid or extends beyond packet";
            LOG_WARNING(str_error.str());
            break;
        }
    }
}

// Enhanced packet handler
void handle_packet(const std::vector<u8>& data, 
                  const boost::asio::ip::udp::endpoint& sender,
                  std::function<void(const std::vector<u8>&, const boost::asio::ip::udp::endpoint&)> send_response) {
    
    LOG_INFO("========================================");
    
    std::ostringstream info;
    info << "PACKET from " << sender.address().to_string() << ":" << sender.port() 
         << " (" << data.size() << " bytes)";
    LOG_INFO(info.str());
    
    if (data.size() >= 2) {
        uint16_t opcode = data[0] | (data[1] << 8);
        
        std::string opcode_name;
        switch (opcode) {
            case 0x0100: opcode_name = "Session Request"; break;
            case 0x0200: opcode_name = "Session Response"; break;
            case 0x0300: opcode_name = "Multi Packet"; break;
            case 0x0400: opcode_name = "Disconnect"; break;
            case 0x0500: opcode_name = "Ping"; break;
            case 0x0600: opcode_name = "Net Status Request"; break;
            case 0x0700: opcode_name = "Net Status Response"; break;
            case 0x0800: opcode_name = "Data"; break;
            case 0x0900: opcode_name = "Data Fragment"; break;
            case 0x0d00: opcode_name = "Acknowledge"; break;
            case 0x1500: opcode_name = "Out of Order"; break;
            default:     opcode_name = "Unknown"; break;
        }
        
        std::ostringstream opcode_info;
        opcode_info << "SOE Opcode: 0x" << std::hex << std::uppercase << std::setfill('0') 
                    << std::setw(4) << opcode << " (" << opcode_name << ")";
        LOG_INFO(opcode_info.str());
        
        // Handle different packet types
        switch (opcode) {
            case 0x0100: { // Session Request
                if (data.size() >= 14) {
                    LOG_INFO("=== Processing Session Request ===");
                    uint32_t conn_id = data[6] | (data[7] << 8) | (data[8] << 16) | (data[9] << 24);
                    
                    std::ostringstream detail;
                    detail << "  Connection ID: 0x" << std::hex << std::uppercase << conn_id;
                    LOG_INFO(detail.str());
                    
                    LOG_INFO("=== Sending Session Response ===");
                    std::vector<u8> response = create_session_response(conn_id);
                    send_response(response, sender);
                    LOG_INFO("SOE session established successfully!");
                }
                break;
            }
                
            case 0x0500: // Ping
                LOG_INFO("=== Ping Packet ===");
                LOG_INFO("Client sending keep-alive ping");
                break;
                
            case 0x0700: // Net Status Response
                LOG_INFO("=== Net Status Response ===");
                LOG_INFO("Client reporting network status");
                break;
                
            case 0x0900: { // Data Fragment - LOGIN ATTEMPT
                LOG_INFO("=== Data Fragment Packet ===");
                LOG_INFO("Processing SWG login attempt...");
                
                // First, do manual analysis
                debug_login_packet(data);
                
                try {
                    // Parse login request with fixed offset
                    login::LoginRequest login_req = 
                        login::SWGLoginProtocol::parse_login_request(data);
                    
                    LOG_INFO("=== Login Request Details ===");
                    LOG_INFO_F("  Username: '{}'", login_req.username);
                    LOG_INFO_F("  Password: '{}'", login_req.password);
                    LOG_INFO_F("  Client Version: '{}'", login_req.client_version);
                    
                    // Only proceed if we got a valid username
                    if (!login_req.username.empty()) {
                        // Authenticate with account manager
                        AccountManager& account_mgr = AccountManager::instance();
                        LoginResult result = account_mgr.authenticate(login_req.username, login_req.password);
                        
                        LOG_INFO("=== Authentication Result ===");
                        switch (result) {
                            case LoginResult::SUCCESS:
                                LOG_INFO("Login successful!");
                                break;
                            case LoginResult::INVALID_CREDENTIALS:
                                LOG_INFO("Login failed - invalid credentials");
                                break;
                            case LoginResult::ACCOUNT_DISABLED:
                                LOG_INFO("Login failed - account disabled");
                                break;
                            default:
                                LOG_INFO("Login failed - unknown error");
                                break;
                        }
                        
                        // Get account ID if successful
                        u32 account_id = 0;
                        if (result == LoginResult::SUCCESS) {
                            auto account = account_mgr.get_account(login_req.username);
                            if (account) {
                                account_id = account->account_id;
                            }
                        }
                        
                        // Create and send login response
                        LOG_INFO("=== Sending Login Response ===");
                        std::vector<u8> login_response = 
                            login::SWGLoginProtocol::create_login_response(result, account_id);
                        
                        std::vector<u8> soe_response = 
                            login::SWGLoginProtocol::wrap_in_soe_data(login_response, 1);
                        
                        send_response(soe_response, sender);
                        LOG_INFO("Login response sent to client!");
                        
                        // If successful, client should proceed to server list
                        if (result == LoginResult::SUCCESS) {
                            LOG_INFO("Client should now request server list!");
                        }
                        
                    } else {
                        LOG_WARNING("Skipping authentication - username is empty (parsing failed)");
                    }
                    
                } catch (const std::exception& e) {
                    LOG_ERROR_F("Error processing login: {}", e.what());
                }
                break;
            }
                
            default:
                LOG_INFO("=== Unhandled Packet Type ===");
                break;
        }
    }
    
    LOG_INFO("Raw data:" + hex_dump(data));
    LOG_INFO("========================================");
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    init_logger(LogLevel::DEBUG_LEVEL);
    
    LOG_INFO("=== SWG:ANH Modern Login Server ===");
    LOG_INFO("Version: 1.0.0-dev");
    LOG_INFO("Fixed Protocol Parser - Ready for Login!");
    
    // Initialize configuration
    Config& config = Config::instance();
    LOG_INFO("=== Server Configuration ===");
    LOG_INFO_F("Auto-create accounts: {}", config.get_bool("auto_create_accounts") ? "YES" : "NO");
    LOG_INFO_F("Debug mode: {}", config.get_bool("debug_login") ? "YES" : "NO");
    LOG_INFO_F("Server name: {}", config.get("server_name"));
    
    // Initialize account manager
    AccountManager& account_mgr = AccountManager::instance();
    account_mgr.create_test_accounts();
    LOG_INFO_F("Loaded {} test accounts", account_mgr.get_account_count());
    
    try {
        UdpServer server(44453);
        
        server.set_packet_handler([](const std::vector<u8>& data, 
                                    const boost::asio::ip::udp::endpoint& sender,
                                    std::function<void(const std::vector<u8>&, const boost::asio::ip::udp::endpoint&)> send_func) {
            handle_packet(data, sender, send_func);
        });
        
        server.start();
        LOG_INFO("Login server started with FIXED parsing!");
        LOG_INFO("Try connecting with username 'test' and password 'test'");
        
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        server.stop();
        
    } catch (const std::exception& e) {
        std::ostringstream error_msg;
        error_msg << "Server error: " << e.what();
        LOG_FATAL(error_msg.str());
        return 1;
    }
    
    LOG_INFO("Server shutdown complete");
    return 0;
}