import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time
from esphome.components.network import IPAddress
from esphome.const import (
    CONF_ID,
    CONF_MOSI_PIN,
    CONF_MISO_PIN,
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_NAME,
    CONF_IP_ADDRESS,
    CONF_PORT,
    CONF_FORMAT,
    CONF_TIME_ID,
)

CONF_TRANSPORT = "transport"

CONF_GDO0_PIN = "gdo0_pin"
CONF_GDO2_PIN = "gdo2_pin"

CONF_LED_PIN = "led_pin"
CONF_LED_BLINK_TIME = "led_blink_time"

CONF_WMBUS_ID = "wmbus_id"

CODEOWNERS = ["@SzczepanLeon"]

DEPENDENCIES = ["time"]
AUTO_LOAD = ["sensor"]

wmbus_ns = cg.esphome_ns.namespace('wmbus')
WMBusComponent = wmbus_ns.class_('WMBusComponent', cg.Component)
Client = wmbus_ns.struct('Client')
Format = wmbus_ns.enum("Format")
Transport = wmbus_ns.enum("Transport")

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
    cv.Optional(CONF_FORMAT, default="RTLWMBUS"): cv.templatable(validate_format),
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(WMBusComponent),
    cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_MOSI_PIN, default=13): pins.internal_gpio_output_pin_schema,
    cv.Optional(CONF_MISO_PIN, default=12): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_CLK_PIN,  default=14): pins.internal_gpio_output_pin_schema,
    cv.Optional(CONF_CS_PIN,   default=2):  pins.internal_gpio_output_pin_schema,
    cv.Optional(CONF_GDO0_PIN, default=5):  pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_GDO2_PIN, default=4):  pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_LED_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_LED_BLINK_TIME, default="300ms"): cv.positive_time_period,
    cv.Optional(CONF_CLIENTS):  cv.ensure_list(CLIENT_SCHEMA),
})

def safe_ip(ip):
    if ip is None:
        return IPAddress(0, 0, 0, 0)
    return IPAddress(*ip.args)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    mosi = await cg.gpio_pin_expression(config[CONF_MOSI_PIN])
    miso = await cg.gpio_pin_expression(config[CONF_MISO_PIN])
    clk = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    gdo0 = await cg.gpio_pin_expression(config[CONF_GDO0_PIN])
    gdo2 = await cg.gpio_pin_expression(config[CONF_GDO2_PIN])

    cg.add(var.add_cc1101(mosi, miso, clk, cs, gdo0, gdo2))

    time = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time))

    for conf in config.get(CONF_CLIENTS, []):
        cg.add(var.add_client(conf[CONF_NAME],
                              safe_ip(conf[CONF_IP_ADDRESS]), conf[CONF_PORT], conf[CONF_TRANSPORT],
                              conf[CONF_FORMAT]))

    if CONF_LED_PIN in config:
        led_pin = await cg.gpio_pin_expression(config[CONF_LED_PIN])
        cg.add(var.set_led_pin(led_pin))
        cg.add(var.set_led_blink_time(config[CONF_LED_BLINK_TIME].total_milliseconds))

    cg.add_library(
        None,
        None,
        "https://github.com/SzczepanLeon/wMbus-lib#1.1.1",
    )
