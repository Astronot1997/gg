#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <cmath>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>



//#define Debug 1
// Define servos for steering and locomotion
Servo steeringServos[6];
Servo locomotionServos[6];

// Define servo pins % this pins are for D0 to D12 on arduino 3 1 26 25 17 16 27 14 12 13 5 23 19 18
int locomotionPins[6] = { 26, 17, 14, 13, 5, 19 };
int steeringPins[6] = { 25, 16, 27, 12, 23, 18 };
int steeringPWMOffsets[6] = { 90, 100, 91, 96, 109, 101 };
int locomotionPWMOffsetsLow[6] = { 89, 87, 89, 89, 87, 89 };
int locomotionPWMOffsetsHigh[6] = { 99, 97, 99, 99, 97, 99 };
int steerPWMs[6] = { 0, 0, 0, 0, 0, 0 };
int locoPWMs[6] = { 0, 0, 0, 0, 0, 0 };
float oldSteerPWMs[6] = { 0, 0, 0, 0, 0, 0 };
float oldLocoPWMs[6] = { 0, 0, 0, 0, 0, 0 };
float ratio = 0.05f;
float gecici_ratio;

float teker_konumlari[][6] = { { -0.15, 0.75 }, { 0.15, 0.75 }, { -0.15, 0 }, { 0.15, 0 }, { -0.15, -0.75 }, { 0.15, -0.75 } };

float r[] = { 1000, 0 };
float v = 0;

// Previous time for servo update
unsigned long lastServoUpdateTime = 0;

// Replace with your network credentials
const char *ssid = "AktekinGuduru";
const char *password = "gizembahadir";

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
//Joystick
const uint channel1Pin = 2;  // Kanal 1 için PWM sinyali
const uint channel2Pin = 4;  // Kanal 2 için PWM sinyali

// Joystick üzerindeki düğme için giriş pini
const uint buttonPin = 35;  // Düğme pini (INPUT_PULLUP ile kullanıyoruz)

// PWM sinyallerinin beklenen minimum ve maksimum değerleri (mikro saniye cinsinden)
const uint pwmMin = 1000;
const uint pwmMax = 2000;



String aio_username = "ahmetcancmz";  // Adafruit IO kullanıcı adınız
String aio_key = "";                  // Adafruit IO Key

const String version_feed = "version";  // Adafruit IO'daki version feed adı
const String url_feed = "url";          // Adafruit IO'daki url feed adı
const char* feed_name = "mesaj";        // Feed adı
String firmwareURL = "";
String version = "";
String currentversion = "";  // JSON'dan çekilecek "version" değişkeni
String url_value = "";       // JSON'dan çekilecek "url" değişkeni
//--------------------------------------------------------------------------------
const String base64_chars =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";
String decoded2 = "";
int base64_char_to_value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;  // Geçersiz karakter
}

String base64_decode(String input) {
  String output = "";
  int buffer = 0, bits_collected = 0;

  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c == '=') break;  // Padding karakterlerini atla

    int value = base64_char_to_value(c);
    if (value < 0) continue;  // Geçersiz karakterleri atla

    buffer = (buffer << 6) | value;
    bits_collected += 6;

    if (bits_collected >= 8) {
      bits_collected -= 8;
      char decoded_char = (buffer >> bits_collected) & 0xFF;
      output += decoded_char;
    }
  }

  return output;
}





void wifiBaglan() {
  Serial.print("WiFi'ye bağlanılıyor...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi bağlantısı tamamlandı!");
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.localIP());
}
//---------------------------------------Wİ-Fİ-----------------------------------------





//------------------------------------------OTA----------------------------------------




void performOTAUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(firmwareURL);  // GitHub'daki bin dosyasının raw linki
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      bool canBegin = Update.begin(contentLength);

      if (canBegin) {
        Serial.println("OTA güncellemesi başlıyor...");
        size_t written = Update.writeStream(http.getStream());

        if (written == contentLength) {
          Serial.println("OTA güncellemesi başarıyla tamamlandı.");
        } else {
          Serial.printf("OTA yazma hatası: %d\n", written);
        }

        if (Update.end()) {
          currentversion = version;
          Serial.println("OTA güncellemesi tamamlandı, yeniden başlatılıyor...");
          Serial.println(currentversion);
          ESP.restart();
        } else {
          Serial.printf("OTA güncellemesi başarısız! Hata: %s\n", Update.errorString());
        }
      } else {
        Serial.println("OTA güncellemesi başlatılamadı!");
      }
    } else {
      Serial.printf("HTTP isteği başarısız, hata kodu: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi bağlantısı yok!");
    wifiBaglan();
  }
}
//------------------------------------------OTA----------------------------------------


void vericekme() {
  if (WiFi.status() == WL_CONNECTED) {  // WiFi bağlıysa
    HTTPClient http;

    // Version feed'inden veri çek
    String version_url = "https://io.adafruit.com/api/v2/" + aio_username + "/feeds/" + version_feed + "/data/last";
    http.begin(version_url.c_str());
    http.addHeader("X-AIO-Key", aio_key);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String version_response = http.getString();
      //Serial.println("Gelen Version JSON: " + version_response);

      // JSON verisini ayrıştır ve sadece "value" değerini al
      StaticJsonDocument<300> doc;  // JSON ayrıştırma için buffer
      DeserializationError error = deserializeJson(doc, version_response);

      if (error) {
        Serial.print("JSON Ayrıştırma Hatası: ");
        Serial.println(error.f_str());
      } else {
        String value = doc["value"];
        Serial.print("Version Değeri (Sadece value): ");
        Serial.println(value);
        version = value;
      }
    } else {
      Serial.println("Version feed alinamadi.");
    }
    http.end();

    // URL feed'inden veri çek
    String url_feed_url = "https://io.adafruit.com/api/v2/" + aio_username + "/feeds/" + url_feed + "/data/last";
    http.begin(url_feed_url.c_str());
    http.addHeader("X-AIO-Key", aio_key);
    httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String url_response = http.getString();
      //Serial.println("Gelen URL JSON: " + url_response);

      // JSON verisini ayrıştır ve sadece "value" değerini al
      StaticJsonDocument<300> doc;  // JSON ayrıştırma için buffer
      DeserializationError error = deserializeJson(doc, url_response);

      if (error) {
        Serial.print("JSON Ayrıştırma Hatası: ");
        Serial.println(error.f_str());
      } else {
        String value = doc["value"];
        Serial.print("URL Değeri (Sadece value): ");
        Serial.println(value);
        firmwareURL = value;
      }
    } else {
      Serial.println("URL feed alınamadı.");
    }
    http.end();
  } else {
    Serial.println("WiFi bağlantısı yok!");
    wifiBaglan();
  }
  if (currentversion == version) {
    Serial.println("Cihazınız güncel durumdadır.");
  } else {
    Serial.println("Güncelleme başlatılıyor...");
    performOTAUpdate();
  }
}

//-------------------------------------------Veri ve güncelleme----------

void setup() {
  Serial.begin(115200);
  pinMode(channel1Pin, INPUT);
  pinMode(channel2Pin, INPUT);
  pinMode(buttonPin, INPUT);
  // Attach all servos
  for (int i = 0; i < 6; i++) {
    steeringServos[i].attach(steeringPins[i]);
    locomotionServos[i].attach(locomotionPins[i]);
  }

  // Set all wheels to straight position at the start
  for (int i = 0; i < 6; i++) {
    oldSteerPWMs[i] = (float)steeringPWMOffsets[i];
    steerPWMs[i] = (float)steeringPWMOffsets[i];
    oldLocoPWMs[i] = (locomotionPWMOffsetsLow[i] + locomotionPWMOffsetsHigh[i]) / 2.0f;
  }
  delay(1000);  // Wait for initialization
  String encoded = "YWlvX2tBZWw5OER2WTdZdGF0RDZVNnlRUnQ3NXFUYjA=";
  String decoded = base64_decode(encoded);
  aio_key = decoded;
  // Connect to Wi-Fi network with SSID and password
  wifiBaglan();
  if (WiFi.status() == WL_CONNECTED) {  // WiFi bağlıysa
    HTTPClient http;

    // Version feed'inden veri çek
    String version_url = "https://io.adafruit.com/api/v2/" + aio_username + "/feeds/" + version_feed + "/data/last";
    http.begin(version_url.c_str());
    http.addHeader("X-AIO-Key", aio_key);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String version_response = http.getString();
      //Serial.println("Gelen Version JSON: " + version_response);

      // JSON verisini ayrıştır ve sadece "value" değerini al
      StaticJsonDocument<300> doc;  // JSON ayrıştırma için buffer
      DeserializationError error = deserializeJson(doc, version_response);

      if (error) {
        Serial.print("JSON Ayrıştırma Hatası: ");
        Serial.println(error.f_str());
      } else {
        String value = doc["value"];
        Serial.print("Version Değeri (Sadece value): ");
        Serial.println(value);
        currentversion = value;
      }
    } else {
      Serial.println("Version feed alınamadı.");
    }
    http.end();
  } else {
    Serial.println("WiFi bağlantısı yok!");
    wifiBaglan();
  }
  server.begin();
}

