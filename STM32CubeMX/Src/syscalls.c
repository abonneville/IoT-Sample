/*
 * syscalls.c
 *
 * Not a complete implementation, but provides definition for various STL and
 * related libraries as required.
 *
 *  Created on: Feb 23, 2019
 *      Author: Andrew
 */

//#include "stm32l4xx_hal.h"
#include "usbd_cdc_if.h"


/**
  * @brief  Used by C++ STL (cout) and C printf to send a message
  * @note   Redirects message out USB port
  * @param  file: not used
  * @param  ptr: pointer to first character to be sent
  * @parm   len: how many characters to be sent
  * @retval 0 = message not sent, non-zero indicates how many bytes transferred
  */
int _write(int file, char *ptr, int len)
{
	file = file; // suppress compiler warning

	USBD_StatusTypeDef status = (USBD_StatusTypeDef)CDC_Transmit_FS((uint8_t *)ptr, len);

	// USB enumeration between host and device can/will be delayed on startup, so cout buffer would need
	// special handling to establish & maintain data stream (e.g cout.fail() then cout.clear()
	//
	// TODO always return success???
//	return (status == USBD_OK ? len : 0); // requires special handling when USB link lost
	return len; // no special handling required
}







