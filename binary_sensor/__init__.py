import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_LAMBDA,
    CONF_TYPE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
)
from .. import (
    bsb_ns,
    BSBComponent,
    BSBQueryCallackArgs,
    CONF_BSB_ID, 
    CONF_BSB_TYPE,
    CONF_BSB_CMD,
    CONF_BSB_DATA,
    BSB_QUERY_BASE_SCHEMA,
)

DEPENDENCIES = ["bsb"]

BSBBinarySensor = bsb_ns.class_(
    "BSBBinarySensor",
    binary_sensor.BinarySensor,
    cg.Component,
    cg.Parented.template(BSBComponent),
)

BSBBinarySensorLambda = bsb_ns.class_(
    "BSBBinarySensorLambda",
    BSBBinarySensor,
)

BSBBinarySensorSimple = bsb_ns.class_(
    "BSBBinarySensorSimple",
    BSBBinarySensor,
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        "lambda": binary_sensor.binary_sensor_schema()
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBBinarySensorLambda),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
                cv.Required(CONF_LAMBDA): cv.returning_lambda,
            }
        ),
        "simple": binary_sensor.binary_sensor_schema()
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBBinarySensorSimple),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
            }
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BSB_ID])
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_parented(var, parent)
    await cg.register_component(var, config)

    cg.add(var.setType(config[CONF_BSB_TYPE]))
    cg.add(var.setCmd(config[CONF_BSB_CMD]))
    if CONF_BSB_DATA in config:
        cg.add(var.setData(config[CONF_BSB_DATA]))

    if config[CONF_TYPE] == "lambda":
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(BSBQueryCallackArgs, "x")], return_type=cg.float_
        )
        cg.add(var.setLambda(lambda_))
