# Roteiro de apresentacao - 3 minutos

## 0:00 a 0:30 - Abertura

Somos o grupo da AstroBand, uma pulseira inteligente inspirada em missoes espaciais. O problema escolhido e o monitoramento de pessoas em ambientes extremos, onde temperatura corporal, batimentos, localizacao e emergencia precisam ser acompanhados rapidamente.

## 0:30 a 1:05 - Conexao com o desafio

A exploracao espacial exige telemetria constante dos astronautas. A nossa proposta traz esse conceito para aplicacoes na Terra, como bombeiros, mineradores, equipes de resgate e trabalhadores em plataformas de petroleo.

## 1:05 a 1:50 - Prototipo IoT

No Wokwi, usamos um ESP32 com Wi-Fi. O potenciometro simula temperatura corporal, o sensor HC-SR04 simula distancia/localizacao e o botao representa emergencia. O LED vermelho pisca como batimento cardiaco, o LED azul indica status da missao e o OLED mostra telas locais com batimentos, temperatura, localizacao e status.

## 1:50 a 2:30 - API, MQTT e dashboard

O ESP32 executa um WebServer REST com os endpoints obrigatorios: `/heartbeat`, `/temperature`, `/location` e `/astronaut`. Como o Wokwi gratuito isola o IP interno do ESP32 e impede o acesso direto pelo navegador, tambem adicionamos uma publicacao MQTT para demonstrar o dashboard funcionando ao vivo. Assim, o dashboard recebe a telemetria em tempo real, atualiza os cards, o grafico, o status da missao e o alerta de emergencia.

## 2:30 a 3:00 - Encerramento

A AstroBand demonstra como tecnologias pensadas para ambientes espaciais podem gerar valor na Terra. Mesmo com sensores simulados, o projeto mostra uma arquitetura IoT completa: coleta, processamento, comunicacao, interface local, API REST e visualizacao remota em tempo real.

## Frase para justificar o uso do MQTT na demonstracao

"A API REST exigida esta implementada no ESP32. O MQTT foi usado como canal auxiliar de demonstracao porque, no Wokwi gratuito, o navegador nao acessa diretamente o IP interno do ESP32. Dessa forma conseguimos apresentar o dashboard funcionando ao vivo sem remover os endpoints REST."
