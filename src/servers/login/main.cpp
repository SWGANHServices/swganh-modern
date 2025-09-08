// Main application: src/servers/login/main.cpp
#include "login_server.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

std::unique_ptr<swganh::login::LoginServer> g_server;

void SignalHandler(int signal) {
    std::cout << "\n[MAIN] Received signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->Stop();
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        // Create and configure server
        g_server = std::make_unique<swganh::login::LoginServer>();
        g_server->SetGalaxyName("SWG:ANH Modern");
        g_server->SetGalaxyId(1);
        g_server->SetMaxPlayers(3000);
        g_server->SetOnlineStatus(true);
        
        // Start server
        std::string bind_address = "0.0.0.0";
        uint16_t port = 44453;
        
        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--address" && i + 1 < argc) {
                bind_address = argv[++i];
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "Options:" << std::endl;
                std::cout << "  --address <ip>    Bind address (default: 0.0.0.0)" << std::endl;
                std::cout << "  --port <port>     Port number (default: 44453)" << std::endl;
                std::cout << "  --help            Show this help" << std::endl;
                return 0;
            }
        }
        
        if (!g_server->Start(bind_address, port)) {
            std::cout << "[MAIN] Failed to start login server" << std::endl;
            return 1;
        }
        
        // Main loop
        std::cout << "[MAIN] Server running. Press Ctrl+C to stop." << std::endl;
        
        auto last_stats = std::chrono::steady_clock::now();
        while (g_server->IsRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print stats every 30 seconds
            auto now = std::chrono::steady_clock::now();
            if (now - last_stats > std::chrono::seconds(30)) {
                g_server->PrintStats();
                last_stats = now;
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "[MAIN] Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "[MAIN] Server shutdown complete" << std::endl;
    return 0;
}
