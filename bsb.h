#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace bsb {

class BSBPacket {
public:
    uint8_t src_addr;
    uint8_t dst_addr;

    uint8_t type;
    uint32_t cmd;

    std::vector<uint8_t> data;

    void dump() const;

    static uint32_t parseCmd(const std::vector<uint8_t>& data, bool isReply = true);

    static std::vector<uint8_t> serializeCmd(uint32_t cmd);

    bool parse(const std::vector<uint8_t>& data, bool isReply = true);

    std::vector<uint8_t> serialize() const;
};

class BSBOutboundPacket {

public:
    std::vector<uint8_t> data;

    uint64_t start_time;
};

struct BSBQueryCallackArgs {

    enum Error {
        ERR_OK = 0,
        ERR_TIMEOUT = 1,
    };

    Error error;

    BSBPacket query;

    BSBPacket reply;
};

class BSBQuery {

public:
    BSBPacket query;

    uint64_t start_time;

    std::function<void(BSBQueryCallackArgs)> callback;

};

class BSBComponent : public uart::UARTDevice, public Component {

public:

    void loop();

    bool sendData(std::vector<uint8_t> data);

    bool sendPacket(const BSBPacket& packet);

    bool sendQuery(const BSBPacket& packet, std::function<void(BSBQueryCallackArgs)> callback);

    bool sendQuery(uint8_t type, uint32_t cmd, const std::vector<uint8_t>& data, std::function<void(BSBQueryCallackArgs)> callback);

    bool sendQuery(uint8_t type, uint32_t cmd, std::function<void(BSBQueryCallackArgs)> callback);

    bool isReply(const BSBPacket& query, const BSBPacket& reply);

    void setSrcAddress(uint8_t addr);

    void setDstAddress(uint8_t addr);

protected:
    std::vector<uint8_t> m_buffer;

    std::vector<BSBOutboundPacket> m_outbound_packets;

    std::vector<BSBQuery> m_queries;

    uint8_t m_src_addr;

    uint8_t m_dst_addr;

    bool readByte(uint8_t* byte);

    void on_packet(const BSBPacket& packet);

    bool isBusFree();

};
}
}
