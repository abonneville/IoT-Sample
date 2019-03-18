
//#include <iostream>
#include <string>
#include <cstdio>

//#include "main.hpp"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "thread.hpp"
#include "ticks.hpp"
#include "stm32l4xx_hal.h"
#include "usb_device.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
char obuf[BUFSIZ] = {}; // output buffer
char ibuf[64] = {}; // input buffer, match to USB
static char commandLineBuffer[1024] = {};


/* Private function prototypes -----------------------------------------------*/
extern "C" int _write(int file, char *ptr, int len);

using namespace cpp_freertos;
using namespace std;

class TestThread : public Thread {

    public:

        TestThread()
           : Thread("TestThread", 100, 2)
        {
            Start();
        };

    protected:

        virtual void Run() {
            while (true) {

//                DelayUntil(1000);

            	GPIOB->ODR ^= GPIO_ODR_OD14;
            	fgets(commandLineBuffer, sizeof(commandLineBuffer), stdin);
            	printf("%lu - %s", Ticks::GetTicks(), commandLineBuffer);
            	fflush(stdout);


//            	_write(0, buf, sizeof(buf)); // send a really big random buffer out the USB port


//            	cout << "Hello world - " << Ticks::GetTicks() << endl;
//            	cout << "Second message " << endl;
//
//            	cerr << "Hello world - " << Ticks::GetTicks() << "\r" << endl;
//            	clog << "Hello world - " << Ticks::GetTicks() << "\r" << endl;

            	// USB enumeration between host and device can/will be delayed on startup, so cout buffer will need
            	// special handling to establish data stream
/*            	if (cout.fail()) {
            		cout.clear();
            	}
*/

            }
        };

    private:
};



/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	// User allocated buffer to streaming messages over USB, flushing is manually controlled
	setvbuf(stdin,  ibuf, _IOLBF, sizeof(ibuf));
	setvbuf(stdout, obuf, _IOFBF, sizeof(obuf));


  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();

  TestThread thread;
  Thread::StartScheduler();


  /* Infinite loop */
  while (1)
  {
    }
}


// TODO determine what to be done with global new delete and ucHeap
#if 0
void * operator new( size_t size )
{
	return pvPortMalloc(size);
}

/*
void * operator new
{
	return pvPortMalloc( size );
}
*/

void operator delete( void * ptr )
{
	vPortFree( ptr );
}

/*
void operator delete
{
	vPortFree( ptr );
}
*/

#endif

