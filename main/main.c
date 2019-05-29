#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "mongoose.h"

#define WIFI_SSID "xxxxx"
#define WIFI_PASS "xxxxx"

#define MG_LISTEN_ADDR "1883"

static esp_err_t event_handler(void *ctx, system_event_t *event) {
  (void) ctx;
  (void) event;
  return ESP_OK;
}

static void mg_ev_handler(struct mg_connection *nc, int ev, void *p) {

  mg_mqtt_broker(nc, ev, p);

  switch (ev) {
    case MG_EV_POLL:
      ; //nothing to do
      break;
    case MG_EV_MQTT_CONNECT:
      printf("MG_EV_MQTT_CONNECT\n");
      break;
    case MG_EV_MQTT_DISCONNECT:
      printf("MG_EV_MQTT_DISCONNECT\n");
      break;
    case MG_EV_MQTT_PUBLISH:
      printf("MG_EV_MQTT_PUBLISH\n");
      break;
    case MG_EV_MQTT_SUBSCRIBE:
      printf("MG_EV_MQTT_SUBSCRIBE\n");
      break;
    case MG_EV_MQTT_UNSUBSCRIBE:
      printf("MG_EV_MQTT_UNSUBSCRIBE\n");
      break;
      case MG_EV_MQTT_PINGREQ:
        printf("MG_EV_MQTT_PINGREQ\n");
        break;
    default:
      ; // printf("MG_EV: %d\n", ev);
      break;
  }
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();

  tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

  /* IPアドレスの設定 */
  tcpip_adapter_ip_info_t ipInfo;
  IP4_ADDR(&ipInfo.ip, 192,168,99,99);
  IP4_ADDR(&ipInfo.gw, 192,168,99,1);
  IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
  tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  /* Initializing WiFi */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t sta_config = {
      .sta = {.ssid = WIFI_SSID, .password = WIFI_PASS, .bssid_set = false}};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  /* Starting Mongoose */
  struct mg_mgr mgr;
  struct mg_connection *nc;
  struct mg_mqtt_broker brk;

  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, MG_LISTEN_ADDR, mg_ev_handler);
  if (nc == NULL) {
    printf("Error setting up listener!\n");
    return;
  }

  // mg_set_protocol_http_websocket(nc);
  mg_mqtt_broker_init(&brk, NULL);
  nc->priv_2 = &brk;
  mg_set_protocol_mqtt(nc);

  printf("MQTT broker started on %s\n", MG_LISTEN_ADDR);

  // print the local IP address
  tcpip_adapter_ip_info_t ip_info;
  ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
  printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
  printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
  printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));

  /* Processing events */
  while (1) {
    mg_mgr_poll(&mgr, 1000);
  }
}
