from contextlib import suppress
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import spi
from esphome.cpp_generator import LambdaExpression
from esphome.const import (
    CONF_ID,
    CONF_RESET_PIN,
    CONF_IRQ_PIN,
    CONF_TRIGGER_ID,
    CONF_FORMAT,
    CONF_DATA,
    CONF_FREQUENCY,
)

CONF_GDO0_PIN = "gdo0_pin"
CONF_GDO2_PIN = "gdo2_pin"
CONF_POLLING_INTERVAL = "polling_interval"
from pathlib import Path

CODEOWNERS = ["@SzczepanLeon", "@kubasaw"]

DEPENDENCIES = ["esp32", "spi"]

AUTO_LOAD = ["wmbus_common"]

MULTI_CONF = True

CONF_RADIO_ID = "radio_id"
CONF_ON_FRAME = "on_frame"
CONF_RADIO_TYPE = "radio_type"
CONF_MARK_AS_HANDLED = "mark_as_handled"

radio_ns = cg.esphome_ns.namespace("wmbus_radio")
RadioComponent = radio_ns.class_("Radio", cg.Component)
RadioTransceiver = radio_ns.class_(
    "RadioTransceiver", spi.SPIDevice, cg.Component)
Frame = radio_ns.class_("Frame")
FrameOutputFormat = Frame.enum("OutputFormat")
FramePtr = Frame.operator("ptr")
FrameTrigger = radio_ns.class_(
    "FrameTrigger", automation.Trigger.template(FramePtr))

TRANSCEIVER_NAMES = {
    r.stem.removeprefix("transceiver_").upper()
    for r in Path(__file__).parent.glob("transceiver_*.cpp")
    if r.is_file()
}

def validate_radio_config(config):
    """Validate that required pins are present for the selected radio type."""
    radio_type = config[CONF_RADIO_TYPE]

    if radio_type == "CC1101":
        # CC1101 requires GDO0 and GDO2 pins
        if CONF_GDO0_PIN not in config:
            raise cv.Invalid(f"CC1101 requires '{CONF_GDO0_PIN}' to be specified")
        if CONF_GDO2_PIN not in config:
            raise cv.Invalid(f"CC1101 requires '{CONF_GDO2_PIN}' to be specified")
        # CC1101 has no hardware reset pin (uses software SRES strobe)
        if CONF_RESET_PIN in config:
            raise cv.Invalid(f"CC1101 does not have a hardware reset pin (uses software reset). Remove '{CONF_RESET_PIN}'")
        # IRQ_PIN not used for CC1101
        if CONF_IRQ_PIN in config:
            raise cv.Invalid(f"CC1101 does not use '{CONF_IRQ_PIN}', use '{CONF_GDO0_PIN}' and '{CONF_GDO2_PIN}' instead")
    elif radio_type == "SX1276":
        # SX1276 requires reset and IRQ pins
        if CONF_RESET_PIN not in config:
            raise cv.Invalid(f"SX1276 requires '{CONF_RESET_PIN}' to be specified")
        if CONF_IRQ_PIN not in config:
            raise cv.Invalid(f"SX1276 requires '{CONF_IRQ_PIN}' to be specified")
        # GDO pins not used for SX1276
        if CONF_GDO0_PIN in config or CONF_GDO2_PIN in config:
            raise cv.Invalid(f"SX1276 does not use GDO pins, use '{CONF_IRQ_PIN}' instead")

    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RadioComponent),
            cv.GenerateID(CONF_RADIO_ID): cv.declare_id(RadioTransceiver),
            cv.Required(CONF_RADIO_TYPE): cv.one_of(*TRANSCEIVER_NAMES, upper=True),
            # Conditional pins - validated by validate_radio_config
            cv.Optional(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_IRQ_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_GDO0_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_GDO2_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_FREQUENCY, default=868.95): cv.float_range(min=300.0, max=928.0),
            # Advanced: Polling interval for CC1101 (milliseconds)
            # Lower values = better reception but higher CPU load
            # At 100kbps, data arrives at 12.5 bytes/ms, FIFO is 64 bytes
            # Default 2ms is recommended. Values >5ms may cause FIFO overflow and frame loss.
            cv.Optional(CONF_POLLING_INTERVAL, default=2): cv.int_range(min=1, max=10),
            cv.Optional(CONF_ON_FRAME): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FrameTrigger),
                    cv.Optional(CONF_MARK_AS_HANDLED, default=False): cv.boolean,
                }
            ),
        }
    )
    .extend(spi.spi_device_schema())
    .extend(cv.COMPONENT_SCHEMA),
    validate_radio_config,
)


