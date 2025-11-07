# Omnidirectional Smart Car Project - MicroBots Hackathon

## Overview
This project uses a CodeCell (runs on ESP32-C3) to run a mechanum-driven mini robot buddy. It features a web interface that allows users to control a device via joystick inputs. The buddy also has emotions based on various variables/environmental disturbances such as shaking.

> [!NOTE]
> This project does not contain the OLED display code. This was removed for demo and physical design purposes but may get added at a later date.

## Project Structure
```
my-arduino-project
├── src
│   ├── main.ino        # Main Arduino code with web server setup
│   └── index.html      # HTML code for the web interface
└── README.md           # Project documentation
```

## Files Description
- 'src/main.ino: Contains the main logic for the Arduino, including the structure definition for incoming data, web server setup, and request handling.
- **src/index.html**: Contains the HTML layout and styling for the Buddy Controller, along with JavaScript functions for joystick input handling.

## Setup Instructions
1. Clone the repository to your local machine.
2. Open the project in your Arduino IDE.
3. Upload the `main.ino` file to your microcontroller.
4. Connect to the device's Wi-Fi network.
5. Open a web browser and navigate to the device's IP address with port **80**.

## Usage
- Use the joystick on the web interface to control the device.
- The buttons allow for additional commands such as entering full screen and resetting the heading.

## License
This project is licensed under the MIT License.
