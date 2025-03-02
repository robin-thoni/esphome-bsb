import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
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

BSBDiagSensor = sensor.Sensor

BSBSensor = bsb_ns.class_(
    "BSBSensor",
    sensor.Sensor,
    cg.Component,
    cg.Parented.template(BSBComponent),
)

BSBSensorLambda = bsb_ns.class_(
    "BSBSensorLambda",
    BSBSensor,
)

BSBSensorTemp = bsb_ns.class_(
    "BSBSensorTemp",
    BSBSensor,
)

BSBSensorEnum = bsb_ns.class_(
    "BSBSensorEnum",
    BSBSensor,
)

BSBSensorPercent = bsb_ns.class_(
    "BSBSensorPercent",
    BSBSensor,
)

diag_prefix = "diag-"
DiagType = BSBComponent.enum("DiagType")

# Keep in sync w/ DiagType in bsb.h
DIAG_MAPPING = {
    "parse-errors": DiagType.PARSE_ERRORS,
    "query-timeouts": DiagType.QUERY_TIMEOUTS,
    "outbound-packet-timeouts": DiagType.OUTBOUND_PACKET_TIMEOUTS,
    "bus-busy": DiagType.BUS_BUSY,
    "outbound-packet-list-full": DiagType.OUTBOUND_PACKET_LIST_FILL,
    "outbound-query-list-full": DiagType.OUTBOUND_QUERY_LIST_FILL,
}

DIAG_BASE_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=0,
    state_class="total_increasing"
).extend(
    {
        cv.GenerateID(): cv.declare_id(BSBDiagSensor),
        cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
    }
)

CONFIG_SCHEMA = cv.typed_schema(
    {
        **{
            f"{diag_prefix}{k}": DIAG_BASE_SCHEMA for k, v in DIAG_MAPPING.items()
        },
        "lambda": sensor.sensor_schema()
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBSensorLambda),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
                cv.Required(CONF_LAMBDA): cv.returning_lambda,
            }
        ),
        "temp": sensor.sensor_schema(
            unit_of_measurement="°C",
            accuracy_decimals=1,
            device_class="temperature",
            state_class="measurement",
        )
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBSensorTemp),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
            }
        ),
        "enum": sensor.sensor_schema(
        )
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBSensorEnum),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
            }
        ),
        "percent": sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            state_class="measurement",
        )
        .extend(BSB_QUERY_BASE_SCHEMA)
        .extend(cv.polling_component_schema("60s"))
        .extend(
            {
                cv.GenerateID(): cv.declare_id(BSBSensorPercent),
                cv.GenerateID(CONF_BSB_ID): cv.use_id(BSBComponent),
            }
        ),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_BSB_ID])
    var = await sensor.new_sensor(config)

    if config[CONF_TYPE].startswith(diag_prefix):
        diag_type = config[CONF_TYPE][len(diag_prefix):]
        cg.add(parent.setDiagSensor(DIAG_MAPPING[diag_type], var))
    else:
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
