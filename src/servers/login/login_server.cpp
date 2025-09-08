// Implementation: src/servers/login/login_server.cpp
#include "login_server.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace swganh::login {

LoginServer::LoginServer() {
    udp_server_ = std::make_unique<network::UDPServer>();
    soe_handler_ = std::make_unique<network::BasicSOEHandler>();
    
    // Set up SOE handler to forward game packets to us
    soe_handler_->SetPacketHandler([this](network::ClientSession* session, const network::SOEPacket& packet) {
        HandleGamePacket(session, packet);
    });
    
    udp_server_->SetSOEHandler(std::unique_ptr<network::SOEProtocolHandler>(soe_handler_.release()));
}

LoginServer::~LoginServer() {
    Stop();
}

bool LoginServer::Start(const std::string& bind_address, uint16_t port) {
    std::cout << "========================================" << std::endl;
    std::cout << "    SWG:ANH Modern Login Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Version: " << server_version_ << std::endl;
    std::cout << "Build Date: " << build_date_ << std::endl;
    std::cout << "Galaxy: " << galaxy_name_ << " (ID: " << galaxy_id_ << ")" << std::endl;
    std::cout << "Max Players: " << max_players_ << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (!udp_server_->Start(bind_address, port)) {
        std::cout << "[LOGIN] Failed to start UDP server" << std::endl;
        return false;
    }
    
    std::cout << "[LOGIN] Login server started successfully!" << std::endl;
    std::cout << "[LOGIN] Listening on " << bind_address << ":" << port << std::endl;
    std::cout << "[LOGIN] Ready for client connections..." << std::endl;
    return true;
}

void LoginServer::Stop() {
    if (udp_server_) {
        udp_server_->Stop();
    }
    std::cout << "[LOGIN] Login server stopped" << std::endl;
}

bool LoginServer::IsRunning() const {
    return udp_server_ && udp_server_->IsRunning();
}

uint32_t LoginServer::GetConnectedClients() const {
    // TODO: Get actual count from SOE handler
    return current_players_;
}

void LoginServer::PrintStats() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "\n========== LOGIN SERVER STATS ===========" << std::endl;
    std::cout << "Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "Status: " << (IsRunning() ? "RUNNING" : "STOPPED") << std::endl;
    std::cout << "Galaxy: " << galaxy_name_ << " (Online: " << (online_status_ ? "YES" : "NO") << ")" << std::endl;
    std::cout << "Connected Clients: " << GetConnectedClients() << "/" << max_players_ << std::endl;
    
    if (udp_server_) {
        std::cout << "Packets Received: " << udp_server_->GetPacketsReceived() << std::endl;
        std::cout << "Packets Sent: " << udp_server_->GetPacketsSent() << std::endl;
        std::cout << "Bytes Received: " << udp_server_->GetBytesReceived() << std::endl;
        std::cout << "Bytes Sent: " << udp_server_->GetBytesSent() << std::endl;
    }
    std::cout << "==========================================\n" << std::endl;
}

void LoginServer::HandleGamePacket(network::ClientSession* session, const network::SOEPacket& packet) {
    // Skip SOE header and read the game packet opcode
    auto packet_copy = packet;
    packet_copy.ReadUInt16(); // Skip SOE opcode
    packet_copy.ReadUInt16(); // Skip SOE sequence
    
    if (packet_copy.Size() < 8) { // Need at least 4 bytes for game opcode
        std::cout << "[LOGIN] Game packet too small: " << packet_copy.Size() << " bytes" << std::endl;
        return;
    }
    
    uint32_t game_opcode = packet_copy.ReadUInt32();
    
    std::cout << "[LOGIN] Game packet from session " << session->session_id 
              << " opcode: 0x" << std::hex << game_opcode << std::dec << std::endl;
    
    switch (game_opcode) {
        case LOGIN_CLIENT_ID:
            HandleLoginClientId(session, packet_copy);
            break;
            
        case LOGIN_ENUM_CLUSTER:
            HandleEnumCluster(session, packet_copy);
            break;
            
        case LOGIN_CLUSTER_STATUS:
            HandleClusterStatus(session, packet_copy);
            break;
            
        default:
            std::cout << "[LOGIN] Unknown game opcode: 0x" << std::hex << game_opcode << std::dec << std::endl;
            break;
    }
}

