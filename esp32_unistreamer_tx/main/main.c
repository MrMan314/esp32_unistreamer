// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define BOARD_ESP32CAM_AITHINKER
// #define BOARD_ESP32S3_WROOM
// #define BOARD_ESP32S3_XIAO
#define BOARD_ESP32S3_GOOUUU
// #define BOARD_ESP32S3_XIAO

/**
 * 2. Kconfig setup
 *
 * If you have a Kconfig file, copy the content from
 *  https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
 * In case you haven't, copy and paste this Kconfig file inside the src directory.
 * This Kconfig file has definitions that allows more control over the camera and
 * how it will be initialized.
 */

/**
 * 3. Enable PSRAM on sdkconfig:
 *
 * CONFIG_ESP32_SPIRAM_SUPPORT=y
 *
 * More info on
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
 */

// ================================ CODE ======================================

#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include <esp_private/wifi.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"

#if defined(CONFIG_CAMERA_AF_SUPPORT) && CONFIG_CAMERA_AF_SUPPORT
#include "esp_camera_af.h"
#endif

#include "camera_pinout.h"
#include "packet.h"
#include "gf256.h"

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define FEC_RATE 3

extern int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
	// looks good...
	return 0;
}

static const char *TAG = "esp32_unistreamer:tx";

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
	.pin_pwdn = CAM_PIN_PWDN,
	.pin_reset = CAM_PIN_RESET,
	.pin_xclk = CAM_PIN_XCLK,
	.pin_sccb_sda = CAM_PIN_SIOD,
	.pin_sccb_scl = CAM_PIN_SIOC,

	.pin_d7 = CAM_PIN_D7,
	.pin_d6 = CAM_PIN_D6,
	.pin_d5 = CAM_PIN_D5,
	.pin_d4 = CAM_PIN_D4,
	.pin_d3 = CAM_PIN_D3,
	.pin_d2 = CAM_PIN_D2,
	.pin_d1 = CAM_PIN_D1,
	.pin_d0 = CAM_PIN_D0,
	.pin_vsync = CAM_PIN_VSYNC,
	.pin_href = CAM_PIN_HREF,
	.pin_pclk = CAM_PIN_PCLK,

	//XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
	.xclk_freq_hz = 20000000,
	.ledc_timer = LEDC_TIMER_0,
	.ledc_channel = LEDC_CHANNEL_0,

	.pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
	.frame_size = FRAMESIZE_HD,	//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

	.jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
	.fb_count = 2,	   //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
	.fb_location = CAMERA_FB_IN_PSRAM,
	.grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
	//initialize the camera
	esp_err_t err = esp_camera_init(&camera_config);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Camera Init Failed");
		return err;
	}

	return ESP_OK;
}

#if defined(CONFIG_CAMERA_AF_SUPPORT) && CONFIG_CAMERA_AF_SUPPORT
static void maybe_init_autofocus(void)
{
	sensor_t *s = esp_camera_sensor_get();
	if (!s) {
		ESP_LOGW(TAG, "AF: no sensor handle");
		return;
	}

	if (!esp_camera_af_is_supported(s)) {
		ESP_LOGI(TAG, "AF: not supported by this sensor");
		return;
	}

	esp_camera_af_config_t af_cfg = {
		.mode = ESP_CAMERA_AF_MODE_AUTO,
		.timeout_ms = CONFIG_CAMERA_AF_DEFAULT_TIMEOUT_MS,
	};

	esp_err_t ret = esp_camera_af_init(s, &af_cfg);
	if (ret != ESP_OK) {
		ESP_LOGW(TAG, "AF init failed: %s", esp_err_to_name(ret));
		return;
	}

	ESP_LOGI(TAG, "AF initialized (AUTO mode)");
}
#endif
#endif

packet_t packet = {0};
uint8_t mac[] = {0x67, 0x41, 0x67, 0x41, 0x67, 0x41};
uint8_t my_mac[6];

static SemaphoreHandle_t tx_done_sem;

void wifi_tx_done_cb(uint8_t ifidx, uint8_t *data, uint16_t *data_len, bool txStatus) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (tx_done_sem != NULL) {
		xSemaphoreGiveFromISR(tx_done_sem, &xHigherPriorityTaskWoken);
	}
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	// Init dummy AP to specify a channel and get WiFi hardware into
	// a mode where we can send the actual fake beacon frames.
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t ap_config = {
		.ap = {
			.ssid = "esp32-beaconspam",
			.ssid_len = 0,
			.password = "dummypassword",
			.channel = 8,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = 1,
			.max_connection = 4,
			.beacon_interval = 60000
		}
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

	ESP_ERROR_CHECK(esp_wifi_config_80211_tx_rate(WIFI_IF_AP, WIFI_PHY_RATE_MCS3_LGI));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW20));
