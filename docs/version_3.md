# Szczepan's esphome custom components

This repository contains a collection of my custom components
for [ESPHome](https://esphome.io/).

[!["Buy Me A Coffee"](https://github.com/SzczepanLeon/esphome-components/blob/main/docs/buy_me_coffe.png)](https://www.buymeacoffee.com/szczepan)
\
[!["Kup mi kawę"](https://github.com/SzczepanLeon/esphome-components/blob/main/docs/postaw_kawe.png)](https://buycoffee.to/szczepanleon)


## 1. Usage

Use latest [ESPHome](https://esphome.io/)
with external components and add this to your `.yaml` definition:

```yaml
external_components:
  - source: github://SzczepanLeon/esphome-components@3.2.3
```


## 2. Components

### 2.1. `wmbus`

Component to receive wMBus frame (via CC1101), create HA sensor and send decoded value.
You can also use this component with wmbusmeters HA addon:
https://github.com/SzczepanLeon/esphome-components/blob/main/docs/wmbus.md

[!["CC1101 to D1 mini PCB"](https://github.com/SzczepanLeon/esphome-components/blob/main/docs/pcb_v2.png)](https://www.pcbway.com/project/shareproject/CC1101_to_ESP_D1_mini_277f34e1.html)


#### 2.1.1. Example

```yaml
time:
  - platform: sntp
    id: time_sntp

external_components:
  - source: github://SzczepanLeon/esphome-components@3.2.3
    components: [ wmbus ]

wmbus:
  frequency: 434.475
  mosi_pin: GPIO13
  miso_pin: GPIO5
  clk_pin:  GPIO2
  cs_pin:   GPIO14
  gdo0_pin: GPIO15
  gdo2_pin: GPIO16

  led_pin: GPIO0
  led_blink_time: "1s"

  clients:
    - name: "wmbusmeters"
      ip_address: "10.0.0.1"
      port: 7227

sensor:
  - platform: wmbus
    meter_id: 0x24202020
    type: apator162
    key: "00000000000000000000000000000000"
    lqi:
      name: "My lqi"
    rssi:
      name: "My RSSI"
    total_water_m3:
      name: "My cold water"

  - platform: wmbus
    meter_id: 0x12345678
    type: amiplus
    mode: T1C1
    rssi:
      name: "My RSSI"
    total_energy_consumption_kwh:
      name: "My consumption in kWh"
      icon: "mdi:power-plug"
    voltage_at_phase_1_v:
      name: "My V1"

  - platform: wmbus
    meter_id: 0xAB125432
    type: bmeters
    key: "00004000000300000500001000000600"
    lqi:
      name: "My lqi"
    total_water_m3:
      id: "my_hot_water"
      filters:
        - offset: 325.0
```

> **_NOTE:_**  For Apator16-2 water meter please add also text sensor (for debug purposes):

```
text_sensor:
  - platform: wmbus
    name: "Text debug for Apator 16-2"
```


Configuration variables:
------------------------

In wmbus platform:

- **frequency** (*Optional*): CC1101 Rx frequency in MHz. Defaults to ``868.950 MHz``.
- **sync_mode** (*Optional*): If you have problems with MQTT set it to True. Defaults to ``False``.
- **mosi_pin** (*Optional*): CC1101 MOSI pin connection. Defaults to ``GPIO13``.
- **miso_pin** (*Optional*): CC1101 MISO pin connection. Defaults to ``GPIO12``.
- **clk_pin** (*Optional*): CC1101 CLK pin connection. Defaults to ``GPIO14``.
- **cs_pin** (*Optional*): CC1101 CS pin connection. Defaults to ``GPIO2``.
- **gdo0_pin** (*Optional*): CC1101 GDO0 pin connection. Defaults to ``GPIO5``.
- **gdo2_pin** (*Optional*): CC1101 GDO2 pin connection. Defaults to ``GPIO4``.
- **led_pin** (*Optional*): Pin where LED is connected. It will blink on each telegram. You can use all options from [Pin Schema](https://esphome.io/guides/configuration-types.html#config-pin-schema).
- **led_blink_time** (*Optional*): How long LED will stay ON. Defaults to ``300 ms``.
- **log_unknown** (*Optional*): Show telegrams from not configured meters in log. Defaults to ``True``.
- **clients** (*Optional*):
  - **name** (**Required**): The name for this client.
  - **ip_address** (**Required**): IP address.
  - **port** (**Required**): Port number.
  - **format** (*Optional*): Telegram format to send. HEX or RTLWMBUS. Defaults to ``RTLWMBUS``.
  - **transport** (*Optional*): TCP or UDP. Defaults to ``TCP``.



Meter/sensor:

- **meter_id** (*Optional*, int): Meter ID. Can be specified as decimal or hex. Defaults to ``0``.
- **type** (*Optional*, string):  Meter type. Currently `amiplus`, `apator08`, `apator162`, `apatoreitn`, `bmeters`, `c5isf`, `compact5`, `dme07`, `elf`, `evo868`, `fhkvdataiii`, `flowiq2200`, `hydrocalm3`, `hydrodigit`, `hydrus`, `iperl`, `itron`, `izar`, `kamheat`, `mkradio3`, `mkradio4`, `mkradio4a`, `multical21`, `qheat`, `qwater`, `sharky774`, `topaseskr`, `ultrimis`, `unismart`, `vario451` are supported. Defaults to ``unknown``.
- **key** (*Optional*): Key for meter, used in payload decoding process. Defaults to ``""``.
- **add_prefix** (*Optional*): Add prefix (meter_id) to sensor name. Defaults to ``True``.
- **mode** (*Optional*): Frame to process (T1, C1 or both). Defaults to ``T1``.
- **sensor_type** (*Optional*): Sensor type from list below. For example ``total_water_m3``
  - **id** (*Optional*, string): Manually specify the ID for code generation. At least one of **id** and **name** must be specified.
  - **name** (*Optional*, string): The name for the sensor. At least one of **id** and **name** must be specified.
  - All other options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor).


------------------------

Supported sensors (sensor_type) for meters:

- `amiplus`
  - total_energy_consumption_kwh
  - current_power_consumption_kw
  - total_energy_production_kwh
  - current_power_production_kw
  - voltage_at_phase_1_v
  - voltage_at_phase_2_v
  - voltage_at_phase_3_v
- `apatoreitn`
  - current_hca
  - previous_hca
  - temp_room_avg_c
- `apator08`
  - total_water_m3
- `apator162`
  - total_water_m3
- `bmeters`
  - total_water_m3
- `c5isf`
  - total_heating_kwh
  - total_water_m3
  - flow_temperature_c
  - return_temperature_c
  - power_kw
  - volume_flow_lh
- `compact5`
  - current_heating_kwh
  - previous_heating_kwh
- `dme07`
  - total_water_m3
- `elf`
  - total_energy_consumption_kwh
  - current_power_consumption_kw
  - total_water_m3
- `evo868`
  - total_water_m3
- `fhkvdataiii`
  - current_hca
  - previous_hca
  - temp_room_c
  - temp_radiator_c
- `flowiq2200`
  - total_water_m3
  - target_water_m3
  - volume_flow_lh
  - max_flow_lh
  - status
- `hydrocalm3`
  - total_heating_kwh
  - total_water_m3
- `hydrodigit`
  - total_water_m3
- `hydrus`
  - total_water_m3
- `itron`
  - total_water_m3
- `izar`
  - total_water_m3
  - last_month_total_water_m3
  - current_month_total_water_l
  - transmit_period_s
  - remaining_battery_life_y
  - current_alarms
  - previous_alarms
- `kamheat`
  - total_energy_consumption_kwh
  - volume_flow_lh
  - power_kw
  - flow_temperature_c
  - return_temperature_c
- `mkradio3`
  - total_water_m3
- `mkradio4`
  - total_water_m3
- `mkradio4a`
  - target_water_m3
- `multical21`
  - total_water_m3
  - target_water_m3
  - flow_temperature_c (depends on meter configuration)
  - external_temperature_c (depends on meter configuration)
- `qheat`
  - total_energy_consumption_kwh
- `qwater`
  - total_water_m3
- `rfmtx1`
  - total_water_m3
- `sharky774`
  - total_energy_consumption_gj
  - total_energy_consumption_kwh
  - power_kw
  - total_volume_m3
  - volume_flow_lh
  - flow_temperature_c
  - return_temperature_c
  - operating_time_d
- `topaseskr`
  - total_water_m3
- `ultrimis`
  - total_water_m3
- `unismart`
  - total_gas_m3
- `vario451`
  - total_heating_kwh
  - total_heating_gj


## 3. Author & License

Szczepan, GPL, 2022-2024
