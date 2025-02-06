#include <WiFi.h>
#include <esp_now.h>
#include <CodeCell.h>
#include <DriveCell.h>
#include <math.h>
#include <WebServer.h>
#include "web_controller.h"

// Motor Position Key
// 0 - Front Left
// 1 - Front Right
// 2 - Back Left
// 3 - Back Right

#define MOTOR0_MODIFIER -1
#define MOTOR1_MODIFIER 1
#define MOTOR2_MODIFIER -1
#define MOTOR3_MODIFIER -1

#define MOTOR0_PIN1 3
#define MOTOR0_PIN2 2

#define MOTOR1_PIN1 6
#define MOTOR1_PIN2 5

#define MOTOR2_PIN1 7
#define MOTOR2_PIN2 9

#define MOTOR3_PIN1 1
#define MOTOR3_PIN2 8

bool currentlyDriving = false;


int driveMotor(int motorId, float speed);
void mecanumDrive(float x, float y, float rotation, boolean fieldOriented);

int stop();

int getRotation();

DriveCell frontLeftDriveCell(MOTOR0_PIN1, MOTOR0_PIN2);
DriveCell frontRightDriveCell(MOTOR1_PIN1, MOTOR1_PIN2);
DriveCell backRightDriveCell(MOTOR3_PIN1, MOTOR3_PIN2);
DriveCell backLeftDriveCell(MOTOR2_PIN1, MOTOR2_PIN2);

CodeCell codeCell;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message
{
    char command[32];
    int value;
} struct_message;

// Create a struct_message called incomingReadings to hold incoming sensor readings
struct_message incomingReadings;

// Web server running on port 80
WebServer server(80);

unsigned long deltaTime = millis();

bool fieldCentricMode = false;

// Callback function that will be executed when data is received
void OnDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *incomingData, int len)
{
    // memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    // Serial.print("Bytes received: ");
    // Serial.println(len);
    // Serial.print("Command: ");
    // Serial.println(incomingReadings.command);
    // Serial.print("Value: ");
    // Serial.println(incomingReadings.value);

    // // Handle the received command
    // if (strcmp(incomingReadings.command, "on") == 0)
    // {
    //     Serial.println("LIGHT on");
    //     codeCell.LED(0, 0, 250);
    // }
    // else if (strcmp(incomingReadings.command, "off") == 0)
    // {
    //     Serial.println("LIGHT off");
    //     codeCell.LED(0, 0, 0);
    // }
    // else if (strcmp(incomingReadings.command, "speed") == 0)
    // {
    //     int speed = incomingReadings.value;
    //     Serial.print("Motor speed set to: ");
    //     Serial.println(speed);
    // }
}

void handleRoot()
{
    server.send(200, "text/html", index_html);
}

void handleControl()
{
    if (server.method() == HTTP_POST)
    {
        String command = server.arg("command");
        int leftX = server.arg("leftX").toInt();
        int leftY = server.arg("leftY").toInt();
        int rightX = server.arg("rightX").toInt();

        if (command == "fieldCentricOn")
        {
            fieldCentricMode = true;
            Serial.print("Field Centric Mode: ");
            Serial.println(fieldCentricMode);
        }
        else if (command == "fieldCentricOff")
        {
            fieldCentricMode = false;
            Serial.print("Field Centric Mode: ");
            Serial.println(fieldCentricMode);
        }
        else if (command == "resetHeading")
        {
            Serial.println("Heading reset");
            stop();
        }
        else
        {
            String leftX = server.arg("leftX");
            String leftY = server.arg("leftY");
            String rightX = server.arg("rightX");

            Serial.print("Left Joystick X: ");
            Serial.println(leftX);
            Serial.print("Left Joystick Y: ");
            Serial.println(leftY);
            Serial.print("Right Joystick X: ");
            Serial.println(rightX);

            // Handle the joystick values as needed
            // For example, you can convert them to integers and use them to control motors
            int lx = leftX.toInt();
            int ly = leftY.toInt();
            int rx = rightX.toInt();

            // Example: Control the motors
            mecanumDrive((float)(map(lx, 0, 100, -100, 100)) / 100, (float)(map(ly, 0, 100, -100, 100)) / 100, (float)(map(rx, 0, 100, -100, 100)) / 100, fieldCentricMode);


            // Example: Print the values
            Serial.printf("Left Joystick: (%d, %d), Right Joystick: (%d)\n", lx, ly, rx);
        }

        server.send(200, "text/plain", "OK");
    }
    else
    {
        server.send(405, "text/plain", "Method Not Allowed");
    }
}