/** this code fails for some reason...
	wifi_tx_rate_config_t tx_rate_config = {
		.phymode = WIFI_PHY_MODE_HT40,
		.rate = WIFI_PHY_RATE_MCS7_LGI,
	};

	ESP_ERROR_CHECK(esp_wifi_config_80211_tx(WIFI_IF_AP, &tx_rate_config));
*/
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

	tx_done_sem = xSemaphoreCreateBinary();
	ESP_ERROR_CHECK(esp_wifi_set_tx_done_cb(wifi_tx_done_cb));

#if ESP_CAMERA_SUPPORTED
	if(ESP_OK != init_camera()) {
		return;
	}

#if defined(CONFIG_CAMERA_AF_SUPPORT) && CONFIG_CAMERA_AF_SUPPORT
	// Initialize autofocus if configured and supported by the sensor.
	// In menuconfig: Component config → Camera configuration → Enable autofocus support
	maybe_init_autofocus();
#endif
	sensor_t *s = esp_camera_sensor_get();
	s->set_vflip(s, 1);

	ESP_ERROR_CHECK(esp_read_mac(my_mac, ESP_MAC_WIFI_SOFTAP));

	packet.packet_header.header.frame_control.type=0b10;

	packet.packet_header.snap_header.dsap = 0xAA;
	packet.packet_header.snap_header.ssap = 0xAA;
	packet.packet_header.snap_header.control = 0x03;
	packet.packet_header.snap_header.ethertype = 0xB588;
	
	memcpy(packet.packet_header.header.dest_addr, mac, sizeof(mac));
	memcpy(packet.packet_header.header.src_addr, my_mac, sizeof(mac));
	memcpy(packet.packet_header.header.bssid, my_mac, sizeof(mac));
	memcpy(packet.packet_header.snap_header.oui, my_mac, 3);

	int seq = 0;
	int prev_time, cur_time;
	float fps;

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(100);
	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
		prev_time = esp_timer_get_time();
		//ESP_LOGI(TAG, "Taking picture...");
		camera_fb_t *pic = esp_camera_fb_get();

		ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

		packet.packet_header.header.seq_ctrl.sequence = seq;
		packet.frame_len = pic->len;

		int n_packets = CEIL_DIV(pic->len, DATA_SIZE);
		int n_parity = CEIL_DIV(n_packets, FEC_RATE);

		uint8_t *parity_data = malloc(n_parity * DATA_SIZE);

		// compute parity
		for (int i = 0; i < DATA_SIZE; i++) {
			uint8_t poly[n_packets];
			gf_polyreg(pic->buf + i, n_packets, poly, DATA_SIZE);
			for (int j = 0; j < n_parity; j++) {
				parity_data[DATA_SIZE * j + i] = gf256_polycalc_exp2(poly, n_packets, j + n_packets);
			}
		}

		int frag = 0;
		for (int i = 0; i < pic->len; i += DATA_SIZE, frag++) {

			packet.packet_header.header.seq_ctrl.fragment = frag;

			memcpy(packet.data, pic->buf + i, DATA_SIZE);
			ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, &packet, sizeof(packet_t), false));
			if (xSemaphoreTake(tx_done_sem, pdMS_TO_TICKS(100)) != pdTRUE) {
				ESP_LOGE(TAG, "transmission FAILED!");
			}
		}

		// transmit parity
		for (int i = 0; i < n_parity; i++, frag++) {
			packet.packet_header.header.seq_ctrl.fragment = frag;
			memcpy(packet.data, parity_data + i * DATA_SIZE, DATA_SIZE);
			ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, &packet, sizeof(packet_t), false));
			if (xSemaphoreTake(tx_done_sem, pdMS_TO_TICKS(100)) != pdTRUE) {
				ESP_LOGE(TAG, "transmission FAILED!");
			}
		}
		//ESP_LOGI(TAG, "picture transmission done");
		esp_camera_fb_return(pic);
		seq++;

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		cur_time = esp_timer_get_time();
		fps = 1000000.0/(cur_time - prev_time);
		ESP_LOGI(TAG, "fps: %.2f", fps);
	}
#else
	ESP_LOGE(TAG, "Camera support is not available for this chip");
	return;
#endif
}
