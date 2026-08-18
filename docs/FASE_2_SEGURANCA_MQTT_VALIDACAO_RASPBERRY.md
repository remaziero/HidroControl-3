# HIDROCONTROL-3
## Fase 2 — Segurança MQTT
### Validação do Broker Privado no Raspberry Pi 3

**Data da validação:** 17/08/2026  
**Status:** VALIDADO — Comunicação MQTT bidirecional operacional

---

## 1. Objetivo

Este documento registra a validação da arquitetura MQTT segura do projeto HIDROCONTROL-3 utilizando um Raspberry Pi 3 como broker MQTT privado.

Foram validados:

- conexão segura entre o Mosquitto privado e o HiveMQ Cloud;
- bridge MQTT utilizando TLS;
- autenticação no broker público;
- transporte de telemetria dos dispositivos para o broker privado;
- transporte de comandos do broker privado para os dispositivos;
- funcionamento com QoS 1;
- comunicação bidirecional com os dispositivos:
  - ac220-000110
  - ac220-888334
- redundância de conectividade de rede do Raspberry Pi.

Nenhuma senha é registrada neste documento.

---

## 2. Arquitetura validada

Fluxo de telemetria:

AC220 / Wokwi
    |
    | MQTT seguro
    v
HiveMQ Cloud
    |
    | Bridge MQTT TLS - porta 8883
    v
Raspberry Pi 3
Mosquitto Privado
    |
    | MQTT local - porta 1884
    v
Aplicações locais / futuros serviços

Fluxo de comandos:

Aplicação local
    |
    v
Mosquitto Privado
Raspberry Pi 3
    |
    | Bridge MQTT TLS - porta 8883
    v
HiveMQ Cloud
    |
    v
AC220 / Wokwi

A comunicação foi validada nos dois sentidos.

---

## 3. Broker público

Broker utilizado:

HiveMQ Cloud

Endpoint configurado:

3f64e3d334e045448a9277d405a8f0da.s1.eu.hivemq.cloud

Porta:

8883

Segurança:

- TLS habilitado;
- TLS 1.2 validado;
- certificado do servidor validado;
- autenticação por username/password;
- acesso controlado por Permissions no HiveMQ Cloud.

Credencial utilizada pela bridge:

hidrocontrol-bridge-v2

Permission associada:

bridge-hivemq-private-v2

Topic Filter autorizado para a bridge:

hidrocontrol/+/+

Permissão:

PUBLISH_SUBSCRIBE

A senha não deve ser armazenada no repositório Git.

---

## 4. Broker privado

Equipamento:

Raspberry Pi 3

Software:

Eclipse Mosquitto 2.0.21

Arquivo principal da configuração específica do HIDROCONTROL-3:

/etc/mosquitto/conf.d/hidrocontrol.conf

Listener privado:

listener 1884 127.0.0.1

Acesso anônimo:

allow_anonymous false

Arquivo de autenticação:

/etc/mosquitto/passwd

Usuário administrativo local:

hidrocontrol-admin

O listener está restrito a 127.0.0.1.

Consequentemente, a porta 1884 não fica diretamente exposta à rede LAN.

---

## 5. Configuração da bridge

Nome da conexão:

hivemq-cloud-para-privado

Servidor remoto:

3f64e3d334e045448a9277d405a8f0da.s1.eu.hivemq.cloud:8883

Client ID:

hidrocontrol-bridge-notemint

Username remoto:

hidrocontrol-bridge-v2

Protocolo:

MQTT v3.1.1

Configuração TLS:

bridge_cafile /etc/ssl/certs/ca-certificates.crt
bridge_tls_version tlsv1.2
bridge_insecure false

Configuração adicional:

try_private false
start_type automatic
restart_timeout 10
cleansession true
notifications false

---

## 6. Tópicos da bridge

### Dispositivo ac220-000110

Telemetria recebida do HiveMQ:

hidrocontrol/ac220-000110/telemetry

Direção:

HiveMQ -> Mosquitto privado

QoS:

1

Comandos enviados ao HiveMQ:

hidrocontrol/ac220-000110/modo

