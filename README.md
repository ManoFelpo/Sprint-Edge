# Detector de Gol IoT — “Passa a Bola”

Sistema IoT que detecta gol com dois sensores por lado (feixe IR + vibração).  
O ESP32 decide o evento e publica, via **MQTT**, o **placar acumulado** no tópico **`match/score`**.  
O Node-RED e o aplicativo MQTT Client exibem o placar em tempo real.

---

## Integrantes
- Felipe Santos Nunes — RM563919
- Felipe Ramalho Junqueira Berto — RM562148

---

## Objetivo
Implementar a arquitetura de uma aplicação IoT capaz de coletar e transmitir, em tempo real, o placar de uma partida de futebol de mesa/mini-campo, com visualização simultânea em dashboard web e aplicativo móvel.

---

## Arquitetura
```
mermaid
flowchart LR
  subgraph Campo
    IRL[IR – Esquerda] --> ESP32
    VBL[Vib – Esquerda] --> ESP32
    IRR[IR – Direita] --> ESP32
    VBR[Vib – Direita] --> ESP32
  end
  ESP32 -->|MQTT pub: match/score| Broker[(HiveMQ Cloud)]
  Broker -->|MQTT sub| NodeRED[Node-RED Dashboard]
  Broker -->|MQTT sub| App[MQTT Client (Android)]
```

### Lógica de detecção
1. **Sensor IR** arma o lado ao detectar a passagem da bola (borda HIGH→LOW).  
2. **Sensor de vibração** confirma o gol se ocorrer em até **300 ms** após o IR.  
3. Lockout de **2 s** evita contagens duplicadas.  
4. O ESP32 mantém `scoreLeft` e `scoreRight` e publica no tópico:
```json
{"Tigers":2,"Lions":1,"t_ms":123456}
```

---

## Recursos necessários
- **ESP32 DevKit v1** (ou simulação Wokwi)  
- 2 sensores **IR break-beam**  
- 2 sensores **SW-420** (vibração da rede)  
- Broker **HiveMQ Cloud** (MQTT TLS 8883)  
- **Node-RED** para dashboard  
- **MQTT Client App** (Android/iOS)  

---

## Como rodar

### 1️⃣ Simulação no Wokwi
1. Abra a pasta `wokwi/` no site **wokwi.com**, clique em **Import Project** e selecione `diagram.json`.  
2. No arquivo `firmware/esp32_goal_detector.ino`, edite:
   ```
   const char* MQTT_HOST   = "<seu cluster>.s1.eu.hivemq.cloud";
   const char* MQTT_USER   = "<seu usuário>";
   const char* MQTT_PASSWD = "<sua senha>";
   ```
3. Clique ▶ para iniciar a simulação.  
   O ESP32 conecta-se ao Wi-Fi `Wokwi-GUEST` (sem senha) e publica o placar em `match/score`.  
4. No **HiveMQ Web Client** (porta **8884** WebSocket TLS), faça **Subscribe** em `match/score` e veja o placar chegar ao vivo.

---

### 2️⃣ Dashboard em Node-RED
1. Abra o Node-RED (local ou em servidor) e importe `nodered/flows.json`.  
2. Edite o nó **MQTT Broker** com o host, usuário e senha do seu HiveMQ Cloud.  
3. Deploy.  
   O dashboard exibirá:
   * **Placar em tempo real** (gols esquerda/direita);  
   * Histórico de gols recebidos.

---

### 3️⃣ Aplicativo móvel (MQTT Client)
O sistema também permite acompanhar o placar **diretamente pelo celular**, usando o aplicativo **MQTT Client (Android)** ou outro compatível.

**Configuração do app:**
- **Name:** SensorDeGol  
- **Broker URL:** `7a6f646726c34e7890ffe14576912033.s1.eu.hivemq.cloud`  
- **Port:** `8883`  
- **Protocol:** SSL/TLS habilitado  
- **Client ID:** qualquer string única (ex: `mqtt_app_gol`)  
- **Username:** `SensorDeGol`  
- **Password:** `1aA2bB3cC4dD`  

**Tópico assinado:**  
```
match/score
```

**Funcionamento:**  
1. O ESP32 publica o placar atualizado após cada gol.  
2. O broker HiveMQ Cloud retransmite a mensagem para todos os inscritos.  
3. O app recebe instantaneamente os valores de `Tigers` e `Lions`, exibindo o resultado ao vivo.

Essa etapa comprova a **mobilidade e escalabilidade** do projeto, mostrando que o sistema IoT pode ser monitorado de qualquer lugar.

---

### 4️⃣ Hardware real (opcional)
1. Monte os sensores:
   * IR-Left → GPIO 18  
   * Vib-Left → GPIO 19  
   * IR-Right → GPIO 21  
   * Vib-Right → GPIO 22  
   * LEDs (opcional) → GPIO 25 e 26 (com resistores de 220 Ω)
2. Compile e carregue `esp32_goal_detector.ino` no seu ESP32.  
3. Conecte-o à rede Wi-Fi configurada e assine `match/score` no HiveMQ Web Client, Node-RED ou app MQTT Client.

---

## Tópicos MQTT
- **`match/score`** – placar acumulado:
  ```json
  { "Tigers": N, "Lions": M, "t_ms": 123456 }
  ```
- **`match/status`** – estado online (mensagem retained):
  ```json
  { "online": true }
  ```

---

## Licença
MIT

---

## Vídeo de Apresentação
[🎥 Acesso à apresentação](https://youtu.be/CnNR35LjmcM)
