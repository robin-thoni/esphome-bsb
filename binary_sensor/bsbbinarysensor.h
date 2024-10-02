#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

#include "../bsb.h"

namespace esphome {
namespace bsb {

class BSBBinarySensor : public binary_sensor::BinarySensor, public PollingComponent, public Parented<BSBComponent> {

public:
    void setType(uint8_t type);

    void setCmd(uint32_t cmd);

    void setData(const std::vector<uint8_t>& data);

    void update() override;

protected:
    uint8_t m_type;

    uint32_t m_cmd;

    std::vector<uint8_t> m_data;

    virtual bool parseReply(BSBQueryCallackArgs args, bool* result);

};

class BSBBinarySensorLambda : public BSBBinarySensor {

public:
    void setLambda(std::function<bool(BSBQueryCallackArgs)> callback);

    virtual bool parseReply(BSBQueryCallackArgs args, bool* result);

protected:
    std::function<bool(BSBQueryCallackArgs)> m_callback;

};

class BSBBinarySensorSimple : public BSBBinarySensor {

public:
    virtual bool parseReply(BSBQueryCallackArgs args, bool* result);

};

}
}
