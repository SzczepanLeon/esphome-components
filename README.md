Version 5 based on Kuba's dirty [fork](https://github.com/IoTLabs-pl/esphome-components).

> **_NOTE:_**  Component with CC1101 support is here:
[version 4](https://github.com/SzczepanLeon/esphome-components/tree/version_4)
[version 3](https://github.com/SzczepanLeon/esphome-components/tree/version_3)
[version 2](https://github.com/SzczepanLeon/esphome-components/tree/version_2)


# TODO:
- Add backward support for CC1101
- Prepare packages for ready made boards (like UltimateReader) with displays, leds etc.
- Aggresive cleanup of wmbusmeters classes/structs
- Refactor traces/logs

# DONE:
- Add support for SX1262 (with limited frame length)
- Reuse CRCs and frame parsers from wmbusmeters
- Refactor 3out6 decoder
- Migrate to esp-idf and drop Arduino!
- Add support for SX1276
- Run receiver in separate task
- Drop all non wmbus related components from rf code part
- Allow to specify ASCII decription key
- Divide codebase to separate components (radio for radio communication, meter for meters (on which sensor may subscribe) and common for wmbusmeters code)
- Add triggers:
  - Radio->on packet (allow to blink on frame/telegram)
  - Meter->on telegram (allow e.g. to send whole telegram to MQTT)
- Re-pull of wmbusmeters code from upstream
- Reimplement TCP and UCP senders. Should be classes with common interface to use as action under Radio->on packet trigger
- Reimplement HEX and RTLWMBUS formatter to use as parameter of TCP/UDP action


# Usage example:
```yaml
esphome:
  name: wmbus
  friendly_name: WMBus
  platformio_options:
    upload_speed: 921600

external_components:
  - source: github://SzczepanLeon/esphome-components@main

esp32:
  board: heltec_wifi_lora_32_V2
  flash_size: 8MB
  framework:
    type: esp-idf
  
logger:
  id: component_logger
  level: DEBUG
  baud_rate: 115200

wifi:
  networks:
    - ssid: !secret wifi_ssid
      password: !secret wifi_password

api:

web_server:
  version: 3 

time:
  - platform: homeassistant

spi:
  clk_pin:
    number: GPIO5
    ignore_strapping_warning: true
  mosi_pin: GPIO27
  miso_pin: GPIO19

socket_transmitter:
  id: my_socket
  ip_address: 192.168.1.1
  port: 3333
  protocol: TCP

mqtt:
  broker: test.mosquitto.org
  port: 1883
  client_id: some_client_id

wmbus_radio:
  radio_type: SX1276
  cs_pin: GPIO18
  reset_pin: GPIO14
  irq_pin: GPIO35
  on_frame:
    - then:
        - logger.log:
            format: "RSSI: %ddBm T: %s (%d)"
            args: [ frame->rssi(), frame->as_hex().c_str(), frame->data().size() ]
    - then:
        - repeat:
            count: 3
            then:
              - output.turn_on: status_led
              - delay: 100ms
              - output.turn_off: status_led
              - delay: 100ms
    - mark_as_handled: True
      then:
        - mqtt.publish:
            topic: wmbus-test/telegram_rtl
            payload: !lambda return frame->as_rtlwmbus();
    - mark_as_handled: True
      then:
        - socket_transmitter.send:
            data: !lambda return frame->as_hex();

wmbus_meter:
  - id: electricity_meter
    meter_id: 0x0101010101
    type: amiplus
    key: 00000000000000000000000000000000
    mode: 
      - T1
      - C1
  - id: heat_meter
    meter_id: 12321
    type: hydrocalm3
    on_telegram:
      then:
        - wmbus_meter.send_telegram_with_mqtt:
            topic: wmbus-test/telegram

output:
  - platform: gpio
    id: vext_output
    pin: GPIO21
  - platform: gpio
    id: oled_reset
    pin: GPIO16
    inverted: True
  - platform: gpio
    id: status_led
    pin: GPIO25

sensor:
  - platform: wmbus_meter
    parent_id: heat_meter
    field: total_heating_kwh
    device_class: energy
    name: Zużycie energii cieplnej
    accuracy_decimals: 4
    state_class: total_increasing

  - platform: wmbus_meter
    parent_id: electricity_meter
    field: current_power_consumption_kw
    name: Moc aktualna
    accuracy_decimals: 0
    device_class: power
    unit_of_measurement: W
    state_class: measurement
    filters:
      - multiply: 1000

  - platform: wmbus_meter
    parent_id: electricity_meter
    field: total_energy_consumption_kwh
    name: Zużycie energii
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing

  - platform: wmbus_meter
    parent_id: electricity_meter
    field: rssi_dbm
    name: Electricity Meter RSSI

text_sensor:
  - platform: wmbus_meter
    parent_id: electricity_meter
    field: timestamp
    name: Electricity Meter timestamp

  - platform: wmbus_meter
    parent_id: electricity_meter
    field: timestamp_zulu
    name: Electricity Meter timestamp zulu

  - platform: wmbus_meter
    parent_id: electricity_meter
    field: current_alarms
    name: Electricity Meter alarms
```

## Radio Configuration

### SX1276
For SX1276 radio you need to configure SPI instance as usual in ESPHome and additionally specify reset pin and IRQ pin (as DIO1). Interrupts are triggered on non empty FIFO.

### SX1262
For SX1262 radio, the configuration is similar but with additional options:

```yaml
wmbus_radio:
  radio_type: SX1262
  cs_pin: GPIO23
  reset_pin: GPIO4
  irq_pin: GPIO7
  busy_pin: GPIO19           # Optional but recommended for proper timing
  rx_gain: BOOSTED           # BOOSTED (default) or POWER_SAVING
  rf_switch: false           # Set to true if DIO2 controls RF switch
```

**SX1262-specific options:**
- `busy_pin`: Optional GPIO for BUSY signal. Recommended for reliable operation.
- `rx_gain`: RX gain mode - `BOOSTED` (better sensitivity, default) or `POWER_SAVING` (lower power)
- `rf_switch`: Set to `true` if your board uses DIO2 to control the RF switch

Tested on M5Stack Stamp C6LoRa (ESP32-C6). 

In order to pull latest wmbusmeters code run:
```bash
git subtree pull --prefix components/wmbus_common https://github.com/wmbusmeters/wmbusmeters.git <REF> --squash
```