//-------------------------------------------------------------------------------

float teker_hiz_bul(float r[], float T[], float v) {
  float beta = v * sqrt((pow((r[0] - T[0]), 2) + pow((r[1] - T[1]), 2)) / (pow(r[0], 2) + pow(r[1], 2)));

  return beta;
}


//-------------------------------------------------------------------------------


float teker_aci_bul(float r[], float T[], float v) {
  float theta = -90 + atan2(r[0] - T[0], r[1] - T[1]) * 180.0f / 3.14f;

  if (theta > 90.0f) {
    theta = theta - 180.0f;
  } else if (theta < -90.0f) {

    theta = theta + 180.0f;
  }
  return theta;
}


//-------------------------------------------------------------------------------

void RobotTurn(float aci_deg, int i) {  // Function to make a right turn

  int PWM = (int)aci_deg;
  
  #ifdef Debug
  Serial.print(" Turn:");    
  Serial.print(PWM);
  #endif  
  // Adjust steering servos to turn right
  // First three wheels (front half)

  int servoPWM = max(0, min(PWM + steeringPWMOffsets[i], 255));

  steerPWMs[i] = servoPWM;
}


//-------------------------------------------------------------------------------


void RobotMove(float hiz_m_s, int i) {
  
  
  int PWM = (int)(hiz_m_s * 50.0f / 20.0f * 100.0f);
  #ifdef Debug
  Serial.print(" Move:");   
  Serial.print(PWM); 
  #endif 
         

  // left half motors
  if (i % 2 == 0 && i != 0) {
    locoPWMs[i] = -PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
  } else {
    locoPWMs[i] = PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
  }
  // Birinci motor istisna
  //int i = 0;
  //locoPWMs[i] = PWM + (locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2;
}


//-------------------------------------------------------------------------------


