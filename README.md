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

A component to receive wMBus frame (via CC1101) and send it via TCP to wmbusmeters (as HEX or rtl-wmbus format).

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
```

On client side you can use netcat to receive packets:
```bash
nc -lk 7227
```
or add to wmbusmeters.conf:
```
device=rtlwmbus:CMD(nc -lk 7227)
```

## 3. Author & License

Szczepan, MIT, 2022
