// src/core/network/soe_protocol.hpp
#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>
#include <unordered_map>

namespace swganh::network {

// SOE Protocol constants
constexpr uint16_t SOE_CRC_SEED = 0xDEAD;
constexpr uint16_t SOE_MAX_PACKET_SIZE = 496;
constexpr uint16_t SOE_SESSION_REQUEST = 0x01;
constexpr uint16_t SOE_SESSION_RESPONSE = 0x02;
constexpr uint16_t SOE_MULTI_PACKET = 0x03;
constexpr uint16_t SOE_DISCONNECT = 0x05;
constexpr uint16_t SOE_PING = 0x06;
constexpr uint16_t SOE_NET_STATUS_REQUEST = 0x07;
constexpr uint16_t SOE_NET_STATUS_RESPONSE = 0x08;
constexpr uint16_t SOE_DATA_CHANNEL_A = 0x09;
constexpr uint16_t SOE_DATA_CHANNEL_B = 0x0A;
constexpr uint16_t SOE_DATA_CHANNEL_C = 0x0B;
constexpr uint16_t SOE_DATA_CHANNEL_D = 0x0C;
constexpr uint16_t SOE_DATA_FRAG_A = 0x0D;
constexpr uint16_t SOE_DATA_FRAG_B = 0x0E;
constexpr uint16_t SOE_DATA_FRAG_C = 0x0F;
constexpr uint16_t SOE_DATA_FRAG_D = 0x10;
constexpr uint16_t SOE_ACK_A = 0x11;
constexpr uint16_t SOE_ACK_B = 0x12;
constexpr uint16_t SOE_ACK_C = 0x13;
constexpr uint16_t SOE_ACK_D = 0x14;
constexpr uint16_t SOE_OUT_OF_ORDER_A = 0x15;
constexpr uint16_t SOE_OUT_OF_ORDER_B = 0x16;
constexpr uint16_t SOE_OUT_OF_ORDER_C = 0x17;
constexpr uint16_t SOE_OUT_OF_ORDER_D = 0x18;

// SOE Packet structure
struct SOEHeader {
    uint16_t opcode;
    uint16_t sequence{0};
    uint16_t update_type{0};
    
    explicit SOEHeader(uint16_t op) : opcode(op) {}
};

class SOEPacket {
public:
    SOEPacket() = default;
    explicit SOEPacket(uint16_t opcode);
    explicit SOEPacket(const std::vector<uint8_t>& data);
    
    // Packet building
    void WriteUInt8(uint8_t value);
    void WriteUInt16(uint16_t value);
    void WriteUInt32(uint32_t value);
    void WriteUInt64(uint64_t value);
    void WriteString(const std::string& value);
    void WriteData(const std::vector<uint8_t>& data);
    void WriteData(const uint8_t* data, size_t length);
    
    // Packet reading
    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    std::string ReadString();
    std::vector<uint8_t> ReadData(size_t length);
    
    // Packet management
    void Reset();
    size_t Size() const { return data_.size(); }
    bool Empty() const { return data_.empty(); }
    const std::vector<uint8_t>& GetData() const { return data_; }
    std::vector<uint8_t>& GetData() { return data_; }
    
    // SOE specific
    void SetOpcode(uint16_t opcode);
    uint16_t GetOpcode() const;
    void SetSequence(uint16_t sequence);
    uint16_t GetSequence() const;
    
    // CRC calculation
    uint16_t CalculateCRC() const;
    void AppendCRC();
    bool ValidateCRC() const;
    
private:
    std::vector<uint8_t> data_;
    size_t read_position_{0};
    
    void EnsureSpace(size_t bytes);
};

// Client session state
enum class SessionState {
    Disconnected,
    Connecting,
    CRCHandshake,
    SessionHandshake,
    Connected,
    Disconnecting
};

struct ClientSession {
    uint32_t session_id{0};
    uint32_t crc_seed{SOE_CRC_SEED};
    uint32_t connection_id{0};
    SessionState state{SessionState::Disconnected};
    
