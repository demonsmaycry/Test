#include "infisolar_pi18.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace esphome {
namespace infisolar_pi18 {

static const char *const TAG = "infisolar.pi18";

namespace {

std::string bytes_to_hex(const std::vector<uint8_t> &data) {
  std::string out;
  out.reserve(data.size() * 3);
  for (size_t i = 0; i < data.size(); ++i) {
    if (i != 0) {
      out.push_back(' ');
    }
    char buf[4];
    snprintf(buf, sizeof(buf), "%02X", data[i]);
    out.append(buf);
  }
  return out;
}

}  // namespace

void InfisolarPI18Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Infisolar PI18 UART bridge");
  if (!this->perform_handshake_()) {
    ESP_LOGE(TAG, "Initial handshake failed - check wiring and inverter state");
  }

  if (this->protocol_sensor_ != nullptr && !this->protocol_id_.empty()) {
    this->protocol_sensor_->publish_state(this->protocol_id_);
  }
  if (this->model_sensor_ != nullptr && !this->model_name_.empty()) {
    this->model_sensor_->publish_state(this->model_name_);
  }
}

void InfisolarPI18Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Infisolar PI18:");
  ESP_LOGCONFIG(TAG, "  Protocol ID: %s", this->protocol_id_.empty() ? "(unknown)" : this->protocol_id_.c_str());
  ESP_LOGCONFIG(TAG, "  Model name: %s", this->model_name_.empty() ? "(unknown)" : this->model_name_.c_str());
}

void InfisolarPI18Component::update() {
  if (this->protocol_id_.empty() || this->model_name_.empty()) {
    ESP_LOGW(TAG, "Handshake data missing, retrying identification");
    this->perform_handshake_();
    if (this->protocol_sensor_ != nullptr && !this->protocol_id_.empty()) {
      this->protocol_sensor_->publish_state(this->protocol_id_);
    }
    if (this->model_sensor_ != nullptr && !this->model_name_.empty()) {
      this->model_sensor_->publish_state(this->model_name_);
    }
  }
  // Polling for live telemetry can be implemented here once the register map is mapped.
}

bool InfisolarPI18Component::perform_handshake_() {
  static const char *const HANDSHAKE_STAGE[] = {"protocol", "model"};
  const std::string commands[] = {"^P005PI", "^P006GMN"};
  std::string *targets[] = {&this->protocol_id_, &this->model_name_};

  for (size_t stage = 0; stage < 2; ++stage) {
    Frame frame;
    bool success = false;
    for (uint8_t attempt = 0; attempt < this->handshake_retry_; ++attempt) {
      ESP_LOGI(TAG, "[HS] TX %s (stage=%zu attempt=%u)", commands[stage].c_str(), stage + 1, attempt + 1);
      if (!this->request_frame_(commands[stage], &frame)) {
        ESP_LOGW(TAG, "[HS] Timeout waiting response (stage=%zu attempt=%u)", stage + 1, attempt + 1);
        continue;
      }

      ESP_LOGD(TAG, "[HS] RX ascii: %s", payload_to_ascii_(frame.raw).c_str());
      ESP_LOGD(TAG, "[HS] RX raw HEX: %s", bytes_to_hex(frame.raw).c_str());
      ESP_LOGD(TAG, "[HS] RX payload HEX: %s", bytes_to_hex(frame.payload).c_str());
      ESP_LOGD(TAG, "[HS] RX payload ASCII: %s", payload_to_ascii_(frame.payload).c_str());
      if (frame.has_crc) {
        ESP_LOGD(TAG,
                 "[HS] CRC reported=%02X%02X computed=%02X%02X", 
                 frame.received_crc >> 8,
                 frame.received_crc & 0xFF,
                 frame.computed_crc >> 8,
                 frame.computed_crc & 0xFF);
      }
      if (frame.has_crc && !frame.valid_crc) {
        ESP_LOGE(TAG,
                 "[HS] CRC mismatch: got %02X %02X expected %02X %02X",
                 frame.received_crc >> 8,
                 frame.received_crc & 0xFF,
                 frame.computed_crc >> 8,
                 frame.computed_crc & 0xFF);
        continue;
      }

      if (frame.length_mismatch) {
        ESP_LOGW(TAG,
                 "[HS] Declared payload length %u but received %u bytes",
                 frame.declared_len,
                 static_cast<unsigned>(frame.payload.size()));
      }

      if (frame.payload.empty()) {
        ESP_LOGW(TAG, "[HS] Empty payload received at stage %zu", stage + 1);
        continue;
      }

      const std::string payload_ascii = this->decode_handshake_payload_(stage, frame);
      *targets[stage] = payload_ascii;
      ESP_LOGI(TAG,
               "[HS] %s detected: %s (type=%c len=%u)",
               HANDSHAKE_STAGE[stage],
               payload_ascii.c_str(),
               frame.type == 0 ? '?' : frame.type,
               frame.declared_len);
      if (stage == 0 && this->protocol_sensor_ != nullptr) {
        this->protocol_sensor_->publish_state(*targets[stage]);
      } else if (stage == 1 && this->model_sensor_ != nullptr) {
        this->model_sensor_->publish_state(*targets[stage]);
      }
      success = true;
      break;
    }

    if (!success) {
      return false;
    }
  }

  return true;
}

