import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID
from . import panasonic_fv30buy3w_ns, PanasonicFV30BUY3W, CONF_PANASONIC_FV30BUY3W_ID

CONF_MODE = "mode"
CONF_TIMER = "timer"

FanModeSelect = panasonic_fv30buy3w_ns.class_(
    "FanModeSelect", select.Select, cg.Component
)
FanTimerSelect = panasonic_fv30buy3w_ns.class_(
    "FanTimerSelect", select.Select, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PANASONIC_FV30BUY3W_ID): cv.use_id(PanasonicFV30BUY3W),
        cv.Optional(CONF_MODE): select.select_schema(FanModeSelect),
        cv.Optional(CONF_TIMER): select.select_schema(FanTimerSelect),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PANASONIC_FV30BUY3W_ID])

    if CONF_MODE in config:
        sel = await select.new_select(
            config[CONF_MODE],
            options=["待機", "換氣", "取暖", "乾燥熱", "乾燥涼"],
        )
        cg.add(sel.set_parent(parent))
        cg.add(parent.set_mode_select(sel))

    if CONF_TIMER in config:
        sel = await select.new_select(
            config[CONF_TIMER],
            options=["15分", "30分", "1小時", "3小時", "6小時", "24小時"],
        )
        cg.add(sel.set_parent(parent))
        cg.add(parent.set_timer_select(sel))
