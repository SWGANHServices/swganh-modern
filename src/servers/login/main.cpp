// src/servers/login/main.cpp
#include "core/logger.hpp"
#include "core/types.hpp"
#include "network/udp_server.hpp"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

using namespace swganh;
using boost::asio::ip::udp;

class LoginServer {
public:
    LoginServer() : udp_server_(44453) {
        LOG_INFO("Initializing SWG:ANH Modern Login Server");
        
        // Set up packet handler
        udp_server_.set_packet_handler([this](const core::byte_vector& data, const udp::endpoint& endpoint) {
            handle_packet(data, endpoint);
        });
    }
    
    void start() {
        LOG_INFO("Starting Login Server...");
        udp_server_.start();
        
        if (udp_server_.is_running()) {
            LOG_INFO("Login Server is running on port ", udp_server_.port());
            LOG_INFO("Waiting for SWG client connections...");
        } else {
            LOG_FATAL("Failed to start Login Server");
        }
    }
    
    void stop() {
        LOG_INFO("Shutting down Login Server...");
        udp_server_.stop();
    }
    
    bool is_running() const {
        return udp_server_.is_running();
    }

private:
    void handle_packet(const core::byte_vector& data, const udp::endpoint& endpoint) {
        LOG_INFO("Received packet from client ", endpoint.address().to_string(), 
                ":", endpoint.port(), " (", data.size(), " bytes)");
        
        // Log packet contents in hex for debugging
        std::string hex_dump;
        for (size_t i = 0; i < data.size() && i < 32; ++i) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", data[i]);
            hex_dump += hex;
        }
        if (data.size() > 32) {
            hex_dump += "...";
        }
        
        LOG_DEBUG("Packet data: ", hex_dump);
        
        // For now, just send a simple response to show we're alive
        send_simple_response(endpoint);
    }
    
    void send_simple_response(const udp::endpoint& endpoint) {
        // Create a simple SOE session response packet
        // This is a placeholder - we'll implement proper SOE protocol later
        core::byte_vector response = {
            0x00, 0x01,  // SOE opcode (session request response)
            0x00, 0x02,  // Length
            0x01, 0x00,  // Session established
            0x00, 0x00   // CRC placeholder
        };
        
        LOG_DEBUG("Sending response to ", endpoint.address().to_string(), ":", endpoint.port());
        udp_server_.send_packet(response, endpoint);
    }
    
    network::UdpServer udp_server_;
};

// Global server instance for signal handling
std::unique_ptr<LoginServer> g_server;

void signal_handler(int signal) {
    LOG_INFO("Received signal ", signal, ", shutting down gracefully...");
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    // Set up logging
    core::Logger::instance().set_console_output(true);
    core::Logger::instance().set_level(core::LogLevel::DEBUG);
    core::Logger::instance().set_file_output("logs/login_server.log");
    
    LOG_INFO("=== SWG:ANH Modern Login Server ===");
    LOG_INFO("Version: 1.0.0-dev");
    LOG_INFO("Build: ", __DATE__, " ", __TIME__);
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // Create and start server
        g_server = std::make_unique<LoginServer>();
        g_server->start();
        
        // Main server loop
        while (g_server && g_server->is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception: ", e.what());
        return 1;
    }
    
    LOG_INFO("Login Server shutdown complete");
    return 0;
}

// src/servers/login/CMakeLists.txt
add_executable(login_server
    main.cpp
)

target_include_directories(login_server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(login_server
    swganh_core
    swganh_network
    ${Boost_LIBRARIES}
    pthread
)