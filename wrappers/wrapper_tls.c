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

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "infra_types.h"
#include "wrappers_defs.h"

#include "esp_tls.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "wrapper_tls";

/**
 * @brief Set malloc/free function.
 *
 * @param [in] hooks: @n Specify malloc/free function you want to use
 *
 * @retval DTLS_SUCCESS : Success.
   @retval        other : Fail.
 * @see None.
 * @note None.
 */
DLL_HAL_API int HAL_DTLSHooks_set(dtls_hooks_t *hooks)
{
    return (int)1;
}

/**
 * @brief Establish a DSSL connection.
 *
 * @param [in] p_options: @n Specify paramter of DTLS
   @verbatim
           p_host : @n Specify the hostname(IP) of the DSSL server
             port : @n Specify the DSSL port of DSSL server
    p_ca_cert_pem : @n Specify the root certificate which is PEM format.
   @endverbatim
 * @return DSSL handle.
 * @see None.
 * @note None.
 */
DLL_HAL_API DTLSContext *HAL_DTLSSession_create(coap_dtls_options_t  *p_options)
{
    return (DTLSContext *)1;
}

/**
 * @brief Destroy the specific DSSL connection.
 *
 * @param[in] context: @n Handle of the specific connection.
 *
 * @return The result of free dtls session
 * @retval DTLS_SUCCESS : Read success.
 * @retval DTLS_INVALID_PARAM : Invalid parameter.
 * @retval DTLS_INVALID_CA_CERTIFICATE : Invalid CA Certificate.
 * @retval DTLS_HANDSHAKE_IN_PROGRESS : Handshake in progress.
 * @retval DTLS_HANDSHAKE_FAILED : Handshake failed.
 * @retval DTLS_FATAL_ALERT_MESSAGE : Recv peer fatal alert message.
 * @retval DTLS_PEER_CLOSE_NOTIFY : The DTLS session was closed by peer.
 * @retval DTLS_SESSION_CREATE_FAILED : Create session fail.
 * @retval DTLS_READ_DATA_FAILED : Read data fail.
 */
DLL_HAL_API unsigned int HAL_DTLSSession_free(DTLSContext *context)
{
    return (unsigned)1;
}

/**
 * @brief Read data from the specific DSSL connection with timeout parameter.
 *        The API will return immediately if len be received from the specific DSSL connection.
 *
 * @param [in] context @n A descriptor identifying a DSSL connection.
 * @param [in] p_data @n A pointer to a buffer to receive incoming data.
 * @param [in] p_datalen @n The length, in bytes, of the data pointed to by the 'p_data' parameter.
 * @param [in] timeout_ms @n Specify the timeout value in millisecond. In other words, the API block 'timeout_ms' millisecond maximumly.
 * @return The result of read data from DSSL connection
 * @retval DTLS_SUCCESS : Read success.
 * @retval DTLS_FATAL_ALERT_MESSAGE : Recv peer fatal alert message.
 * @retval DTLS_PEER_CLOSE_NOTIFY : The DTLS session was closed by peer.
 * @retval DTLS_READ_DATA_FAILED : Read data fail.
 * @see None.
 */
DLL_HAL_API unsigned int HAL_DTLSSession_read(DTLSContext *context,
        unsigned char *p_data,
        unsigned int *p_datalen,
        unsigned int timeout_ms)
{
    return (unsigned)1;
}

/**
 * @brief Write data into the specific DSSL connection.
 *
 * @param [in] context @n A descriptor identifying a connection.
 * @param [in] p_data @n A pointer to a buffer containing the data to be transmitted.
 * @param [in] p_datalen @n The length, in bytes, of the data pointed to by the 'p_data' parameter.
 * @retval DTLS_SUCCESS : Success.
   @retval        other : Fail.
 * @see None.
 */
DLL_HAL_API unsigned int HAL_DTLSSession_write(DTLSContext *context,
        const unsigned char *p_data,
        unsigned int *p_datalen)
{
    return (unsigned)1;
}

extern void *HAL_Malloc(uint32_t size);
extern void HAL_Free(void *ptr);

static ssl_hooks_t g_ssl_hooks = { HAL_Malloc, HAL_Free};

int32_t HAL_SSL_Destroy(uintptr_t handle)
{
    struct esp_tls *tls = (struct esp_tls *)handle;

    if (!tls) {
        return ESP_FAIL;
    }

    esp_tls_conn_delete(tls);

    return ESP_OK;
}


