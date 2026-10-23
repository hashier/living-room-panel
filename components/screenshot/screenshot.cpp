#include "screenshot.h"

#include <cstring>

#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <lvgl.h>

#include "esphome/core/log.h"

namespace esphome::screenshot {

static const char *const TAG = "screenshot";
static const uint32_t BMP_HEADER_SIZE = 54;

void Screenshot::setup() {
  // A capture renders the screen and the top layer once each (measured 279 ms), only on request
  this->warn_if_blocking_over_ = 35;  // centiseconds
  this->done_ = xSemaphoreCreateBinary();
  this->base_->init();
  this->base_->add_handler(this);
}

bool Screenshot::canHandle(AsyncWebServerRequest *request) const {
  char url[AsyncWebServerRequest::URL_BUF_SIZE];
  return request->method() == HTTP_GET && request->url_to(url) == "/screenshot";
}

void Screenshot::handleRequest(AsyncWebServerRequest *request) {
  this->requested_ = true;
  httpd_req_t *req = *request;
  if (xSemaphoreTake(this->done_, pdMS_TO_TICKS(5000)) != pdTRUE || !this->ok_) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_send(req, "capture failed", HTTPD_RESP_USE_STRLEN);
    return;
  }
  httpd_resp_set_type(req, "image/bmp");
  httpd_resp_send(req, reinterpret_cast<const char *>(this->bmp_), this->bmp_size_);
}

void Screenshot::loop() {
  if (!this->requested_.exchange(false))
    return;
  this->ok_ = this->capture_();
  xSemaphoreGive(this->done_);
}

static void put_u16(uint8_t *p, uint16_t v) {
  p[0] = v & 0xFF;
  p[1] = v >> 8;
}

static void put_u32(uint8_t *p, uint32_t v) {
  for (int i = 0; i < 4; i++)
    p[i] = (v >> (8 * i)) & 0xFF;
}

bool Screenshot::capture_() {
  lv_draw_buf_t *screen = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_RGB565);
  if (screen == nullptr) {
    ESP_LOGW(TAG, "Snapshot of the screen failed");
    return false;
  }
  // The top layer (page dots, connection dot) is transparent apart from its widgets
  lv_draw_buf_t *top = lv_snapshot_take(lv_layer_top(), LV_COLOR_FORMAT_ARGB8888);

  const uint32_t w = screen->header.w;
  const uint32_t h = screen->header.h;
  const uint32_t row = w * 3;  // 24 bit, a multiple of 4 for the 480 px wide panel
  const size_t size = BMP_HEADER_SIZE + row * h;
  if (this->bmp_size_ != size) {
    heap_caps_free(this->bmp_);
    this->bmp_ = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
    this->bmp_size_ = this->bmp_ == nullptr ? 0 : size;
  }
  if (this->bmp_ == nullptr) {
    ESP_LOGW(TAG, "No memory for a %u byte image", (unsigned) size);
    lv_draw_buf_destroy(screen);
    if (top != nullptr)
      lv_draw_buf_destroy(top);
    return false;
  }

  // BITMAPFILEHEADER + BITMAPINFOHEADER; a negative height means rows run top to bottom
  uint8_t *p = this->bmp_;
  memset(p, 0, BMP_HEADER_SIZE);
  p[0] = 'B';
  p[1] = 'M';
  put_u32(p + 2, size);
  put_u32(p + 10, BMP_HEADER_SIZE);
  put_u32(p + 14, 40);
  put_u32(p + 18, w);
  put_u32(p + 22, static_cast<uint32_t>(-static_cast<int32_t>(h)));
  put_u16(p + 26, 1);
  put_u16(p + 28, 24);
  put_u32(p + 34, row * h);

  for (uint32_t y = 0; y < h; y++) {
    const auto *src = reinterpret_cast<const uint16_t *>(screen->data + y * screen->header.stride);
    const uint8_t *over = top == nullptr ? nullptr : top->data + y * top->header.stride;
    uint8_t *dst = p + BMP_HEADER_SIZE + y * row;
    for (uint32_t x = 0; x < w; x++) {
      uint16_t c = src[x];
      uint32_t r = ((c >> 11) & 0x1F) * 255 / 31;
      uint32_t g = ((c >> 5) & 0x3F) * 255 / 63;
      uint32_t b = (c & 0x1F) * 255 / 31;
      if (over != nullptr) {
        // ARGB8888 is stored as B, G, R, A in memory
        const uint8_t *o = over + x * 4;
        uint32_t a = o[3];
        b = (o[0] * a + b * (255 - a)) / 255;
        g = (o[1] * a + g * (255 - a)) / 255;
        r = (o[2] * a + r * (255 - a)) / 255;
      }
      dst[x * 3 + 0] = b;
      dst[x * 3 + 1] = g;
      dst[x * 3 + 2] = r;
    }
  }

  lv_draw_buf_destroy(screen);
  if (top != nullptr)
    lv_draw_buf_destroy(top);
  ESP_LOGD(TAG, "Captured %ux%u", (unsigned) w, (unsigned) h);
  return true;
}

}  // namespace esphome::screenshot