bool InfisolarPI18Component::request_frame_(const std::string &command, Frame *frame) {
  this->flush_input_();
  this->write_array(reinterpret_cast<const uint8_t *>(command.data()), command.size());
  this->write_byte('\r');
  this->flush();
  return this->read_frame_(frame);
}

bool InfisolarPI18Component::read_frame_(Frame *frame, uint32_t timeout_ms) {
  frame->raw.clear();
  frame->ascii.clear();
  frame->payload.clear();
  frame->type = 0;
  frame->declared_len = 0;
  frame->length_mismatch = false;
  frame->has_crc = false;
  frame->valid_crc = false;
  frame->received_crc = 0;
  frame->computed_crc = 0;

  const uint32_t start = millis();
  while ((millis() - start) < timeout_ms) {
    if (!this->available()) {
      delay(5);
      continue;
    }
    const uint8_t byte = this->read();
    frame->raw.push_back(byte);
    if (byte == '\r') {
      break;
    }
  }

  if (frame->raw.empty() || frame->raw.back() != '\r') {
    return false;
  }

  frame->raw.pop_back();  // strip trailing CR
  if (frame->raw.empty()) {
    return false;
  }

  if (frame->raw.back() == 0x03) {
    // ETX-terminated frame with no CRC
    frame->raw.pop_back();
  } else if (frame->raw.size() >= 3) {
    frame->has_crc = true;
    frame->received_crc = (static_cast<uint16_t>(frame->raw[frame->raw.size() - 2]) << 8) |
                          static_cast<uint16_t>(frame->raw[frame->raw.size() - 1]);
    frame->raw.resize(frame->raw.size() - 2);
    frame->computed_crc = crc16_xmodem_(frame->raw);
    frame->valid_crc = frame->computed_crc == frame->received_crc;
  }

  frame->ascii.assign(frame->raw.begin(), frame->raw.end());
  if (!this->parse_frame_contents_(frame)) {
    ESP_LOGW(TAG, "[HS] Unable to decode frame body: %s", payload_to_ascii_(frame->raw).c_str());
    return false;
  }

  return true;
}

void InfisolarPI18Component::flush_input_() {
  while (this->available()) {
    this->read();
  }
}

uint16_t InfisolarPI18Component::crc16_xmodem_(const std::vector<uint8_t> &data) {
  uint16_t crc = 0x0000;
  for (uint8_t byte : data) {
    crc ^= static_cast<uint16_t>(byte) << 8;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x8000) {
        crc = static_cast<uint16_t>(((crc << 1) ^ 0x1021) & 0xFFFF);
      } else {
        crc = static_cast<uint16_t>((crc << 1) & 0xFFFF);
      }
    }
  }
  return crc;
}

bool InfisolarPI18Component::parse_frame_contents_(Frame *frame) {
  if (frame->raw.size() < 5 || frame->raw[0] != '^') {
    return false;
  }

  frame->type = static_cast<char>(frame->raw[1]);

  const auto is_digit = [](uint8_t value) { return std::isdigit(static_cast<unsigned char>(value)) != 0; };
  if (!is_digit(frame->raw[2]) || !is_digit(frame->raw[3]) || !is_digit(frame->raw[4])) {
    return false;
  }

  frame->declared_len = static_cast<uint16_t>((frame->raw[2] - '0') * 100 + (frame->raw[3] - '0') * 10 +
                                               (frame->raw[4] - '0'));
  const size_t available = frame->raw.size() - 5;
  if (available < frame->declared_len) {
    frame->length_mismatch = true;
  }

  const size_t payload_len = std::min<size_t>(frame->declared_len, available);
  frame->payload.assign(frame->raw.begin() + 5, frame->raw.begin() + 5 + payload_len);
  return true;
}

std::string InfisolarPI18Component::payload_to_ascii_(const std::vector<uint8_t> &payload) {
  std::string ascii;
  ascii.reserve(payload.size());
  for (uint8_t byte : payload) {
    if (byte >= 0x20 && byte <= 0x7E) {
      ascii.push_back(static_cast<char>(byte));
    } else {
      ascii.push_back('?');
    }
  }
  return ascii;
}

std::string InfisolarPI18Component::decode_handshake_payload_(size_t stage, const Frame &frame) const {
  std::string ascii = payload_to_ascii_(frame.payload);

  if (stage == 0) {
    if (!ascii.empty() && ascii.back() == ';') {
      ascii.pop_back();
    }
    if (!ascii.empty() && ascii.rfind("PI", 0) != 0) {
      ascii.insert(0, "PI");
    } else if (ascii.empty()) {
      ascii = "PI";
    }
  }

  return ascii;
}

}  // namespace infisolar_pi18
}  // namespace esphome

