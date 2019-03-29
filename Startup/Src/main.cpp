
#include <string>
#include <cstdio>

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "thread.hpp"
#include "ticks.hpp"
#include "stm32l4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"

extern uint8_t UserRxBufferFS[];
extern uint8_t UserTxBufferFS[];


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
//static char obuf[BUFSIZ] = {}; // device/library output buffer
//static char ibuf[CDC_DATA_FS_OUT_PACKET_SIZE] = {}; // device/library input buffer, match to USB host out packet size


/* Private function prototypes -----------------------------------------------*/

using namespace cpp_freertos;
using namespace std;


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	// Redirect IO library to use buffers allocated by USB driver
	setvbuf(stdin,  (char *)UserRxBufferFS, _IOLBF, APP_RX_DATA_SIZE);
	setvbuf(stdout, (char *)UserTxBufferFS, _IOFBF, APP_TX_DATA_SIZE);


	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USB_DEVICE_Init();

	ResponseInterface rspThread;
	CommandInterface cmdThread = CommandInterface(rspThread);

	Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}



