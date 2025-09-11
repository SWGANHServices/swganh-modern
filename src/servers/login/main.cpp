#include "core/logger.hpp"
#include "network/udp_server.hpp"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

using namespace swganh;
using boost::asio::ip::udp;

std::unique_ptr<network::UdpServer> g_server;

void signal_handler(int signal) {
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    core::Logger::instance().set_console_output(true);
    core::Logger::instance().set_level(core::LogLevel::DEBUG);
    
    LOG_INFO("=== SWG:ANH Modern Login Server ===");
    LOG_INFO("Version: 1.0.0-dev");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        g_server = std::make_unique<network::UdpServer>(44453);
        
        g_server->set_packet_handler([](const core::byte_vector& data, const udp::endpoint& endpoint) {
            LOG_INFO("Received packet from ", endpoint.address().to_string(), ":", endpoint.port(), " (", data.size(), " bytes)");
        });
        
        g_server->start();
        
        if (g_server->is_running()) {
            LOG_INFO("Login server is running on port 44453");
            LOG_INFO("Waiting for SWG client connections...");
            LOG_INFO("Press Ctrl+C to stop");
            
            while (g_server && g_server->is_running()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
    } catch (const std::exception& e) {
        LOG_FATAL("Exception: ", e.what());
        return 1;
    }
    
    LOG_INFO("Server shutdown complete");
    return 0;
}
