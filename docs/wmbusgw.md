# How to use `wmbusgw` with `wmbusmeters` HA addon

You need at least [wmbusmeters 1.11.0](https://github.com/weetmuts/wmbusmeters/releases/tag/1.11.0) version.

## 1. `wmbusgw` configuration
You can use UDP or TCP as transport layer.

UDP example is below:
```yaml
wmbusgw:
  clients:
    - name: "wmbusmeters_udp"
      ip_address: "10.1.2.3"
      port: 7011
      format: RTLWMBUS
      transport: UDP
```


TCP example is below:
```yaml
wmbusgw:
  clients:
    - name: "wmbusmeters_udp"
      ip_address: "10.1.2.3"
      port: 7011
      format: RTLWMBUS
      transport: TCP
```

  - **ip_address**: IP address of Home Assistant
  - **port**: port where telegrams will be sent
  - **format**: telegram format should be set to ``RTLWMBUS``


## 2. `wmbusmeters` addon configuration
After instalation please go to addon configuration and enable port forwarding by clicking `Show disabled ports`:
![](https://github.com/SzczepanLeon/esphome-components/blob/main/docs/disabled_ports.png)
On right side please write port number from `wmbusgw` configuration (in this example *7011*).


In `wmbusmeters` HA addon configuration please change device to:

UDP case:
```yaml
device=rtlwmbus:CMD(/usr/bin/nc -lku 9011)
```


TCP case:
```yaml
device=rtlwmbus:CMD(/usr/bin/nc -lk 9022)
```

Add meters:
```yaml
meters:  
  - |-  
    name=water_from_izar  
    driver=izar  
    id=123456  
    key=  
  - |-  
    name=water_from_apator  
    driver=apator162  
    id=9876543  
    key=00000000000000000000000000000000
```

Add MQTT configuration:
```yaml
mqtt:  
  host: 10.1.2.44  
  port: "1883"  
  user: myUser  
  password: myPass
```


## 3. Home Assistant configuration
Now you can add meter in HA:

```yaml
mqtt:
  sensor:
  - name: "Main water meter"
    unique_id: main_water_meter_total_m3
    object_id: main_water_meter_total_m3
    state_topic: "wmbusmeters/water_from_izar"
    value_template: "{{ value_json['total_m3']|default(0.000) }}"
    unit_of_measurement: m³
    device_class: water
    state_class: total_increasing
    icon: mdi:water
```

You can also enable and use MQTT auto discovery feature (very limited).
![](https://github.com/SzczepanLeon/esphome-components/blob/main/docs/mqtt_discovery.png)

And check discovered metters in MQTT integration.