Direção:

Mosquitto privado -> HiveMQ

QoS:

1

### Dispositivo ac220-888334

Telemetria recebida do HiveMQ:

hidrocontrol/ac220-888334/telemetry

Direção:

HiveMQ -> Mosquitto privado

QoS:

1

Comandos enviados ao HiveMQ:

hidrocontrol/ac220-888334/modo

Direção:

Mosquitto privado -> HiveMQ

QoS:

1

---

## 7. Comandos de modo

O firmware aceita payload MQTT em texto simples.

Valores válidos:

NORMAL
DUO
MIX

Também são aceitos:

0 = NORMAL
1 = DUO
2 = MIX

Tópico:

hidrocontrol/<deviceId>/modo

Exemplo:

hidrocontrol/ac220-888334/modo

Payload:

NORMAL

---

## 8. Testes de conectividade externa

Foi validado acesso à Internet a partir do Raspberry Pi.

DNS do endpoint HiveMQ foi resolvido corretamente.

A porta TCP 8883 foi testada com sucesso.

Exemplo do teste:

nc -vz 3f64e3d334e045448a9277d405a8f0da.s1.eu.hivemq.cloud 8883

Resultado:

Connection succeeded.

---

## 9. Validação TLS

Foi realizada conexão direta utilizando OpenSSL.

Resultado obtido:

CONNECTION ESTABLISHED
Protocol version: TLSv1.2
Ciphersuite: ECDHE-RSA-AES128-GCM-SHA256
Peer certificate: CN=*.s1.eu.hivemq.cloud
Verification: OK

Isso confirmou:

- comunicação TCP;
- negociação TLS;
- certificado remoto;
- cadeia de confiança;
- compatibilidade TLS entre Raspberry Pi e HiveMQ Cloud.

---

## 10. Validação MQTT direta com HiveMQ

Foi realizado teste com mosquitto_sub utilizando a credencial:

hidrocontrol-bridge-v2

Também foi utilizado explicitamente o Client ID:

hidrocontrol-bridge-notemint

Resultados:

CONNECT enviado com sucesso.

CONNACK recebido:

CONNACK (0)

SUBSCRIBE enviado com sucesso.

SUBACK recebido.

PINGREQ/PINGRESP também foram observados, comprovando estabilidade da sessão MQTT.

---

## 11. Validação da bridge

Com log de diagnóstico habilitado no Mosquitto foram observadas as etapas:

Connecting bridge (step 1)

Connecting bridge (step 2)

Bridge sending CONNECT

Received CONNACK

Bridge sending SUBSCRIBE

Received SUBACK

Também foi observada recepção real de PUBLISH proveniente do HiveMQ Cloud.

Exemplo validado:

hidrocontrol/ac220-888334/telemetry

O Mosquitto privado respondeu com PUBACK.

Isso comprovou funcionamento da bridge com QoS 1.

---

## 12. Validação da telemetria

Foi realizada assinatura no broker privado utilizando:

hidrocontrol/+/telemetry

Foram recebidas simultaneamente mensagens dos dispositivos:

hidrocontrol/ac220-000110/telemetry

e

hidrocontrol/ac220-888334/telemetry

Isso comprovou o fluxo:

Dispositivo
-> HiveMQ Cloud
-> Bridge TLS
-> Mosquitto privado
-> Cliente MQTT local

---

## 13. Validação dos comandos — ac220-888334

Foi publicado no broker privado:

Tópico:

hidrocontrol/ac220-888334/modo

Payload:

NORMAL

QoS:

1

O comando atravessou:

Mosquitto privado
-> Bridge
-> HiveMQ Cloud
-> ac220-888334

A telemetria posterior do dispositivo confirmou:

"modo":"NORMAL"

Teste aprovado.

---

## 14. Validação dos comandos — ac220-000110

Foi publicado no broker privado:

Tópico:

hidrocontrol/ac220-000110/modo

Payload:

DUO

QoS:

1

O comando atravessou:

Mosquitto privado
-> Bridge
-> HiveMQ Cloud
-> ac220-000110

