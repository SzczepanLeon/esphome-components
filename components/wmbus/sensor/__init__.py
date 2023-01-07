import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_CUBIC_METER,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_TOTAL_INCREASING,
)

CONF_METERS = 'meters'
ICON_WATER = "mdi:water"
DEVICE_CLASS_WATER = 'water'

from .. import (
    CONF_KEY,
    CONF_TYPE,
    CONF_WMBUS_ID,
    CONF_METER_ID,
    METER_LISTENER_SCHEMA,
    wmbus_ns,
)

CODEOWNERS = ["@SzczepanLeon"]

WMBusSensor= wmbus_ns.class_('WMBusSensor', sensor.Sensor, cg.Component)

CONFIG_SCHEMA = sensor.sensor_schema(
    WMBusSensor,
    unit_of_measurement=UNIT_CUBIC_METER,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_WATER,
    state_class=STATE_CLASS_TOTAL_INCREASING,
    icon=ICON_WATER,
).extend(METER_LISTENER_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_METER_ID], config[CONF_TYPE].lower(), config[CONF_KEY])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)
    wmbus = await cg.get_variable(config[CONF_WMBUS_ID])
    cg.add(wmbus.register_wmbus_listener(var))