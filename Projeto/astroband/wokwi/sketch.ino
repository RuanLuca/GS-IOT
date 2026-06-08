#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =========================
// CONFIGURACAO DO WIFI
// =========================
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// =========================
// CONFIGURACAO DO DISPLAY OLED
// =========================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

// =========================
// CONFIGURACAO MQTT
// =========================
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "astroband-mission-kit-001";
const char* TOPIC_STATUS = "fiap/iot/astroband/status";
const char* TOPIC_TELEMETRIA = "fiap/iot/astroband/telemetria";

// =========================
// PINOS DO CIRCUITO
// =========================
const int POT_TEMPERATURA = 34;
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int BOTAO_EMERGENCIA = 14;
const int LED_BATIMENTO = 26;
const int LED_STATUS = 27;
const int OLED_SDA = 21;
const int OLED_SCL = 22;

// =========================
// REGRAS DA SIMULACAO
// =========================
const float TEMP_MIN = 34.0;
const float TEMP_MAX = 40.5;
const float TEMP_ALERTA = 38.0;
const int DISTANCIA_SEGURA_CM = 150;
const int DISTANCIA_ALERTA_CM = 220;
const unsigned long INTERVALO_API_LOG = 3000;
const unsigned long INTERVALO_OLED = 2500;
const unsigned long INTERVALO_MQTT = 2000;

unsigned long ultimoLogApi = 0;
unsigned long ultimoPiscaBatimento = 0;
unsigned long ultimaTrocaTela = 0;
unsigned long ultimoEnvioMqtt = 0;

bool estadoLedBatimento = false;
bool estadoLedStatus = false;
int telaAtual = 0;

struct TelemetriaAstroBand {
  int heartbeat;
  float temperature;
  int distance;
  bool emergency;
  String missionStatus;
};

// =========================
// FUNCOES DE APOIO
// =========================
float mapFloat(float valor, float inMin, float inMax, float outMin, float outMax) {
  return (valor - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

void adicionarCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void enviarJson(const JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  adicionarCors();
  server.send(200, "application/json", payload);
}

// =========================
// CONEXAO WIFI
// =========================
void conectarWifi() {
  Serial.print("Conectando ao WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println(" conectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao broker MQTT...");

    String clientId = String(MQTT_CLIENT_ID) + "-" + String(random(1000, 9999));

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado!");
      mqttClient.publish(TOPIC_STATUS, "{\"status\":\"online\",\"device\":\"ASTROBAND-001\"}", true);
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 2 segundos");
      delay(2000);
    }
  }
}

// =========================
// LEITURA DOS SENSORES
// =========================
float lerTemperatura() {
  int leitura = analogRead(POT_TEMPERATURA);
  float temperatura = mapFloat(leitura, 0, 4095, TEMP_MIN, TEMP_MAX);
  return round(temperatura * 10.0) / 10.0;
}

int lerDistanciaCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracao == 0) {
    return 0;
  }

  return duracao / 58;
}

bool lerEmergencia() {
  return digitalRead(BOTAO_EMERGENCIA) == LOW;
}

int simularBatimentos(float temperatura, int distancia, bool emergencia) {
  int batimentos = 72 + random(-5, 6);

  if (temperatura >= TEMP_ALERTA) {
    batimentos += 18;
  }

  if (distancia > DISTANCIA_SEGURA_CM) {
    batimentos += 8;
  }

  if (emergencia) {
    batimentos += 22;
  }

  return constrain(batimentos, 55, 135);
}

String classificarBatimentos(int heartbeat) {
  if (heartbeat < 60) {
    return "attention";
  }

  if (heartbeat <= 100) {
    return "normal";
  }

  if (heartbeat <= 120) {
    return "attention";
  }

  return "critical";
}

String classificarTemperatura(float temperatura) {
  if (temperatura >= TEMP_ALERTA) {
    return "critical";
  }

  if (temperatura < 35.5) {
    return "attention";
  }

  return "normal";
}

String classificarLocalizacao(int distancia) {
  if (distancia == 0) {
    return "unknown";
  }

  if (distancia <= DISTANCIA_SEGURA_CM) {
    return "safe";
  }

  if (distancia <= DISTANCIA_ALERTA_CM) {
    return "attention";
  }

  return "danger";
}

