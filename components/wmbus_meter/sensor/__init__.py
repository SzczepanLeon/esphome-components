from esphome import config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_UNIT_OF_MEASUREMENT

from .. import wmbus_meter_ns
from ..base_sensor import BASE_SCHEMA, register_meter, BaseSensor, CONF_FIELD
from ...wmbus_common.units import get_human_readable_unit


RegularSensor = wmbus_meter_ns.class_("Sensor", BaseSensor, sensor.Sensor)


def default_unit_of_measurement(config):
    config.setdefault(
        CONF_UNIT_OF_MEASUREMENT,
        get_human_readable_unit(config[CONF_FIELD].rsplit("_").pop()),
    )

    return config


CONFIG_SCHEMA = cv.All(
    BASE_SCHEMA.extend(sensor.sensor_schema(RegularSensor)),
    default_unit_of_measurement,
)


async def to_code(config):
    sensor_ = await sensor.new_sensor(config)
    await register_meter(sensor_, config)
