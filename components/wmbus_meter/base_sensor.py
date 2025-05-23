import esphome.config_validation as cv
import esphome.codegen as cg
from . import Meter, wmbus_meter_ns

CONF_PARENT_ID = "parent_id"
CONF_FIELD = "field"

BaseSensor = wmbus_meter_ns.class_("BaseSensor", cg.Component)

BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PARENT_ID): cv.use_id(Meter),
        cv.Required(CONF_FIELD): cv.string_strict,
    }
)


async def register_meter(obj, config):
    meter = await cg.get_variable(config[CONF_PARENT_ID])
    await cg.register_parented(obj, meter)
    cg.add(obj.set_field_name(config[CONF_FIELD]))
    await cg.register_component(obj, config)
