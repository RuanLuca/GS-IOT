# Arquitetura AstroBand

```text
Sensores e entradas
  - Potenciometro: temperatura corporal simulada
  - HC-SR04: distancia/localizacao simulada
  - Push button: emergencia
        ↓
ESP32
  - Leitura dos sensores
  - Classificacao de status
  - Controle dos LEDs
  - Atualizacao do OLED
        ↓
WebServer REST
  - GET /heartbeat
  - GET /temperature
  - GET /location
  - GET /astronaut
        ↓
Canal MQTT de demonstracao
  - Broker HiveMQ
  - Topico fiap/iot/astroband/telemetria
        ↓
Dashboard Web
  - Cards de metricas
  - Grafico simples
  - Status da missao
  - Alerta de emergencia
        ↓
Usuario
  - Equipe de controle da missao
  - Coordenador de operacao terrestre
```

## Observacao sobre REST e MQTT

A API REST continua implementada no ESP32 e atende aos endpoints exigidos. Para a apresentacao no Wokwi, o dashboard tambem recebe dados via MQTT, pois o navegador comum nao acessa diretamente o IP interno `10.10.0.2` da simulacao sem o Wokwi Private IoT Gateway.

## Explicacao para todos os integrantes

Cada integrante pode explicar a arquitetura em uma parte:

1. **Camada fisica:** sensores simples representam tecnologias reais. O potenciometro simula temperatura, o ultrassonico simula localizacao e o botao representa emergencia.
2. **Camada de processamento:** o ESP32 le os dados, calcula batimentos simulados, classifica riscos e controla os LEDs e o OLED.
3. **Camada de comunicacao:** o ESP32 usa Wi-Fi, publica uma API REST local com respostas JSON e tambem envia telemetria MQTT para a demonstracao ao vivo.
4. **Camada de visualizacao:** o dashboard HTML, CSS e JavaScript recebe os dados em tempo real por MQTT no Wokwi; com gateway, tambem pode consultar o endpoint `/astronaut`.
5. **Valor da solucao:** a mesma ideia usada no espaco pode apoiar bombeiros, resgatistas, mineradores e trabalhadores em locais perigosos.
