# Home Assistant integration

The ESPHome bridge defined in [`../esphome/example.yaml`](../esphome/example.yaml)
exposes two text sensors (`Infisolar Protocol` and `Infisolar Model`) using the
native ESPHome API. Once the node is added to Home Assistant, the entities are
created automatically and can be displayed in dashboards without additional
configuration.

If you want to create a compact diagnostic panel, copy the following snippet to
`configuration.yaml` and restart Home Assistant:

```yaml
template:
  - sensor:
      - name: "Infisolar Protocol"
        unique_id: infisolar_protocol_shadow
        state: "{{ states('text_sensor.infisolar_protocol') }}"
      - name: "Infisolar Model"
        unique_id: infisolar_model_shadow
        state: "{{ states('text_sensor.infisolar_model') }}"
  - binary_sensor:
      - name: "Infisolar Link"
        unique_id: infisolar_link_shadow
        state: "{{ not is_state('text_sensor.infisolar_protocol', 'unavailable') }}"
```

The helper sensors mirror the ESPHome entities and can be used in automations
or Lovelace cards alongside other devices.
