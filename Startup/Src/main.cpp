
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
char buf[BUFSIZ * 8];


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
/*
            int SimulatedWorkDelayMs = 0;
            int TaskPeriodMs = 500;
            TickType_t TaskPeriodTicks = Ticks::MsToTicks(TaskPeriodMs);
*/

        	// ResetDelayUntil();
        	uint8_t randomArray[] = " - USB Large Message";

            while (true) {
/*
                TickType_t Start = Ticks::GetTicks();

                SimulatedWorkDelayMs += 33;
                if (SimulatedWorkDelayMs > TaskPeriodMs) {
                    SimulatedWorkDelayMs = 0;
                    TaskPeriodMs += 500;
                    TaskPeriodTicks = Ticks::MsToTicks(TaskPeriodMs);
                }
                TickType_t ticks = Ticks::MsToTicks(SimulatedWorkDelayMs);

                //
                //  Simulate variable length processing time
                //
                Delay(ticks);

                DelayUntil(TaskPeriodTicks);

                TickType_t End = Ticks::GetTicks();

                TickType_t Diff = End - Start;
                TickType_t PeriodMeasuredMs = Ticks::TicksToMs(Diff);

*/

//                DelayUntil(1000);

            	GPIOB->ODR ^= GPIO_ODR_OD14;

            	_write(0, buf, sizeof(buf)); // send a really big random buffer out the USB port

/*
            	for (int index = 0; index < 10; index++)
            	{
                	printf("Message burst - %u - %lu \n", index, Ticks::GetTicks());
            	}
            	printf("\n");
*/
/*
            	for (int index = 0; index < 10; index++)
            	{
                	cout << "Message burst - " << index << " - " << Ticks::GetTicks() << endl;
            	}
            	cout << endl;
*/
/*
            	cout << Ticks::GetTicks();
            	for (uint32_t index = 0; index < 50; index++)
            	{
                	cout << randomArray;
            	}
            	cout << endl;

            	cout << Ticks::GetTicks() << endl;

*/
/*
            	printf("%lu", Ticks::GetTicks());
            	for (uint32_t i = 0; i < 50; i++)
            	{
					for (uint32_t index = 0; index < sizeof(randomArray) - 1; index++)
					{
						printf("%c", randomArray[index]);
					}
            	}
            	printf("\n");
            	fflush(stdout);
*/
/*
            	printf("%lu\n", Ticks::GetTicks());
            	fflush(stdout);

            	printf("%lu\n", Ticks::GetTicks());
            	fflush(stdout);
*/
//            	printf("Hello world - %lu \n", Ticks::GetTicks());
//            	printf("Second Msg. - %lu \n\n", Ticks::GetTicks());
//            	fflush(stdout);

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
	setbuf(stdout, buf);


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

