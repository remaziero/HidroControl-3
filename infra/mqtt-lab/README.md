.# Laboratório MQTT — HidroControl-3

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

## Etapa 1 - Broker Público

Objetivo:
Disponibilizar um broker MQTT funcional para o laboratório.

Status:
✔ concluído

Características:

- Mosquitto 2.0.18
- Porta 1883
- Sem TLS
- Sem usuários
- Sem ACL
- Sem bridge
- allow_anonymous=true
- Persistência desabilitada

Testes executados:

✔ Broker iniciou corretamente.

✔ mosquitto_sub conectado.

✔ mosquitto_pub publicou mensagem.

✔ Mensagem recebida com sucesso.

## Etapa 1 — Broker Público

Status: concluída e testada.

Características:

- Mosquitto 2.0.18;
- listener na porta 1883;
- acesso anônimo temporariamente permitido;
- sem TLS;
- sem usuários;
- sem ACL ativa;
- sem persistência;
- sem bridge.

Teste realizado:

- conexão com `mosquitto_sub`;
- publicação com `mosquitto_pub`;
- recebimento da mensagem no tópico `hidrocontrol/lab/teste`.

## Etapa 2 — Broker Privado

Status: concluída e testada.

Características:

- Mosquitto 2.0.18;
- listener na porta 1884;
- acesso anônimo temporariamente permitido;
- sem TLS;
- sem usuários;
- sem ACL ativa;
- sem persistência;
- sem bridge.

Teste realizado:

- conexão com `mosquitto_sub`;
- publicação com `mosquitto_pub`;
- recebimento da mensagem no tópico
  `hidrocontrol/privado/lab/teste`.

## Estado atual do laboratório

Os brokers Público e Privado estão operacionais e independentes.

| Componente | Porta | Situação |
|---|---:|---|
| Broker Público | 1883 | Operacional |
| Broker Privado | 1884 | Operacional |
| Bridge | — | Ainda não configurada |
| Usuários | — | Ainda não configurados |
| ACL | — | Ainda não configurada |
| TLS | — | Ainda não configurado |

A próxima etapa será a criação de uma bridge unidirecional do Broker
Público para o Broker Privado.
