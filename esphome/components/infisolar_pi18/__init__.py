CODEOWNERS = ["@openai"]

from esphome.components import text_sensor, uart
from esphome.const import CONF_ID
import esphome.config_validation as cv
import esphome.codegen as cg

AUTO_LOAD = ["text_sensor", "uart"]

CONF_PROTOCOL_SENSOR = "protocol_sensor"
CONF_MODEL_SENSOR = "model_sensor"
CONF_HANDSHAKE_RETRY = "handshake_retry"

infisolar_ns = cg.esphome_ns.namespace("infisolar_pi18")
InfisolarPI18Component = infisolar_ns.class_("InfisolarPI18Component", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(InfisolarPI18Component),
    cv.Optional(CONF_PROTOCOL_SENSOR): cv.use_id(text_sensor.TextSensor),
    cv.Optional(CONF_MODEL_SENSOR): cv.use_id(text_sensor.TextSensor),
    cv.Optional(CONF_HANDSHAKE_RETRY, default=3): cv.int_range(min=1, max=10),
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await uart.register_uart_device(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_handshake_retry(config[CONF_HANDSHAKE_RETRY]))

    if CONF_PROTOCOL_SENSOR in config:
        sensor = await cg.get_variable(config[CONF_PROTOCOL_SENSOR])
        cg.add(var.set_protocol_sensor(sensor))
    if CONF_MODEL_SENSOR in config:
        sensor = await cg.get_variable(config[CONF_MODEL_SENSOR])
        cg.add(var.set_model_sensor(sensor))
