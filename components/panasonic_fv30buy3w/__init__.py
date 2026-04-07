import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN

CODEOWNERS = ["@kirintw"]
AUTO_LOAD = ["select", "text_sensor"]

panasonic_fv30buy3w_ns = cg.esphome_ns.namespace("panasonic_fv30buy3w")
PanasonicFV30BUY3W = panasonic_fv30buy3w_ns.class_(
    "PanasonicFV30BUY3W", cg.Component
)

CONF_PANASONIC_FV30BUY3W_ID = "panasonic_fv30buy3w_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PanasonicFV30BUY3W),
        cv.Required(CONF_PIN): cv.int_range(min=0, max=39),
        # TODO: proxy mode — 預留第二個 pin
        # cv.Optional("panel_pin"): cv.int_range(min=0, max=39),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pin(config[CONF_PIN]))
