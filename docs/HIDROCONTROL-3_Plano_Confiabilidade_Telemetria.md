# HIDROCONTROL-3 — Plano de confiabilidade da telemetria

**Branch:** `feature/v5-mqtt-self-recovery`  
**Branch-base:** `feature/v5-sta-ap-recovery`  
**Commit-base:** `10a626b`  
**Data do planejamento:** 2026-09-14

## Contexto

Foi observado que um dispositivo deixou de publicar telemetria enquanto o LED continuou piscando. O indicador atual demonstra apenas atividade do firmware e não comprova conectividade nem entrega de mensagens.

## Objetivo

Implementar recuperação em camadas para:

- confirmar publicações de forma confiável;
- recuperar conexões de modo controlado;
- reiniciar automaticamente após falhas prolongadas;
- permitir reinicialização remota individual;
- fazer o LED representar o estado real;
- registrar ausência, recuperação e motivo no servidor.

## Ordem de implementação

### 0. Preservação e inspeção

- Trabalhar somente nesta branch.
- Confirmar o estado Git antes de alterações.
- Inspecionar as rotinas atuais de rede, mensageria, tarefas e LED.
- Não alterar firmware, servidor e página simultaneamente.
- Compilar a linha-base antes da primeira mudança.

### 1. Confirmação real de publicação

Registrar separadamente:

- estado da rede;
- estado da sessão de mensageria;
- última tentativa de envio;
- última confirmação;
- falhas consecutivas;
- tempo desde a última confirmação.

O estado de conexão, isoladamente, não será considerado prova de publicação.

### 2. Reconexão escalonada

Aplicar recuperação progressiva:

1. repetir a operação;
2. reconstruir a sessão de mensageria;
3. recuperar a conexão de rede;
4. aguardar intervalos progressivos;
5. reiniciar somente depois de exceder limites definidos.

Os tempos serão definidos após medir o ciclo atual de telemetria.

### 3. Reinicialização automática

Antes de reiniciar:

- colocar as saídas em estado seguro;
- registrar o motivo;
- transmitir o diagnóstico, quando possível;
- aguardar um intervalo curto;
- executar reinicialização controlada.

Prevenir ciclos contínuos de reinicialização.

### 4. Reinicialização remota individual

Cada dispositivo deverá aceitar um comando individual e seguro.

Requisitos:

- entrega confirmada;
- comando não persistente;
- autorização restrita;
- proteção contra repetição;
- saídas em estado seguro;
- confirmação do pedido e do retorno.

Os detalhes do protocolo serão mantidos fora deste documento público.

### 5. Controle pela página

Para cada dispositivo:

- exibir botão de reinicialização;
- solicitar confirmação explícita;
- acompanhar envio, aceite, queda e retorno;
- declarar sucesso somente após a retomada da telemetria.

### 6. LED como indicador real

Proposta inicial:

| Comportamento | Estado |
|---|---|
| Piscada lenta | comunicação e publicações confirmadas |
| Duas piscadas agrupadas | sessão de mensageria indisponível |
| Piscada rápida | rede indisponível |
| Aceso contínuo antes do reinício | falha prolongada |

O padrão definitivo será validado nos testes.

### 7. Supervisão no servidor

Registrar por dispositivo:

- última telemetria;
- tempo de ausência;
- último estado conhecido;
- recuperação solicitada;
- retorno observado;
- motivo informado.

O servidor só declarará recuperação após observar nova telemetria.

## Estratégia de testes

1. Compilar a linha-base.
2. Testar inicialmente em ambiente simulado.
3. Simular perda da sessão mantendo a rede.
4. Simular perda de rede.
5. Simular ausência de confirmações.
6. Validar recuperação sem reinicialização.
7. Forçar reinicialização automática.
8. Testar o comando remoto individual.
9. Validar todos os estados do LED.
10. Confirmar os registros no servidor.
11. Validar em apenas um dispositivo físico.
12. Manter os demais intactos até a validação.
13. Observar funcionamento prolongado antes da expansão.

## Evolução física futura

Se a recuperação por software não cobrir travamentos completos, avaliar posteriormente um supervisor independente ou controle externo de alimentação. Qualquer intervenção em equipamento ligado à rede elétrica exigirá projeto e isolamento adequados.

## Checklist de retomada

1. Buscar e selecionar esta branch no computador de trabalho.
2. Confirmar branch, commit e árvore Git.
3. Não incorporar cópias antigas ou arquivos de backup.
4. Localizar o tratamento de eventos da mensageria.
5. Localizar a tarefa atual do LED.
6. Localizar as rotinas de recuperação de rede.
7. Confirmar a qualidade de serviço usada na telemetria.
8. Documentar o ciclo e os tempos atuais.
9. Implementar somente a confirmação real de publicação.
10. Compilar e testar antes da etapa seguinte.

## Critério de sucesso

A etapa estará concluída quando:

- o LED normal representar publicação confirmada;
- falhas temporárias forem recuperadas sem reinicialização;
- falhas prolongadas resultarem em reinicialização segura;
- o comando individual afetar somente o equipamento escolhido;
- a página acompanhar o ciclo completo;
- o servidor registrar ausência, recuperação e motivo;
- todos os dispositivos permanecerem estáveis em observação prolongada.
