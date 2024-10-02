#include "bsb.h"
#include "esphome/components/uart/uart_debugger.h"
#include "Arduino.h"

static const char *const TAG = "BSB";

namespace esphome {
namespace bsb {

uint16_t _crc_xmodem_update (uint16_t crc, uint8_t data) {
    crc = crc ^ ((uint16_t)data << 8);
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint16_t _push_back_with_crc(std::vector<uint8_t>& data, uint16_t crc, uint8_t b) {
    data.push_back(b);
    return _crc_xmodem_update(crc, b);
}

void BSBPacket::dump() const {
    ESP_LOGI(TAG, "BSBPacket dump:");
    ESP_LOGI(TAG, "  Src Address: 0x%02X", src_addr);
    ESP_LOGI(TAG, "  Dst Address: 0x%02X", dst_addr);
    ESP_LOGI(TAG, "  Type: 0x%02X", type);
    ESP_LOGI(TAG, "  Cmd: 0x%08X", cmd);
    std::string res;
    char buf[5];
    for (size_t i = 0; i < data.size(); i++) {
        if (i > 0) {
            res += ' ';
        }
        sprintf(buf, "%02X", data[i]);
        res += buf;
    }
    ESP_LOGI(TAG, "  Data: %s", res.c_str());
}

uint32_t BSBPacket::parseCmd(const std::vector<uint8_t>& data, bool isReply) {
    if (isReply) {
        return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    }
    return data[1] << 24 | data[0] << 16 | data[2] << 8 | data[3];
}

std::vector<uint8_t> BSBPacket::serializeCmd(uint32_t cmd) {
    uint8_t A2 = (cmd & 0xff000000) >> 24;
    uint8_t A1 = (cmd & 0x00ff0000) >> 16;
    uint8_t A3 = (cmd & 0x0000ff00) >> 8;
    uint8_t A4 = (cmd & 0x000000ff);
    return std::vector<uint8_t>({A1, A2, A3, A4});
}

bool BSBPacket::parse(const std::vector<uint8_t>& data, bool isReply) {
    if (data.size() < 11 || (data[0] != 0xDE && data[0] != 0xDC)) {
        return false;
    }

    uint16_t crc = 0;
    for (auto i = 0; i < data.size(); ++i) {
        crc = _crc_xmodem_update(crc, data[i]);
    }
    if (crc) {
        return false;
    }

    src_addr = data[1] & 0x7F;
    dst_addr = data[2];
    type = data[4];
    cmd = BSBPacket::parseCmd(std::vector<uint8_t>(data.begin() + 5, data.begin() + 9), isReply);
    this->data = std::vector<uint8_t>(data.begin() + 9, data.end() - 2);
    return true;
}

std::vector<uint8_t> BSBPacket::serialize() const {
    uint16_t crc = 0;
    std::vector<uint8_t> d;
    d.reserve(data.size() + 11);
    crc = _push_back_with_crc(d, crc, 0xDE);
    crc = _push_back_with_crc(d, crc, src_addr | 0x80);
    crc = _push_back_with_crc(d, crc, dst_addr);
    crc = _push_back_with_crc(d, crc, data.size() + 11);
    crc = _push_back_with_crc(d, crc, type);
    const auto cmd_ = serializeCmd(cmd);
    for (auto i = 0; i < cmd_.size(); ++i) {
        crc = _push_back_with_crc(d, crc, cmd_[i]);
    }
    for (auto i = 0; i < data.size(); ++i) {
        crc = _push_back_with_crc(d, crc, data[i]);
    }
    d.push_back(crc >> 8);
    d.push_back(crc & 0xFF);

    return d;
}

void BSBComponent::loop()
{
    const auto now = millis();
    for (auto it = m_outbound_packets.begin(); it != m_outbound_packets.end(); ) {
        if (now - it->start_time > 1000) {
            ESP_LOGW(TAG, "Outbound packet timeout");
            it = m_outbound_packets.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_queries.begin(); it != m_queries.end(); ) {
        if (now - it->start_time > 1000) {
            ESP_LOGW(TAG, "Query timeout");
            BSBQueryCallackArgs args;
            args.error = BSBQueryCallackArgs::ERR_TIMEOUT;
            it->callback(args);
            it = m_queries.erase(it);
        } else {
            ++it;
        }
    }

    while(available()) {
        // ESP_LOGI(TAG, "isBusFree: %i", isBusFree());
        uint8_t byte;
        if (!readByte(&byte)) {
            return;
        }

        if (m_buffer.size() == 0 && (byte == 0xDE || byte == 0xDC)) {
            m_buffer.push_back(byte);
        } else if (m_buffer.size() != 0) {
            m_buffer.push_back(byte);
            if (m_buffer.size() >= 4) {
                uint8_t length = m_buffer[3];
                if (m_buffer.size() >= length) {

                    // TODO just check if local address is us?
                    bool is_outbound_packet = false;
                    for (auto i = 0; i < m_outbound_packets.size(); ++i) {
                        if (m_outbound_packets[i].data == m_buffer) {
                            m_outbound_packets.erase(m_outbound_packets.begin() + i);
                            is_outbound_packet = true;
                            ESP_LOGVV(TAG, "Found outbound packet");
                            break;
                        }
                    }
                    if (!is_outbound_packet) {
                        #ifdef USE_UART_DEBUGGER
                        uart::UARTDebug::log_hex(uart::UARTDirection::UART_DIRECTION_RX, m_buffer, ' ');
                        #endif

                        BSBPacket packet;
                        if (packet.parse(m_buffer)) {
                            on_packet(packet);
                        } else {
                            ESP_LOGW(TAG, "Failed to parse incomming packet");
                        }
                    }
                    m_buffer.clear();
                }
            }
        }
    }
}

bool BSBComponent::sendData(std::vector<uint8_t> data) {
    if (isBusFree()) {
        if (m_outbound_packets.size() > 10) { // TODO Arbitrary number to avoid overflow
            ESP_LOGW(TAG, "Outbound packets list is full");
            return false;
        }
        #ifdef USE_UART_DEBUGGER
        uart::UARTDebug::log_hex(uart::UARTDirection::UART_DIRECTION_TX, data, ' ');
        #endif

        BSBOutboundPacket outbound_packet;
        outbound_packet.data = data;
        outbound_packet.start_time = millis();
        m_outbound_packets.push_back(outbound_packet);

        for (auto i = 0; i < data.size(); ++i) {
            data[i] ^= 0xFF;
        }
        write_array(data);
        flush();
        return true;
    } else {
        ESP_LOGW(TAG, "Bus is not free");
        return false;
    }
}

bool BSBComponent::sendPacket(const BSBPacket& packet) {
    const auto& data = packet.serialize();
    return sendData(data);
}

bool BSBComponent::sendQuery(const BSBPacket& packet, std::function<void(BSBQueryCallackArgs)> callback) {
    if (m_queries.size() > 10) { // TODO Arbitrary number to avoid overflow
        ESP_LOGW(TAG, "Queries list is full");
        return false;
    }
    if (sendPacket(packet)) {
        BSBQuery query;
        query.query = packet;
        query.callback = callback;
        query.start_time = millis();
        m_queries.push_back(query);
        return true;
    } else {
        return false;
    }
}

bool BSBComponent::sendQuery(uint8_t type, uint32_t cmd, const std::vector<uint8_t>& data, std::function<void(BSBQueryCallackArgs)> callback) {
    BSBPacket packet;
    packet.src_addr = m_src_addr;
    packet.dst_addr = m_dst_addr;
    packet.type = type;
    packet.cmd = cmd;
    packet.data = data;
    return sendQuery(packet, callback);
}

bool BSBComponent::sendQuery(uint8_t type, uint32_t cmd, std::function<void(BSBQueryCallackArgs)> callback) {
    return sendQuery(type, cmd, {}, callback);
}

bool BSBComponent::isReply(const BSBPacket& query, const BSBPacket& reply) {
    if (query.src_addr != reply.dst_addr) {
        return false;
    }
    if (query.dst_addr != reply.src_addr) {
        return false;
    }

    // TODO Make sure it works for everything
    // TODO How to handle TYPE_ERR/0x08 and TYPE_QRE/0x11
    if (query.type + 1 != reply.type) {
        return false;
    }

    if (query.cmd != reply.cmd) {
        return false;
    }

    return true;
}

void BSBComponent::setSrcAddress(uint8_t addr) {
    m_src_addr = addr;
}

void BSBComponent::setDstAddress(uint8_t addr) {
    m_dst_addr = addr;
}

void BSBComponent::on_packet(const BSBPacket& packet) {
    for (auto i = 0; i < m_queries.size(); ++i) {
        if (isReply(m_queries[i].query, packet)) {
            ESP_LOGVV(TAG, "Found reply packet");
            BSBQueryCallackArgs callbackArgs;
            callbackArgs.error = BSBQueryCallackArgs::ERR_OK;
            callbackArgs.query = m_queries[i].query;
            callbackArgs.reply = packet;
            m_queries[i].callback(callbackArgs);
            m_queries.erase(m_queries.begin() + i);
            break;
        }
    }
    #if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
    packet.dump();
    #endif
}

bool BSBComponent::readByte(uint8_t* byte) {
    if (!read_byte(byte)) {
        return false;
    }
    *byte ^= 0xFF;
    return true;
}

bool BSBComponent::isBusFree() {
    return true; // TODO for testing
    // return digitalRead(16); // TODO hardcoded for testing
}

}
}
