from esphome import config_validation as cv
from esphome import codegen as cg
from esphome import automation
from esphome.const import CONF_ID, CONF_IP_ADDRESS, CONF_PORT, CONF_PROTOCOL, CONF_DATA

AUTO_LOAD = ["socket"]

MULTI_CONF = True

CODEOWNERS = ["@SzczepanLeon", "@kubasaw"]


socket_ns = cg.esphome_ns.namespace("socket_transmitter")
SocketTransmitter = socket_ns.class_("SocketTransmitter", cg.Component)
SocketTransmitterSendAction = socket_ns.class_(
    "SocketTransmitterSendAction", automation.Action
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SocketTransmitter),
        cv.Required(CONF_IP_ADDRESS): cv.All(cv.ipv4address, cv.string),
        cv.Required(CONF_PORT): cv.port,
        cv.Required(CONF_PROTOCOL): cv.enum(
            {
                "TCP": cg.RawExpression("SOCK_STREAM"),
                "UDP": cg.RawExpression("SOCK_DGRAM"),
            },
            upper=True,
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_host(config[CONF_IP_ADDRESS]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_protocol(config[CONF_PROTOCOL]))

    await cg.register_component(var, config)


SOCKET_SEND_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(SocketTransmitter),
        cv.Required(CONF_DATA): cv.templatable(cv.string_strict),
    }
)


@automation.register_action(
    "socket_transmitter.send", SocketTransmitterSendAction, SOCKET_SEND_ACTION_SCHEMA
)
async def socket_transmitter_send_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(
        action_id, cg.TemplateArguments(cg.std_string, *template_arg), paren
    )

    template_ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_data(template_))

    return var
