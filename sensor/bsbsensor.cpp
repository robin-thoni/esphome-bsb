#include "bsbsensor.h"

static const char *const TAG = "BSB_SENSOR";

namespace esphome {
namespace bsb {

void BSBSensor::setType(uint8_t type) {
    m_type = type;
}

void BSBSensor::setCmd(uint32_t cmd) {
    m_cmd = cmd;
}

void BSBSensor::setData(const std::vector<uint8_t>& data) {
    m_data = data;
}

void BSBSensor::update() {
    parent_->sendQuery(m_type, m_cmd, m_data, [this](BSBQueryCallackArgs args) {
        if (args.error == BSBQueryCallackArgs::ERR_OK) {
            float result = NAN;
            if (parseReply(args, &result)) {
                publish_state(result);
            }
        }
    });
}

bool BSBSensor::parseReply(BSBQueryCallackArgs args, float* result) {
    return false;
}

void BSBSensorLambda::setLambda(std::function<float(BSBQueryCallackArgs)> callback) {
    m_callback = callback;
}

bool BSBSensorLambda::parseReply(BSBQueryCallackArgs args, float* result) {
    *result = m_callback(args);
    return true;
}

bool BSBSensorTemp::parseReply(BSBQueryCallackArgs args, float* result) {
    if (args.reply.data.size() == 3) {
        *result = (args.reply.data[1] << 8 | args.reply.data[2]) / 64.0f;
        return true;
    } else {
        ESP_LOGW(TAG, "Unable to decode temp: %s", format_hex_pretty(args.reply.data).c_str());
        return false;
    }
}

bool BSBSensorEnum::parseReply(BSBQueryCallackArgs args, float* result) {
    if (args.reply.data.size() == 3) {
        *result = args.reply.data[1] << 8 | args.reply.data[2];
        return true;
    } else {
        ESP_LOGW(TAG, "Unable to decode enum: %s", format_hex_pretty(args.reply.data).c_str());
        return false;
    }
}

bool BSBSensorPercent::parseReply(BSBQueryCallackArgs args, float* result) {
    if (args.reply.data.size() == 2) {
        *result = args.reply.data[1];
        return true;
    } else {
        ESP_LOGW(TAG, "Unable to decode percent: %s", format_hex_pretty(args.reply.data).c_str());
        return false;
    }
}

}
}
