// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* System headers */
#include <platform.h>
#include <xs1.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* Library headers */
#include "rtos/drivers/gpio/api/rtos_gpio.h"
#include "rtos/drivers/spi/api/rtos_spi_master.h"
#include "sl_wfx_iot_wifi.h"
#include "wifi.h"

/* App headers */
#include "app_conf.h"

#define kernel_printf( FMT, ... )    rtos_printf("KERNEL", FMT, ##__VA_ARGS__)

/* Devices */
static rtos_spi_master_t spi_master_ctx_s;
static rtos_spi_master_device_t wifi_device_ctx_s;
static rtos_gpio_t gpio_ctx_s;

static rtos_spi_master_t *spi_master_ctx = &spi_master_ctx_s;
static rtos_spi_master_device_t *wifi_device_ctx = &wifi_device_ctx_s;
static rtos_gpio_t *gpio_ctx = &gpio_ctx_s;

/* Network Settings */
const uint8_t ucIPAddress[ipIP_ADDRESS_LENGTH_BYTES] = {
                                ( uint8_t ) IPconfig_IP_ADDR_OCTET_0,
                                ( uint8_t ) IPconfig_IP_ADDR_OCTET_1,
                                ( uint8_t ) IPconfig_IP_ADDR_OCTET_2,
                                ( uint8_t ) IPconfig_IP_ADDR_OCTET_3 };

const uint8_t ucNetMask[ipIP_ADDRESS_LENGTH_BYTES] = {
                                ( uint8_t ) IPconfig_NET_MASK_OCTET_0,
                                ( uint8_t ) IPconfig_NET_MASK_OCTET_1,
                                ( uint8_t ) IPconfig_NET_MASK_OCTET_2,
                                ( uint8_t ) IPconfig_NET_MASK_OCTET_3 };

const uint8_t ucGatewayAddress[ipIP_ADDRESS_LENGTH_BYTES] = {
                                ( uint8_t ) IPconfig_GATEWAY_OCTET_0,
                                ( uint8_t ) IPconfig_GATEWAY_OCTET_1,
                                ( uint8_t ) IPconfig_GATEWAY_OCTET_2,
                                ( uint8_t ) IPconfig_GATEWAY_OCTET_3 };

const uint8_t ucDNSServerAddress[ipIP_ADDRESS_LENGTH_BYTES] = {
                                ( uint8_t ) IPconfig_DNS_SERVER_OCTET_0,
                                ( uint8_t ) IPconfig_DNS_SERVER_OCTET_1,
                                ( uint8_t ) IPconfig_DNS_SERVER_OCTET_2,
                                ( uint8_t ) IPconfig_DNS_SERVER_OCTET_3 };

const uint8_t ucMACAddress[ipMAC_ADDRESS_LENGTH_BYTES] = {
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_0,
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_1,
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_2,
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_3,
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_4,
                                ( uint8_t ) IPconfig_MAC_ADDR_OCTET_5 };

void vApplicationMallocFailedHook( void )
{
    kernel_printf("Malloc Failed!");
    configASSERT(0);
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char* pcTaskName )
{
    kernel_printf("Stack Overflow! %s", pcTaskName);
    configASSERT(0);
}

static void scan(void)
{
    WIFIScanResult_t scan_results[10];

    if (WIFI_Scan(scan_results, 10) == eWiFiSuccess) {
        WIFIScanResult_t *scan_result = scan_results;

        for (int i = 0; i < 10; i++) {
            uint8_t no_bssid[wificonfigMAX_BSSID_LEN] = {0};

            if (memcmp(scan_result->ucBSSID, no_bssid, wificonfigMAX_BSSID_LEN) == 0) {
                break;
            }

#define HWADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define HWADDR_ARG(hwaddr) (hwaddr)[0], (hwaddr)[1], (hwaddr)[2], (hwaddr)[3], (hwaddr)[4], (hwaddr)[5]
            rtos_printf("Scan result %d:\n", i);
            rtos_printf("\tBSSID: " HWADDR_FMT "\n", i, HWADDR_ARG(scan_result->ucBSSID));
            rtos_printf("\tSSID: %s\n", scan_result->cSSID);
            rtos_printf("\tSecurity: %s dBm\n", scan_result->xSecurity == eWiFiSecurityOpen ? "open" :
            		scan_result->xSecurity == eWiFiSecurityWEP ? "WEP" : "WPA");
            rtos_printf("\tChannel: %d\n", (int) scan_result->cChannel);
            rtos_printf("\tStrength: %d dBm\n\n", (int) scan_result->cRSSI);

            scan_result++;
        }
    }
}

