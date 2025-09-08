// tests/test_soe_protocol.cpp
#include "../src/core/network/soe_protocol.hpp"
#include <iostream>
#include <cassert>
#include <vector>

using namespace swganh::network;

void TestSOEPacketBasics() {
    std::cout << "Testing SOE packet basics..." << std::endl;
    
    // Test packet creation
    SOEPacket packet(SOE_SESSION_REQUEST);
    assert(packet.GetOpcode() == SOE_SESSION_REQUEST);
    assert(!packet.Empty());
    
    // Test writing data
    packet.WriteUInt32(12345);
    packet.WriteString("test");
    packet.WriteUInt8(255);
    
    // Test reading data back
    SOEPacket read_packet(packet.GetData());
    read_packet.ReadUInt16(); // Skip opcode
    uint32_t value = read_packet.ReadUInt32();
    assert(value == 12345);
    
    std::string str = read_packet.ReadString();
    assert(str == "test");
    
    uint8_t byte_val = read_packet.ReadUInt8();
    assert(byte_val == 255);
    
    std::cout << "✓ SOE packet basics: PASSED" << std::endl;
}

void TestSOEPacketSequencing() {
    std::cout << "Testing SOE packet sequencing..." << std::endl;
    
    SOEPacket packet(SOE_DATA_CHANNEL_A);
    
    // Test sequence number setting/getting
    packet.SetSequence(1337);
    assert(packet.GetSequence() == 1337);
    
    packet.SetSequence(65535); // Max uint16
    assert(packet.GetSequence() == 65535);
    
    std::cout << "✓ SOE packet sequencing: PASSED" << std::endl;
}

void TestCRCCalculation() {
    std::cout << "Testing CRC calculation..." << std::endl;
    
    const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t crc1 = SOEProtocolHandler::CalculateChecksum(test_data, 4, SOE_CRC_SEED);
    
    // CRC should be deterministic
    uint16_t crc2 = SOEProtocolHandler::CalculateChecksum(test_data, 4, SOE_CRC_SEED);
    assert(crc1 == crc2);
    
    // Different data should produce different CRC
    const uint8_t test_data2[] = {0x05, 0x06, 0x07, 0x08};
    uint16_t crc3 = SOEProtocolHandler::CalculateChecksum(test_data2, 4, SOE_CRC_SEED);
    assert(crc1 != crc3);
    
    // Test packet CRC validation
    SOEPacket packet(SOE_PING);
    packet.WriteUInt32(0x12345678);
    packet.AppendCRC();
    
    assert(packet.ValidateCRC());
    
    std::cout << "✓ CRC calculation: PASSED" << std::endl;
}

void TestBasicSOEHandler() {
    std::cout << "Testing BasicSOEHandler..." << std::endl;
    
    BasicSOEHandler handler;
    
    // Test session creation
    handler.CreateSession("127.0.0.1", 12345);
    
    auto* session = handler.GetSessionByEndpoint("127.0.0.1", 12345);
    assert(session != nullptr);
    assert(session->remote_address == "127.0.0.1");
    assert(session->remote_port == 12345);
    assert(session->state == SessionState::Connecting);
    
    // Test session lookup by ID
    uint32_t session_id = session->session_id;
    auto* session_by_id = handler.GetSession(session_id);
    assert(session_by_id == session);
    
    // Test multiple sessions
    handler.CreateSession("192.168.1.1", 54321);
    auto* session2 = handler.GetSessionByEndpoint("192.168.1.1", 54321);
    assert(session2 != nullptr);
    assert(session2 != session);
    assert(session2->session_id != session_id);
    
    // Test session cleanup
    handler.DestroySession(session_id);
    assert(handler.GetSession(session_id) == nullptr);
    assert(handler.GetSessionByEndpoint("127.0.0.1", 12345) == nullptr);
    
    // Second session should still exist
    assert(handler.GetSessionByEndpoint("192.168.1.1", 54321) != nullptr);
    
    std::cout << "✓ BasicSOEHandler: PASSED" << std::endl;
}

void TestPacketOpcodes() {
    std::cout << "Testing packet opcode handling..." << std::endl;
    
    // Test all major SOE opcodes
    std::vector<uint16_t> opcodes = {
        SOE_SESSION_REQUEST,
        SOE_SESSION_RESPONSE,
        SOE_MULTI_PACKET,
        SOE_DISCONNECT,
        SOE_PING,
        SOE_NET_STATUS_REQUEST,
        SOE_NET_STATUS_RESPONSE,
        SOE_DATA_CHANNEL_A,
        SOE_DATA_CHANNEL_B,
        SOE_DATA_CHANNEL_C,
        SOE_DATA_CHANNEL_D,
        SOE_ACK_A,
        SOE_ACK_B,
        SOE_ACK_C,
        SOE_ACK_D
    };
    
    for (uint16_t opcode : opcodes) {
        SOEPacket packet(opcode);
        assert(packet.GetOpcode() == opcode);
        
        // Test that opcode is correctly stored in data
        auto data = packet.GetData();
        assert(data.size() >= 2);
        uint16_t stored_opcode = data[0] | (data[1] << 8);
        assert(stored_opcode == opcode);
    }
    
    std::cout << "✓ Packet opcodes: PASSED" << std::endl;
}

void TestSessionStates() {
    std::cout << "Testing session state management..." << std::endl;
    
    BasicSOEHandler handler;
    handler.CreateSession("10.0.0.1", 9999);
    
    auto* session = handler.GetSessionByEndpoint("10.0.0.1", 9999);
    assert(session != nullptr);
    
    // Test initial state
    assert(session->state == SessionState::Disconnected || 
           session->state == SessionState::Connecting);
    
    // Test state transitions
    session->state = SessionState::Connected;
    assert(session->state == SessionState::Connected);
    
    session->state = SessionState::Disconnecting;
    assert(session->state == SessionState::Disconnecting);
    
    std::cout << "✓ Session states: PASSED" << std::endl;
}

void TestPacketUtilities() {
    std::cout << "Testing packet utilities..." << std::endl;
    
    // Test packet validation
    std::vector<uint8_t> valid_packet = {0x01, 0x00, 0x00, 0x00}; // Minimal valid packet
    assert(SOEProtocolHandler::ValidatePacket(valid_packet));
    
    std::vector<uint8_t> invalid_packet = {0x01}; // Too small
    assert(!SOEProtocolHandler::ValidatePacket(invalid_packet));
    
    // Test opcode extraction
    std::vector<uint8_t> test_packet = {0x05, 0x00, 0x12, 0x34}; // Opcode 0x0005
    uint16_t opcode = SOEProtocolHandler::GetPacketOpcode(test_packet);
    assert(opcode == 0x0005);
    
    std::cout << "✓ Packet utilities: PASSED" << std::endl;
}

int main() {
    std::cout << "=== Running SOE Protocol Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        TestSOEPacketBasics();
        TestSOEPacketSequencing();
        TestCRCCalculation();
        TestBasicSOEHandler();
        TestPacketOpcodes();
        TestSessionStates();
        TestPacketUtilities();
        
        std::cout << std::endl;
        std::cout << "🎉 All tests PASSED!" << std::endl;
        std::cout << "SOE Protocol implementation is working correctly." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << "❌ Test FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << std::endl;
        std::cout << "❌ Test FAILED: Unknown exception" << std::endl;
        return 1;
    }
}
