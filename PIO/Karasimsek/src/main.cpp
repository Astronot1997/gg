#include <Arduino.h>
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-servo-motor-web-server-arduino-ide/
*********/
#include <WiFi.h>
#include <ESP32Servo.h>
#include <cmath>

// Define servos for steering and locomotion
Servo steeringServos[6];
Servo locomotionServos[6];

// Define servo pins % this pins are for D0 to D12 on arduino 3 1 26 25 17 16 27 14 12 13 5 23 19 18
int locomotionPins[6] = {26, 17, 14, 13, 5, 19};
int steeringPins[6] = {25, 16, 27, 12, 23, 18};
int steeringPWMOffsets[6] = {90, 100, 91, 96, 109, 101};
int locomotionPWMOffsetsLow[6] = {89, 87, 89, 89, 87, 89};
int locomotionPWMOffsetsHigh[6] = {99, 97, 99, 99, 97, 99};
int steerPWMs[6] = {0, 0, 0, 0, 0, 0};
int locoPWMs[6] = {0, 0, 0, 0, 0, 0};
float oldSteerPWMs[6] = {0, 0, 0, 0, 0, 0};
float oldLocoPWMs[6] = {0, 0, 0, 0, 0, 0};
float ratio = 0.05;

float teker_konumlari[][6] = {{-0.15, 0.75}, {0.15, 0.75}, {-0.15, 0}, {0.15, 0}, {-0.15, -0.75}, {0.15, -0.75}};

float r[] = {1000, 0};
float v = 0;

// Previous time for servo update
unsigned long lastServoUpdateTime = 0;