int wifi_conn_mgr_event_cb(int event, char *ssid, char *password)
{
	switch (event) {
	case WIFI_CONN_MGR_EVENT_STARTUP:
        rtos_printf("Directing WiFi manager to go into station mode\n");
        return WIFI_CONN_MGR_MODE_STATION;

    case WIFI_CONN_MGR_EVENT_CONNECT_FAILED:

    	scan();

        rtos_printf("Scan complete.  Program exitting now...\n");
        _Exit(0);

        rtos_printf("Directing WiFi manager to start a soft AP\n");
        rtos_printf("\tSSID is %s\n", appconfSOFT_AP_SSID);
        if (strlen(appconfSOFT_AP_PASSWORD) > 0) {
        	rtos_printf("\tPassword is %s\n", appconfSOFT_AP_PASSWORD);
        } else {
        	rtos_printf("\tThere is no password\n");
        }
        strcpy(ssid, appconfSOFT_AP_SSID);
        strcpy(password, appconfSOFT_AP_PASSWORD);
        return WIFI_CONN_MGR_MODE_SOFT_AP;

    case WIFI_CONN_MGR_EVENT_CONNECTED:
        rtos_printf("Connected to %s\n", ssid);
        return WIFI_CONN_MGR_MODE_STATION; /* this is ignored */

    case WIFI_CONN_MGR_EVENT_DISCONNECTED:
        if (ssid[0] != '\0') {
            rtos_printf("Disconnected from %s\n", ssid);
        } else {
            rtos_printf("Disconnected from AP\n");
        }

        return WIFI_CONN_MGR_MODE_STATION;

    case WIFI_CONN_MGR_EVENT_SOFT_AP_STARTED:
        rtos_printf("Soft AP %s started\n", ssid);
        return WIFI_CONN_MGR_MODE_SOFT_AP; /* this is ignored */

    case WIFI_CONN_MGR_EVENT_SOFT_AP_STOPPED:
        rtos_printf("Soft AP %s stopped. Going into station mode\n", ssid);
        return WIFI_CONN_MGR_MODE_STATION;

    default:
        return WIFI_CONN_MGR_MODE_STATION;
	}
}

static void wifi_setup_task(void *args)
{
    const rtos_gpio_port_id_t wifi_wup_rst_port = rtos_gpio_port(WIFI_WUP_RST_N);
    const rtos_gpio_port_id_t wifi_irq_port = rtos_gpio_port(WIFI_WIRQ);

    vTaskDelay(pdMS_TO_TICKS(100));

    sl_wfx_host_set_hif(wifi_device_ctx,
                        gpio_ctx,
                        wifi_irq_port, 0,
                        wifi_wup_rst_port, 0,
                        wifi_wup_rst_port, 1);

    rtos_printf("Start FreeRTOS_IP\n");
    FreeRTOS_IPInit(ucIPAddress,
                    ucNetMask,
                    ucGatewayAddress,
                    ucDNSServerAddress,
                    ucMACAddress);

    rtos_printf("Start WiFi connection manager\n");
    wifi_conn_mgr_start(
                appconfWIFI_CONN_MNGR_TASK_PRIORITY,
                appconfWIFI_DHCP_SERVER_TASK_PRIORITY);

    vTaskDelete(NULL);
}

static void wifi_start(void)
{
    xTaskCreate((TaskFunction_t) wifi_setup_task,
                "wifi_setup_task",
                RTOS_THREAD_STACK_SIZE(wifi_setup_task),
                NULL,
                appconfWIFI_SETUP_TASK_PRIORITY,
                NULL);
}

static void platform_init(void)
{
    rtos_gpio_init(
            gpio_ctx);

    rtos_spi_master_init(
            spi_master_ctx,
            XS1_CLKBLK_1,
            WIFI_CS_N,
            WIFI_CLK,
            WIFI_MOSI,
            WIFI_MISO);

    rtos_spi_master_device_init(
            wifi_device_ctx,
            spi_master_ctx,
            1, /* WiFi CS pin is on bit 1 of the CS port */
            SPI_MODE_0,
            spi_master_source_clock_ref,
            0, /* 50 MHz */
            spi_master_sample_delay_2, /* what should this be? 2? 3? 4? */
            0, /* should this be > 0 if the above is 3-4 ? */
            1,
            0,
            0);
}

static void platform_start(void)
{
    rtos_gpio_start(gpio_ctx);
    rtos_spi_master_start(spi_master_ctx, appconfSPI_MASTER_TASK_PRIORITY);
}

static void vApplicationDaemonTaskStartup(void *arg)
{
    platform_start();
    wifi_start();
    while(1){;}
    vTaskDelete(NULL);
}

void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    (void) c1;
    (void) c2;
    (void) c3;

    platform_init();

    xTaskCreate((TaskFunction_t) vApplicationDaemonTaskStartup,
                "vApplicationDaemonTaskStartup",
                RTOS_THREAD_STACK_SIZE(vApplicationDaemonTaskStartup),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    kernel_printf("Start scheduler");
    vTaskStartScheduler();

    return;
}