#if CONFIG_SSL_USING_WOLFSSL
#include "lwip/err.h"
#include "lwip/apps/sntp.h"
#include <sys/socket.h>
#include <netdb.h>

static void initialize_sntp(void)
{
    static bool get_time_flag = false;
    if(get_time_flag){
        return;
    }
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp1.aliyun.com");
    sntp_setservername(1, "ntp2.aliyun.com");
    sntp_setservername(2, "ntp3.aliyun.com");
    sntp_init();
    get_time_flag = true;
}

static void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    while(timeinfo.tm_year < (2019 - 1900)) {
        ESP_LOGI(TAG, "Waiting for system time to be set... ");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

static void host_run_nds(const char *host)
{
    while(true){
        struct hostent *hp = gethostbyname(host);
        if (hp == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }else{
            break;
        }
    }
}
#endif

uintptr_t HAL_SSL_Establish(const char *host, uint16_t port, const char *ca_crt, uint32_t ca_crt_len)
{
#if CONFIG_SSL_USING_WOLFSSL
    host_run_nds(host);
    obtain_time();
#endif
    esp_tls_cfg_t cfg = {
        .cacert_pem_buf  = (const unsigned char *)ca_crt,
        .cacert_pem_bytes = ca_crt_len,
        .timeout_ms = CONFIG_TLS_ESTABLISH_TIMEOUT_MS,
    };

#ifdef CONFIG_IDF_TARGET_ESP8266
    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);
#endif
    struct esp_tls *tls = esp_tls_conn_new(host, strlen(host), port, &cfg);

#ifdef CONFIG_IDF_TARGET_ESP8266
    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
#endif

    return (uintptr_t)tls;
}

int HAL_SSLHooks_set(ssl_hooks_t *hooks)
{
    if (hooks == NULL || hooks->malloc == NULL || hooks->free == NULL) {
        return ESP_FAIL;
    }

    g_ssl_hooks.malloc = hooks->malloc;
    g_ssl_hooks.free = hooks->free;

    return ESP_OK;
}

static void HAL_utils_ms_to_timeval(int timeout_ms, struct timeval *tv)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms - (tv->tv_sec * 1000)) * 1000;
}

static int ssl_poll_read(esp_tls_t *tls, int timeout_ms)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(tls->sockfd, &readset);
    struct timeval timeout;
    HAL_utils_ms_to_timeval(timeout_ms, &timeout);

    return select(tls->sockfd + 1, &readset, NULL, NULL, &timeout);
}

int HAL_SSL_Read(uintptr_t handle, char *buf, int len, int timeout_ms)
{
    int poll, ret;
    struct esp_tls *tls = (struct esp_tls *)handle;

    if (esp_tls_get_bytes_avail(tls) <= 0) {
        if ((poll = ssl_poll_read(tls, timeout_ms)) <= 0) {
            return poll;
        }
    }

    ret = esp_tls_conn_read(tls, (void *)buf, len);

    if (ret < 0) {
        ESP_LOGE(TAG, "esp_tls_conn_read error, errno:%s", strerror(errno));
    }

    return ret;
}

static int ssl_poll_write(esp_tls_t *tls, int timeout_ms)
{
    fd_set writeset;
    FD_ZERO(&writeset);
    FD_SET(tls->sockfd, &writeset);
    struct timeval timeout;
    HAL_utils_ms_to_timeval(timeout_ms, &timeout);
    return select(tls->sockfd + 1, NULL, &writeset, NULL, &timeout);
}

int HAL_SSL_Write(uintptr_t handle, const char *buf, int len, int timeout_ms)
{
    int poll, ret;
    struct esp_tls *tls = (struct esp_tls *)handle;

    if ((poll = ssl_poll_write(tls, timeout_ms)) <= 0) {
        ESP_LOGW(TAG, "Poll timeout or error, errno=%s, fd=%d, timeout_ms=%d", strerror(errno), tls->sockfd, timeout_ms);
        return poll;
    }

    ret = esp_tls_conn_write(tls, (const void *) buf, len);

    if (ret < 0) {
        ESP_LOGE(TAG, "esp_tls_conn_write error, errno=%s", strerror(errno));
    }

    return ret;
}
