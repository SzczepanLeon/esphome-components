# Szczepan's esphome custom components

This repository contains a collection of my custom components
for [ESPHome](https://esphome.io/).

## 1. Usage

Use latest [ESPHome](https://esphome.io/)
with external components and add this to your `.yaml` definition:

```yaml
external_components:
  - source: github://SzczepanLeon/esphome-components@main
```

## 2. Components

### 2.1. `wmbusgw`

Component to receive wMBus frame (via CC1101) and send it via TCP or UDP to wmbusmeters (as HEX or rtl-wmbus format).

#### 2.1.1. Example

```yaml
time:
  - platform: sntp
    id: time_sntp

external_components:
  - source: github://SzczepanLeon/esphome-components@main
    components: [ wmbusgw ]

wmbusgw:
  mosi_pin: GPIO13
  miso_pin: GPIO5
  clk_pin:  GPIO2
  cs_pin:   GPIO14
  gdo0_pin: GPIO15
  gdo2_pin: GPIO16
  clients:
    - name: "wmbusmeters"
      ip_address: "10.0.0.1"
      port: 7227
      format: rtlwMBus
    - name: "tests"
      ip_address: "10.1.2.3"
      port: 6116
    - name: "tests"
      ip_address: "10.4.5.6"
      port: 7337
      transport: UDP
```

On client side you can use netcat to receive packets:
```bash
nc -lk 7227
```
or
```bash
nc -lku 7337
```
or add to wmbusmeters.conf:
```
device=rtlwmbus:CMD(nc -lk 7227)
```

Configuration variables:
------------------------

- **mosi_pin** (*Required*): CC1101 MOSI pin connection.
- **miso_pin** (*Required*): CC1101 MISO pin connection.
- **clk_pin** (*Required*): CC1101 CLK pin connection.
- **cs_pin** (*Required*): CC1101 CS pin connection.
- **gdo0_pin** (*Required*): CC1101 GDO0 pin connection.
- **gdo2_pin** (*Required*): CC1101 GDO2 pin connection.
- **reboot_timeout** (*Optional*): The amount of time to wait before rebooting when no data from CC1101 is received or no packets are transmited over TCP to clients. Defaults to ``3min``.
- **clients** (*Optional*):
  - **name** (*Required*): The name for this client.
  - **ip_address** (*Required*): IP address.
  - **port** (*Required*): Port number.
  - **format** (*Optional*): Telegram format to send. HEX or RTLWMBUS. Defaults to ``HEX``.
  - **transport** (*Optional*): TCP or UDP. Defaults to ``TCP``.

#### 2.1.2. wmbusmeters HA addon 
You can also use this component with wmbusmeters HA addon:
https://github.com/SzczepanLeon/esphome-components/blob/main/docs/wmbusgw.md

## 3. Author & License

Szczepan, MIT, 2022