void LoginServer::HandleLoginClientId(network::ClientSession* session, const network::SOEPacket& packet) {
    std::cout << "[LOGIN] Client ID packet from session " << session->session_id << std::endl;
    
    // For now, just acknowledge the client
    // TODO: Read username/password and validate
    
    // Send login success response
    auto response = CreateLoginResponseOk();
    
    // Wrap in SOE data packet
    network::SOEPacket soe_response(network::SOE_DATA_CHANNEL_A);
    soe_response.SetSequence(++session->server_sequence);
    soe_response.WriteData(response.GetData());
    
    udp_server_->SendPacketToSession(session->session_id, soe_response.GetData());
    
    std::cout << "[LOGIN] Sent login OK to session " << session->session_id << std::endl;
}

void LoginServer::HandleEnumCluster(network::ClientSession* session, const network::SOEPacket& packet) {
    std::cout << "[LOGIN] Enum cluster request from session " << session->session_id << std::endl;
    
    // Send cluster list
    auto response = CreateClusterListResponse();
    
    // Wrap in SOE data packet
    network::SOEPacket soe_response(network::SOE_DATA_CHANNEL_A);
    soe_response.SetSequence(++session->server_sequence);
    soe_response.WriteData(response.GetData());
    
    udp_server_->SendPacketToSession(session->session_id, soe_response.GetData());
    
    std::cout << "[LOGIN] Sent cluster list to session " << session->session_id << std::endl;
}

void LoginServer::HandleClusterStatus(network::ClientSession* session, const network::SOEPacket& packet) {
    std::cout << "[LOGIN] Cluster status request from session " << session->session_id << std::endl;
    
    // For now, just log the request
    // TODO: Send actual cluster status
}

network::SOEPacket LoginServer::CreateClusterListResponse() {
    network::SOEPacket response;
    
    // LOGIN_CLUSTER_LIST opcode
    response.WriteUInt32(LOGIN_CLUSTER_LIST);
    
    // Cluster count
    response.WriteUInt8(1); // One galaxy
    
    // Galaxy information
    response.WriteUInt32(galaxy_id_);                    // Galaxy ID
    response.WriteString(galaxy_name_);                  // Galaxy name
    response.WriteUInt32(current_players_);              // Current players
    response.WriteUInt32(max_players_);                  // Max players
    response.WriteUInt32(online_status_ ? 1 : 0);        // Online status
    response.WriteUInt32(0);                             // Recommended (0 = not recommended)
    response.WriteString("127.0.0.1");                   // Zone server IP
    response.WriteUInt16(44463);                         // Zone server port
    response.WriteUInt32(1);                             // Population level (1-5)
    response.WriteUInt32(60);                            // Max characters per account
    response.WriteUInt32(0);                             // Distance
    
    return response;
}

network::SOEPacket LoginServer::CreateLoginResponseOk() {
    network::SOEPacket response;
    
    // Simple OK response - in a real implementation this would be more complex
    response.WriteUInt32(0x12345678); // Placeholder opcode for login OK
    response.WriteUInt8(1);           // Success flag
    response.WriteString("Login successful");
    
    return response;
}

network::SOEPacket LoginServer::CreateLoginResponseFailed(const std::string& reason) {
    network::SOEPacket response;
    
    response.WriteUInt32(0x12345679); // Placeholder opcode for login failed
    response.WriteUInt8(0);           // Failure flag
    response.WriteString(reason);
    
    return response;
}

} // namespace swganh::login
