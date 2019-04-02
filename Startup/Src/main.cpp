
#include <string>
#include <cstdio>

#include "main.h"
#include "stm32l4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include "CommandInterface.hpp"
#include "ResponseInterface.hpp"

using namespace cpp_freertos;
using namespace std;



/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t freeRTOSMemoryScheme = configUSE_HEAP_SCHEME; /* used by NXP thread aware debugger */

static ResponseInterface rspThread;
static CommandInterface cmdThread = CommandInterface(rspThread);

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	// Redirect IO library to use buffers allocated by USB driver
	setvbuf(stdin,  (char *)UserRxBufferFS, _IOLBF, APP_RX_DATA_SIZE);
	setvbuf(stdout, (char *)UserTxBufferFS, _IOFBF, APP_TX_DATA_SIZE);

	/* Force compiler to "keep" variable. */
	freeRTOSMemoryScheme = freeRTOSMemoryScheme;

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USB_DEVICE_Init();

	/* Note: When the RTOS scheduler is started, the main stack pointer (MSP) is reset,
	 * effectively wiping out all local main() variables and objects. Do not declare any
	 * C/C++ threads in main(). If objects need to be declared in main, then change the code
	 * within prvPortStartFirstTask() to retain the MSP value.
	 */
	Thread::StartScheduler();

	/* Infinite loop */
	while (1)
	{
	}
}