String classificarMissao(const TelemetriaAstroBand& dados) {
  if (dados.emergency) {
    return "emergency";
  }

  if (classificarTemperatura(dados.temperature) == "critical" ||
      classificarBatimentos(dados.heartbeat) == "critical" ||
      classificarLocalizacao(dados.distance) == "danger") {
    return "critical";
  }

  if (classificarTemperatura(dados.temperature) == "attention" ||
      classificarBatimentos(dados.heartbeat) == "attention" ||
      classificarLocalizacao(dados.distance) == "attention") {
    return "attention";
  }

  return "normal";
}

TelemetriaAstroBand lerTelemetria() {
  TelemetriaAstroBand dados;
  dados.temperature = lerTemperatura();
  dados.distance = lerDistanciaCm();
  dados.emergency = lerEmergencia();
  dados.heartbeat = simularBatimentos(dados.temperature, dados.distance, dados.emergency);
  dados.missionStatus = classificarMissao(dados);
  return dados;
}

// =========================
// ATUADORES LOCAIS
// =========================
void atualizarLedBatimento(int heartbeat) {
  unsigned long intervalo = 60000UL / max(heartbeat, 1) / 2;

  if (millis() - ultimoPiscaBatimento >= intervalo) {
    ultimoPiscaBatimento = millis();
    estadoLedBatimento = !estadoLedBatimento;
    digitalWrite(LED_BATIMENTO, estadoLedBatimento ? HIGH : LOW);
  }
}

void atualizarLedStatus(const String& statusMissao) {
  if (statusMissao == "normal") {
    digitalWrite(LED_STATUS, HIGH);
    return;
  }

  unsigned long intervalo = statusMissao == "emergency" ? 180 : 450;

  if (millis() % intervalo < intervalo / 2) {
    estadoLedStatus = true;
  } else {
    estadoLedStatus = false;
  }

  digitalWrite(LED_STATUS, estadoLedStatus ? HIGH : LOW);
}

// =========================
// INTERFACE OLED
// =========================
void imprimirLinha(int y, const String& texto, int tamanho = 1) {
  display.setTextSize(tamanho);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, y);
  display.println(texto);
}

void atualizarOLED(const TelemetriaAstroBand& dados) {
  if (millis() - ultimaTrocaTela >= INTERVALO_OLED) {
    ultimaTrocaTela = millis();
    telaAtual = (telaAtual + 1) % 4;
  }

  display.clearDisplay();

  if (telaAtual == 0) {
    imprimirLinha(0, "BATIMENTOS", 1);
    imprimirLinha(22, String(dados.heartbeat) + " bpm", 2);
    imprimirLinha(50, classificarBatimentos(dados.heartbeat), 1);
  } else if (telaAtual == 1) {
    imprimirLinha(0, "TEMPERATURA", 1);
    imprimirLinha(22, String(dados.temperature, 1) + " C", 2);
    imprimirLinha(50, classificarTemperatura(dados.temperature), 1);
  } else if (telaAtual == 2) {
    imprimirLinha(0, "LOCALIZACAO", 1);
    imprimirLinha(22, String(dados.distance) + " cm", 2);
    imprimirLinha(50, classificarLocalizacao(dados.distance), 1);
  } else {
    imprimirLinha(0, "STATUS DA MISSAO", 1);
    imprimirLinha(22, dados.missionStatus, 2);
    imprimirLinha(50, dados.emergency ? "SOS acionado" : "Equipe monitorando", 1);
  }

  display.display();
}

// =========================
// API REST
// =========================
void handleHeartbeat() {
  TelemetriaAstroBand dados = lerTelemetria();

  StaticJsonDocument<128> doc;
  doc["heartbeat"] = dados.heartbeat;
  doc["status"] = classificarBatimentos(dados.heartbeat);

  enviarJson(doc);
}

void handleTemperature() {
  TelemetriaAstroBand dados = lerTelemetria();

  StaticJsonDocument<128> doc;
  doc["temperature"] = dados.temperature;
  doc["status"] = classificarTemperatura(dados.temperature);

  enviarJson(doc);
}

