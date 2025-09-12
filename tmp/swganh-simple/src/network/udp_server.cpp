// File: src/network/udp_server.cpp
#include "udp_server.hpp"
#include "../core/logger.hpp"

namespace swganh {

UdpServer::UdpServer(u16 port) 
    : port_(port), running_(false), io_context_(), work_guard_(io_context_.get_executor()) {
}

UdpServer::~UdpServer() {
    stop();
}

void UdpServer::set_packet_handler(PacketHandler handler) {
    packet_handler_ = std::move(handler);
    LOG_DEBUG("Packet handler set");
}

void UdpServer::start() {
    if (running_) {
        LOG_WARNING("Server already running");
        return;
    }

    try {
        // Create socket and bind to all interfaces
        socket_ = std::make_unique<udp::socket>(io_context_, 
            udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"), port_));
        
        running_ = true;
        start_receive();
        
        // Start IO context in separate thread
        io_thread_ = std::thread([this]() {
            LOG_DEBUG("UDP server IO thread started");
            io_context_.run();
            LOG_DEBUG("UDP server IO thread stopped");
        });
        
        LOG_INFO_F("UDP server started on port {}", port_);
        
    } catch (const std::exception& e) {
        LOG_ERROR_F("Failed to start UDP server: {}", e.what());
        throw;
    }
}

void UdpServer::stop() {
    if (!running_) return;
    
    LOG_INFO("Stopping UDP server...");
    running_ = false;
    
    if (socket_) {
        socket_->close();
    }
    
    work_guard_.reset();
    io_context_.stop();
    
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    LOG_INFO("UDP server stopped");
}

void UdpServer::send_packet(const std::vector<u8>& data, const udp::endpoint& target) {
    if (!socket_ || !running_) {
        LOG_ERROR("Cannot send packet - server not running");
        return;
    }
    
    // Post send operation to IO context
    io_context_.post([this, data, target]() {
        try {
            socket_->send_to(boost::asio::buffer(data), target);
            LOG_DEBUG_F("Sent {} bytes to {}:{}", data.size(), 
                       target.address().to_string(), target.port());
        } catch (const std::exception& e) {
            LOG_ERROR_F("Failed to send packet: {}", e.what());
        }
    });
}

void UdpServer::start_receive() {
    if (!socket_ || !running_) return;
    
    socket_->async_receive_from(
        boost::asio::buffer(receive_buffer_),
        sender_endpoint_,
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            handle_receive(error, bytes_transferred);
        }
    );
}

void UdpServer::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if (!error && bytes_transferred > 0) {
        // Convert buffer to vector
        std::vector<u8> packet_data(receive_buffer_.begin(), 
                                   receive_buffer_.begin() + bytes_transferred);
        
        // Call packet handler with send capability
        if (packet_handler_) {
            // Create send function for the handler
            auto send_func = [this](const std::vector<u8>& response_data, const udp::endpoint& target) {
                send_packet(response_data, target);
            };
            
            packet_handler_(packet_data, sender_endpoint_, send_func);
        } else {
            LOG_INFO_F("Received {} bytes from {}:{} (no handler)", 
                      bytes_transferred,
                      sender_endpoint_.address().to_string(),
                      sender_endpoint_.port());
        }
    } else if (error) {
        LOG_ERROR_F("Receive error: {}", error.message());
    }
    
    // Continue receiving if still running
    if (running_) {
        start_receive();
    }
}

} // namespace swganh