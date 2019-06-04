/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <netinet/in.h>
#include <string.h>

#include "infra_defs.h"
#include "iot_import_awss.h"

#include "esp_log.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static awss_recv_80211_frame_cb_t s_sniffer_cb;

static const char *TAG = "wrapper_awss";

static void HAL_Awss_Monitor_callback(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    int with_fcs = 0;
    int link_type = AWSS_LINK_TYPE_NONE;
    uint16_t len = 0;
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)recv_buf;
    int8_t rssi;

    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) {
        return;
    }

    rssi = pkt->rx_ctrl.rssi;

#ifdef CONFIG_IDF_TARGET_ESP8266
    uint8_t total_num = 1;
    uint16_t seq_buf;
    len = pkt->rx_ctrl.sig_mode ? pkt->rx_ctrl.HT_length : pkt->rx_ctrl.legacy_length;

    if (pkt->rx_ctrl.aggregation) {
        total_num = pkt->rx_ctrl.ampdu_cnt;
    }

    for (uint8_t count = 0; count < total_num; count++) {
        if (total_num > 1) {
            len = *(uint16_t *)(pkt->payload + 40 + 2 * count);
        }

        if (type == WIFI_PKT_MISC && pkt->rx_ctrl.aggregation == 1) {
            len -= 4;
        }

        if (s_sniffer_cb) {
            // printf("%s %d\n", __func__, __LINE__);
            s_sniffer_cb((char *)pkt->payload, len - 4, link_type, with_fcs, rssi);
        }

        if (total_num > 1) {
            seq_buf = *(uint16_t *)(pkt->payload + 22) >> 4;
            seq_buf++;
            *(uint16_t *)(pkt->payload + 22) = (seq_buf << 4) | (*(uint16_t *)(pkt->payload + 22) & 0xF);
        }
    }

#else

    if (s_sniffer_cb) {
        len = pkt->rx_ctrl.sig_len;
        s_sniffer_cb((char *)pkt->payload, len - 4, link_type, with_fcs, rssi);
    }

#endif
}

/**
 * @brief   设置Wi-Fi网卡工作在监听(Monitor)模式, 并在收到802.11帧的时候调用被传入的回调函数
 *
 * @param[in] cb @n A function pointer, called back when wifi receive a frame.
 */
void HAL_Awss_Open_Monitor(_IN_ awss_recv_80211_frame_cb_t cb)
{
    if (!cb) {
        return;
    }

    s_sniffer_cb = cb;

    ESP_ERROR_CHECK(esp_wifi_set_channel(6, 0));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(HAL_Awss_Monitor_callback));

#ifdef CONFIG_IDF_TARGET_ESP8266
    extern void esp_wifi_set_promiscuous_data_len(uint32_t);
    esp_wifi_set_promiscuous_data_len(512);
#endif

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    ESP_LOGI(TAG, "Open monitor mode");
}

void HAL_Awss_Close_Monitor(void)
{
    if (!s_sniffer_cb) {
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL));

    s_sniffer_cb = NULL;

    ESP_LOGI(TAG, "Close monitor mode");

}

#ifndef CONFIG_TARGET_PLATFORM_MALIYUN

int HAL_Awss_Connect_Ap(
    _IN_ uint32_t connection_timeout_ms,
    _IN_ char ssid[HAL_MAX_SSID_LEN],
    _IN_ char passwd[HAL_MAX_PASSWD_LEN],
    _IN_OPT_ enum AWSS_AUTH_TYPE auth,
    _IN_OPT_ enum AWSS_ENC_TYPE encry,
    _IN_OPT_ uint8_t bssid[ETH_ALEN],
    _IN_OPT_ uint8_t channel)
{
    uint32_t connect_ms = 0;
    wifi_config_t wifi_config = { 0 };

    if (ssid) {
        memcpy(wifi_config.sta.ssid, ssid, HAL_MAX_SSID_LEN - 1);
    }

    if (passwd) {
        memcpy(wifi_config.sta.password, passwd, HAL_MAX_PASSWD_LEN - 1);
    }

    if (bssid != NULL) {
        memcpy(wifi_config.sta.bssid, bssid, ETH_ALEN);
        wifi_config.sta.bssid_set = true;
    }

    wifi_config.sta.channel = channel;

    ESP_LOGI(TAG, "ssid: %s, password: %s, channel: %d",
             wifi_config.sta.ssid, wifi_config.sta.password, channel);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    tcpip_adapter_ip_info_t local_ip;

    while (connect_ms < connection_timeout_ms) {
        esp_err_t ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);

        if ((ESP_OK == ret) && (local_ip.ip.addr != INADDR_ANY)) {
            ESP_LOGI(TAG, "AP connected");
            return SUCCESS_RETURN;
        } else {
            ESP_LOGI(TAG, "Connecting AP");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            connect_ms += 500;
        }
    }

    return FAIL_RETURN;
}
#endif


/**
 * @brief   获取配网服务(`AWSS`)的超时时间长度, 单位是毫秒
 *
 * @return  超时时长, 单位是毫秒
 * @note    推荐时长是60,0000毫秒
 */

int HAL_Awss_Get_Timeout_Interval_Ms(void)
{
    return CONFIG_AWSS_TIMEOUT_INTERVAL_MS;
}


/**
 * @brief   获取在每个信道(`channel`)上扫描的时间长度, 单位是毫秒
 *
 * @return  时间长度, 单位是毫秒
 * @note    推荐时长是200毫秒到400毫秒
 */
int HAL_Awss_Get_Channelscan_Interval_Ms(void)
{
    return CONFIG_AWSS_CHANNELSCAN_INTERVAL_MS;
}


int HAL_Awss_Open_Ap(const char *ssid, const char *passwd, int beacon_interval, int hide)
{
    return (int)1;
}

int HAL_Awss_Close_Ap(void)
{
    return (int)1;
}

/**
 * @brief   设置Wi-Fi网卡切换到指定的信道(channel)上
 *
 * @param[in] primary_channel @n Primary channel.
 * @param[in] secondary_channel @n Auxiliary channel if 40Mhz channel is supported, currently
 *              this param is always 0.
 * @param[in] bssid @n A pointer to wifi BSSID on which awss lock the channel, most HAL
 *              may ignore it.
 */
void HAL_Awss_Switch_Channel(char primary_channel, char secondary_channel, uint8_t bssid[ETH_ALEN])
{
    esp_wifi_set_channel(primary_channel, secondary_channel);
}
