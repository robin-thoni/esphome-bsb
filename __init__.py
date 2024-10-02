import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import uart

DEPENDENCIES = ["uart"]

CONF_BSB_ID = "bsb_id"
CONF_LOCAL_ADDRESS = "local_address"
CONF_REMOTE_ADDRESS = "remote_address"

CONF_BSB_TYPE = "bsb_type"
CONF_BSB_CMD = "bsb_cmd"
CONF_BSB_DATA = "bsb_data"

bsb_ns = cg.esphome_ns.namespace("bsb")
BSBComponent = bsb_ns.class_(
    "BSBComponent", cg.Component
)

BSBQueryCallackArgs = bsb_ns.class_(
    "BSBQueryCallackArgs",
)

def validate_raw_data(value):
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, str):
        return value
    if isinstance(value, list):
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        "data must either be a string wrapped in quotes or a list of bytes"
    )

BSB_QUERY_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_BSB_TYPE): cv.int_range(min=0x00, max=0xFF),
        cv.Required(CONF_BSB_CMD): cv.int_range(min=0x00, max=0xFFFFFFFF),
        cv.Optional(CONF_BSB_DATA): cv.templatable(validate_raw_data),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BSBComponent),
            cv.Optional(CONF_LOCAL_ADDRESS, default=0x42): cv.int_range(min=0x00, max=0xFF),
            cv.Optional(CONF_REMOTE_ADDRESS, default=0x00): cv.int_range(min=0x00, max=0xFF),
        }
    ).extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.setSrcAddress(config[CONF_LOCAL_ADDRESS]))
    cg.add(var.setDstAddress(config[CONF_REMOTE_ADDRESS]))
