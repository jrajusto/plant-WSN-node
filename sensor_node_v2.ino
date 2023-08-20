#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include <DHT.h>
#include <MQ135.h>
#include <BH1750FVI.h>

#define DHT_PIN D7 //connection of tempearaure and humidity sensor
#define DHT_TYPE DHT11
#define SMS_PIN D3  // conenction of connection of soil moisutre sensor
#define AQS_PIN D6 //connection of Air quality sensor
#define UPDATE_DELAY 10000
#define SENSOR_PIN A0
#define LED_PIN D5

BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

const char ssid[] = SECRET_SSID; //wifi ssid
const char pass[] = SECRET_PASS; //wifi password
const int mqttPort = 1883;  

const char mqttServer[] = ""; //lan address
//const char mqttUser[] = "username";
//const char mqttPassword[] = "";

float temp;
float humidity;

int lightIntensity;
int airQuality;
int soilMoistureLevel;

WiFiClient espClient;
PubSubClient client(espClient);

MQ135 mq135_sensor(SENSOR_PIN);

DHT DHTsensor(DHT_PIN,DHT_TYPE);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial){
    
  }
  WiFi.mode(WIFI_STA);
  DHTsensor.begin();
  LightSensor.begin();  
  pinMode(AQS_PIN,OUTPUT);
  pinMode(SMS_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);

  pinMode(SENSOR_PIN,INPUT);
  digitalWrite(AQS_PIN,LOW);
  digitalWrite(SMS_PIN,HIGH);
  digitalWrite(LED_PIN,HIGH);
  delay(1000);
  connectWiFi();
  Serial.println("\n Connection to wifi done.");
  initMQTT();
  Serial.println("MQTT connection done....");
  
  

}

void loop() {
  // put your main code here, to run repeatedly:
 
  digitalWrite(AQS_PIN,HIGH);
  digitalWrite(SMS_PIN,LOW);
  Serial.println("Reading temperature adnd humidity");
  readDHT();
  delay(500);
  Serial.println("Reading light intensity");
  readLDR();
  delay(500); 
  Serial.println("Reading air quality");
  readAQS();
  delay(500);
  Serial.println("Reading soil moisture");
  readSMS();
  connectMQTT();
  connectWiFi();
  uploadData();
  printData();
  
  digitalWrite(AQS_PIN,HIGH);
  digitalWrite(SMS_PIN,LOW);
  //ESP.deepSleep(1.8e11);
  delay(1000);
}

void readDHT(){
  temp = DHTsensor.readTemperature();
  humidity = DHTsensor.readHumidity();
  if(isnan(temp) or isnan(humidity)){
    delay(1000);
      readDHT();
  }
}

void readLDR(){
  lightIntensity = LightSensor.GetLightIntensity();
  if(lightIntensity == 0){
    Serial.println("Reading light intensity");
    readLDR;
  }
}

void readAQS(){
  digitalWrite(AQS_PIN,LOW);
  digitalWrite(SMS_PIN,LOW);
  //delay(60000*2);
  delay(30000);
  airQuality = mq135_sensor.getCorrectedPPM(temp, humidity); //temp,humidity
  airQuality = mq135_sensor.getCorrectedPPM(temp, humidity); //temp,humidity
}

void readSMS(){
  digitalWrite(AQS_PIN,HIGH);
  digitalWrite(SMS_PIN,HIGH);
  delay(1000);
  soilMoistureLevel = analogRead(SENSOR_PIN);
  //soilMoistureLevel = map(soilMoistureLevel,350,915,100,0); //1
  soilMoistureLevel = map(soilMoistureLevel,600,915,100,0); //2
  //soilMoistureLevel = map(soilMoistureLevel,730,920,100,0); //3
  //soilMoistureLevel = map(soilMoistureLevel,500,917,100,0); //4
  if (soilMoistureLevel > 100)
    soilMoistureLevel = 100;

  if (soilMoistureLevel < 0)
    soilMoistureLevel = 0;

    
  //soilMoistureLevel = 100-soilMoistureLevel;
}

void connectWiFi(){
  if(WiFi.status() != WL_CONNECTED){
    digitalWrite(LED_PIN,HIGH);
    Serial.print("\n Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid,pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
    digitalWrite(LED_PIN,LOW);
  }
  digitalWrite(LED_PIN,LOW);
}

void initMQTT(){
  client.setServer(mqttServer,mqttPort);
  client.setCallback(callback);
  
  connectMQTT();
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived in topic:  ");
  Serial.println(topic);

  
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");
}

void uploadData(){
  static char tempString[7];
  dtostrf(temp, 6, 2, tempString);

  static char humidityString[7];
  dtostrf(humidity, 6, 2, humidityString);

  static char airQualityString[7];
  dtostrf(airQuality, 6, 2, airQualityString);

  static char soilMoistureLevelString[7];
  dtostrf(soilMoistureLevel, 6, 2, soilMoistureLevelString);

  static char lightIntensityString[7];
  dtostrf(lightIntensity, 6, 2, lightIntensityString);
  
  client.publish("nodeA2/temperature",tempString);
  client.publish("nodeA2/humidity",humidityString);
  client.publish("nodeA2/airQuality",airQualityString);
  client.publish("nodeA2/soilMoisture",soilMoistureLevelString);
  client.publish("nodeA2/lightIntensity",lightIntensityString);
  Serial.println("publishing data");
  //delay(5000);
  
}

void printData(){
  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Air Quality: ");
  Serial.println(airQuality);
  Serial.print("Soil Moisture: ");
  Serial.println(soilMoistureLevel);
  Serial.print("Light Intensity: ");
  Serial.println(lightIntensity);
}

void connectMQTT(){
  while(!client.connected()){
    Serial.println("Connecting to MQTT...");
  
    if (client.connect("ESP8266Client")) {
  
      Serial.println("connected");
    } 
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(5000);
    }
  }
  if(!client.loop())
    client.connect("ESP8266Client");
}
