import esphome.config_validation as cv
from esphome.const import SOURCE_FILE_EXTENSIONS, CONF_ID
from esphome import codegen as cg
from pathlib import Path

CODEOWNERS = ["@SzczepanLeon", "@kubasaw"]
CONF_DRIVERS = "drivers"

wmbus_common_ns = cg.esphome_ns.namespace("wmbus_common")
WMBusCommon = wmbus_common_ns.class_("WMBusCommon", cg.Component)

# Enable .cpp files to be picked up as source files (wmbusmeters library uses .cpp)

AVAILABLE_DRIVERS = {
    f.stem.removeprefix("driver_") for f in Path(__file__).parent.glob("driver_*.cpp")
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


def FILTER_SOURCE_FILES():
    """Return set of driver source files to exclude from compilation."""
    return {f"driver_{name}.cpp" for name in AVAILABLE_DRIVERS - _registered_drivers}


def _keep_symbol(driver):
    """Symbol defined by KEEP_DRIVER() in driver_<name>.cpp."""
    return f"wmbus_driver_{driver.replace('-', '_')}_linked"


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], sorted(_registered_drivers))
    await cg.register_component(var, config)

    # Each driver registers itself from a file-scope static initializer and
    # nothing references its translation unit. ESPHome archives the sources
    # into a static library for native ESP-IDF builds, so the linker drops
    # those archive members and the drivers are never registered - the build
    # succeeds and every meter ends up with a null driver on the device.
    # Referencing one symbol per driver from main.cpp keeps the object files.
    drivers = sorted(_registered_drivers)
    if not drivers:
        return

    for driver in drivers:
        cg.add_global(cg.RawStatement(f"extern bool {_keep_symbol(driver)};"))

    refs = ", ".join(f"&{_keep_symbol(driver)}" for driver in drivers)
    cg.add_global(
        cg.RawStatement(
            f"static bool *const wmbus_kept_drivers[] __attribute__((used)) = {{{refs}}};"
        )
    )
