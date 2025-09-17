#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ======== WIFI + MQTT (HiveMQ Cloud) ========
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_HOST = "7a6f646726c34e7890ffe14576912033.s1.eu.hivemq.cloud";
const int   MQTT_PORT = 8883;
const char* MQTT_USER = "SensorDeGol";
const char* MQTT_PASSWD = "1aA2bB3cC4dD";

WiFiClientSecure net;
PubSubClient mqtt(net);

// tópicos
const char* TOPIC_SCORE  = "match/score";   // << novo: placar acumulado
const char* TOPIC_STATUS = "match/status";

// Pinos (botões simulam sensores: apertado = LOW)
const int PIN_IR_L  = 18;   // botão IR esquerda
const int PIN_VIB_L = 19;   // botão vib esquerda
const int PIN_IR_R  = 21;   // botão IR direita
const int PIN_VIB_R = 22;   // botão vib direita
const int PIN_LED_L = 25;   // LED gol esquerda
const int PIN_LED_R = 26;   // LED gol direita

// Times por lado (só para debug/prints locais se quiser usar)
const char* TEAM_LEFT  = "Tigers";
const char* TEAM_RIGHT = "Lions";

// Janelas/tempos
const unsigned long WINDOW_MS  = 300;
const unsigned long LOCKOUT_MS = 2000;
const unsigned long LED_MS     = 500;

// estado da detecção
bool armedL=false, armedR=false;
unsigned long armAtL=0, armAtR=0, lastGoalAt=0, ledOffL=0, ledOffR=0;
int pirL=HIGH, pvbL=HIGH, pirR=HIGH, pvbR=HIGH;

// placar acumulado
int scoreLeft = 0;
int scoreRight = 0;

const char* mqttStateStr(int s){
  switch(s){case-4:return"TIMEOUT";case-3:return"LOST";case-2:return"FAILED";
  case-1:return"DISC";case0:return"OK";case1:return"BAD_PROTO";case2:return"BAD_ID";
  case3:return"UNAV";case4:return"BAD_CRED";case5:return"UNAUTH";default:return"?";}
}

void wifiConnect(){
  Serial.print("WiFi "); Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while(WiFi.status()!=WL_CONNECTED){ delay(400); Serial.print("."); }
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());
}

bool mqttConnect(){
  net.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  String cid="esp32-gol-"+String((uint32_t)ESP.getEfuseMac(),HEX);
  Serial.print("MQTT "); Serial.print(MQTT_HOST); Serial.print(":"); Serial.println(MQTT_PORT);
  bool ok=mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASSWD , TOPIC_STATUS, 0, true, "{\"online\":false}");
  Serial.print("MQTT connect: "); Serial.println(ok? "OK": mqttStateStr(mqtt.state()));
  if(ok) mqtt.publish(TOPIC_STATUS, "{\"online\":true}", true);
  return ok;
}
void ensureMQTT(){ if(!mqtt.connected()) mqttConnect(); mqtt.loop(); }

// ---- NOVO: publica o placar acumulado ----
void publishScore() {
  char buf[160];
  snprintf(buf, sizeof(buf),
           "{\"Tigers\":%d,\"Lions\":%d,\"t_ms\":%lu}",
           scoreLeft, scoreRight, (unsigned long)millis());
  bool ok = mqtt.publish(TOPIC_SCORE, buf);
  Serial.print("PUB SCORE "); Serial.println(ok ? buf : "FALHA");
}

void setup(){
  Serial.begin(115200); delay(200);
  pinMode(PIN_IR_L,INPUT_PULLUP); pinMode(PIN_VIB_L,INPUT_PULLUP);
  pinMode(PIN_IR_R,INPUT_PULLUP); pinMode(PIN_VIB_R,INPUT_PULLUP);
  pinMode(PIN_LED_L,OUTPUT); pinMode(PIN_LED_R,OUTPUT);
  digitalWrite(PIN_LED_L,LOW); digitalWrite(PIN_LED_R,LOW);
  wifiConnect(); mqttConnect();
}

void loop(){
  ensureMQTT();
  unsigned long now=millis();
  bool locked=(now-lastGoalAt<LOCKOUT_MS);

  // apaga LEDs quando chegar a hora
  if(ledOffL && now>=ledOffL){ digitalWrite(PIN_LED_L,LOW); ledOffL=0; }
  if(ledOffR && now>=ledOffR){ digitalWrite(PIN_LED_R,LOW); ledOffR=0; }

  // expira janelas
  if(armedL && now-armAtL>WINDOW_MS) armedL=false;
  if(armedR && now-armAtR>WINDOW_MS) armedR=false;

  // leituras atuais (botão solto=HIGH, pressionado=LOW)
  int irL=digitalRead(PIN_IR_L),  vibL=digitalRead(PIN_VIB_L);
  int irR=digitalRead(PIN_IR_R),  vibR=digitalRead(PIN_VIB_R);

  // borda IR arma janela
  if(!locked && pirL==HIGH && irL==LOW){ armedL=true; armAtL=now; Serial.println("[L] IR"); }
  if(!locked && pirR==HIGH && irR==LOW){ armedR=true; armAtR=now; Serial.println("[R] IR"); }

  // borda vib dentro da janela => gol + atualiza placar
  if(!locked && armedL && pvbL==HIGH && vibL==LOW){
    armedL=false; lastGoalAt=now;
    digitalWrite(PIN_LED_L,HIGH); ledOffL=now+LED_MS;
    scoreLeft++;           // << incrementa
    publishScore();        // << envia placar acumulado
  }
  if(!locked && armedR && pvbR==HIGH && vibR==LOW){
    armedR=false; lastGoalAt=now;
    digitalWrite(PIN_LED_R,HIGH); ledOffR=now+LED_MS;
    scoreRight++;          // << incrementa
    publishScore();        // << envia placar acumulado
  }

  // atualiza estados anteriores
  pirL=irL; pvbL=vibL; pirR=irR; pvbR=vibR;
}
