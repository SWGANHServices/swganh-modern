// src/network/udp_server.hpp
#pragma once

#include "core/types.hpp"
#include "core/logger.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <functional>
#include <memory>
#include <thread>

namespace swganh::network {

using boost::asio::ip::udp;

class UdpServer {
public:
    using PacketHandler = std::function<void(const core::byte_vector&, const udp::endpoint&)>;
    
    explicit UdpServer(core::u16 port);
    ~UdpServer();
    
    // Start/stop the server
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Set packet handler
    void set_packet_handler(PacketHandler handler);
    
    // Send packet to endpoint
    void send_packet(const core::byte_vector& data, const udp::endpoint& endpoint);
    
    // Get server info
    core::u16 port() const { return port_; }
    
private:
    void start_receive();
    void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_send(const boost::system::error_code& error, std::size_t bytes_transferred);
    void io_thread();
    
    core::u16 port_;
    bool running_{false};
    
    boost::asio::io_context io_context_;
    std::unique_ptr<udp::socket> socket_;
    std::unique_ptr<std::thread> io_thread_;
    
    udp::endpoint remote_endpoint_;
    std::array<core::u8, 1024> receive_buffer_;
    
    PacketHandler packet_handler_;
};

} // namespace swganh::network

// src/network/udp_server.cpp
#include "udp_server.hpp"
#include <iostream>

namespace swganh::network {

UdpServer::UdpServer(core::u16 port) : port_(port) {
    LOG_INFO("Creating UDP server on port ", port);
}

UdpServer::~UdpServer() {
    stop();
}

void UdpServer::start() {
    if (running_) {
        LOG_WARNING("UDP server already running");
        return;
    }
    
    try {
        socket_ = std::make_unique<udp::socket>(io_context_, udp::endpoint(udp::v4(), port_));
        
        LOG_INFO("UDP server started on port ", port_);
        running_ = true;
        
        start_receive();
        
        // Start IO thread
        io_thread_ = std::make_unique<std::thread>([this]() { io_thread(); });
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start UDP server: ", e.what());
        running_ = false;
    }
}

void UdpServer::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping UDP server");
    running_ = false;
    
    io_context_.stop();
    
    if (io_thread_ && io_thread_->joinable()) {
        io_thread_->join();
    }
    
    if (socket_) {
        socket_->close();
        socket_.reset();
    }
    
    LOG_INFO("UDP server stopped");
}

void UdpServer::set_packet_handler(PacketHandler handler) {
    packet_handler_ = std::move(handler);
}

void UdpServer::send_packet(const core::byte_vector& data, const udp::endpoint& endpoint) {
    if (!socket_ || !running_) {
        LOG_ERROR("Cannot send packet: server not running");
        return;
    }
    
    socket_->async_send_to(
        boost::asio::buffer(data),
        endpoint,
        boost::bind(&UdpServer::handle_send, this,
                   boost::asio::placeholders::error,
                   boost::asio::placeholders::bytes_transferred)
    );
}

void UdpServer::start_receive() {
    if (!socket_ || !running_) {
        return;
    }
    
    socket_->async_receive_from(
        boost::asio::buffer(receive_buffer_),
        remote_endpoint_,
        boost::bind(&UdpServer::handle_receive, this,
                   boost::asio::placeholders::error,
                   boost::asio::placeholders::bytes_transferred)
    );
}

void UdpServer::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error && bytes_transferred > 0) {
        // Create packet data
        core::byte_vector packet_data(receive_buffer_.begin(), 
                                     receive_buffer_.begin() + bytes_transferred);
        
        LOG_DEBUG("Received ", bytes_transferred, " bytes from ", 
                 remote_endpoint_.address().to_string(), ":", remote_endpoint_.port());
        
        // Call packet handler if set
        if (packet_handler_) {
            packet_handler_(packet_data, remote_endpoint_);
        }
        
        // Continue receiving
        start_receive();
        
    } else if (error != boost::asio::error::operation_aborted) {
        LOG_ERROR("UDP receive error: ", error.message());
        if (running_) {
            start_receive();
        }
    }
}

void UdpServer::handle_send(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (error) {
        LOG_ERROR("UDP send error: ", error.message());
    } else {
        LOG_DEBUG("Sent ", bytes_transferred, " bytes");
    }
}

void UdpServer::io_thread() {
    LOG_DEBUG("UDP server IO thread started");
    
    while (running_) {
        try {
            io_context_.run();
            break;
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in UDP server IO thread: ", e.what());
            if (!running_) break;
            
            // Reset context for retry
            io_context_.reset();
        }
    }
    
    LOG_DEBUG("UDP server IO thread stopped");
}

} // namespace swganh::network