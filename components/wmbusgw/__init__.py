import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time
from esphome.components.network import IPAddress
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_IP_ADDRESS,
    CONF_PORT,
    CONF_FORMAT,
    CONF_TIME_ID,
    CONF_REBOOT_TIMEOUT,
    CONF_MOSI_PIN,
    CONF_MISO_PIN,
    CONF_CLK_PIN,
    CONF_CS_PIN,
)

CONF_GDO0_PIN = "gdo0_pin"
CONF_GDO2_PIN = "gdo2_pin"

CONF_TRANSPORT = "transport"

DEPENDENCIES = ["time"]

CODEOWNERS = ["@SzczepanLeon"]

wmbusgw_ns = cg.esphome_ns.namespace('wmbusgw')
WMBusGwComponent = wmbusgw_ns.class_('WMBusGwComponent', cg.Component)
Client = wmbusgw_ns.struct('Client')
Format = wmbusgw_ns.enum("Format")
Transport = wmbusgw_ns.enum("Transport")
Cc1101 = wmbusgw_ns.struct('Cc1101')

FORMAT = {
    "HEX": Format.FORMAT_HEX,
    "RTLWMBUS": Format.FORMAT_RTLWMBUS,
}
validate_format = cv.enum(FORMAT, upper=True)

TRANSPORT = {
    "TCP": Transport.TRANSPORT_TCP,
    "UDP": Transport.TRANSPORT_UDP,
}
validate_transport = cv.enum(TRANSPORT, upper=True)

CONF_CLIENTS = 'clients'

CLIENT_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Client),
    cv.Required(CONF_NAME): cv.string_strict,
    cv.Required(CONF_IP_ADDRESS): cv.ipv4,
    cv.Required(CONF_PORT): cv.port,
    cv.Optional(CONF_TRANSPORT, default="TCP"): cv.templatable(validate_transport),
    cv.Optional(CONF_FORMAT, default="HEX"): cv.templatable(validate_format),
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WMBusGwComponent),
    cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_schema,
    cv.Required(CONF_MISO_PIN): pins.internal_gpio_input_pin_schema,
    cv.Required(CONF_CLK_PIN):  pins.internal_gpio_output_pin_schema,
    cv.Required(CONF_CS_PIN):   pins.internal_gpio_output_pin_schema,
    cv.Required(CONF_GDO0_PIN): pins.internal_gpio_input_pin_schema,
    cv.Required(CONF_GDO2_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_REBOOT_TIMEOUT, default="3min"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_CLIENTS):  cv.ensure_list(CLIENT_SCHEMA),
})

def safe_ip(ip):
    if ip is None:
        return IPAddress(0, 0, 0, 0)
    return IPAddress(*ip.args)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    mosi = yield cg.gpio_pin_expression(config[CONF_MOSI_PIN])
    miso = yield cg.gpio_pin_expression(config[CONF_MISO_PIN])
    clk = yield cg.gpio_pin_expression(config[CONF_CLK_PIN])
    cs = yield cg.gpio_pin_expression(config[CONF_CS_PIN])
    gdo0 = yield cg.gpio_pin_expression(config[CONF_GDO0_PIN])
    gdo2 = yield cg.gpio_pin_expression(config[CONF_GDO2_PIN])

    cg.add(var.add_cc1101(mosi, miso, clk, cs, gdo0, gdo2))
    cg.add(var.set_reboot_timeout(config[CONF_REBOOT_TIMEOUT]))

    time_ = yield cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))

    for conf in config.get(CONF_CLIENTS, []):
        cg.add(var.add_client(conf[CONF_NAME],
                              safe_ip(conf[CONF_IP_ADDRESS]), conf[CONF_PORT], conf[CONF_TRANSPORT],
                              conf[CONF_FORMAT]))

    cg.add_library(
        None,
        None,
        "https://github.com/SzczepanLeon/wMbus-lib@0.9.14",
    )
