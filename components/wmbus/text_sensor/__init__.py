import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
)

from .. import (
    CONF_TYPE,
    CONF_WMBUS_ID,
    CONF_METER_ID,
    TEXT_LISTENER_SCHEMA,
    wmbus_ns,
)

CODEOWNERS = ["@SzczepanLeon"]

WMBusTextSensor= wmbus_ns.class_('WMBusTextSensor', text_sensor.TextSensor, cg.Component)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    WMBusTextSensor,
).extend(TEXT_LISTENER_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_METER_ID], config[CONF_TYPE].lower())
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)
    wmbus = await cg.get_variable(config[CONF_WMBUS_ID])
    cg.add(wmbus.register_wmbus_listener(var))
