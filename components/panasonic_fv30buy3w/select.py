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
                "待機",
                "換氣 15分", "換氣 30分", "換氣 1小時", "換氣 3小時", "換氣 6小時", "換氣 24小時",
                "取暖 15分", "取暖 30分", "取暖 1小時", "取暖 3小時",
                "乾燥熱 15分", "乾燥熱 30分", "乾燥熱 1小時", "乾燥熱 3小時", "乾燥熱 6小時",
                "乾燥涼 15分", "乾燥涼 30分", "乾燥涼 1小時", "乾燥涼 3小時", "乾燥涼 6小時", "乾燥涼 24小時",
            ],
        )
        cg.add(sel.set_parent(parent))
        cg.add(parent.set_mode_select(sel))