void setup()
{
    Serial.begin(115200);
    codeCell.Init(LIGHT + MOTION_GYRO + MOTION_ROTATION);

    // Configure GPIO pins
  pinMode(MOTOR0_PIN1, OUTPUT);
  pinMode(MOTOR0_PIN2, OUTPUT);
  pinMode(MOTOR1_PIN1, OUTPUT);
  pinMode(MOTOR1_PIN2, OUTPUT);
  pinMode(MOTOR2_PIN1, OUTPUT);
  pinMode(MOTOR2_PIN2, OUTPUT);
  pinMode(MOTOR3_PIN1, OUTPUT);
  pinMode(MOTOR3_PIN2, OUTPUT);

  // Explicitly set GPIO pins to LOW
  digitalWrite(MOTOR0_PIN1, LOW);
  digitalWrite(MOTOR0_PIN2, LOW);
  digitalWrite(MOTOR1_PIN1, LOW);
  digitalWrite(MOTOR1_PIN2, LOW);
  digitalWrite(MOTOR2_PIN1, LOW);
  digitalWrite(MOTOR2_PIN2, LOW);
  digitalWrite(MOTOR3_PIN1, LOW);
  digitalWrite(MOTOR3_PIN2, LOW);

    frontLeftDriveCell.Init();
    frontRightDriveCell.Init();
    backRightDriveCell.Init();
    backLeftDriveCell.Init();

    frontLeftDriveCell.Tone();
    frontRightDriveCell.Tone();
    backRightDriveCell.Tone();
    backLeftDriveCell.Tone();

    resetRotation();

    stop();

    // Initialize WiFi
    WiFi.softAP("BuddyBot", "123456789");
    Serial.println("BuddyBot Network Initializing");

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_register_recv_cb(OnDataRecv);

    // // Once ESPNow is successfully Init, we will register for send CB to
    // // get the status of the sent packet
    // esp_now_register_send_cb(OnDataSent);

    // // Register the peer
    // esp_now_peer_info_t peerInfo;
    // memcpy(peerInfo.peer_addr, clientMAC, 6);
    // peerInfo.channel = 0;  
    // peerInfo.encrypt = false;
  
    // // Add the peer
    // if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    //   Serial.println("Failed to add peer");
    //   return;
    // }

    // Start the web server
    server.on("/", handleRoot);
    server.on("/control", HTTP_POST, handleControl);
    server.begin();
}

void loop()
{
    codeCell.Run();

    server.handleClient();

    // // Every 5 seconds send the emotion command
    // if (millis() - deltaTime > 5000) {
    //     Serial.println("test");
    //     sendEmotion();
    //     deltaTime = millis();
    // }
}

int sendEmotion() {
    // outgoingReadings.value = 3;
    // strcpy(outgoingReadings.command, "emotion");
    // esp_err_t result = esp_now_send(clientMAC, (uint8_t *) &outgoingReadings, sizeof(outgoingReadings));
    // Serial.println(result);
    // return result == ESP_OK;
    return 1;
}

