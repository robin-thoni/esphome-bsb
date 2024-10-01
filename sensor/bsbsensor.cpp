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
        auto res = m_callback(args);
        publish_state(res);
    });
}

}
}
