// src/core/network/udp_server.hpp
#pragma once

#include "soe_protocol.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

namespace swganh::network {

class UDPServer {
public:
    UDPServer();
    ~UDPServer();
    
    // Server lifecycle
    bool Start(const std::string& bind_address, uint16_t port);
    void Stop();
    bool IsRunning() const { return running_; }
    
    // Configuration
    void SetSOEHandler(std::unique_ptr<SOEProtocolHandler> handler);
    void SetMaxPacketSize(size_t size) { max_packet_size_ = size; }
    void SetWorkerThreads(size_t count) { worker_thread_count_ = count; }
    
    // Statistics
    uint64_t GetPacketsReceived() const { return packets_received_; }
    uint64_t GetPacketsSent() const { return packets_sent_; }
    uint64_t GetBytesReceived() const { return bytes_received_; }
    uint64_t GetBytesSent() const { return bytes_sent_; }
    
    // Sending packets
    bool SendPacket(const std::vector<uint8_t>& data, 
                   const std::string& address, 
                   uint16_t port);
    
    bool SendPacketToSession(uint32_t session_id, const std::vector<uint8_t>& data);
    
private:
    // Network implementation
    void NetworkThreadMain();
    void ProcessIncomingData();
    void HandleClientPacket(const std::vector<uint8_t>& data,
                           const std::string& client_addr,
                           uint16_t client_port);
    
    // Socket management
    bool InitializeSocket();
    void CleanupSocket();
    std::string GetLastSocketError();
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread network_thread_;
    size_t worker_thread_count_{1};
    
    // Network state
    socket_t socket_{INVALID_SOCKET_VALUE};
    std::string bind_address_{"0.0.0.0"};
    uint16_t port_{44453};
    size_t max_packet_size_{1024};
    
    // Protocol handling
    std::unique_ptr<SOEProtocolHandler> soe_handler_;
    
    // Statistics
    std::atomic<uint64_t> packets_received_{0};
    std::atomic<uint64_t> packets_sent_{0};
    std::atomic<uint64_t> bytes_received_{0};
    std::atomic<uint64_t> bytes_sent_{0};
    
    // Platform-specific initialization
    static bool InitializeNetworking();
    static void CleanupNetworking();
    static std::atomic<bool> networking_initialized_;
};

} // namespace swganh::network
