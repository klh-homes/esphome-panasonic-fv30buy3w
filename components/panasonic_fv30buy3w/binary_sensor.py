import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import panasonic_fv30buy3w_ns, PanasonicFV30BUY3W, CONF_PANASONIC_FV30BUY3W_ID

CONF_HOST_CONNECTION = "host_connection"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PANASONIC_FV30BUY3W_ID): cv.use_id(PanasonicFV30BUY3W),
        cv.Optional(CONF_HOST_CONNECTION): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PANASONIC_FV30BUY3W_ID])

    if CONF_HOST_CONNECTION in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_HOST_CONNECTION])
        cg.add(parent.set_host_connection(sens))
