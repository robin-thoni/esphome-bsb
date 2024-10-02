#include "bsbbinarysensor.h"

static const char *const TAG = "BSB_BINARY_SENSOR";

namespace esphome {
namespace bsb {

void BSBBinarySensor::setType(uint8_t type) {
    m_type = type;
}

void BSBBinarySensor::setCmd(uint32_t cmd) {
    m_cmd = cmd;
}

void BSBBinarySensor::setData(const std::vector<uint8_t>& data) {
    m_data = data;
}

void BSBBinarySensor::update() {
    parent_->sendQuery(m_type, m_cmd, m_data, [this](BSBQueryCallackArgs args) {
        if (args.error == BSBQueryCallackArgs::ERR_OK) {
            bool result = false;
            if (parseReply(args, &result)) {
                publish_state(result);
            }
        }
    });
}

bool BSBBinarySensor::parseReply(BSBQueryCallackArgs args, bool* result) {
    return false;
}

void BSBBinarySensorLambda::setLambda(std::function<bool(BSBQueryCallackArgs)> callback) {
    m_callback = callback;
}

bool BSBBinarySensorLambda::parseReply(BSBQueryCallackArgs args, bool* result) {
    *result = m_callback(args);
    return true;
}

bool BSBBinarySensorSimple::parseReply(BSBQueryCallackArgs args, bool* result) {
    if (args.reply.data.size() == 2) {
        *result = args.reply.data[1];
        return true;
    } else {
        ESP_LOGW(TAG, "Unable to decode boolean: %s", format_hex_pretty(args.reply.data).c_str());
        return false;
    }
}

}
}
