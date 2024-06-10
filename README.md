# Hearth Attack Buzzer

This project is to replace our apartment's heart attack inducing door buzzer with a Matter compatible ESP32 device.

## Install ESP-IDF
https://github.com/espressif/esp-idf

### ESP-IDF Commit
Might be required but unsure, keeping it just in case: `6b1f40b9bfb91ec82fab4a60e5bfb4ca0c9b062f`.

## Notes

`depot_tools` need to be checked out in a parent directory. https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up

```
idf.py set-target esp32c6
idf.py menuconfig --style default
```

## Development setup

```bash
cd extlib/esp-idf
./install.fish
. export.fish
export ESPPORT=/dev/ttyACM0
```

## Useful Commands

https://learn.microsoft.com/en-us/windows/wsl/connect-usb#attach-a-usb-device

```powershell
# As administrator
usbipd bind -b 2-2 --force


usbipd attach --busid 2-2 --wsl --auto-attach
```

Should appear in WSL as `/dev/ttyASM0`.

```bash
sudo usermod -a -G dialout $USER
#sudo chown root:dialout /dev/ttyACM0
sudo chmod 666 /dev/ttyACM0
```
