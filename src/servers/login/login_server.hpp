// src/servers/login/login_server.hpp
#pragma once

#include "../../core/network/udp_server.hpp"
#include "../../core/network/soe_protocol.hpp"
#include <memory>
#include <string>
#include <atomic>

namespace swganh::login {

// SWG-specific packet opcodes (after SOE layer)
constexpr uint32_t LOGIN_CLIENT_ID = 0x41131B75;
constexpr uint32_t LOGIN_CLUSTER_STATUS = 0x3436AEB6;
constexpr uint32_t LOGIN_ENUM_CLUSTER = 0xC11C63B9;
constexpr uint32_t LOGIN_CLUSTER_LIST = 0x3A2A7CD8;

class LoginServer {
public:
    LoginServer();
    ~LoginServer();
    
    // Server lifecycle
    bool Start(const std::string& bind_address = "0.0.0.0", uint16_t port = 44453);
    void Stop();
    bool IsRunning() const;
    
    // Configuration
    void SetGalaxyName(const std::string& name) { galaxy_name_ = name; }
    void SetGalaxyId(uint32_t id) { galaxy_id_ = id; }
    void SetMaxPlayers(uint32_t max_players) { max_players_ = max_players; }
    void SetOnlineStatus(bool online) { online_status_ = online; }
    
    // Statistics
    uint32_t GetConnectedClients() const;
    void PrintStats() const;
    
private:
    // Network handling
    void HandleGamePacket(network::ClientSession* session, const network::SOEPacket& packet);
    
    // SWG Protocol handlers
    void HandleLoginClientId(network::ClientSession* session, const network::SOEPacket& packet);
    void HandleEnumCluster(network::ClientSession* session, const network::SOEPacket& packet);
    void HandleClusterStatus(network::ClientSession* session, const network::SOEPacket& packet);
    
    // Response builders
    network::SOEPacket CreateClusterListResponse();
    network::SOEPacket CreateLoginResponseOk();
    network::SOEPacket CreateLoginResponseFailed(const std::string& reason);
    
    // Server state
    std::unique_ptr<network::UDPServer> udp_server_;
    std::unique_ptr<network::BasicSOEHandler> soe_handler_;
    
    // Galaxy configuration
    std::string galaxy_name_{"SWG:ANH Modern"};
    uint32_t galaxy_id_{1};
    uint32_t max_players_{3000};
    std::atomic<bool> online_status_{true};
    uint32_t current_players_{0};
    
    // Server info
    std::string server_version_{"1.0.0"};
    std::string build_date_{__DATE__};
};

} // namespace swganh::login