// Replace with your network credentials
const char *ssid = "BAHADIRSURFACE";
const char *password = "12345678";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Decode HTTP GET value
String valueString = String(5);
String valueString2 = String(5);
int pos1 = 0;
int pos2 = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup()
{
  Serial.begin(115200);

  // Attach all servos
  for (int i = 0; i < 6; i++)
  {
    steeringServos[i].attach(steeringPins[i]);
    locomotionServos[i].attach(locomotionPins[i]);
  }

  // Set all wheels to straight position at the start
  for (int i = 0; i < 6; i++)
  {
    oldSteerPWMs[i] = (float)steeringPWMOffsets[i];
    steerPWMs[i] = (float)steeringPWMOffsets[i];
    oldLocoPWMs[i] = (locomotionPWMOffsetsLow[i] + locomotionPWMOffsetsHigh[i]) / 2.0;
  }
  delay(1000); // Wait for initialization

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

float teker_hiz_bul(float r[], float T[], float v)
{
  float beta = v * sqrt((pow((r[0] - T[0]), 2) + pow((r[1] - T[1]), 2)) / (pow(r[0], 2) + pow(r[1], 2)));

  return beta;
}

float teker_aci_bul(float r[], float T[], float v)
{
  float theta = -90 + atan2(r[0] - T[0], r[1] - T[1]) * 180.0 / 3.14;

  if (theta > 90.0)
  {
    theta = theta - 180.0;
  }
  else if (theta < -90.0)
  {

    theta = theta + 180.0;
  }
  return theta;
}

void RobotTurn(float aci_deg)
{ // Function to make a right turn

  int PWM = (int)aci_deg;
  Serial.print(" Turn:");
  Serial.print(PWM);
  // Adjust steering servos to turn right
  for (int i = 0; i < 2; i++)
  { // First three wheels (front half)

    int servoPWM = max(0, min(PWM + steeringPWMOffsets[i], 255));

    steerPWMs[i] = servoPWM;
  }
  for (int i = 4; i < 6; i++)
  { // Last three wheels (back half)
    int servoPWM = max(0, min(-PWM + steeringPWMOffsets[i], 255));

    steerPWMs[i] = servoPWM;
  }
}

void RobotMove(float hiz_m_s)
{

  int PWM = (int)(hiz_m_s * 4.0 * 100);

  Serial.print(" Move:");
  Serial.print(PWM);

  // left half motors
  for (int i = 0; i < 6; i = i + 2)
  {
    locoPWMs[i] = -PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
  }

  // right half motors
  for (int i = 1; i < 6; i = i + 2)
  {
    locoPWMs[i] = PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
  }
  // Birinci motor istisna
  // int i = 0;
  // locoPWMs[i] = PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
}

void updateServos()
{

  if (millis() - lastServoUpdateTime > 1000)
  {

    lastServoUpdateTime = millis();

    for (int i = 0; i < 6; i++)
    {
      float T[2] = {teker_konumlari[i][0], teker_konumlari[i][1]};

      float teker_hizi = teker_hiz_bul(r, T, v);
      float teker_acisi = teker_aci_bul(r, T, v);

      Serial.printf("\n%d teker icin, T=[%.0f,%.0f], Teker Acisi: %.0f, Teker Hizi: %.0f", i, T[0], T[1], teker_acisi, teker_hizi);

      // teker aci e hizleri pwm donustur

      RobotTurn(teker_acisi);
      RobotMove(teker_hizi);
    }

    // Servoya PWM degerlerini ata
    for (int i = 0; i < 6; i++)
    {
      if (isnan(steerPWMs[i]))
      {
        steerPWMs[i] = 0.0;
      }
      // Steer
      oldSteerPWMs[i] = (1 - ratio) * oldSteerPWMs[i] + ratio * steerPWMs[i];
      steeringServos[i].write((int)oldSteerPWMs[i]);

      // Serial.print((int)oldSteerPWMs[i]);

      // Serial.print(" ");

      if (isnan(locoPWMs[i]))
      {
        locoPWMs[i] = 0.0;
      }
      // Loco
      oldLocoPWMs[i] = (1.0 - ratio) * oldLocoPWMs[i] + ratio * locoPWMs[i];
      if (oldLocoPWMs[i] < locomotionPWMOffsetsHigh[i] && oldLocoPWMs[i] > locomotionPWMOffsetsLow[i])
      {
        locomotionServos[i].write((locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2);
        // Serial.print(" G ");
      }
      else
      {
        locomotionServos[i].write((int)oldLocoPWMs[i]);

        // Serial.print((int)oldLocoPWMs[i]);

        // Serial.print(" ");
      }
    }

    // Serial.println(".");
  }
}

void loop()
{
  updateServos();
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
            client.println(".slider { width: 300px; }</style>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");

            // Web Page
            client.println("</head><body><h1>Gezegen Gezgini Robot</h1>");

            // İlk kayıcı dönüş için
            client.println("<p>DONUS YARICAPI [cm]: <span id=\"turnPWM\"></span></p>");
            client.println("<input type=\"range\" min=\"-100\" max=\"100\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\"" + valueString + "\"/>");

            // 2.  kayıcı ileri hız için
            client.println("<p>HIZ [cm/s]:  <span id=\"movePWM\"></span></p>");
            client.println("<input type=\"range\" min=\"-20\" max=\"20\" class=\"slider\" id=\"moveSlider\" onchange=\"servo(parseInt(this.value) + Number(400))\" value=\"" + valueString + "\"/>");

            // 3.  kayıcı oran için
            client.println("<p>ORAN: <span id=\"oran\"></span></p>");
            client.println("<input type=\"range\" min=\"0\" max=\"100\" class=\"slider\" id=\"oranKayici\" onchange=\"servo(parseInt(this.value) + Number(800))\" value=\"" + valueString + "\"/>");

            client.println("<button onclick=\"servo( Number(400))\" >Dur</button>");

            client.println("<script>var slider = document.getElementById(\"servoSlider\"); Moveslider = document.getElementById(\"moveSlider\");");
            client.println("var servoP = document.getElementById(\"turnPWM\"); servoP.innerHTML = slider.value; servoP2 = document.getElementById(\"movePWM\"); servoP2.innerHTML = Moveslider.value;  servoP3 = document.getElementById(\"oran\"); servoP3.innerHTML = oranKayici.value;");
            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
            client.println("Moveslider.oninput = function() { Moveslider.value = this.value; servoP2.innerHTML = this.value; }");
            client.println("oranKayici.oninput = function() { oranKayici.value = this.value; servoP3.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");

            client.println("</body></html>");

            // GET /?value=180& HTTP/1.1
            if (header.indexOf("GET /?value=") >= 0)
            {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1 + 1, pos2);

              // Rotate the servo
              int myval = valueString.toInt();
              if (myval < 200)
              {

                r[0] = myval;

                ;
              }
              else if (myval > 600)
              {
                ratio = (myval - 800) / 100.0;
              }
              else
              {

                v = (float)(myval - 400) / 100;
              }

              Serial.println(valueString);
            }
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
