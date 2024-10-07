# Burger Pager

Burger Pager is a Flipper Zero app modified from the original `ble_spam` to work with pagers on SubGHz frequencies. It currently supports various protocols and features an intuitive interface for easy control and signal manipulation.

## Features

- **Automatic Signal Search and Exploit**: Automatically find and exploit all nearby pagers using the **Find and Bruteforce** mode.
- **Bruteforce Mode**: Set custom `station_id` and `pager_id` values to iterate through signals.
- **Custom Attack Duration**: Configure attack duration for precise control.
- **User-Friendly Interface**: Enhanced GUI for simplified navigation and control.

## Supported Pagers

| Pager Model          | Call | Turn Off | Tested |
|----------------------|------|----------|--------|
| Retekess TD112   | ✅   | ✅       | Yes    |
| Retekess TD157   | ✅   | ✅       | No    |
| Kromix W2270     | ✅   | ✅       | No    |
| Retekess TD158   | ✅   | ✅       | No    |

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

Enjoy responsibly testing pagers with Burger Pager!
