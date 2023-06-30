import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    CONF_TYPE,
)

from .. import (
    WMBusComponent,
    CONF_WMBUS_ID,
    wmbus_ns,
)

CODEOWNERS = ["@SzczepanLeon"]

AUTO_LOAD = ["wmbus"]

WMBusListener = wmbus_ns.class_('WMBusListener')

CONF_METER_ID = "meter_id"
CONF_LISTENER_ID = "listener_id"

TEXT_LISTENER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LISTENER_ID): cv.declare_id(WMBusListener),
        cv.GenerateID(CONF_WMBUS_ID): cv.use_id(WMBusComponent),
        cv.Optional(CONF_METER_ID, default=0xAFFFFFF5): cv.hex_int,
        cv.Optional(CONF_TYPE, default="text"): cv.string_strict,
    }
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    #
).extend(TEXT_LISTENER_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_LISTENER_ID], config[CONF_METER_ID], config[CONF_TYPE].lower(), "")
    sens = await text_sensor.new_text_sensor(config)
    cg.add(var.add_sensor(sens))
    wmbus = await cg.get_variable(config[CONF_WMBUS_ID])
    cg.add(wmbus.register_wmbus_listener(var))