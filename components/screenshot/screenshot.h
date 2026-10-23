#pragma once

#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"

namespace esphome::screenshot {

/// Serves GET /screenshot as a 24-bit BMP of the active LVGL screen plus the top layer.
///
/// HTTP requests arrive on the httpd task, but LVGL may only be used from the main loop.
/// The handler therefore only raises a flag and waits; loop() renders the snapshot and
/// signals back, then the handler sends the finished image.
class Screenshot : public Component, public AsyncWebHandler {
 public:
  explicit Screenshot(web_server_base::WebServerBase *base) : base_(base) {}

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  bool capture_();

  web_server_base::WebServerBase *base_;
  SemaphoreHandle_t done_{nullptr};
  std::atomic<bool> requested_{false};
  uint8_t *bmp_{nullptr};
  size_t bmp_size_{0};
  bool ok_{false};
};

}  // namespace esphome::screenshot
