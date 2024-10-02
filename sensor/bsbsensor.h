#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "../bsb.h"

namespace esphome {
namespace bsb {

class BSBSensor : public sensor::Sensor, public PollingComponent, public Parented<BSBComponent> {

public:
    void setType(uint8_t type);

    void setCmd(uint32_t cmd);

    void setData(const std::vector<uint8_t>& data);

    void update() override;

protected:
    uint8_t m_type;

    uint32_t m_cmd;

    std::vector<uint8_t> m_data;

    virtual bool parseReply(BSBQueryCallackArgs args, float* result);

};

class BSBSensorLambda : public BSBSensor {

public:
    void setLambda(std::function<float(BSBQueryCallackArgs)> callback);

    virtual bool parseReply(BSBQueryCallackArgs args, float* result);

protected:
    std::function<float(BSBQueryCallackArgs)> m_callback;

};

class BSBSensorTemp : public BSBSensor {

public:
    virtual bool parseReply(BSBQueryCallackArgs args, float* result);
};

}
}
