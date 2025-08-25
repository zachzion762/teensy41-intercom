#include <QNEthernet.h>   // QNEthernet (Shawn Silverman)
#include <PubSubClient.h>
#include "secrets.h"      // <-- this file is local-only; not committed

using namespace qindesign::network;

// -------- Buttons (press = GND) --------
static const int PINS[] = {1,3,5,7,9,10,12,24,26,28,30,32};
static const int N = sizeof(PINS)/sizeof(PINS[0]);

// Topics / names (safe to keep public)
static const char* BASE_TOPIC  = "intercom";
static const char* AVAIL_TOPIC = "intercom/availability";
static const char* DISC_BASE   = "homeassistant/binary_sensor";
static const char* DEVICE_NAME = "teensy-intercom";

// -------- Internals --------
EthernetClient net;
PubSubClient   mqtt(net);

const int LEDPIN = 13;
const uint32_t DEBOUNCE_MS   = 20;
const uint32_t REDISCOVER_MS = 300000; // 5 min

int lastStable = -1, lastRead = -1;
uint32_t lastChange = 0, lastDiscovery = 0, lastStatus = 0;

static inline void blinkN(uint8_t n, uint16_t on=80, uint16_t off=120){
  for(uint8_t i=0;i<n;i++){ digitalWrite(LEDPIN,HIGH); delay(on); digitalWrite(LEDPIN,LOW); delay(off); }
}

static inline int readPressed(){
  for(int i=0;i<N;i++) if(digitalRead(PINS[i])==LOW) return i;
  return -1;
}

static void publishButtonState(int idx, const char* s){
  char t[96];
  snprintf(t, sizeof(t), "%s/button/%d", BASE_TOPIC, idx+1);
  mqtt.publish(t, s, false);
}

static void publishAllOff(){
  for(int i=0;i<N;i++) publishButtonState(i,"OFF");
}

static void publishDiscovery(){
  char dev_id[96];
  // unique but non-sensitive id (from secrets.h)
  snprintf(dev_id, sizeof(dev_id), "%s-%s", DEVICE_NAME, DEVICE_UNIQ_SUFFIX);

  for(int i=0;i<N;i++){
    int n = i+1;
    char disc_topic[160];  snprintf(disc_topic,sizeof(disc_topic), "%s/%s_%d/config", DISC_BASE, DEVICE_NAME, n);
    char state_topic[128]; snprintf(state_topic,sizeof(state_topic), "%s/button/%d", BASE_TOPIC, n);
    char unique_id[96];    snprintf(unique_id,sizeof(unique_id), "%s_btn_%d", dev_id, n);

    char payload[640];
    snprintf(payload,sizeof(payload),
      "{"
        "\"name\":\"Intercom Button %d\","
        "\"uniq_id\":\"%s\","
        "\"stat_t\":\"%s\","
        "\"pl_on\":\"ON\",\"pl_off\":\"OFF\","
        "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"Teensy Intercom\",\"mf\":\"PJRC\",\"mdl\":\"Teensy 4.1\",\"sw\":\"Intercom v1.0\"}"
      "}",
      n, unique_id, state_topic, AVAIL_TOPIC, dev_id
    );
    mqtt.publish(disc_topic, payload, true);
  }
  lastDiscovery = millis();
}

static void ensureMQTT(){
#if defined(MQTT_HOST_NAME)
  mqtt.setServer(MQTT_HOST_NAME, MQTT_PORT);
#else
  mqtt.setServer(MQTT_HOST_IP, MQTT_PORT);
#endif
  mqtt.setKeepAlive(20);

  while(!mqtt.connected()){
    bool ok = mqtt.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS,
                           AVAIL_TOPIC, 1, true, "offline");
    if(ok){
      mqtt.publish(AVAIL_TOPIC, "online", true);
      publishDiscovery();
      publishAllOff();
      blinkN(3);
    }else{
      if(Ethernet.linkState()) blinkN(2); else blinkN(1);
      delay(700);
    }
  }
}

void setup(){
  pinMode(LEDPIN, OUTPUT);
  for(int i=0;i<N;i++) pinMode(PINS[i], INPUT_PULLUP);

#if defined(USE_FIXED_MAC)
  Ethernet.setMACAddress(FIXED_MAC);
#endif

#if defined(STATIC_NET)
  Ethernet.begin(T_IP, T_MASK, T_GW);
  Ethernet.setDNSServerIP(T_DNS);
#else
  Ethernet.begin();  // DHCP by default
#endif

  // wait up to ~8s for link
  uint32_t t0 = millis();
  while(!Ethernet.linkState() && (millis()-t0 < 8000)){ blinkN(1); }

  if(Ethernet.linkState()) blinkN(2);
  ensureMQTT();
}

void loop(){
  if(Ethernet.linkState()){
    if(!mqtt.connected()) ensureMQTT();
    mqtt.loop();
  }

  if(mqtt.connected() && millis()-lastDiscovery > REDISCOVER_MS) publishDiscovery();

  uint32_t now = millis();
  int cur = readPressed();
  if(cur != lastRead){ lastRead = cur; lastChange = now; }
  if((now - lastChange) >= DEBOUNCE_MS && cur != lastStable){
    if(cur >= 0){
      if(lastStable >= 0 && lastStable != cur) publishButtonState(lastStable,"OFF");
      publishButtonState(cur,"ON");
    }else{
      if(lastStable >= 0) publishButtonState(lastStable,"OFF");
    }
    lastStable = cur;
  }

  if(now - lastStatus > 2000){
    lastStatus = now;
    if(!Ethernet.linkState()) blinkN(1);
    else if(!mqtt.connected()) blinkN(2);
    else blinkN(3);
  }
}
