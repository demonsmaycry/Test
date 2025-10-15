#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace infisolar_pi18 {

struct Frame {
  std::string ascii;
  std::vector<uint8_t> raw;
  std::vector<uint8_t> payload;
  char type{0};
  uint16_t declared_len{0};
  bool length_mismatch{false};
  bool has_crc{false};
  bool valid_crc{false};
  uint16_t received_crc{0};
  uint16_t computed_crc{0};
};

class InfisolarPI18Component : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_handshake_retry(uint8_t retry) { handshake_retry_ = retry; }

  const std::string &protocol_id() const { return protocol_id_; }
  const std::string &model_name() const { return model_name_; }

  void set_protocol_sensor(text_sensor::TextSensor *sensor) { protocol_sensor_ = sensor; }
  void set_model_sensor(text_sensor::TextSensor *sensor) { model_sensor_ = sensor; }

 protected:
  bool perform_handshake_();
  bool request_frame_(const std::string &command, Frame *frame);
  bool read_frame_(Frame *frame, uint32_t timeout_ms = 300);
  bool parse_frame_contents_(Frame *frame);
  void flush_input_();
  static uint16_t crc16_xmodem_(const std::vector<uint8_t> &data);
  static std::string payload_to_ascii_(const std::vector<uint8_t> &payload);
  std::string decode_handshake_payload_(size_t stage, const Frame &frame) const;

 private:
  std::string protocol_id_;
  std::string model_name_;
  uint8_t handshake_retry_{3};
  text_sensor::TextSensor *protocol_sensor_{nullptr};
  text_sensor::TextSensor *model_sensor_{nullptr};
};

}  // namespace infisolar_pi18
}  // namespace esphome