int driveMotor(int motorId, float speed) {
  currentlyDriving = true;
  bool direction;

  switch(motorId) {
    case 0:
      speed = MOTOR0_MODIFIER * speed;
      direction = speed >= 0;
      
      speed = abs(speed);
      speed *= 100;
      if(speed < 50 && speed > 0) {
        speed = 50;
      }
      frontLeftDriveCell.Drive(direction, speed);
      return 1;
    case 1:
      speed = MOTOR1_MODIFIER * speed;
      direction = speed >= 0;

      speed = abs(speed);
      speed *= 100;
      if(speed < 50 && speed > 0) {
        speed = 50;
      }
      frontRightDriveCell.Drive(direction, speed);
      return 1;
    case 2:
      speed = MOTOR2_MODIFIER * speed;
      direction = speed >= 0;
      
      speed = abs(speed);
      speed *= 100;
      if(speed < 50 && speed > 0) {
        speed = 50;
      }
      backLeftDriveCell.Drive(direction, speed);
      return 1;
    case 3:
      speed = MOTOR3_MODIFIER * speed;
      direction = speed >= 0;
      
      speed = abs(speed);
      speed *= 100;
      if(speed < 50 && speed > 0) {
        speed = 50;
      }
      backRightDriveCell.Drive(direction, speed);
      return 1;
    default:
      return 0;
  }
  return 0;
}

void mecanumDrive(float x, float y, float rotation, boolean fieldOriented) {
  float heading = getRotation();

  if(fieldOriented) {
    x = x * cos(-heading) - y * sin(-heading);
    y = x * sin(-heading) + y * cos(-heading);
  }

  y = -y;

  float denominator = fmax(fabs(x) + fabs(y) + fabs(rotation), 1);

  float frontLeft = (y + x + rotation) / denominator;
  float frontRight = (y - x - rotation) / denominator;;
  float backLeft = (y - x + rotation) / denominator;
  float backRight = (y + x - rotation) / denominator;

  driveMotor(0, frontLeft);
  driveMotor(1, frontRight);
  driveMotor(2, backLeft);
  driveMotor(3, backRight);
}

int getRotation() {
    float myRoll, myPitch, myYaw;
  myCodeCell.Motion_RotationRead(myRoll, myPitch, myYaw);
    return myYaw;
} 

int stop() {
  currentlyDriving = false;

  driveMotor(0, 0);
  driveMotor(1, 0);
  driveMotor(2, 0);
  driveMotor(3, 0);

  return 1;
}

// #include <WiFi.h>
// #include <esp_now.h>
// #include <CodeCell.h>
// #include <math.h>
// #include <WebServer.h>

// CodeCell codeCell;

// // Structure example to receive data
// // Must match the sender structure
// typedef struct struct_message
// {
//     char command[32];
//     int value;
// } struct_message;

// // Create a struct_message called incomingReadings to hold incoming sensor readings
// struct_message incomingReadings;

// // Web server running on port 80
// WebServer server(80);

// bool fieldCentricMode = false;

// const char *index_html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//   <meta name="viewport" content="width=device-width, initial-scale=1">
//   <style>
//     html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }
//     .joystick-container { display: inline-block; margin: 20px; }
//     .joystick { width: 100px; height: 100px; background: #d3d3d3; border-radius: 50%; position: relative; }
//     .knob { width: 30px; height: 30px; background: #04AA6D; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }
//     .button { margin: 5px; padding: 5px 5px; font-size: 10px; }
//   </style>
// </head>
// <body>
//   <title>Buddy Controller</title>
//   <h1>Buddy Controller</h1>
//   <button class="button" onclick="enterFullScreen()">Enter Full Screen</button>
//   <button class="button" onclick="fieldCentric()">Field Centric</button>
//   <button class="button" onclick="sendCommand('resetHeading')">Reset Heading</button>
//   <br>
//   <br>
//   <div class="joystick-container" style="margin-right: 32vw;">
//     <div class="joystick" id="leftJoystick">
//       <div class="knob" id="leftKnob"></div>
//     </div>
//     <p><span id="leftX">0</span>, <span id="leftY">0</span></p>
//   </div>
//   <div class="joystick-container">
//     <div class="joystick" id="rightJoystick">
//       <div class="knob" id="rightKnob"></div>
//     </div>
//     <p><span id="rightX">0</span></p>
//   </div>
//   <script>
//     let leftJoystick = document.getElementById('leftJoystick');
//     let leftKnob = document.getElementById('leftKnob');
//     let rightJoystick = document.getElementById('rightJoystick');
//     let rightKnob = document.getElementById('rightKnob');