async def to_code(config):
    cg.add(cg.LineComment("WMBus RadioTransceiver"))

    config[CONF_RADIO_ID].type = radio_ns.class_(
        config[CONF_RADIO_TYPE], RadioTransceiver
    )
    radio_var = cg.new_Pvariable(config[CONF_RADIO_ID])

    # Configure pins based on radio type
    radio_type = config[CONF_RADIO_TYPE]
    if radio_type == "CC1101":
        # CC1101 uses GDO0 and GDO2 pins (no hardware reset)
        gdo0_pin = await cg.gpio_pin_expression(config[CONF_GDO0_PIN])
        cg.add(radio_var.set_gdo0_pin(gdo0_pin))

        gdo2_pin = await cg.gpio_pin_expression(config[CONF_GDO2_PIN])
        cg.add(radio_var.set_gdo2_pin(gdo2_pin))

        # Set frequency if specified
        if CONF_FREQUENCY in config:
            cg.add(radio_var.set_frequency(config[CONF_FREQUENCY]))

        # Set polling interval (CC1101 only - used for polling-based reception)
        if CONF_POLLING_INTERVAL in config:
            cg.add(radio_var.set_polling_interval(config[CONF_POLLING_INTERVAL]))
    elif radio_type == "SX1276":
        # SX1276 uses reset and IRQ pins
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(radio_var.set_reset_pin(reset_pin))

        irq_pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
        cg.add(radio_var.set_irq_pin(irq_pin))

    await spi.register_spi_device(radio_var, config)
    await cg.register_component(radio_var, config)

    cg.add(cg.LineComment("WMBus Component"))
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_radio(radio_var))

    await cg.register_component(var, config)

    for conf in config.get(CONF_ON_FRAME, []):
        trig = cg.new_Pvariable(
            conf[CONF_TRIGGER_ID], var, conf[CONF_MARK_AS_HANDLED])
        await automation.build_automation(
            trig,
            [(FramePtr, "frame")],
            conf,
        )


with suppress(ImportError):
    from ..socket_transmitter import (
        SOCKET_SEND_ACTION_SCHEMA,
        SocketTransmitterSendAction,
    )

    FRAME_SOCKET_SEND_SCHEMA = SOCKET_SEND_ACTION_SCHEMA.extend(
        {
            cv.Required(CONF_FORMAT): cv.one_of(
                "hex",
                "raw",
                "rtlwmbus",
                lower=True,
            ),
            cv.Optional(CONF_DATA): cv.invalid(
                "If you want to specify data to be sent, use generic 'socket_transmitter.send' action"
            ),
        }
    )

    @automation.register_action(
        "wmbus_radio.send_frame_with_socket",
        SocketTransmitterSendAction,
        FRAME_SOCKET_SEND_SCHEMA,
    )
    async def send_frame_with_socket_to_code(config, action_id, template_arg, args):
        output_type = {
            "hex": cg.std_string,
            "raw": cg.std_vector.template(cg.uint8),
            "rtlwmbus": cg.std_string,
        }[config[CONF_FORMAT]]

        paren = await cg.get_variable(config[CONF_ID])
        var = cg.new_Pvariable(
            action_id, cg.TemplateArguments(output_type, *template_arg), paren
        )
        template_ = LambdaExpression(
            f"return frame.as_{config[CONF_FORMAT]}();", args, ""
        )

        cg.add(var.set_data(template_))

        return var
