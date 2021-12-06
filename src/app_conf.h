// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef APP_CONF_H_
#define APP_CONF_H_

/* Network Setup Configuration */
#define appconfSOFT_AP_SSID         "xcore.ai"
#define appconfSOFT_AP_PASSWORD     "xmos"

/* Task Priorities */
#define appconfSTARTUP_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define appconfSPI_MASTER_TASK_PRIORITY         (configMAX_PRIORITIES-1)
#define appconfWIFI_SETUP_TASK_PRIORITY         (configMAX_PRIORITIES/2-1)
#define appconfWIFI_CONN_MNGR_TASK_PRIORITY     (configMAX_PRIORITIES-3)
#define appconfWIFI_DHCP_SERVER_TASK_PRIORITY   (configMAX_PRIORITIES-3)

#endif /* APP_CONF_H_ */
