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

void BSBSensorLambda::setLambda(std::function<float(BSBQueryCallackArgs)> callback) {
    m_callback = callback;
}

void BSBSensorLambda::update() {
    parent_->sendQuery(m_type, m_cmd, m_data, [this](BSBQueryCallackArgs args) {
        if (args.error == BSBQueryCallackArgs::ERR_OK) {
            auto res = m_callback(args);
            publish_state(res);
        }
    });
}

void BSBSensorTemp::update() {
    parent_->sendQuery(m_type, m_cmd, m_data, [this](BSBQueryCallackArgs args) {
        if (args.error == BSBQueryCallackArgs::ERR_OK && args.reply.data.size() == 3) {
            auto res = (args.reply.data[1] << 8 | args.reply.data[2]) / 64.0f;
            publish_state(res);
        } else {
            ESP_LOGW(TAG, "Unable to decode temp: %s", format_hex_pretty(args.reply.data).c_str());
        }
    });
}

}
}
