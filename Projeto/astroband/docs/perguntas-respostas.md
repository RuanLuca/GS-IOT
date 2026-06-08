# Perguntas provaveis e respostas

## Por que voces nao usaram sensores medicos reais?

Porque a proposta e academica e demonstrativa. Usamos componentes simples para representar sensores reais e manter o foco na arquitetura IoT, conforme a abordagem de prototipacao da disciplina.

## O que representa o potenciometro?

Ele simula a variacao da temperatura corporal. Ao girar o potenciometro no Wokwi, a leitura muda entre aproximadamente 34.0 C e 40.5 C.

## Como os batimentos cardiacos sao medidos?

Nesta prova de conceito eles sao simulados por software e representados pelo LED vermelho piscando. Em uma versao fisica, poderiamos usar um sensor como MAX30102.

## Qual e a funcao do HC-SR04?

Ele simula distancia/localizacao do astronauta em relacao a uma referencia. No mundo real, essa camada poderia ser substituida por GPS, UWB, BLE beacon ou comunicacao via infraestrutura da missao.

## Por que usar API REST?

Porque REST e simples de testar, usa HTTP e retorna JSON, facilitando a integracao entre ESP32 e dashboard web.

## Como o dashboard recebe os dados?

Na versao de demonstracao no Wokwi, o JavaScript assina o topico MQTT `fiap/iot/astroband/telemetria` e atualiza os cards em tempo real. A API REST continua implementada no ESP32, mas o acesso direto ao IP interno `10.10.0.2` depende do Wokwi Private IoT Gateway.

## Por que o dashboard usa MQTT se o requisito pede API REST?

Porque a API REST esta implementada no ESP32 com os endpoints obrigatorios. O MQTT foi adicionado como canal auxiliar para a apresentacao presencial, ja que o Wokwi gratuito isola o IP interno do ESP32 e impede o navegador de acessar diretamente o WebServer REST.

## O que acontece quando o botao de emergencia e pressionado?

O campo `emergency` passa para `true`, o status da missao muda para emergencia, o LED azul pisca mais rapidamente e o dashboard exibe o alerta.

## Como isso se conecta com a Terra?

A mesma arquitetura pode monitorar profissionais em ambientes perigosos, como bombeiros, resgatistas, mineradores e operadores em plataformas de petroleo.

## O projeto usa IA generativa?

Nesta versao, o foco principal esta em IoT e arquitetura de dados. Como evolucao, a IA generativa poderia gerar relatorios automaticos da missao, resumir incidentes e sugerir acoes para a equipe.

## Qual e o principal aprendizado tecnico?

Integrar sensores, atuadores, Wi-Fi, API REST, JSON, interface local OLED e dashboard web em uma solucao completa de IoT.
