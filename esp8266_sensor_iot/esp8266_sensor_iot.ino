#include <Arduino.h>
#include <PubSubClient.h> /*  MQTT Broker Library  */
#include <ESP8266WiFi.h>  /*  WiFi Shield ESP8266 ESP-12f Library */
#include <time.h>         /*  Time setting library  */

float Vin = 5.0;    // [V]
float Rt = 10000;   // Resistor t [ohm]
float R0 = 10000;   // value of rct in T0 [ohm]
float T0 = 298.15;  // use T0 in Kelvin [K]
float Vout = 0.0;   // Vout in A0
float Rout = 0.0;   // Rout in A0
// use the datasheet to get this data.
float T1 = 273.15;           // [K] in datasheet 0º C
float T2 = 373.15;           // [K] in datasheet 100° C
float RT1 = 35563;           // [ohms]  resistence in T1
float RT2 = 549;             // [ohms]   resistence in T2
float beta = 0.0;            // initial parameters [K]
float Rinf = 0.0;            // initial parameters [ohm]
float TempKelvin = 0.0;      // variable output
float TempCelsius = 0.0;     // variable output
float TempFahrenheit = 0.0;  // variable output


const char* ssid = "SSID"; /*  Network's name  */
const char* password = "PASSWORD";            /*  Network's password  */

const char* topic = "mosquitto_sensor_topic";    /*  MQTT Broker topic */
const char* server = "ec2-18-220-182-251.us-east-2.compute.amazonaws.com"; /*  MQTT broker host */
const int timezone = -3;       /*  Timezone (Brazil)   */
String clientName = "ESP8266"; /*  Publisher device identifier  */

/*  Structure to store DHT sensor data  */
struct Metrics {
  float celsius;
  float fahrenheit;
  float kelvin;
};

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

WiFiClient wifiClient;                                   /*  WiFi Shield client */
PubSubClient client(server, 1883, callback, wifiClient); /*  MQTT Broker client  */

/*
 * Method: setup
 * ----------------------------
 *   Initial setup of clients and connections.
 *   
 *   Initialize:
 *      Serial port.
 *      WiFi connection.
 *      Time client.
 *      Client name.
 *      MQTT Broker connection.
 */
void setup() {
  Serial.begin(115200);
  delay(10);
  wifiConnect();
  configureTime();
  defineClientName();
  connectMQTT();
}

/*
 * Method: loop
 * ----------------------------
 *   Non-stoping method that reads and publish sensor data.
 *   If DHT sensor lectures are invalid, stops iterations and initiates a new one.
 *   
 *   Gets date/time information.
 *   Reads sensor data.
 *   Creates payload.
 *   Prints results.
 *   Verifies MQTT Broker connection.
 *   Sends payload
 */
void loop() {
  String payload;
  getCalcBetaReinf();
  time_t now = time(nullptr);
  String datetime = buildDateTime(now);
  struct Metrics* metric = readSensor();

  if (metric == NULL) return;

  payload = createPayload(datetime, now, metric);

  printResults(datetime, metric);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Disconnected from WiFi.");
    wifiConnect();
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnected from MQTT Broker.");
    connectMQTT();
  }

  publishMQTT(payload);

  Serial.println();

  delay(3000);
}


/*
 * Function: defineClientName
 * ----------------------------
 *   Builds the name of the client and gets the MAC address.
 *
 */

void defineClientName() {
  String macStr;
  uint8_t mac[6];
  WiFi.macAddress(mac);

  for (int i = 0; i < 6; ++i) {
    macStr += String(mac[i], 16);
    if (i < 5)
      macStr += ':';
  }

  clientName += "-";
  clientName += macStr;
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
}


/*
 * Method: connectMQTT
 * ----------------------------
 *   Establish connection with MQTT broker.
 *   Doesn't  finish until conection is stablished.
 *
 */
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT Broker: ");
    Serial.print(server);
    Serial.print(" as ");
    Serial.println(clientName);
    if (client.connect((char*)clientName.c_str())) {
      Serial.println("Connected to MQTT broker");
      Serial.print("Topic is: ");
      Serial.println(topic);
    } else {
      Serial.println("MQTT connect failed");
      Serial.println("Will reset and try again...");
      delay(500);
    }
  }
  Serial.println("");
}

/*
 * Method: configureTime
 * ----------------------------
 *   Gets date/time info for an specific timezone.
 *
 */
void configureTime() {
  configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("\nWaiting for time");

  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Time configured.");
  Serial.println("");
}

/*
 * Method: wifiConnect
 * ----------------------------
 *   Establish WiFi connection.
 *   Doesn't  finish until conection is stablished.
 *
 */
