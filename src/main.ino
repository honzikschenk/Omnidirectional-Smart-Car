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
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Command: ");
    Serial.println(incomingReadings.command);
    Serial.print("Value: ");
    Serial.println(incomingReadings.value);

    // Handle the received command
    if (strcmp(incomingReadings.command, "on") == 0)
    {
        Serial.println("LIGHT on");
        codeCell.LED(0, 0, 250);
    }
    else if (strcmp(incomingReadings.command, "off") == 0)
    {
        Serial.println("LIGHT off");
        codeCell.LED(0, 0, 0);
    }
    else if (strcmp(incomingReadings.command, "speed") == 0)
    {
        int speed = incomingReadings.value;
        Serial.print("Motor speed set to: ");
        Serial.println(speed);
    }
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
