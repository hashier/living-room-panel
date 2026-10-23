# Living room panel

ESPHome config for a Guition ESP32-S3-4848S040 (4" 480×480 touch screen, no relay module) used as a
Home Assistant wall panel: heating, air quality, lights and screen settings.

<img src="screenshots/hers/heating.png" width="240" alt="Heating, pink"> <img src="screenshots/hers/bedroom.png" width="240" alt="Bedroom, pink">
<img src="screenshots/hers/air.png" width="240" alt="Air, pink"> <img src="screenshots/hers/lights.png" width="240" alt="Lights, pink">

<img src="screenshots/heating.png" width="240" alt="Heating"> <img src="screenshots/bedroom.png" width="240" alt="Bedroom">
<img src="screenshots/air.png" width="240" alt="Air"> <img src="screenshots/lights.png" width="240" alt="Lights">
<img src="screenshots/screen.png" width="240" alt="Screen">

## Using it

- Swipe left and right to change pages.
- There are two page sets: a light pink one without the Screen page, which the panel starts with, and a
  dark one with all pages. A tap with three fingers switches between them; after 2 minutes without a
  touch the panel goes back to the pink one. The colours of both are in `theme.h`.
- Touches with two or more fingers never press a button.
- The dot in the top right corner is dim green while all is well, yellow without Home Assistant and red
  without WiFi. After 2 minutes without Home Assistant the readings change to "--":

<img src="screenshots/no-home-assistant.png" width="240" alt="No Home Assistant"> <img src="screenshots/no-wifi.png" width="240" alt="No WiFi, readings stale">

## Building

- Flash over WiFi: `esphome run panel.yaml --device OTA`
- First flash over USB: `esphome run panel.yaml --device <port> --upload_speed 115200`
  (the board's CH340 USB serial chip is only reliable at 115200)
- Logs: `esphome logs panel.yaml --device OTA`, or `--device <port>` over USB
- Back up the factory firmware before the first flash:
  `esptool --port <port> --baud 115200 read-flash 0 0x1000000 factory-flash.bin`,
  restore it with `esptool --port <port> --baud 115200 write-flash 0 factory-flash.bin`

## Previews

`python3 preview.py` builds the config for the computer instead of the panel and renders every page with
demo values into `.esphome/preview/`, the screenshots above included. `--set hers` or `--set mine` picks
the page set.

The panel itself serves its current screen at `GET /screenshot` (`components/screenshot`).