void updateServos(int period_ms) {

  if (millis() - lastServoUpdateTime > period_ms) {

    lastServoUpdateTime = millis();

    for (int i = 0; i < 6; i++) {
      float T[2] = { teker_konumlari[i][0], teker_konumlari[i][1] };

      float teker_hizi = teker_hiz_bul(r, T, v);
      float teker_acisi = teker_aci_bul(r, T, v);
      #ifdef Debug
      Serial.printf("\n%d.Teker=[%.0f,%.0f], Aci: %.0fder, Hiz: %.0f cm/s", i, T[0], T[1], teker_acisi, teker_hizi * 100);
#endif 

      
// limitle
      
float hizlim=0.05f;
float acilim=30.0f;

      if (teker_acisi>acilim) {
        teker_acisi=acilim;

      }else if (teker_acisi<-acilim)
      {
        teker_acisi=-acilim;
      }else if (isnan(teker_acisi)) {
        teker_acisi=0;

      }
      if (teker_hizi>hizlim) {
        teker_hizi=hizlim;

      }else if (teker_hizi<-hizlim)
      {
        teker_hizi=-hizlim;
      }else if (isnan(teker_hizi)) {
        teker_hizi=0;

      }

      // teker aci e hizleri pwm donustur

      
      RobotTurn(teker_acisi, i);
      RobotMove(teker_hizi, i);
    }
    #ifdef Debug
        Serial.print("\nPWMLER ");
#endif 

    // Servoya PWM degerlerini ata
    for (int i = 0; i < 6; i++) {
      #ifdef Debug
            Serial.printf(" M%d ",i);
#endif 

      if (isnan(steerPWMs[i])) {
        steerPWMs[i] = 0.0f;
      }
      // Steer
      oldSteerPWMs[i] = (1 - ratio) * oldSteerPWMs[i] + ratio * steerPWMs[i];
      steeringServos[i].write((int)oldSteerPWMs[i]);
      #ifdef Debug
             Serial.print((int)oldSteerPWMs[i]);
       Serial.print(" ");
#endif 


      if (isnan(locoPWMs[i])) {
        locoPWMs[i] = 0.0f;
      }
      // Loco
      oldLocoPWMs[i] = (1.0f - ratio) * oldLocoPWMs[i] + ratio * locoPWMs[i];
      if (oldLocoPWMs[i] < locomotionPWMOffsetsHigh[i] && oldLocoPWMs[i] > locomotionPWMOffsetsLow[i]) {
        locomotionServos[i].write((locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2);
        #ifdef Debug
          Serial.print((locomotionPWMOffsetsHigh[i] + locomotionPWMOffsetsLow[i]) / 2);
  #endif 
      } else {
        locomotionServos[i].write((int)oldLocoPWMs[i]);
        #ifdef Debug
          Serial.print((int)oldLocoPWMs[i]);
  #endif 


      }

      #ifdef Debug
            Serial.print(" ");
#endif 


    }

  }
}


//-------------------------------------------------------------------------------



void loop() {
  #ifdef Debug
  delay(100);
#endif 
delay(100);

  unsigned long pwmValue1 = pulseIn(channel1Pin, HIGH, 25000);
  unsigned long pwmValue2 = pulseIn(channel2Pin, HIGH, 25000);
  unsigned long pwmValue3 = pulseIn(buttonPin, HIGH);
  float mappedValue1 = (float)(pwmValue1-1000ul)/1000.0f*20.0f;
  float mappedValue2 = (float)(pwmValue2-1000ul)/1000.0f*100.0f;
  Serial.printf("\nMv1:%d,Mv2:%d,Pwm3:%d",mappedValue1,mappedValue2,pwmValue3);

  
  updateServos(10);


  if (pwmValue3 < 1500) {

    Serial.println("INTERNET");
    WiFiClient client = server.available();  // Listen for incoming clients

    if (client) {  // If a new client connects,
      currentTime = millis();
      previousTime = currentTime;
      Serial.println("New Client.");                                             // print a message out in the serial port
      String currentLine = "";                                                   // make a String to hold incoming data from the client
      while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
        currentTime = millis();
        if (client.available()) {  // if there's bytes to read from the client,
          char c = client.read();  // read a byte, then
          Serial.write(c);         // print it out the serial monitor
          header += c;
          if (c == '\n') {  // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
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
              if (header.indexOf("GET /?value=") >= 0) {
                pos1 = header.indexOf('=');
                pos2 = header.indexOf('&');
                valueString = header.substring(pos1 + 1, pos2);

                // Rotate the servo
                int myval = valueString.toInt();
                if (myval < 200) 
                {
                  r[0] = myval / 100.0f;
                } 
                else if (myval > 600) 
                {
                  ratio = (myval - 800) / 100.0f;
                } 
                else 
                {
                  v = (float)(myval - 400) / 100.0f;
                }
                Serial.println(valueString);
              }
              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else {  // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
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
  }else{
float kucuk_deger = 1.0f;
    if (mappedValue2>kucuk_deger) {
      mappedValue2=kucuk_deger;

    }else if (mappedValue2<-kucuk_deger)
    {
      mappedValue2=-kucuk_deger;
    }else if (isnan(mappedValue2)) {
      mappedValue2=0;

    }
    
    
    r[0] = 1.0f/atan2(mappedValue2,500.0f);
    Serial.printf("\nJoystick - DY=%f",r[0]);

    v=mappedValue1/100.0f;
    ratio = 100.0f / 100.0f;
  }
  //vericekme();
}