//     function sendJoystickValues(leftX, leftY, rightX) {
//       fetch('/control', {
//         method: 'POST',
//         headers: {
//           'Content-Type': 'application/x-www-form-urlencoded',
//         },
//         body: `leftX=${leftX}&leftY=${leftY}&rightX=${rightX}`
//       })
//       .then(response => response.text())
//       .then(data => console.log(data));
//     }

//     // Changes the button to be green when on and red when off
//     function fieldCentric() {
//       let button = document.getElementsByClassName('button')[1];
//       if (button.style.backgroundColor === 'green') {
//         button.style.backgroundColor = 'red';
//         sendCommand('fieldCentricOff');
//       } else {
//         button.style.backgroundColor = 'green';
//         sendCommand('fieldCentricOn');
//       }
//     }

//     function sendCommand(command) {
//       fetch('/control', {
//         method: 'POST',
//         headers: {
//           'Content-Type': 'application/x-www-form-urlencoded',
//         },
//         body: `command=${command}`
//       })
//       .then(response => response.text())
//       .then(data => console.log(data));
//     }

//     function updateKnobPosition(knob, x, y) {
//       knob.style.left = `${x}%`;
//       knob.style.top = `${y}%`;
//     }

//     function handleJoystickMove(event, joystick, knob, isLeft) {
//       let rect = joystick.getBoundingClientRect();
//       let x, y;
//       if (event.touches) {
//         x = ((event.touches[0].clientX - rect.left) / rect.width) * 100;
//         y = ((event.touches[0].clientY - rect.top) / rect.height) * 100;
//       } else {
//         x = ((event.clientX - rect.left) / rect.width) * 100;
//         y = ((event.clientY - rect.top) / rect.height) * 100;
//       }
//       x = Math.max(0, Math.min(100, x));
//       y = Math.max(0, Math.min(100, y));
//       updateKnobPosition(knob, x, y);

//       if (isLeft) {
//         document.getElementById('leftX').innerText = x.toFixed(0);
//         document.getElementById('leftY').innerText = y.toFixed(0);
//         sendJoystickValues(x.toFixed(0), y.toFixed(0), document.getElementById('rightX').innerText);
//       } else {
//         document.getElementById('rightX').innerText = x.toFixed(0);
//         sendJoystickValues(document.getElementById('leftX').innerText, document.getElementById('leftY').innerText, x.toFixed(0));
//       }
//     }

//     function startJoystickMove(event, joystick, knob, isLeft) {
//       function moveHandler(e) {
//         handleJoystickMove(e, joystick, knob, isLeft);
//       }

//       function endHandler() {
//         document.removeEventListener('mousemove', moveHandler);
//         document.removeEventListener('mouseup', endHandler);
//         document.removeEventListener('touchmove', moveHandler);
//         document.removeEventListener('touchend', endHandler);

//         // Reset the knob position to the center
//         if (isLeft) {
//           document.getElementById('leftX').innerText = '50';
//           document.getElementById('leftY').innerText = '50';
//           sendJoystickValues(50, 50, document.getElementById('rightX').innerText);
//         } else {
//           document.getElementById('rightX').innerText = '50';
//           sendJoystickValues(document.getElementById('leftX').innerText, document.getElementById('leftY').innerText, 50);
//         }
        
//         updateKnobPosition(knob, 50, 50);
//       }

//       document.addEventListener('mousemove', moveHandler);
//       document.addEventListener('mouseup', endHandler);
//       document.addEventListener('touchmove', moveHandler);
//       document.addEventListener('touchend', endHandler);
//     }

//     leftJoystick.addEventListener('mousedown', (event) => startJoystickMove(event, leftJoystick, leftKnob, true));
//     leftJoystick.addEventListener('touchstart', (event) => startJoystickMove(event, leftJoystick, leftKnob, true));
//     rightJoystick.addEventListener('mousedown', (event) => startJoystickMove(event, rightJoystick, rightKnob, false));
//     rightJoystick.addEventListener('touchstart', (event) => startJoystickMove(event, rightJoystick, rightKnob, false));