void handleLocation() {
  TelemetriaAstroBand dados = lerTelemetria();

  StaticJsonDocument<128> doc;
  doc["distance"] = dados.distance;
  doc["status"] = classificarLocalizacao(dados.distance);

  enviarJson(doc);
}

void handleAstronaut() {
  TelemetriaAstroBand dados = lerTelemetria();

  StaticJsonDocument<256> doc;
  doc["heartbeat"] = dados.heartbeat;
  doc["temperature"] = dados.temperature;
  doc["distance"] = dados.distance;
  doc["emergency"] = dados.emergency;
  doc["status"] = dados.missionStatus;

  enviarJson(doc);
}

void handleRoot() {
  StaticJsonDocument<256> doc;
  doc["project"] = "AstroBand";
  doc["message"] = "API REST online";
  doc["endpoints"][0] = "/heartbeat";
  doc["endpoints"][1] = "/temperature";
  doc["endpoints"][2] = "/location";
  doc["endpoints"][3] = "/astronaut";

  enviarJson(doc);
}

void handleNotFound() {
  adicionarCors();

  if (server.method() == HTTP_OPTIONS) {
    server.send(204);
    return;
  }

  server.send(404, "application/json", "{\"error\":\"endpoint not found\"}");
}

void publicarMqtt(const TelemetriaAstroBand& dados) {
  StaticJsonDocument<256> doc;
  doc["device"] = "ASTROBAND-001";
  doc["astronaut"] = "Astronauta A-01";
  doc["heartbeat"] = dados.heartbeat;
  doc["temperature"] = dados.temperature;
  doc["distance"] = dados.distance;
  doc["emergency"] = dados.emergency;
  doc["status"] = dados.missionStatus;
  doc["millis"] = millis();

  char payload[256];
  serializeJson(doc, payload);

  mqttClient.publish(TOPIC_TELEMETRIA, payload);

  Serial.println("Telemetria MQTT publicada:");
  Serial.println(payload);
}

void configurarRotas() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/heartbeat", HTTP_GET, handleHeartbeat);
  server.on("/temperature", HTTP_GET, handleTemperature);
  server.on("/location", HTTP_GET, handleLocation);
  server.on("/astronaut", HTTP_GET, handleAstronaut);
  server.onNotFound(handleNotFound);
  server.begin();
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(35));

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BOTAO_EMERGENCIA, INPUT_PULLUP);
  pinMode(LED_BATIMENTO, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Falha ao iniciar OLED SSD1306");
  }

  display.clearDisplay();
  imprimirLinha(0, "AstroBand", 2);
  imprimirLinha(28, "Inicializando", 1);
  display.display();

  conectarWifi();
  configurarRotas();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);
  conectarMqtt();

  Serial.println("API REST AstroBand online.");
  Serial.println("Use os endpoints: /heartbeat, /temperature, /location, /astronaut");
  Serial.println("Dashboard Wokwi: use MQTT automatico no navegador.");
}

// =========================
// LOOP PRINCIPAL
// =========================
void loop() {
  TelemetriaAstroBand dados = lerTelemetria();

  if (!mqttClient.connected()) {
    conectarMqtt();
  }

  mqttClient.loop();
  server.handleClient();
  atualizarLedBatimento(dados.heartbeat);
  atualizarLedStatus(dados.missionStatus);
  atualizarOLED(dados);

  if (millis() - ultimoEnvioMqtt >= INTERVALO_MQTT) {
    ultimoEnvioMqtt = millis();
    publicarMqtt(dados);
  }

  if (millis() - ultimoLogApi >= INTERVALO_API_LOG) {
    ultimoLogApi = millis();
    Serial.print("AstroBand | bpm: ");
    Serial.print(dados.heartbeat);
    Serial.print(" | temp: ");
    Serial.print(dados.temperature);
    Serial.print(" C | distancia: ");
    Serial.print(dados.distance);
    Serial.print(" cm | emergencia: ");
    Serial.print(dados.emergency ? "sim" : "nao");
    Serial.print(" | status: ");
    Serial.println(dados.missionStatus);
  }
}
