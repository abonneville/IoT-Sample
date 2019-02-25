
#include <iostream>
#include <string>

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

/* Private function prototypes -----------------------------------------------*/
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

                DelayUntil(1000);

            	GPIOB->ODR ^= GPIO_ODR_OD14;
            	cout << "Hello world - " << Ticks::GetTicks() << "\r" << endl;
//            	cerr << "Hello world - " << Ticks::GetTicks() << "\r" << endl;



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

