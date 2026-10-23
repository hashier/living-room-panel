"""Render the panel's pages on this computer, without the device.

Builds panel.yaml (or another config given as argument) for ESPHome's host platform, with the
hardware parts swapped for an in-memory display, fills in fixed demo values, and writes one PNG
per page to .esphome/preview/<config name>/. Only the look: no touch, no Home Assistant.

    python3 preview.py                      # panel.yaml, all pages
    python3 preview.py other.yaml --brightness 100 --dim 5
"""
import argparse
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import time
import zlib

REPO = pathlib.Path(__file__).resolve().parent
# LVGL page id -> file name (the Air page's id is living_room)
PAGES = {"heating": "heating", "bedroom": "bedroom", "living_room": "air", "lights": "lights", "screen": "screen"}
NAME = "panel-preview"  # own build folder, so the device's build cache stays intact

# Demo values, the same ones the README screenshots show
FIXTURE = """
id(room_temp).publish_state(21.4);
id(target_temp).publish_state(21.0);
id(target_offset).publish_state(0.0);
id(lr_co2).publish_state(715);
id(lr_pm25).publish_state(8);
id(lr_humidity).publish_state(54);
id(bed_temp).publish_state(18.3);
id(bed_target).publish_state(19.0);
id(bed_humidity).publish_state(55);
id(heating_action).publish_state("idle");
id(bed_action).publish_state("heating");
id(st_win_left).publish_state(true);
id(st_win_right).publish_state(true);
id(st_center).publish_state(true);
id(st_sofa).publish_state(false);
id(dim_after).make_call().set_value(60).perform();
id(off_after_dim).make_call().set_value(120).perform();
id(screensaver_enabled).turn_off();
id(dim_level).make_call().set_value({dim}).perform();
auto call = id(display_backlight).turn_on();
call.set_brightness({brightness} / 100.0f);
call.set_transition_length(0);
call.perform();
"""


def replace_once(text, old, new):
    assert text.count(old) == 1, f"expected once in panel config: {old!r}"
    return text.replace(old, new)


def host_config(text, brightness, dim):
    # Top-level sections, keyed by name
    parts = re.split(r"(?m)^(?=[a-z0-9_]+:)", text)
    sections = {re.match(r"[a-z0-9_]+", p).group(0): p for p in parts if re.match(r"[a-z0-9_]+:", p)}

    for hardware in ("esp32", "psram", "ota", "wifi", "i2c", "spi", "touchscreen", "external_components", "screenshot"):
        sections.pop(hardware)
    sections["host"] = "host:\n\n"
    sections["logger"] = "logger:\n\n"
    sections["api"] = "api:\n\n"  # Home Assistant sensors need it; no key, nothing connects
    sections["output"] = (
        "output:\n  - platform: template\n    id: backlight_output\n    type: float\n"
        "    write_action:\n      - lambda: (void) state;\n\n"
    )
    sections["display"] = (
        "display:\n  - platform: snapshot\n    id: tft_display\n    dimensions: 480x480\n"
        "    update_interval: never\n\n"
    )
    sections["sensor"] = replace_once(
        sections["sensor"], "  - platform: wifi_signal\n    name: WiFi signal\n    update_interval: 60s\n", "")
    # Show the connected state
    interval = replace_once(sections["interval"],
                            "bool wifi_ok = wifi::global_wifi_component->is_connected();", "bool wifi_ok = true;")
    sections["interval"] = replace_once(interval,
                                        "bool ha_ok = wifi_ok && api::global_api_server->is_connected();",
                                        "bool ha_ok = true;")

    esphome = re.sub(r'(?m)^  name: .*$', f'  name: "{NAME}"', sections["esphome"], count=1)
    sections["esphome"] = replace_once(esphome, "      - script.execute: sync_screen_settings\n",
                                       "      - script.execute: sync_screen_settings\n"
                                       "      - script.execute: preview_run\n")

    fixture = "".join(f"          {line}\n" for line in FIXTURE.format(brightness=brightness, dim=dim).strip().splitlines())
    shots = "".join(f"      - lvgl.page.show: {p}\n      - delay: 300ms\n"
                    f'      - snapshot.take: {{ id: tft_display, filename: "{p}" }}\n' for p in PAGES)
    sections["script"] = replace_once(sections["script"], "script:\n", (
        "script:\n  - id: preview_run\n    then:\n      - delay: 500ms\n      - lambda: |-\n"
        f"{fixture}      - delay: 300ms\n{shots}      - lambda: exit(0);\n"))
    return "".join(sections.values())


def bmp_to_png(src, dst):
    d = src.read_bytes()
    offset = struct.unpack_from("<I", d, 10)[0]
    width, height = struct.unpack_from("<ii", d, 18)
    row = width * 3
    order = range(-height) if height < 0 else range(height - 1, -1, -1)  # negative height: top-down
    raw = b"".join(b"\x00" + bytes(c for i in range(0, row, 3) for c in (r[i + 2], r[i + 1], r[i]))
                   for r in (d[offset + y * row:offset + (y + 1) * row] for y in order))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    dst.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, abs(height), 8, 2, 0, 0, 0))
                    + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("config", nargs="?", default=REPO / "panel.yaml", type=pathlib.Path)
    parser.add_argument("--brightness", type=int, default=100, help="screen brightness slider, 5-100")
    parser.add_argument("--dim", type=int, default=20, help="dim brightness slider, 5-100")
    parser.add_argument("--out", type=pathlib.Path, help="output folder (default .esphome/preview/<config name>)")
    args = parser.parse_args()

    work = REPO / ".esphome" / "preview"
    work.mkdir(parents=True, exist_ok=True)
    yaml = work / "preview.yaml"
    yaml.write_text(host_config(args.config.read_text(), args.brightness, args.dim))
    out = args.out or work / args.config.stem
    out.mkdir(parents=True, exist_ok=True)

    esphome = shutil.which("esphome") or str(pathlib.Path.home() / ".local/bin/esphome")
    build = subprocess.run([esphome, "compile", str(yaml)], capture_output=True, text=True)
    if build.returncode:
        sys.exit(build.stdout[-4000:] + build.stderr[-4000:])

    snapshots = work / ".esphome" / "snapshots" / NAME
    shutil.rmtree(snapshots, ignore_errors=True)
    program = work / ".esphome" / "build" / NAME / ".pioenvs" / NAME / "program"
    # The api component keeps it running if exit(0) is never reached
    subprocess.run([program], capture_output=True, timeout=30)

    for page, name in PAGES.items():
        bmp_to_png(snapshots / f"{page}.bmp", out / f"{name}.png")
    print(out)


if __name__ == "__main__":
    start = time.monotonic()
    main()
    print(f"{time.monotonic() - start:.1f} s", file=sys.stderr)
