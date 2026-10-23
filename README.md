# Living room panel

ESPHome config for a Guition ESP32-S3-4848S040 (4" 480×480 touch screen, no relay module) used as a
Home Assistant wall panel: heating, air quality, lights and screen settings.

<img src="screenshots/heating.png" width="240" alt="Heating"> <img src="screenshots/bedroom.png" width="240" alt="Bedroom">
<img src="screenshots/air.png" width="240" alt="Air"> <img src="screenshots/lights.png" width="240" alt="Lights">
<img src="screenshots/screen.png" width="240" alt="Screen">

Swipe left and right to change pages. The screenshots come from the panel itself (`GET /screenshot`, see
`components/screenshot`); the fine dust value, the sofa light being off and the bedroom
heating are made up for the pictures.

A small dim green dot in the corner means all is well. Without Home Assistant it turns yellow, without WiFi
red, and after 2 minutes the readings change to "--" so nobody trusts an old value:

<img src="screenshots/no-home-assistant.png" width="240" alt="No Home Assistant"> <img src="screenshots/no-wifi.png" width="240" alt="No WiFi, readings stale">

- Build and flash over WiFi: `esphome run panel.yaml --device OTA`
- First flash over USB (CH340, only reliable at 115200): `esphome run panel.yaml --device /dev/cu.usbserial-110 --upload_speed 115200`
- Logs over WiFi: `esphome logs panel.yaml --device OTA`, over USB: `esphome logs panel.yaml --device /dev/cu.usbserial-110`
- Back up the factory firmware before the first flash (16 MB, about 25 min at 115200):
  `esptool --port /dev/cu.usbserial-110 --baud 115200 read-flash 0 0x1000000 factory-flash.bin`,
  restore with `esptool --port /dev/cu.usbserial-110 --baud 115200 write-flash 0 factory-flash.bin`