    // Sequence tracking
    uint16_t server_sequence{0};
    uint16_t client_sequence{0};
    uint16_t last_acknowledged{0};
    
    // Timing
    std::chrono::steady_clock::time_point last_activity;
    std::chrono::steady_clock::time_point connect_time;
    
    // Network info
    std::string remote_address;
    uint16_t remote_port{0};
    
    // Buffers
    std::vector<SOEPacket> outbound_queue;
    std::unordered_map<uint16_t, SOEPacket> pending_acks;
    
    ClientSession() {
        last_activity = std::chrono::steady_clock::now();
        connect_time = last_activity;
    }
};

class SOEProtocolHandler {
public:
    SOEProtocolHandler() = default;
    virtual ~SOEProtocolHandler() = default;
    
    // Core protocol handling
    virtual bool ProcessIncomingPacket(const std::vector<uint8_t>& data, 
                                     const std::string& remote_addr, 
                                     uint16_t remote_port) = 0;
    
    virtual std::vector<uint8_t> CreateSessionResponse(uint32_t connection_id, 
                                                      uint32_t crc_seed) = 0;
    
    virtual std::vector<uint8_t> CreatePingResponse() = 0;
    
    virtual std::vector<uint8_t> CreateDisconnectPacket(uint32_t session_id, 
                                                       uint16_t reason) = 0;
    
    // Session management
    virtual void CreateSession(const std::string& remote_addr, 
                              uint16_t remote_port) = 0;
    
    virtual void DestroySession(uint32_t session_id) = 0;
    
    virtual ClientSession* GetSession(uint32_t session_id) = 0;
    
    virtual ClientSession* GetSessionByEndpoint(const std::string& remote_addr, 
                                               uint16_t remote_port) = 0;
    
    // Utilities
    static uint16_t CalculateChecksum(const uint8_t* data, size_t length, uint32_t seed);
    static bool ValidatePacket(const std::vector<uint8_t>& data);
    static uint16_t GetPacketOpcode(const std::vector<uint8_t>& data);
};

// Concrete implementation
class BasicSOEHandler : public SOEProtocolHandler {
public:
    BasicSOEHandler();
    ~BasicSOEHandler() override;
    
    // SOEProtocolHandler implementation
    bool ProcessIncomingPacket(const std::vector<uint8_t>& data, 
                              const std::string& remote_addr, 
                              uint16_t remote_port) override;
    
    std::vector<uint8_t> CreateSessionResponse(uint32_t connection_id, 
                                              uint32_t crc_seed) override;
    
    std::vector<uint8_t> CreatePingResponse() override;
    
    std::vector<uint8_t> CreateDisconnectPacket(uint32_t session_id, 
                                               uint16_t reason) override;
    
    void CreateSession(const std::string& remote_addr, uint16_t remote_port) override;
    void DestroySession(uint32_t session_id) override;
    ClientSession* GetSession(uint32_t session_id) override;
    ClientSession* GetSessionByEndpoint(const std::string& remote_addr, 
                                       uint16_t remote_port) override;
    
    // Additional functionality
    void Update(); // Call regularly to handle timeouts, cleanup, etc.
    void SetPacketHandler(std::function<void(ClientSession*, const SOEPacket&)> handler);
    
private:
    std::unordered_map<uint32_t, std::unique_ptr<ClientSession>> sessions_;
    std::unordered_map<std::string, uint32_t> endpoint_to_session_;
    uint32_t next_session_id_{1};
    
    std::function<void(ClientSession*, const SOEPacket&)> packet_handler_;
    
    // Internal handlers
    void HandleSessionRequest(const SOEPacket& packet, 
                             const std::string& remote_addr, 
                             uint16_t remote_port);
    
    void HandleDataPacket(ClientSession* session, const SOEPacket& packet);
    void HandlePing(ClientSession* session, const SOEPacket& packet);
    void HandleAck(ClientSession* session, const SOEPacket& packet);
    void HandleDisconnect(ClientSession* session, const SOEPacket& packet);
    
    void CleanupTimedOutSessions();
    std::string MakeEndpointKey(const std::string& addr, uint16_t port);
};

} // namespace swganh::network