//     function enterFullScreen() {
//       if (document.documentElement.requestFullscreen) {
//         document.documentElement.requestFullscreen();
//       } else if (document.documentElement.mozRequestFullScreen) { // Firefox
//         document.documentElement.mozRequestFullScreen();
//       } else if (document.documentElement.webkitRequestFullscreen) { // Chrome, Safari and Opera
//         document.documentElement.webkitRequestFullscreen();
//       } else if (document.documentElement.msRequestFullscreen) { // IE/Edge
//         document.documentElement.msRequestFullscreen();
//       }
//     }
//   </script>
// </body>
// </html>
// )rawliteral";

// // Callback function that will be executed when data is received
// void OnDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *incomingData, int len)
// {
//     memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
//     Serial.print("Bytes received: ");
//     Serial.println(len);
//     Serial.print("Command: ");
//     Serial.println(incomingReadings.command);
//     Serial.print("Value: ");
//     Serial.println(incomingReadings.value);

//     // Handle the received command
//     if (strcmp(incomingReadings.command, "on") == 0)
//     {
//         Serial.println("LIGHT on");
//         codeCell.LED(0, 0, 250);
//     }
//     else if (strcmp(incomingReadings.command, "off") == 0)
//     {
//         Serial.println("LIGHT off");
//         codeCell.LED(0, 0, 0);
//     }
//     else if (strcmp(incomingReadings.command, "speed") == 0)
//     {
//         int speed = incomingReadings.value;
//         Serial.print("Motor speed set to: ");
//         Serial.println(speed);
//     }
// }

// void handleRoot()
// {
//     server.send(200, "text/html", index_html);
// }

// void handleControl()
// {
//     if (server.method() == HTTP_POST)
//     {
//         String command = server.arg("command");
//         int leftX = server.arg("leftX").toInt();
//         int leftY = server.arg("leftY").toInt();
//         int rightX = server.arg("rightX").toInt();

//         if (command == "fieldCentricOn")
//         {
//             fieldCentricMode = true;
//             Serial.print("Field Centric Mode: ");
//             Serial.println(fieldCentricMode);
//             // Handle field centric mode
//         }
//         else if (command == "fieldCentricOff")
//         {
//             fieldCentricMode = false;
//             Serial.print("Field Centric Mode: ");
//             Serial.println(fieldCentricMode);
//             // Handle field centric mode
//         }
//         else if (command == "resetHeading")
//         {
//             Serial.println("Heading reset");
//             // Handle heading reset
//         }
//         else
//         {
//             String leftX = server.arg("leftX");
//             String leftY = server.arg("leftY");
//             String rightX = server.arg("rightX");

//             Serial.print("Left Joystick X: ");
//             Serial.println(leftX);
//             Serial.print("Left Joystick Y: ");
//             Serial.println(leftY);
//             Serial.print("Right Joystick X: ");
//             Serial.println(rightX);

//             // Handle the joystick values as needed
//             // For example, you can convert them to integers and use them to control motors
//             int lx = leftX.toInt();
//             int ly = leftY.toInt();
//             int rx = rightX.toInt();

//             // Example: Print the values
//             Serial.printf("Left Joystick: (%d, %d), Right Joystick: (%d)\n", lx, ly, rx);
//         }

//         server.send(200, "text/plain", "OK");
//     }
//     else
//     {
//         server.send(405, "text/plain", "Method Not Allowed");
//     }
// }

// void setup()
// {
//     Serial.begin(115200);
//     codeCell.Init(LIGHT);

//     // Initialize WiFi
//     WiFi.softAP("BuddyBot", "123456789");
//     Serial.println("BuddyBot Network Initializing");

//     // Init ESP-NOW
//     if (esp_now_init() != ESP_OK)
//     {
//         Serial.println("Error initializing ESP-NOW");
//         return;
//     }

//     // Once ESPNow is successfully Init, we will register for recv CB to
//     // get recv packer info
//     esp_now_register_recv_cb(OnDataRecv);

//     // Start the web server
//     server.on("/", handleRoot);
//     server.on("/control", HTTP_POST, handleControl);
//     server.begin();
// }

// void loop()
// {
//     server.handleClient();
// }