void wifiConnect() {

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  delay(500);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  int tries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tries += 1;

    if (tries == 30) {
      tries = 0;
      Serial.println();
      Serial.println("Failed.");
      Serial.print("Trying again");
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/*
 * Function: buildDateTime
 * ----------------------------
 *  Builds date/time information.
 *  
 *  now: Current date/time timestamp.
 *   
 *  returns: date/time string information in 'yyyy-MM-ddTHH:mm:ssZ' format.
 *   
 */
String buildDateTime(time_t now) {
  struct tm* timeinfo;
  timeinfo = localtime(&now);
  String c_year = (String)(timeinfo->tm_year + 1900);
  String c_month = (String)(timeinfo->tm_mon + 1);
  String c_day = (String)(timeinfo->tm_mday);
  String c_hour = (String)(timeinfo->tm_hour);
  String c_min = (String)(timeinfo->tm_min);
  String c_sec = (String)(timeinfo->tm_sec);
  String datetime = c_year + "-" + c_month + "-" + c_day + "T" + c_hour + ":" + c_min + ":" + c_sec;

  if (timezone < 0) {
    datetime += "-0" + (String)(timezone * -100);
  } else {
    datetime += "0" + (String)(timezone * 100);
  }
  return datetime;
}

void getCalcBetaReinf() {
  beta = (log(RT1 / RT2)) / ((1 / T1) - (1 / T2));  // calculo de beta
  Rinf = R0 * exp(-beta / T0);                      // calculo de Rinf
  delay(100);

  Vout = Vin * ((float)(analogRead(0)) / 1024.0);  // calculo de V0 e leitura de A0
  Rout = (Rt * Vout / (Vin - Vout));               // calculo de Rout
  TempKelvin = (beta / log(Rout / Rinf));          // calculo da temp. em Kelvins
  TempCelsius = TempKelvin - 273.15;               // calculo da temp. em Celsius
  TempFahrenheit = (TempCelsius * 9) / 5 + 32;     // calculo da temp. em Fhrenheit
  delay(3000);
}

/*
 * Function: readSensor
 * ----------------------------
 *  Reads data from DHT sensor.
 *  
 *  returns: DHT data of temperatures and heat indexes.
 *   
 */
struct Metrics* readSensor() {

  struct Metrics* metric = (Metrics*)malloc(sizeof(struct Metrics));

  metric->kelvin = TempKelvin;
  metric->celsius = TempCelsius;
  metric->fahrenheit = TempFahrenheit;

  if (isnan(metric->kelvin) || isnan(metric->celsius) || isnan(metric->fahrenheit)) {
    Serial.println("Failed to read from sensor!");
    delay(3000);
    return NULL;
  }

  return metric;
}

/*
 * Function: createPayload
 * ----------------------------
 *  Creates payload to send.
 *  
 *  datetime: String with date/time information.
 *  now: Current date/time timestamp.
 *  metric: DHT sensor data.
 *  
 *  returns: payload in JSON format.
 *   
 */
String createPayload(String datetime, time_t now, Metrics* metric) {

  String payload = "{\"cliente\":";
  payload += "\"" + clientName + "\"";
  payload += ",\"microsegundos\":";
  payload += "\"" + (String)micros() + "\"";
  payload += ",\"data\":";
  payload += "\"" + datetime + "\"";
  payload += ",\"timestamp\":";
  payload += "\"" + (String)now + "\"";
  payload += ",\"celcius\":";
  payload += "\"" + (String)metric->celsius + "\"";
  payload += ",\"fahrenheit\":";
  payload += "\"" + (String)metric->fahrenheit + "\"";
  payload += ",\"kelvin\":";
  payload += "\"" + (String)metric->kelvin + "\"";
  payload += "}";

  return payload;
}

/*
 * Method: printResults
 * ----------------------------
 *   Print collected data.
 *
 */
void printResults(String datetime, Metrics* metric) {
  Serial.println("Data: " + (String)datetime);
  Serial.println("Temperatura: " + (String)metric->celsius + "*C " + (String)metric->kelvin + "*K " + (String)metric->fahrenheit + "*F\t");
}

/*
 * Method: publishMQTT
 * ----------------------------
 *   Sends payload to MQTT Broker.
 *
 */
void publishMQTT(String payload) {

  Serial.print("Sending payload: ");
  Serial.println(payload);
  if (client.publish(topic, (char*)payload.c_str())) {
    Serial.println("Publish ok");
  } else {
    Serial.println("Publish failed");
  }
}
