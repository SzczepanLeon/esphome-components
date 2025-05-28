from esphome.components import text_sensor

from .. import wmbus_meter_ns
from ..base_sensor import BASE_SCHEMA, register_meter, BaseSensor

TextSensor = wmbus_meter_ns.class_(
    "TextSensor", BaseSensor, text_sensor.TextSensor)

CONFIG_SCHEMA = BASE_SCHEMA.extend(text_sensor.text_sensor_schema(TextSensor))


async def to_code(config):
    text_sensor_ = await text_sensor.new_text_sensor(config)
    await register_meter(text_sensor_, config)
