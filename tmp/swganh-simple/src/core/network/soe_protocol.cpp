// src/core/network/soe_protocol.cpp
#include "soe_protocol.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <chrono>

namespace swganh::network {

// SOEPacket Implementation
SOEPacket::SOEPacket(uint16_t opcode) {
    WriteUInt16(opcode);
}

SOEPacket::SOEPacket(const std::vector<uint8_t>& data) : data_(data) {}

void SOEPacket::WriteUInt8(uint8_t value) {
    data_.push_back(value);
}

void SOEPacket::WriteUInt16(uint16_t value) {
    EnsureSpace(2);
    data_.push_back(static_cast<uint8_t>(value & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void SOEPacket::WriteUInt32(uint32_t value) {
    EnsureSpace(4);
    data_.push_back(static_cast<uint8_t>(value & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void SOEPacket::WriteUInt64(uint64_t value) {
    EnsureSpace(8);
    for (int i = 0; i < 8; ++i) {
        data_.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

void SOEPacket::WriteString(const std::string& value) {
    WriteUInt16(static_cast<uint16_t>(value.length()));
    WriteData(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
}

void SOEPacket::WriteData(const std::vector<uint8_t>& data) {
    data_.insert(data_.end(), data.begin(), data.end());
}

void SOEPacket::WriteData(const uint8_t* data, size_t length) {
    data_.insert(data_.end(), data, data + length);
}

uint8_t SOEPacket::ReadUInt8() {
    if (read_position_ >= data_.size()) return 0;
    return data_[read_position_++];
}

uint16_t SOEPacket::ReadUInt16() {
    if (read_position_ + 1 >= data_.size()) return 0;
    uint16_t value = data_[read_position_] | (data_[read_position_ + 1] << 8);
    read_position_ += 2;
    return value;
}

uint32_t SOEPacket::ReadUInt32() {
    if (read_position_ + 3 >= data_.size()) return 0;
    uint32_t value = data_[read_position_] | 
                    (data_[read_position_ + 1] << 8) |
                    (data_[read_position_ + 2] << 16) |
                    (data_[read_position_ + 3] << 24);
    read_position_ += 4;
    return value;
}

uint64_t SOEPacket::ReadUInt64() {
    if (read_position_ + 7 >= data_.size()) return 0;
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data_[read_position_ + i]) << (i * 8);
    }
    read_position_ += 8;
    return value;
}

std::string SOEPacket::ReadString() {
    uint16_t length = ReadUInt16();
    if (read_position_ + length > data_.size()) return "";
    
    std::string result(reinterpret_cast<const char*>(&data_[read_position_]), length);
    read_position_ += length;
    return result;
}

std::vector<uint8_t> SOEPacket::ReadData(size_t length) {
    if (read_position_ + length > data_.size()) return {};
    
    std::vector<uint8_t> result(data_.begin() + read_position_, 
                               data_.begin() + read_position_ + length);
    read_position_ += length;
    return result;
}

void SOEPacket::Reset() {
    data_.clear();
    read_position_ = 0;
}

void SOEPacket::SetOpcode(uint16_t opcode) {
    if (data_.size() < 2) {
        data_.resize(2);
    }
    data_[0] = static_cast<uint8_t>(opcode & 0xFF);
    data_[1] = static_cast<uint8_t>((opcode >> 8) & 0xFF);
}

uint16_t SOEPacket::GetOpcode() const {
    if (data_.size() < 2) return 0;
    return data_[0] | (data_[1] << 8);
}

void SOEPacket::SetSequence(uint16_t sequence) {
    if (data_.size() < 4) {
        data_.resize(4);
    }
    data_[2] = static_cast<uint8_t>(sequence & 0xFF);
    data_[3] = static_cast<uint8_t>((sequence >> 8) & 0xFF);
}

uint16_t SOEPacket::GetSequence() const {
    if (data_.size() < 4) return 0;
    return data_[2] | (data_[3] << 8);
}

uint16_t SOEPacket::CalculateCRC() const {
    return SOEProtocolHandler::CalculateChecksum(data_.data(), data_.size(), SOE_CRC_SEED);
}

void SOEPacket::AppendCRC() {
    uint16_t crc = CalculateCRC();
    WriteUInt16(crc);
}

bool SOEPacket::ValidateCRC() const {
    if (data_.size() < 2) return false;
    
    // Get stored CRC (last 2 bytes)
    uint16_t stored_crc = data_[data_.size() - 2] | (data_[data_.size() - 1] << 8);
    
    // Calculate CRC of packet without the CRC bytes
    uint16_t calculated_crc = SOEProtocolHandler::CalculateChecksum(
        data_.data(), data_.size() - 2, SOE_CRC_SEED);
    
    return stored_crc == calculated_crc;
}

void SOEPacket::EnsureSpace(size_t bytes) {
    if (data_.capacity() < data_.size() + bytes) {
        data_.reserve(std::max(data_.capacity() * 2, data_.size() + bytes));
    }
}

// BasicSOEHandler Implementation
BasicSOEHandler::BasicSOEHandler() {
    std::cout << "[SOE] BasicSOEHandler initialized" << std::endl;
}

BasicSOEHandler::~BasicSOEHandler() {
    std::cout << "[SOE] BasicSOEHandler destroyed" << std::endl;
}

bool BasicSOEHandler::ProcessIncomingPacket(const std::vector<uint8_t>& data, 
                                          const std::string& remote_addr, 
                                          uint16_t remote_port) {
    if (data.size() < 2) {
        std::cout << "[SOE] Packet too small: " << data.size() << " bytes" << std::endl;
        return false;
    }
    
    SOEPacket packet(data);
    uint16_t opcode = packet.GetOpcode();
    
    std::cout << "[SOE] Processing packet - Opcode: 0x" << std::hex << opcode 
              << " from " << remote_addr << ":" << remote_port << std::endl;
    
    switch (opcode) {
        case SOE_SESSION_REQUEST:
            HandleSessionRequest(packet, remote_addr, remote_port);
            break;
            
        case SOE_PING:
            if (auto* session = GetSessionByEndpoint(remote_addr, remote_port)) {
                HandlePing(session, packet);
            }
            break;
            
        case SOE_DATA_CHANNEL_A:
        case SOE_DATA_CHANNEL_B:
        case SOE_DATA_CHANNEL_C:
        case SOE_DATA_CHANNEL_D:
            if (auto* session = GetSessionByEndpoint(remote_addr, remote_port)) {
                HandleDataPacket(session, packet);
            }
            break;
            
        case SOE_ACK_A:
        case SOE_ACK_B:
        case SOE_ACK_C:
        case SOE_ACK_D:
            if (auto* session = GetSessionByEndpoint(remote_addr, remote_port)) {
                HandleAck(session, packet);
            }
            break;
            
        case SOE_DISCONNECT:
            if (auto* session = GetSessionByEndpoint(remote_addr, remote_port)) {
                HandleDisconnect(session, packet);
            }
            break;
            
        default:
            std::cout << "[SOE] Unknown opcode: 0x" << std::hex << opcode << std::endl;
            return false;
    }
    
    return true;
}

std::vector<uint8_t> BasicSOEHandler::CreateSessionResponse(uint32_t connection_id, 
                                                          uint32_t crc_seed) {
    SOEPacket response(SOE_SESSION_RESPONSE);
    response.WriteUInt32(connection_id);
    response.WriteUInt32(crc_seed);
    response.WriteUInt8(2); // Unknown field
    response.WriteUInt8(1); // Unknown field
    response.WriteUInt8(4); // Unknown field
    response.WriteUInt32(SOE_MAX_PACKET_SIZE); // Max packet size
    
    return response.GetData();
}

std::vector<uint8_t> BasicSOEHandler::CreatePingResponse() {
    SOEPacket response(SOE_PING);
    return response.GetData();
}

std::vector<uint8_t> BasicSOEHandler::CreateDisconnectPacket(uint32_t session_id, 
                                                           uint16_t reason) {
    SOEPacket packet(SOE_DISCONNECT);
    packet.WriteUInt32(session_id);
    packet.WriteUInt16(reason);
    return packet.GetData();
}

void BasicSOEHandler::CreateSession(const std::string& remote_addr, uint16_t remote_port) {
    auto session = std::make_unique<ClientSession>();
    session->session_id = next_session_id_++;
    session->remote_address = remote_addr;
    session->remote_port = remote_port;
    session->state = SessionState::Connecting;
    session->connection_id = session->session_id; // Simple mapping for now
    
    std::string endpoint_key = MakeEndpointKey(remote_addr, remote_port);
    
    sessions_[session->session_id] = std::move(session);
    endpoint_to_session_[endpoint_key] = session->session_id;
    
    std::cout << "[SOE] Created session " << session->session_id 
              << " for " << remote_addr << ":" << remote_port << std::endl;
}

void BasicSOEHandler::DestroySession(uint32_t session_id) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        std::string endpoint_key = MakeEndpointKey(it->second->remote_address, 
                                                  it->second->remote_port);
        endpoint_to_session_.erase(endpoint_key);
        sessions_.erase(it);
        
        std::cout << "[SOE] Destroyed session " << session_id << std::endl;
    }
}

ClientSession* BasicSOEHandler::GetSession(uint32_t session_id) {
    auto it = sessions_.find(session_id);
    return it != sessions_.end() ? it->second.get() : nullptr;
}

ClientSession* BasicSOEHandler::GetSessionByEndpoint(const std::string& remote_addr, 
                                                    uint16_t remote_port) {
    std::string endpoint_key = MakeEndpointKey(remote_addr, remote_port);
    auto it = endpoint_to_session_.find(endpoint_key);
    if (it != endpoint_to_session_.end()) {
        return GetSession(it->second);
    }
    return nullptr;
}

void BasicSOEHandler::Update() {
    CleanupTimedOutSessions();
}

void BasicSOEHandler::SetPacketHandler(std::function<void(ClientSession*, const SOEPacket&)> handler) {
    packet_handler_ = std::move(handler);
}

void BasicSOEHandler::HandleSessionRequest(const SOEPacket& packet, 
                                         const std::string& remote_addr, 
                                         uint16_t remote_port) {
    std::cout << "[SOE] Session request from " << remote_addr << ":" << remote_port << std::endl;
    
    // Create new session
    CreateSession(remote_addr, remote_port);
    
    auto* session = GetSessionByEndpoint(remote_addr, remote_port);
    if (!session) {
        std::cout << "[SOE] Failed to create session!" << std::endl;
        return;
    }
    
    session->state = SessionState::Connected;
    
    // For now, we'll just mark as connected
    // TODO: Implement proper handshake sequence
    std::cout << "[SOE] Session " << session->session_id << " established" << std::endl;
}

void BasicSOEHandler::HandleDataPacket(ClientSession* session, const SOEPacket& packet) {
    session->last_activity = std::chrono::steady_clock::now();
    
    // Extract sequence number
    uint16_t sequence = packet.GetSequence();
    session->client_sequence = sequence;
    
    std::cout << "[SOE] Data packet from session " << session->session_id 
              << " sequence " << sequence << std::endl;
    
    // Forward to game packet handler if available
    if (packet_handler_) {
        packet_handler_(session, packet);
    }
}

void BasicSOEHandler::HandlePing(ClientSession* session, const SOEPacket& packet) {
    session->last_activity = std::chrono::steady_clock::now();
    std::cout << "[SOE] Ping from session " << session->session_id << std::endl;
    
    // Respond with pong (same as ping for SOE)
    // TODO: Send ping response back to client
}

void BasicSOEHandler::HandleAck(ClientSession* session, const SOEPacket& packet) {
    session->last_activity = std::chrono::steady_clock::now();
    uint16_t ack_sequence = packet.GetSequence();
    session->last_acknowledged = ack_sequence;
    
    std::cout << "[SOE] ACK from session " << session->session_id 
              << " for sequence " << ack_sequence << std::endl;
}

void BasicSOEHandler::HandleDisconnect(ClientSession* session, const SOEPacket& packet) {
    std::cout << "[SOE] Disconnect from session " << session->session_id << std::endl;
    session->state = SessionState::Disconnecting;
    // Session will be cleaned up on next Update() call
}

void BasicSOEHandler::CleanupTimedOutSessions() {
    auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::minutes(5); // 5 minute timeout
    
    std::vector<uint32_t> to_remove;
    
    for (const auto& [session_id, session] : sessions_) {
        if (session->state == SessionState::Disconnecting ||
            (now - session->last_activity) > timeout) {
            to_remove.push_back(session_id);
        }
    }
    
    for (uint32_t session_id : to_remove) {
        DestroySession(session_id);
    }
}

std::string BasicSOEHandler::MakeEndpointKey(const std::string& addr, uint16_t port) {
    return addr + ":" + std::to_string(port);
}

// Static utility functions
uint16_t SOEProtocolHandler::CalculateChecksum(const uint8_t* data, size_t length, uint32_t seed) {
    // Simple CRC-16 implementation for SOE protocol
    uint16_t crc = static_cast<uint16_t>(seed);
    
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x8408; // CRC-16-CCITT polynomial
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

bool SOEProtocolHandler::ValidatePacket(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return false; // Minimum packet size
    
    SOEPacket packet(data);
    return packet.ValidateCRC();
}

uint16_t SOEProtocolHandler::GetPacketOpcode(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return 0;
    return data[0] | (data[1] << 8);
}

} // namespace swganh::network
