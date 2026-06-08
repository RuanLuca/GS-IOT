# Documentacao tecnica - AstroBand

## Microcontrolador

O ESP32 foi escolhido por possuir Wi-Fi integrado, boa disponibilidade no Wokwi e compatibilidade com as bibliotecas usadas na disciplina.

## Entradas

| Entrada | Pino | Descricao |
|---|---:|---|
| Potenciometro | GPIO 34 | Simula temperatura corporal entre 34.0 C e 40.5 C |
| HC-SR04 TRIG | GPIO 5 | Pulso de disparo do sensor ultrassonico |
| HC-SR04 ECHO | GPIO 18 | Retorno do pulso ultrassonico |
| Botao de emergencia | GPIO 14 | Entrada digital com `INPUT_PULLUP` |

## Saidas

| Saida | Pino | Descricao |
|---|---:|---|
| LED vermelho | GPIO 26 | Pisca conforme batimentos simulados |
| LED azul | GPIO 27 | Indica status da missao |
| OLED SDA | GPIO 21 | Barramento I2C |
| OLED SCL | GPIO 22 | Barramento I2C |

## API REST

A API usa a biblioteca `WebServer.h` e responde em JSON com `ArduinoJson`. O dashboard consome principalmente `/astronaut`, pois esse endpoint consolida todos os dados em uma unica resposta.

## MQTT para demonstracao ao vivo

O projeto tambem usa `PubSubClient` para publicar a mesma telemetria no broker publico HiveMQ. Essa camada foi adicionada para a apresentacao no Wokwi, porque o IP interno do ESP32 simulado, como `10.10.0.2`, nao fica acessivel diretamente pelo navegador sem o Wokwi Private IoT Gateway.

| Recurso | Valor |
|---|---|
| Broker | `broker.hivemq.com` |
| Porta ESP32 | `1883` |
| WebSocket dashboard | `wss://broker.hivemq.com:8884/mqtt` |
| Topico principal | `fiap/iot/astroband/telemetria` |

O ESP32 continua expondo os endpoints REST obrigatorios. O MQTT funciona como canal de telemetria em tempo real para que o dashboard atualize durante a demonstracao presencial.

## Logica de status

- Temperatura normal: entre 35.5 C e 37.9 C.
- Temperatura critica: 38.0 C ou mais.
- Localizacao segura: ate 150 cm.
- Localizacao em atencao: de 151 cm a 220 cm.
- Localizacao perigosa: acima de 220 cm.
- Emergencia: botao pressionado.

## Dashboard

O dashboard e composto por:

- `index.html`: estrutura visual.
- `style.css`: estilo responsivo.
- `script.js`: assinatura MQTT para demonstracao ao vivo, opcao REST para uso com gateway, atualizacao dos cards e desenho do grafico em canvas.

## Observacao sobre simulacao

Em uma versao fisica, a AstroBand poderia usar sensores biometricos reais, GPS, LoRa, 4G/5G ou comunicacao via satelite. No prototipo academico, os componentes simples reduzem complexidade e deixam o foco na arquitetura IoT.