A telemetria posterior confirmou a alteração para:

"modo":"DUO"

Teste aprovado.

---

## 15. Resultado da validação bidirecional

Os dois dispositivos foram validados.

### ac220-000110

Telemetria:

APROVADA

Comando remoto:

APROVADO

### ac220-888334

Telemetria:

APROVADA

Comando remoto:

APROVADO

A arquitetura MQTT do HIDROCONTROL-3 está operacional nos dois sentidos.

---

## 16. Rede do Raspberry Pi

O Raspberry Pi possui múltiplas possibilidades de conexão.

### Ethernet

Perfil:

Wired connection 1

Autoconnect:

yes

Prioridade:

30

Durante a validação:

Interface:

eth0

IP:

192.168.15.15

Route metric:

100

### Wi-Fi residencial

SSID:

VIVOFIBRA-BBD1

Autoconnect:

yes

Prioridade:

20

Durante a validação:

Interface:

wlan0

IP:

192.168.15.14

Route metric:

600

### Hotspot celular

SSID:

S24 FE de Renato

Autoconnect:

yes

Prioridade:

10

O hotspot permanece cadastrado como alternativa de conectividade.

---

## 17. Prioridade efetiva de rede

Durante o teste foram observadas simultaneamente:

default via 192.168.15.1 dev eth0 metric 100

default via 192.168.15.1 dev wlan0 metric 600

Como a menor métrica possui preferência, a Ethernet foi utilizada como rota principal.

Portanto, a estratégia atual é:

1. Ethernet — principal
2. Wi-Fi residencial — redundância
3. Hotspot celular — alternativa adicional

---

## 18. Acesso remoto ao Raspberry Pi

O acesso SSH pela rede residencial foi validado com sucesso.

Durante os testes:

Wi-Fi:

192.168.15.14

Ethernet:

192.168.15.15

Esses endereços foram fornecidos por DHCP e podem mudar futuramente.

Deve-se considerar posteriormente:

- reserva DHCP no roteador; ou
- outra estratégia controlada de endereçamento.

---

## 19. Segurança

Estado atual:

- broker público protegido por TLS;
- autenticação HiveMQ habilitada;
- permissions específicas no HiveMQ;
- bridge autenticada;
- listener privado restrito a localhost;
- allow_anonymous false;
- broker privado protegido por senha;
- senhas não devem ser armazenadas no Git;
- QoS 1 utilizado nos tópicos da bridge.

IMPORTANTE:

Arquivos contendo senhas, chaves privadas ou outros segredos não devem ser versionados em repositório público.

---

## 20. Estado consolidado

Em 17/08/2026 foi comprovada experimentalmente a seguinte arquitetura:

AC220-000110 ─┐
              ├──> HiveMQ Cloud
AC220-888334 ─┘        |
                       | MQTT/TLS :8883
                       | QoS 1
                       v
                Raspberry Pi 3
                Mosquitto Privado
                127.0.0.1:1884
                       |
                       v
                Serviços locais

No sentido contrário:

Serviço local
-> Mosquitto privado
-> Bridge MQTT/TLS
-> HiveMQ Cloud
-> dispositivo
-> execução do comando
-> nova telemetria
-> HiveMQ Cloud
-> Bridge
-> Mosquitto privado

O ciclo completo foi comprovado nos dois dispositivos.

---

## 21. Conclusão

A infraestrutura MQTT segura do HIDROCONTROL-3 atingiu um marco funcional importante.

Foram comprovados:

- broker público operacional;
- broker privado operacional no Raspberry Pi 3;
- bridge MQTT segura;
- TLS 1.2;
- autenticação;
- autorização por tópicos;
- QoS 1;
- recepção de telemetria;
- envio de comandos;
- confirmação dos comandos pela telemetria;
- comunicação com dois dispositivos;
- conectividade Ethernet e Wi-Fi;
- acesso SSH remoto ao Raspberry Pi.

A arquitetura está apta a servir como base para as próximas etapas do HIDROCONTROL-3.

Próximas etapas deverão preservar este estado funcional antes de introduzir novas funcionalidades.
