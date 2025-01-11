/*******************************************************************
 * File Name         : udp_client.c
 * Description       : Defines the udp_client_task
 *
 * Author            : Noah Plank
 *
 ******************************************************************/
#include "udp_client.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "assert.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "wifi.h"
#include "return_val.h"

#define DOOMSDAY 0

#define HOST_IP_ADDR CONFIG_SERVER_IPV4_ADDR
#define PORT CONFIG_SERVER_PORT

#define UDP_RECV_TIMEOUT_IN_SEC CONFIG_UDP_RECV_TIMEOUT_IN_SEC

#define MAXIMUM_RETRY CONFIG_MAXIMUM_RETRY
#define TIME_TO_SLEEP_AFTER_MAX_RETRYS_IN_S CONFIG_TIME_TO_SLEEP_AFTER_MAX_RETRYS_IN_S

#define EXPECTED_SERVER_ANSWER "ACK"

static const char *TAG = "UDP Client";

/// @brief sends udp_data to UDP Server, get usually called by the get_data_task when data is ready or by the deep_sleep_task when the device was woken up by a timer
/// @param param pointer to a udp_client_task_param_t struct
void udp_client_task(void *param)
{
    if (false == wifi_connected)
    {
        if (CONNECTION_FAILED == wifi_init_sta())
        {
            ESP_LOGE(TAG, "Failed to connect to WiFi");
            assert(NULL != *(((udp_client_task_param_t *)param)->sleep_TaskHandle));
            xTaskNotify(*(((udp_client_task_param_t *)param)->sleep_TaskHandle), TIME_TO_SLEEP_AFTER_MAX_RETRYS_IN_S, eSetValueWithOverwrite);
            vTaskDelay(10000 / portTICK_PERIOD_MS); // wait for deep_sleep_task to go to sleep
        }
    }

    char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    uint8_t try_counter = 0;

    while (!DOOMSDAY)
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET; // indicates IPV4 communication
        ip_protocol = IPPROTO_IP;

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = UDP_RECV_TIMEOUT_IN_SEC;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        ESP_LOGI(TAG, "Socket created, destination:  %s  %d", HOST_IP_ADDR, PORT);

        while (!DOOMSDAY)
        {
            if (0 == try_counter)
            {
                xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
            }
            try_counter += 1;

            if (try_counter <= MAXIMUM_RETRY)
            {
                int err = sendto(sock, ((udp_client_task_param_t *)param)->udp_data, sizeof(udp_data_t), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
                ESP_LOGI(TAG, "Message sent, try %d", try_counter);

                struct sockaddr_storage source_addr; 
                socklen_t socklen = sizeof(source_addr);
                int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

                // Error occurred during receiving
                if (len < 0)
                {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                }
                // Data received
                else
                {
                    rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                    ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                    ESP_LOGI(TAG, "%s", rx_buffer);
                    if (strncmp(rx_buffer, EXPECTED_SERVER_ANSWER, 4) == 0)
                    {
                        try_counter = 0;
                        ESP_LOGI(TAG, "Received answer from server, going to sleep...");
                        xTaskNotify(*(((udp_client_task_param_t *)param)->sleep_TaskHandle), 0, eSetValueWithOverwrite);
                        vTaskDelay(10000 / portTICK_PERIOD_MS); // wait for deep_sleep_task to go to sleep
                    }
                }
            }
            else
            {
                try_counter = 0;
                ESP_LOGW(TAG, "Something went wrong. Trying again later, going to sleep...");
                // Telling deep_sleep_task to sleep for TIME_TO_WAIT_BEFORE_RETRYING_IN_S and then try again
                xTaskNotify(*(((udp_client_task_param_t *)param)->sleep_TaskHandle), TIME_TO_SLEEP_AFTER_MAX_RETRYS_IN_S, eSetValueWithOverwrite);
                vTaskDelay(10000 / portTICK_PERIOD_MS); // wait for deep_sleep_task to go to sleep
            }
        }
        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}