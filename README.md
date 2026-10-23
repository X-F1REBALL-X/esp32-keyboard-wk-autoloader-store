# ESP32-S3 Keyboard for WK Autoloader Store

<p align="center">
  <img src="docs/board.jpg" alt="ESP32-S3 N16R8 board" width="420">
</p>

Firmware that turns an ESP32-S3 into a USB HID keyboard for the PS5. On every
boot it waits for the console to appear on your network, then presses
LEFT and ENTER to open the Store tile, which launches
[WK Autoloader Store](https://github.com/X-F1REBALL-X/wk-autoloader).

The board only sends key presses. It contains no exploit, hosts no pages and
sends no payloads.

## Hardware

- ESP32-S3 DevKitC-1 **N16R8** (16 MB flash, 8 MB OPI PSRAM)
- **UART / COM port:** flashing and serial logs from a PC
- **Native USB port:** connects to the PS5 (HID keyboard)

## Requirements

- WK Autoloader Store installed on the PS5 (it replaces the Store tile)
- A 2.4 GHz home Wi-Fi network; the PS5 on the same LAN (Wi-Fi or Ethernet)
- Automatic login on the PS5 is recommended for hands-free boots

## Flashing the release image

Download `ESP32S3_Keyboard_WK_Autoloader_Store_V1.0.0.bin` from the Releases
page. It is a merged 16 MB image and is flashed at address `0x0`. Connect the
board's **UART / COM** port to the PC.

**esptool (any OS):**

```bash
esptool.py --chip esp32s3 --port PORT --baud 921600 write_flash 0x0 ESP32S3_Keyboard_WK_Autoloader_Store_V1.0.0.bin
```

**ESP Flash Download Tool (Windows):**

1. Chip type `ESP32-S3`, work mode `Develop`, load mode `UART`.
2. Add the `.bin` file, tick it, set the address to `0x0`.
3. Select the COM port, baud 921600, SPI mode DIO, then click START.

After flashing, move the cable to the **native USB** port and plug it into the PS5.

## First setup

1. On a phone or computer, join the Wi-Fi network `AutoStore`
   (the password is set in `include/config.h`).
2. Open http://10.1.1.1 (use http, not https). Many devices open it
   automatically as a captive portal.
3. Enter your home Wi-Fi name and password and save. Console IP and board IP
   are optional; leave them blank for auto-detect and DHCP.
4. The board reboots and joins your network.

Do not connect the PS5 itself to `AutoStore`. The access point stays available,
so you can return to http://10.1.1.1 to change settings.

## How the key sequence works

After boot the board joins Wi-Fi and looks for the console on the LAN
(TCP ports 9295 / 41800). Once the console is found and at least 15 seconds
have passed since boot, it runs `data/config/nav_sequence.txt`:

```
0.3 LEFT
0.3 ENTER
```

Each line is `<delay_seconds> <KEY>`, where the delay is the wait after the
previous key. Lines starting with `#` are comments. Supported keys:
`ENTER`, `ESC`, `BACKSPACE`, `TAB`, `SPACE`, `UP`, `DOWN`, `LEFT`, `RIGHT`,
`PAGE_UP`, `PAGE_DOWN`.

If your Store tile is in a different position, edit the file and upload the
filesystem again with `pio run -t uploadfs`.

## Build from source

Requires [PlatformIO Core](https://platformio.org/).

```bash
pio run                                   # build firmware
pio run -t upload   --upload-port PORT    # flash firmware
pio run -t uploadfs --upload-port PORT    # flash LittleFS (nav_sequence.txt)
```

Do not commit `data/wifi.json`; the setup page writes it on the board.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Flasher cannot open the port | Use the UART/COM port; install the CH343 driver if needed |
| Board does not join home Wi-Fi | Use a 2.4 GHz network; reopen http://10.1.1.1 and save again |
| Keys do nothing | Native USB must be connected to the PS5; tune delays in `nav_sequence.txt` |
| Console is never found | Set the console IP on the setup page |

Serial logs: UART/COM port at 115200 baud.

## Credits

- [WK Autoloader Store](https://github.com/X-F1REBALL-X/wk-autoloader)
- Espressif Arduino-ESP32 and PlatformIO

## License

MIT License. See [LICENSE](LICENSE). Copyright (c) 2026 X-F1REBALL-X.
