# Living room panel

ESPHome config for a Guition ESP32-S3-4848S040 (4" 480×480 touch screen, no relay module) used as a
Home Assistant wall panel: heating, air quality, lights and screen settings.

- Build and flash over WiFi: `esphome run panel.yaml --device OTA`
- First flash over USB (CH340, only reliable at 115200): `esphome run panel.yaml --device /dev/cu.usbserial-110 --upload_speed 115200`
- Logs only work over WiFi (`esphome logs panel.yaml --device OTA`); the USB port goes to UART0, the logger to native USB.
- Factory firmware backup: `~/esp32-4848s040-backup/factory-flash-16MB.bin`
  (restore with `esptool --port /dev/cu.usbserial-110 --baud 115200 write-flash 0 <file>`)
