# ESP32-S3 PS5 AutoStore

<p align="center">
  <img src="docs/board.jpg" alt="ESP32-S3 N16R8 board" width="420">
</p>

USB HID board for PS5. Version `1.0.0`.

On every boot the board waits for the console, then sends LEFT and ENTER to
open the Store tile. Jailbreak runs on the console (your own Store host).
This firmware does not host an exploit page and does not send payloads.

USB HID keyboard only. No gamepad spoof. The board does not rewrite the Store
tile URL.

## Quick start

1. Download the flash file:
   [AutoStore-1.0.0.bin](https://github.com/X-F1REBALL-X/Esp32s3-Ps5-AutoStore/releases/download/v1.0.0/AutoStore-1.0.0.bin)

2. Flash it in Chrome or Edge with the
   [Espressif web flasher](https://espressif.github.io/esptool-js/):
   - Connect the board **UART / COM** port to the PC (not native USB).
   - Open the flasher, choose the COM port, baud 921600.
   - Add the file at flash address `0x0`.
   - Click Connect. Lines like `Detecting chip type... ESP32-S3`,
     `Stub running`, `Changing baudrate`, and `Flash ID` are normal.
   - Click Program. Flashing takes about 1 minute. Success looks like
     `Hash of data verified.` then `Leaving... Hard resetting via RTS pin...`.

3. Move the cable from the **UART / COM** port to the board **native USB**
   port, then plug that native USB port into the PS5. Do not use the UART /
   COM port on the console. (Optional: any USB power source also works for
   the Wi-Fi setup step below.)

4. Keep the PS5 on your home network (Ethernet cable or home Wi-Fi). Do not
   join the PS5 to the `AutoStore` Wi-Fi network. Setup is done from your
   phone or computer only:
   - On your phone or computer, join Wi-Fi `AutoStore` first (password
     `12345678`). Wait until it is connected.
   - Then open http://10.1.1.1 in the browser. That address is the board.
     Your phone or computer will get something like 10.1.1.2; that is not the
     setup page.
   - Many phones open the setup page automatically (captive portal). If not,
     type http://10.1.1.1 yourself. Use http, not https. If the phone says
     there is no internet, choose Stay connected / Use network anyway.
   - Enter your home Wi-Fi name and password, then save. Console IP is optional
     (leave blank to auto-detect). Board IP is optional (leave blank for DHCP).
   - The board reboots and joins your home network.

5. Every boot after that is automatic: the board joins your home Wi-Fi,
   finds the console and presses LEFT + ENTER on Store about 15 seconds
   after it is plugged in, both after a console reset and after a full
   restart. If the console is still starting, it presses as
   soon as the console is found.

### Notes

- Plugging the board into the PS5 by USB is normal and expected.
- Only the **board** needs Wi-Fi after setup. The console can use Wi-Fi or an
  Ethernet cable on the same LAN.
- Store on the console must already point to your jailbreak host.
- Automatic login on the PS5 helps hands-free boots.
- SoftAP `AutoStore` stays available so you can open http://10.1.1.1 again to
  change Wi-Fi settings.
- Console IP (if you set it): PS5 Settings > Network > Connection Status.

## Hardware

- ESP32-S3 DevKitC-1 N16R8 (16MB flash / 8MB OPI PSRAM)
- **UART / COM (CH343):** flash and serial from a PC
- **Native USB:** plug into the PS5 (HID keyboard)

## HID sequence

`data/config/nav_sequence.txt` (change only if Store focus differs):

```
0.3 LEFT
0.3 ENTER
```

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Flasher cannot open the port | Use UART/COM, not native USB; install CH343 driver if needed |
| Phone cannot join AutoStore | SoftAP password is `12345678`; board powered |
| Board does not join home Wi-Fi | Use 2.4 GHz SSID; reopen http://10.1.1.1 and save again |
| Keys do nothing | Native USB must be in the PS5; tune delays in nav_sequence |
| Console is never found | Set console IP on the setup page, or wait for auto-scan |

To see board logs, plug the UART/COM port into a PC and open a serial monitor at 115200.

## License

MIT. See `LICENSE`. Copyright (c) 2026 X-F1REBALL-X.

## Repo

Private: https://github.com/X-F1REBALL-X/Esp32s3-Ps5-AutoStore

## Build from source

Needs [PlatformIO Core](https://platformio.org/) (`pio`).

```bash
# Do not put secrets in the tree for a release image.
# Leave data/wifi.json out (setup page writes it on the board).

pio run
pio run -t buildfs

# Merged flash image (bootloader + partitions + app + LittleFS):
esptool.py --chip esp32s3 merge_bin -o AutoStore-1.0.0.bin \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0      .pio/build/esp32-s3-n16r8/bootloader.bin \
  0x8000   .pio/build/esp32-s3-n16r8/partitions.bin \
  0xe000   ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000  .pio/build/esp32-s3-n16r8/firmware.bin \
  0x610000 .pio/build/esp32-s3-n16r8/littlefs.bin
```

Optional CLI flash of the merged file:

```bash
esptool.py --chip esp32s3 --port PORT --baud 921600 write-flash 0x0 AutoStore-1.0.0.bin
```

Or with PlatformIO:

```bash
pio run -t upload --upload-port PORT
pio run -t uploadfs --upload-port PORT
```

Firmware version string: `1.0.0` (`AUTOSTORE_VERSION` in `platformio.ini`).
