# Burger Pager

<div align="center">
   <img src=".readme/demo.gif"/>
</div>

Burger Pager is a Flipper Zero application that allows you to interact with restaurant pager systems using Sub-GHz frequencies. This tool can find, exploit, and send signals to various pagers, enabling you to test the security and functionality of these devices.

## Features

- **Automatic Signal Detection and Exploitation**: Use the **Find and Bruteforce** mode to automatically search for nearby pager signals and perform actions such as calling or turning off pagers.
- **Mass Attack Mode**: Initiate automatic packet capture and exploitation across all targets. This mode automatically detects and captures all pager signals in the vicinity, then performs a comprehensive attack on each detected signal, automating the process of signal hijacking and pager exploitation.
- **Bruteforce Mode**: Manually set custom `station_id` and `pager_id` values to iterate through possible signals and test pagers.
- **Customizable Attack Duration**: Configure the duration of attacks for precise control over signal transmission.
- **User-Friendly Interface**: An intuitive graphical user interface makes navigation and control straightforward.

## Supported Pager Models

The application currently supports the following pager models:

| Pager Model          | Call Functionality | Turn Off Functionality | Tested |
|----------------------|--------------------|------------------------|--------|
| Retekess TD112       | ✅                 | ✅                     | ✅     |
| Retekess TD157       | ✅                 | ✅                     | ❌     |
| Kromix W2270         | ✅                 | ✅                     | ❌     |
| Retekess TD158       | ✅                 | ✅                     | ❌     |

## Installation and Usage

### Prerequisites

- Flipper Zero device
- Flipper Zero Firmware source code

### Building the Application

1. **Clone the Flipper Zero Firmware Repository**

   Open a terminal and run the following commands:

   ```bash
   git clone https://github.com/flipperdevices/flipperzero-firmware
   cd flipperzero-firmware
   ```

2. **Add the Burger Pager Application**

   Clone the Burger Pager repository into the `applications_user` directory:

   ```bash
   git clone https://github.com/michael2to3/burger-pager applications_user/burger-pager
   ```

3. **Build the Firmware with the Burger Pager Application**

   Build and launch the firmware with the added application:

   ```bash
   ./fbt launch APPSRC=applications_user/burger-pager/
   ```

   This command will compile the firmware with the Burger Pager app included and launch it on your Flipper Zero device.

### Running the Application

1. **Navigate to the Application**

   On your Flipper Zero device, navigate to the `Applications` menu and select `Burger Pager`.

2. **Using the Features**

   - **Find and Bruteforce Mode**: Select this mode to automatically search for pager signals and perform actions.
   - **Bruteforce Mode**: Manually configure `station_id` and `pager_id` to test specific signal combinations.
   - **Adjust Attack Duration**: Use the settings menu to set the desired duration for signal transmission.

3. **Stopping the Application**

   To stop any ongoing operations, use the `Stop` option within the application or press the back button on your device.

## References

- [Pagger](https://github.com/meoker/pagger): An inspiration for interacting with pager systems.
- [BLE Spam](https://github.com/John4E656F/fl-BLE_SPAM): Original project modified to create Burger Pager.

## Contributing

Contributions are welcome! If you have suggestions for improvements or new features, please open an issue or submit a pull request.
