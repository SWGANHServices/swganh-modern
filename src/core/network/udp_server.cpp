// Implementation file: src/core/network/udp_server.cpp
#include "udp_server.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

namespace swganh::network {

std::atomic<bool> UDPServer::networking_initialized_{false};

UDPServer::UDPServer() {
    InitializeNetworking();
}

UDPServer::~UDPServer() {
    Stop();
    CleanupNetworking();
}

bool UDPServer::Start(const std::string& bind_address, uint16_t port) {
    if (running_) {
        std::cout << "[UDP] Server already running" << std::endl;
        return false;
    }
    
    bind_address_ = bind_address;
    port_ = port;
    
    if (!InitializeSocket()) {
        std::cout << "[UDP] Failed to initialize socket" << std::endl;
        return false;
    }
    
    running_ = true;
    network_thread_ = std::thread(&UDPServer::NetworkThreadMain, this);
    
    std::cout << "[UDP] Server started on " << bind_address_ << ":" << port_ << std::endl;
    return true;
}

void UDPServer::Stop() {
    if (!running_) return;
    
    std::cout << "[UDP] Stopping server..." << std::endl;
    running_ = false;
    
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    
    CleanupSocket();
    std::cout << "[UDP] Server stopped" << std::endl;
}

void UDPServer::SetSOEHandler(std::unique_ptr<SOEProtocolHandler> handler) {
    soe_handler_ = std::move(handler);
}

bool UDPServer::SendPacket(const std::vector<uint8_t>& data, 
                          const std::string& address, 
                          uint16_t port) {
    if (socket_ == INVALID_SOCKET_VALUE) {
        return false;
    }
    
    struct sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    
#ifdef _WIN32
    dest_addr.sin_addr.s_addr = inet_addr(address.c_str());
#else
    inet_pton(AF_INET, address.c_str(), &dest_addr.sin_addr);
#endif
    
    ssize_t sent = sendto(socket_, 
                         reinterpret_cast<const char*>(data.data()), 
                         data.size(), 
                         0,
                         reinterpret_cast<const struct sockaddr*>(&dest_addr), 
                         sizeof(dest_addr));
    
    if (sent == -1) {
        std::cout << "[UDP] Send failed: " << GetLastSocketError() << std::endl;
        return false;
    }
    
    packets_sent_++;
    bytes_sent_ += sent;
    return true;
}

bool UDPServer::SendPacketToSession(uint32_t session_id, const std::vector<uint8_t>& data) {
    if (!soe_handler_) return false;
    
    auto* session = soe_handler_->GetSession(session_id);
    if (!session) return false;
    
    return SendPacket(data, session->remote_address, session->remote_port);
}

void UDPServer::NetworkThreadMain() {
    std::cout << "[UDP] Network thread started" << std::endl;
    
    while (running_) {
        ProcessIncomingData();
        
        // Update SOE handler for cleanup and maintenance
        if (soe_handler_) {
            auto* basic_handler = dynamic_cast<BasicSOEHandler*>(soe_handler_.get());
            if (basic_handler) {
                basic_handler->Update();
            }
        }
        
        // Small sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "[UDP] Network thread stopped" << std::endl;
}

void UDPServer::ProcessIncomingData() {
    if (socket_ == INVALID_SOCKET_VALUE) return;
    
    std::vector<uint8_t> buffer(max_packet_size_);
    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    
    ssize_t received = recvfrom(socket_,
                               reinterpret_cast<char*>(buffer.data()),
                               buffer.size(),
                               0,
                               reinterpret_cast<struct sockaddr*>(&client_addr),
                               &addr_len);
    
    if (received > 0) {
        buffer.resize(received);
        packets_received_++;
        bytes_received_ += received;
        
        // Convert client address to string
        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
        uint16_t client_port = ntohs(client_addr.sin_port);
        
        HandleClientPacket(buffer, std::string(addr_str), client_port);
    } else if (received == -1) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            std::cout << "[UDP] Receive error: " << error << std::endl;
        }
#else
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "[UDP] Receive error: " << strerror(errno) << std::endl;
        }
#endif
    }
}

void UDPServer::HandleClientPacket(const std::vector<uint8_t>& data,
                                  const std::string& client_addr,
                                  uint16_t client_port) {
    if (!soe_handler_) {
        std::cout << "[UDP] No SOE handler configured" << std::endl;
        return;
    }
    
    // Process packet through SOE protocol handler
    bool handled = soe_handler_->ProcessIncomingPacket(data, client_addr, client_port);
    
    if (!handled) {
        std::cout << "[UDP] Failed to handle packet from " 
                  << client_addr << ":" << client_port << std::endl;
    }
}

bool UDPServer::InitializeSocket() {
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET_VALUE) {
        std::cout << "[UDP] Failed to create socket: " << GetLastSocketError() << std::endl;
        return false;
    }
    
    // Set socket to non-blocking mode
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socket_, FIONBIO, &mode) != 0) {
        std::cout << "[UDP] Failed to set non-blocking mode: " << GetLastSocketError() << std::endl;
        CleanupSocket();
        return false;
    }
#else
    int flags = fcntl(socket_, F_GETFL, 0);
    if (flags == -1 || fcntl(socket_, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cout << "[UDP] Failed to set non-blocking mode: " << strerror(errno) << std::endl;
        CleanupSocket();
        return false;
    }
#endif
    
    // Enable address reuse
    int reuse = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) != 0) {
        std::cout << "[UDP] Failed to set SO_REUSEADDR: " << GetLastSocketError() << std::endl;
    }
    
    // Bind socket
    struct sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(port_);
    
    if (bind_address_ == "0.0.0.0") {
        bind_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
#ifdef _WIN32
        bind_addr.sin_addr.s_addr = inet_addr(bind_address_.c_str());
#else
        inet_pton(AF_INET, bind_address_.c_str(), &bind_addr.sin_addr);
#endif
    }
    
    if (bind(socket_, reinterpret_cast<const struct sockaddr*>(&bind_addr), 
             sizeof(bind_addr)) != 0) {
        std::cout << "[UDP] Failed to bind socket: " << GetLastSocketError() << std::endl;
        CleanupSocket();
        return false;
    }
    
    return true;
}

void UDPServer::CleanupSocket() {
    if (socket_ != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
    }
}

std::string UDPServer::GetLastSocketError() {
#ifdef _WIN32
    int error = WSAGetLastError();
    return "Error " + std::to_string(error);
#else
    return strerror(errno);
#endif
}

bool UDPServer::InitializeNetworking() {
    if (networking_initialized_.exchange(true)) {
        return true; // Already initialized
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "[UDP] WSAStartup failed: " << result << std::endl;
        networking_initialized_ = false;
        return false;
    }
#endif
    
    return true;
}

void UDPServer::CleanupNetworking() {
    if (networking_initialized_.exchange(false)) {
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

} // namespace swganh::network
