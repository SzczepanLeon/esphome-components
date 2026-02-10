import esphome.config_validation as cv
from esphome.const import SOURCE_FILE_EXTENSIONS, CONF_ID
from esphome.loader import get_component, ComponentManifest
from esphome import codegen as cg
from pathlib import Path

CODEOWNERS = ["@SzczepanLeon", "@kubasaw"]
CONF_DRIVERS = "drivers"

wmbus_common_ns = cg.esphome_ns.namespace("wmbus_common")
WMBusCommon = wmbus_common_ns.class_("WMBusCommon", cg.Component)


AVAILABLE_DRIVERS = {
    f.stem.removeprefix("driver_") for f in Path(__file__).parent.glob("driver_*.cc")
}

_registered_drivers = set()


validate_driver = cv.All(
    cv.one_of(*AVAILABLE_DRIVERS, lower=True, space="_"),
    lambda driver: _registered_drivers.add(driver) or driver,
)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WMBusCommon),
        cv.Optional(CONF_DRIVERS, default=set()): cv.All(
            lambda x: AVAILABLE_DRIVERS if x == "all" else set(x) if isinstance(x, list) else x,
            {validate_driver},
        ),
    }
)


class WMBusComponentManifest(ComponentManifest):
    exclude_drivers: set[str]

    @property
    def resources(self):
        exclude_files = {f"driver_{name}.cc" for name in self.exclude_drivers}
        SOURCE_FILE_EXTENSIONS.add(".cc")
        resources = [fr for fr in super(
        ).resources if fr.resource not in exclude_files]
        SOURCE_FILE_EXTENSIONS.discard(".cc")
        return resources


async def to_code(config):
    component = get_component("wmbus_common")
    component.__class__ = WMBusComponentManifest
    component.exclude_drivers = AVAILABLE_DRIVERS - _registered_drivers

    var = cg.new_Pvariable(config[CONF_ID], sorted(_registered_drivers))
    await cg.register_component(var, config)
