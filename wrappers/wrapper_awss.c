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

#include "infra_defs.h"
#include "iot_import_awss.h"

static awss_recv_80211_frame_cb_t s_sniffer_cb;

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
    uint8_t total_num = 1, count;
    uint16_t seq_buf;
    len = pkt->rx_ctrl.sig_mode ? pkt->rx_ctrl.HT_length : pkt->rx_ctrl.legacy_length;

    if (pkt->rx_ctrl.aggregation) {
        total_num = pkt->rx_ctrl.ampdu_cnt;
    }

    for (count = 0; count < total_num; count++) {
        if (total_num > 1) {
            len = *(uint16_t *)(pkt->payload + 40 + 2 * count);
        }

        if (type == WIFI_PKT_MISC && pkt->rx_ctrl.aggregation == 1) {
            len -= 4;
        }

        if (s_sniffer_cb) {
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
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL));

    s_sniffer_cb = NULL;

    ESP_LOGI(TAG, "Close monitor mode");

}

int HAL_Awss_Connect_Ap(
            _IN_ uint32_t connection_timeout_ms,
            _IN_ char ssid[HAL_MAX_SSID_LEN],
            _IN_ char passwd[HAL_MAX_PASSWD_LEN],
            _IN_OPT_ enum AWSS_AUTH_TYPE auth,
            _IN_OPT_ enum AWSS_ENC_TYPE encry,
            _IN_OPT_ uint8_t bssid[ETH_ALEN],
            _IN_OPT_ uint8_t channel)
{
    return (int)1;
}

int HAL_Awss_Get_Channelscan_Interval_Ms(void)
{
    return (int)1;
}

int HAL_Awss_Get_Timeout_Interval_Ms(void)
{
    return (int)1;
}

int HAL_Awss_Open_Ap(const char *ssid, const char *passwd, int beacon_interval, int hide)
{
    return (int)1;
}

void HAL_Awss_Switch_Channel(char primary_channel, char secondary_channel, uint8_t bssid[ETH_ALEN])
{
    return;
}