import re
from collections import namedtuple
from pathlib import Path

import esphome.config_validation as cv
from esphome.const import SOURCE_FILE_EXTENSIONS, CONF_ID
from esphome import codegen as cg

CODEOWNERS = ["@SzczepanLeon", "@kubasaw"]
CONF_DRIVERS = "drivers"

wmbus_common_ns = cg.esphome_ns.namespace("wmbus_common")
WMBusCommon = wmbus_common_ns.class_("WMBusCommon", cg.Component)

# Enable .cpp files to be picked up as source files (wmbusmeters library uses .cpp)

# createMeter() looks a configured type up by the name the driver passes to
# di.setName()/di.addNameAlias(), which is not necessarily the file name -
# driver_kamheat.cpp also answers to "multical603", for instance. Read those
# names out of the sources so that a type no driver registers is rejected while
# validating the configuration, instead of leaving the meter with no
# implementation until it is reported at boot.
_DRIVER_NAME_RE = re.compile(r'\.(setName|addNameAlias)\s*\(\s*"([^"]+)"\s*\)')

Driver = namedtuple("Driver", "name source")


def _registered_names(source):
    """Names driver_<x>.cpp registers itself under, as (name, aliases)."""
    names = {"setName": set(), "addNameAlias": set()}
    for call, name in _DRIVER_NAME_RE.findall(source.read_text(encoding="utf-8")):
        names[call].add(name)

    # Not a user error - the driver cannot be selected at all if we get here.
    if len(names["setName"]) != 1:
        raise ValueError(
            f"{source.name} must call di.setName() with exactly one name, "
            f"found {sorted(names['setName']) or 'none'}"
        )

    return names["setName"].pop(), names["addNameAlias"]


def _collect_drivers():
    """Every name accepted in the config, mapped to the driver providing it."""
    drivers = {}
    for source in sorted(Path(__file__).parent.glob("driver_*.cpp")):
        name, aliases = _registered_names(source)
        driver = Driver(name, source)
        for alias in (name, *aliases):
            if alias in drivers:
                raise ValueError(
                    f"{source.name} and {drivers[alias].source.name} both "
                    f'register the driver name "{alias}"'
                )
            drivers[alias] = driver
    return drivers


DRIVERS = _collect_drivers()

# Names usable as a meter type, aliases included.
AVAILABLE_DRIVERS = set(DRIVERS)
# Canonical names only, so that "drivers: all" does not list a driver twice.
DRIVER_NAMES = {driver.name for driver in DRIVERS.values()}

_registered_drivers = set()


validate_driver = cv.All(
    cv.one_of(*AVAILABLE_DRIVERS, lower=True, space="_"),
    lambda driver: _registered_drivers.add(driver) or driver,
)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WMBusCommon),
        cv.Optional(CONF_DRIVERS, default=set()): cv.All(
            lambda x: DRIVER_NAMES if x == "all" else set(x) if isinstance(x, list) else x,
            {validate_driver},
        ),
    }
)


def _selected_drivers():
    """Drivers the configuration selected, deduplicated across aliases."""
    return {DRIVERS[name] for name in _registered_drivers}


def FILTER_SOURCE_FILES():
    """Return set of driver source files to exclude from compilation."""
    kept = {driver.source.name for driver in _selected_drivers()}
    return {driver.source.name for driver in DRIVERS.values()} - kept


def _keep_symbol(driver):
    """Symbol defined by KEEP_DRIVER() in driver_<name>.cpp."""
    stem = driver.source.stem.removeprefix("driver_").replace("-", "_")
    return f"wmbus_driver_{stem}_linked"


async def to_code(config):
    drivers = sorted(_selected_drivers())
    var = cg.new_Pvariable(config[CONF_ID], [driver.name for driver in drivers])
    await cg.register_component(var, config)

    if not drivers:
        return

    # Reference each selected driver's KEEP_DRIVER symbol from main.cpp so the
    # linker keeps its object file; see KEEP_DRIVER in meters.h.
    symbols = [_keep_symbol(driver) for driver in drivers]

    for symbol in symbols:
        cg.add_global(cg.RawStatement(f"extern bool {symbol};"))

    refs = ", ".join(f"&{symbol}" for symbol in symbols)
    cg.add_global(
        cg.RawStatement(
            f"static bool *const wmbus_kept_drivers[] __attribute__((used)) = {{{refs}}};"
        )
    )
