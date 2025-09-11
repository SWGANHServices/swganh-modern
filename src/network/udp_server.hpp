#pragma once

#include "core/types.hpp"
#include "core/logger.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <array>

namespace swganh::network {

using boost::asio::ip::udp;

class UdpServer {
public:
    using PacketHandler = std::function<void(const core::byte_vector&, const udp::endpoint&)>;
    
    explicit UdpServer(core::u16 port);
    ~UdpServer();
    
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    void set_packet_handler(PacketHandler handler);
    void send_packet(const core::byte_vector& data, const udp::endpoint& endpoint);
    
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

}
