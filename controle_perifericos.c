#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define LED 27
#define DHTPIN 23
#define DHTTYPE DHT11

// dados da rede wifi e do broker MQTT
const char* ssid = "RoteadorTeste";
const char* password = "medson1234";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long ultimoEnvio = 0;

#define TOPICO_STATUS_DESEJADO_LED  
#define TOPICO_STATUS_ATUAL_LED     
#define TOPICO_TEMPERATURA          
#define TOPICO_UMIDADE              

/***************************************************************************
 * conectaWiFi()
 */
void conectaWiFi() {
  // faz a conexão com a rede WiFi
  Serial.println();
  Serial.print("Conectando a rede: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);        // Wi-Fi no modo station
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectada");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

/***************************************************************************
 * conectaMQTT()
 */
void conectaMQTT(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  if (WiFi.status() == WL_CONNECTED){
    // loop até conseguir conexão com o MQTT
    while (!client.connected()){
      Serial.println("Tentativa de conexão MQTT...");
      // cria um client ID randômico
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      
      // tentantiva de conexão
      if (client.connect(clientId.c_str())){
        Serial.println("MQTT conectado!");
        // se conectado, anuncia isso com um tópico especifico desse dispositivo
        String topicoStatusConexao = "autocore/conexao/";
        topicoStatusConexao.concat(WiFi.macAddress());
        client.publish(topicoStatusConexao.c_str(), "1");
        // ... e se increve nos tópicos de interesse, no caso aqui apenas um
        client.subscribe(TOPICO_STATUS_DESEJADO_LED);
      } 
      else{
        Serial.println("Conexão falhou");
        Serial.println("tentando novamente em 5 segundos");
        delay(5000);
      }
    }
  }
}

/***************************************************************************
 * callback()
 */
void callback(char* topic, byte* payload, unsigned int length){
  String receivedTopic = String(topic);
  
  Serial.println("recebeu mensagem");

  if(receivedTopic.equals(TOPICO_STATUS_DESEJADO_LED)){
    if(*payload == '1'){
      digitalWrite(LED, HIGH);      // liga o led
      client.publish(TOPICO_STATUS_ATUAL_LED, "1");  
    }
    if(*payload == '0'){
      digitalWrite(LED, LOW);     // desliga o led
      client.publish(TOPICO_STATUS_ATUAL_LED, "0");
    }
  }
}

/***************************************************************************
 * setup()
 */
void setup() {
  pinMode(LED, OUTPUT);      
  Serial.begin(115200);
  dht.begin();
  conectaWiFi();
  conectaMQTT(); 
}

void loop(){
  // fica atento aos dados MQTT
  client.loop();
  
  // se conexão MQTT caiu, reconecta
  if (!client.connected()){ 
    conectaMQTT();
  }

  // envia temperatura e humidade via MQTT a cada segundo
  if (millis() - ultimoEnvio > 1000){
    ultimoEnvio = millis();
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    if(client.connected()){
      Serial.println("enviando dados sensor via MQTT");
      client.publish(TOPICO_TEMPERATURA, String(temperatura).c_str());
      client.publish(TOPICO_UMIDADE, String(umidade).c_str());
    }
    else{
      Serial.println("Falha no MQTT");  
    }
  }
}
