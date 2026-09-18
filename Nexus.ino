
#include <Arduino.h>
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "AppInsights.h"

#define R1_PIN 18
#define R2_PIN 19

#define DEFAULT_POWER_MODE false

const char *service_name = "CURTAIN_1234";
const char *pop = "abcd1234";

static Device *curtain = NULL;

void curtainStop() {
  digitalWrite(R1_PIN, LOW);
  digitalWrite(R2_PIN, LOW);
  Serial.println("Curtain: STOP");
}

void curtainForward() {
  curtainStop();
  delay(300);

  digitalWrite(R1_PIN, HIGH);
  digitalWrite(R2_PIN, LOW);

  Serial.println("Curtain: FORWARD");
}

void curtainReverse() {
  curtainStop();
  delay(300);

  digitalWrite(R1_PIN, LOW);
  digitalWrite(R2_PIN, HIGH);

  Serial.println("Curtain: REVERSE");
}

void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {

    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32S2
      Serial.printf(
        "\nProvisioning Started with name \"%s\" and PoP \"%s\" on SoftAP\n",
        service_name,
        pop
      );

      WiFiProv.printQR(service_name, pop, "softap");

#else
      Serial.printf(
        "\nProvisioning Started with name \"%s\" and PoP \"%s\" on BLE\n",
        service_name,
        pop
      );

      WiFiProv.printQR(service_name, pop, "ble");
#endif
      break;

    case ARDUINO_EVENT_PROV_INIT:
      WiFiProv.disableAutoStop(10000);
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      WiFiProv.endProvision();
      break;

    default:
      break;
  }
}

void write_callback(
  Device *device,
  Param *param,
  const param_val_t val,
  void *priv_data,
  write_ctx_t *ctx
) {
  const char *param_name = param->getParamName();

  Serial.printf("RainMaker command: %s\n", param_name);

  if (strcmp(param_name, "Forward") == 0) {
    if (val.val.b) {
      curtainForward();
    }
    param->updateAndReport(val);
  }

  else if (strcmp(param_name, "Reverse") == 0) {
    if (val.val.b) {
      curtainReverse();
    }
    param->updateAndReport(val);
  }

  else if (strcmp(param_name, "Stop") == 0) {
    if (val.val.b) {
      curtainStop();
    }
    param->updateAndReport(val);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(R1_PIN, OUTPUT);
  pinMode(R2_PIN, OUTPUT);

  curtainStop();

  Node my_node;

  my_node = RMaker.initNode("Curtain Controller");

  curtain = new Device("Curtain", "custom.device.curtain", NULL);

  if (!curtain) {
    Serial.println("Failed to create Curtain device");
    return;
  }

  Param forward(
    "Forward",
    ESP_RMAKER_PARAM_TOGGLE,
    value(false),
    PROP_FLAG_READ | PROP_FLAG_WRITE
  );

  forward.addUIType(ESP_RMAKER_UI_TOGGLE);
  curtain->addParam(forward);

  Param reverse(
    "Reverse",
    ESP_RMAKER_PARAM_TOGGLE,
    value(false),
    PROP_FLAG_READ | PROP_FLAG_WRITE
  );

  reverse.addUIType(ESP_RMAKER_UI_TOGGLE);
  curtain->addParam(reverse);

  Param stop(
    "Stop",
    ESP_RMAKER_PARAM_TOGGLE,
    value(false),
    PROP_FLAG_READ | PROP_FLAG_WRITE
  );

  stop.addUIType(ESP_RMAKER_UI_TOGGLE);
  curtain->addParam(stop);

  curtain->addCb(write_callback);

  my_node.addDevice(*curtain);

  RMaker.enableOTA(OTA_USING_TOPICS);
  RMaker.enableTZService();
  RMaker.enableSchedule();
  RMaker.enableScenes();

  initAppInsights();

  RMaker.enableSystemService(
    SYSTEM_SERV_FLAGS_ALL,
    2,
    2,
    2
  );

#if CONFIG_IDF_TARGET_ESP32S2

  WiFiProv.initProvision(
    NETWORK_PROV_SCHEME_SOFTAP,
    NETWORK_PROV_SCHEME_HANDLER_NONE
  );

#else

  WiFiProv.initProvision(
    NETWORK_PROV_SCHEME_BLE,
    NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM
  );

#endif

  RMaker.start();

  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32S2

  WiFiProv.beginProvision(
    NETWORK_PROV_SCHEME_SOFTAP,
    NETWORK_PROV_SCHEME_HANDLER_NONE,
    NETWORK_PROV_SECURITY_1,
    pop,
    service_name
  );

#else

  WiFiProv.beginProvision(
    NETWORK_PROV_SCHEME_BLE,
    NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
    NETWORK_PROV_SECURITY_1,
    pop,
    service_name
  );

#endif

  Serial.println("Curtain controller started.");
}

void loop() {
  delay(100);
}

