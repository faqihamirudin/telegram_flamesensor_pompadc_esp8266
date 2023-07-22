#include "CTBot.h"
#include "DHT.h"
#include <BlynkSimpleEsp8266_SSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

DHT dht(D6, DHT11);
CTBot myBot;

const char* ssid = "8810620";    //nama ssid wifi kalian
const char* pass = "alvaro212";  //password wifi kalian
String token = "5764775266:AAEVqU2j6rOkWxQQQtjdQ7odCB5A3qOsi7g";

char auth[] = "3yOE7SCVcv7AUKsXSezANa8kHAImpVeO";

const int lampu = D2;
const int lampu2 = D3;
const int buzzer = D1;
const int flamePin = D0;
const int relay = D5;
int flameValue = 0;

WiFiClient client; // Objek WiFiClient

void setup() {
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  // connect the ESP8266 to the desired access point
  myBot.wifiConnect(ssid, pass);

  // set the telegram bot token
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK");
  else
    Serial.println("\ntestConnection NOK");

  // set the pin connected to the LED to act as output pin
  pinMode(lampu, OUTPUT);
  pinMode(lampu2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(lampu, HIGH); // turn off the led (inverted logic!)

  // set the flame sensor pin as input
  pinMode(flamePin, INPUT);
  dht.begin();
  // set pompa dc
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW); // turn off the led (inverted logic!)

  Blynk.begin(auth, ssid, pass);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  Blynk.run();

  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V3, humidity);

  // Mengirim data suhu dan kelembaban ke server database
  sendSensorData(temperature, humidity);

  // Read the flame sensor value
  int flameValue = analogRead(flamePin);
  // Check if flame is detected
  if (flameValue < 100) { // Adjust the threshold value according to your sensor
    digitalWrite(buzzer, HIGH);
    digitalWrite(lampu2, HIGH); // Turn on the LED
    myBot.sendMessage(1273830798, "RUANGAN TERDETEKSI API! SEGERA CEK RUANGAN!");  // Replace 123456789 with your chat ID or recipient ID
    Blynk.logEvent("api_terdeteksi");
  } else {
    digitalWrite(lampu2, LOW);
    digitalWrite(buzzer, LOW);
  }

  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    if (msg.text.equalsIgnoreCase("ON")) {                   //Perintah dari telegram ke perangkat
      digitalWrite(lampu, HIGH);                            //Lampu dihidupkan
      myBot.sendMessage(msg.sender.id, "LAMPU TELAH DIHIDUPKAN"); //Balasan dari perangkat ke Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("OFF")) {              //Perintah dari telegram ke perangkat
      digitalWrite(lampu, LOW);                              //Lampu dimatikan
      myBot.sendMessage(msg.sender.id, "LAMPU TELAH DIMATIKAN"); //Balasan dari perangkat ke Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("KEDIP")) {              //Perintah dari telegram ke perangkat
      // Lampu berkedip sebanyak 10x
      for (int i = 0; i < 10; i++) {
        digitalWrite(lampu, HIGH);
        delay(150);
        digitalWrite(lampu, LOW);
        delay(150);
      }
      myBot.sendMessage(msg.sender.id, "LAMPU TELAH BERKEDIP SEBANYAK 10x"); //Balasan dari perangkat ke Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("SUHU")) {      // Perintah dari telegram ke perangkat
      float temperature = dht.readTemperature();              // Membaca suhu dari sensor DHT11
      String reply = "Suhu ruangan " + String(temperature) + " °C";
      myBot.sendMessage(msg.sender.id, reply);                // Mengirimkan suhu melalui Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("KELEMBABAN")) {         // Perintah dari telegram ke perangkat
      float humidity = dht.readHumidity();                    // Membaca kelembaban dari sensor DHT11
      String reply = "Kelembaban ruangan " + String(humidity) + " %";
      myBot.sendMessage(msg.sender.id, reply);                // Mengirimkan kelembaban melalui Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("OFF PUMP")) {              //Perintah dari telegram ke perangkat
      digitalWrite(relay, LOW);                              //Lampu dimatikan
      myBot.sendMessage(msg.sender.id, "POMPA TELAH DIMATIKAN"); //Balasan dari perangkat ke Bot Telegram
    }
    else if (msg.text.equalsIgnoreCase("ON PUMP")) {              //Perintah dari telegram ke perangkat
      digitalWrite(relay, HIGH);                              //Lampu dimatikan
      myBot.sendMessage(msg.sender.id, "POMPA TELAH DINYALAKAN"); //Balasan dari perangkat ke Bot Telegram
    }
    else {                                                    // otherwise...
      // generate the message for the sender
      String reply;
      reply = "Selamat Datang di Bot TUGAS BESAR IOT2 " + msg.sender.username + "  . \n\n - Ketik ON = Menyalakan Lampu \n - Ketik KEDIP = Menyalakan Lampu Berkedip 10x \n - Ketik OFF = Mematikan Lampu \n\n - Ketik ON PUMP = Menyalakan Pompa Air \n - Ketik OFF PUMP = Mematikan Pompa Air \n\n - Ketik SUHU = Mengecek Suhu ruangan \n - Ketik KELEMBABAN = Mengecek Kelembaban ruangan \n\n BOT DIBUAT OLEH: \n - FAQIH AMIRUDIN \n - ADE ALFI RAMADHANI.";
      myBot.sendMessage(msg.sender.id, reply);             // and send it
    }
  }
}

BLYNK_WRITE(V2)
{
  int relayPompa = param.asInt();

  if (relayPompa == 1) {
    digitalWrite(relay, HIGH);
  }
  else {
    digitalWrite(relay, LOW);
  }
}

void sendSensorData(float temperature, float humidity) {
  // Membuat objek HTTPClient
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String address;

    address = "http://192.168.0.102/arducoding_tutorial/nodemcu_log/webapi/api/create.php?suhu=";
    address += String(temperature);
    address += "&kelembaban=";
    address += String(humidity);

    http.begin(client, address);
    int httpCode = http.GET();
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
      if (payload.length() > 0) {
        Serial.println(payload);
      }
    }
    http.end();
  }
  else {
    Serial.print("Not connected to WiFi: ");
    Serial.println(ssid);
  }
  delay(3000);
}
