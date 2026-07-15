# Laboratório MQTT — HidroControl-3

Ambiente autocontido para desenvolvimento e validação da infraestrutura MQTT.

## Arquitetura

ESP32 / AC220
→ Broker Público Mosquitto
→ Bridge MQTT
→ Broker Privado Mosquitto

## Portas

- Broker público MQTT: 1883
- Broker público MQTTS: 8883
- Broker privado MQTT local: 2883
- Broker privado MQTTS local: 2884

## Regras

- Nenhuma configuração será feita diretamente em `/etc/mosquitto`.
- Todos os arquivos permanecerão dentro de `infra/mqtt-lab`.
- O broker privado não será exposto à Internet.
- O armazenamento e tratamento ocorrerão somente no ambiente privado.
- A evolução será incremental, testada e documentada.

