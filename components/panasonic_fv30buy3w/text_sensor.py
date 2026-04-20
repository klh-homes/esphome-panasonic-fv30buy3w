import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import panasonic_fv30buy3w_ns, PanasonicFV30BUY3W, CONF_PANASONIC_FV30BUY3W_ID

CONF_TIMER_EXPIRES = "timer_expires"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PANASONIC_FV30BUY3W_ID): cv.use_id(PanasonicFV30BUY3W),
        cv.Optional(CONF_TIMER_EXPIRES): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PANASONIC_FV30BUY3W_ID])

    if CONF_TIMER_EXPIRES in config:
        sens = await text_sensor.new_text_sensor(config[CONF_TIMER_EXPIRES])
        cg.add(parent.set_timer_expires(sens))
