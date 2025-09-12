// File: src/network/udp_server.hpp
#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <thread>
#include <memory>
#include "../core/types.hpp"

namespace swganh {

using boost::asio::ip::udp;

// Packet handler type that can send responses
using PacketHandler = std::function<void(const std::vector<u8>&, const udp::endpoint&, 
                                       std::function<void(const std::vector<u8>&, const udp::endpoint&)>)>;

class UdpServer {
public:
    explicit UdpServer(u16 port);
    ~UdpServer();
    
    void set_packet_handler(PacketHandler handler);
    void start();
    void stop();
    
    // Send packet to specific endpoint
    void send_packet(const std::vector<u8>& data, const udp::endpoint& target);
    
    bool is_running() const { return running_; }

private:
    void start_receive();
    void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
    
    u16 port_;
    bool running_;
    
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::unique_ptr<udp::socket> socket_;
    std::thread io_thread_;
    
    std::array<u8, 1024> receive_buffer_;
    udp::endpoint sender_endpoint_;
    
    PacketHandler packet_handler_;
};

} // namespace swganh