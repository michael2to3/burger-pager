# Burger Pager

Burger Pager is a Flipper Zero application modified from the original `ble_spam` app to work with pagers on SubGHz frequencies. It currently supports the Retekess TD112 protocol and features an intuitive interface for easy interaction.

## Features

- **Automatic Signal Search and Exploit**: Automatically find and spam all pagers nearby using the **Find and Bruteforce** mode.
- **Bruteforce Mode**: Set custom `station_id` and `pager_id` and iterate through signals.
- **Custom Attack Duration**: Adjust how long each attack should be executed.
- **User-Friendly Interface**: Enhanced GUI for simplified navigation and control.

## Supported Pagers

- **Retekess TD112**
- ~~Retekess TD157~~ (planned)
- ~~Kromix W2270~~ (planned)
- ~~Retekess TD158~~ (planned)

## How to Build and Run

1. **Clone Flipper Firmware**:
   ```bash
   git clone https://github.com/flipperdevices/flipperzero-firmware
   cd flipperzero-firmware
   ```

2. **Clone Burger Pager Repository**:
   ```bash
   git clone https://github.com/michael2to3/burger-pager applications_user/burger-pager
   ```

3. **Build and Launch**:
   ```bash
   ./fbt launch APPSRC=applications_user/burger-pager/
   ```

## References

- [Pagger](https://github.com/meoker/pagger)
- [BLE Spam](https://github.com/John4E656F/fl-BLE_SPAM)

Enjoy disrupting pagers responsibly!
