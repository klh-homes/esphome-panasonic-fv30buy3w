import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID
from . import panasonic_fv30buy3w_ns, PanasonicFV30BUY3W, CONF_PANASONIC_FV30BUY3W_ID

CONF_MODE = "mode"

FanModeSelect = panasonic_fv30buy3w_ns.class_(
    "FanModeSelect", select.Select, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PANASONIC_FV30BUY3W_ID): cv.use_id(PanasonicFV30BUY3W),
        cv.Optional(CONF_MODE): select.select_schema(FanModeSelect),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PANASONIC_FV30BUY3W_ID])

    if CONF_MODE in config:
        sel = await select.new_select(
            config[CONF_MODE],
            options=[
                "Standby",
                "Ventilation 15m", "Ventilation 30m", "Ventilation 1h", "Ventilation 3h", "Ventilation 6h", "Ventilation Cont.",
                "Heating 15m", "Heating 30m", "Heating 1h", "Heating 3h",
                "Hot Dry 15m", "Hot Dry 30m", "Hot Dry 1h", "Hot Dry 3h", "Hot Dry 6h",
                "Cool Dry 15m", "Cool Dry 30m", "Cool Dry 1h", "Cool Dry 3h", "Cool Dry 6h", "Cool Dry Cont.",
            ],
        )
        cg.add(sel.set_parent(parent))
        cg.add(parent.set_mode_select(sel))
