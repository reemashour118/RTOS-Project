#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include <time.h>

#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

xSemaphoreHandle SenderSemaphore1 = NULL;
xSemaphoreHandle SenderSemaphore2 = NULL;
xSemaphoreHandle ReciverSemaphore = NULL;

QueueHandle_t testQueue = NULL;

TaskHandle_t sendertask1handle = NULL;
TaskHandle_t sendertask2handle = NULL;
TaskHandle_t receiverTaskhandle = NULL;

TimerHandle_t autoReloadReceiverTimer;
TimerHandle_t autoReloadSenderTimer1;
TimerHandle_t autoReloadSenderTimer2;

#define receiver_TIMER_PERIOD pdMS_TO_TICKS(100)

int32_t transmitted_messages = 0;
int32_t received_messages = 0;
int32_t blocked_messages = 0;

char sentmessage[40];
char recievedmessage[40];
int randomtime1 = 0;
int randomtime2 = 0;
int counter = 0;
int count = 6;
int lower = 0;
int upper = 0;
int i = 0;
static int j = 0;

const int array1[] = {50, 80, 110, 140, 170, 200};
const int array2[] = {150, 200, 250, 300, 350, 400};

void randomtimecalc()
{
  for (int j = 0; j < 6; j++)
  {
    int lower = array1[j], upper = array2[j];
    srand(time(0));
    randomtime1 = (rand() % (upper - lower + 1)) + lower;
    randomtime2 = (rand() % (upper - lower + 1)) + lower;
    counter++;
    randomtime1 += randomtime1;
    randomtime2 += randomtime2;
  }
}

int Generate_Random_Numbers(int j)
{
    int lower = array1[j-1], upper = array2[j-1];
    int result = randomtime1 = (rand() % (upper - lower + 1)) + lower;
	return result;
}

void Printandreset()
{
  printf("Result in range of %d : %d\n",array1[j-1],array2[j-1]);
  printf("total number of blocked messages is %lu\n", blocked_messages);
  printf("total number of transmitted messages is %lu\n", transmitted_messages);
  printf("total number of received messages is %lu\n", received_messages);
  printf("\n\n");
//  printf("average1:  %d\n", randomtime1 / counter);
//  printf("average2: %d\n", randomtime2 / counter);
  transmitted_messages = 0;
  received_messages = 0;
  blocked_messages = 0;
  xQueueReset(testQueue);
}

void clearanddefine()
{
  if (j == 6)
  {
    xTimerDelete(autoReloadSenderTimer1, 0);
    xTimerDelete(autoReloadSenderTimer2, 0);
    xTimerDelete(autoReloadReceiverTimer, 0);
    printf("Game Over\n");
    vTaskEndScheduler();
  } //#define receiver_TIMER_PERIOD pdMS_TO_TICKS(100)
  else
    j++;
}

void sendertask1(void *parameters)
{
  BaseType_t status;
  while (1)
  {
    randomtimecalc();
	#define SEND_TIMER_PERIOD1 pdMS_TO_TICKS(randomtime1)
    xTimerChangePeriod(autoReloadSenderTimer1, Generate_Random_Numbers(j), 0);
    if (xSemaphoreTake(SenderSemaphore1, portMAX_DELAY))
    {
      sprintf(sentmessage, "Time is %lu", xTaskGetTickCount());
      status = xQueueSend(testQueue, sentmessage, 0);
      if (status != pdPASS)
        blocked_messages++;
      else
        transmitted_messages++;
    }
  }
}

void sendertask2(void *parameters)
{
  BaseType_t status;
  while (1)
  {
    randomtimecalc();
	#define SEND_TIMER_PERIOD2 pdMS_TO_TICKS(randomtime2)
    xTimerChangePeriod(autoReloadSenderTimer2, Generate_Random_Numbers(j), 0);

    if (xSemaphoreTake(SenderSemaphore2, portMAX_DELAY))
    {
      sprintf(sentmessage, "Time is %lu", xTaskGetTickCount());

      status = xQueueSend(testQueue, sentmessage, 0);
      if (status != pdPASS)
        blocked_messages++;
      else
        transmitted_messages++;
    }
  }
}

void receiverTask(void *parameters)
{
  BaseType_t status;
  while (1)
  {
    if (xSemaphoreTake(ReciverSemaphore, 100))
    {
      status = xQueueReceive(testQueue, recievedmessage, 0);
      if (status == pdPASS)
        received_messages++;
    }
  }
}

void senderCallback1()
{
  xSemaphoreGive(SenderSemaphore1);
}

void senderCallback2()
{
  xSemaphoreGive(SenderSemaphore2);
}

void receiverCallback()
{
  if (received_messages >= 500)
  {
    Printandreset();
    clearanddefine();
  }
  else
    xSemaphoreGive(ReciverSemaphore);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

// ----- main() ---------------------------------------------------------------

int main(int argc, char *argv[])
{
  clearanddefine();
  randomtimecalc();

  BaseType_t status;
  BaseType_t timerstarted;

  testQueue = xQueueCreate(2, sizeof(sentmessage));

  xTaskCreate(sendertask1, "Sender1", 1024, (void *)0, 1, NULL);
  xTaskCreate(sendertask2, "Sender2", 1024, (void *)0, 1, NULL);
  xTaskCreate(receiverTask, "receiver", 1024, NULL, 2, NULL);

  SenderSemaphore1 = xSemaphoreCreateBinary();
  SenderSemaphore2 = xSemaphoreCreateBinary();
  ReciverSemaphore = xSemaphoreCreateBinary();

  autoReloadSenderTimer1 = xTimerCreate("sender_timer1", randomtime1,pdTRUE, NULL, senderCallback1);
  autoReloadSenderTimer2 = xTimerCreate("sender_timer2", randomtime2,pdTRUE, NULL, senderCallback2);
  autoReloadReceiverTimer = xTimerCreate("receiver_timer", receiver_TIMER_PERIOD,pdTRUE, NULL, receiverCallback);

  xTimerStart(autoReloadSenderTimer1, 0);
  xTimerStart(autoReloadSenderTimer2, 0);
  xTimerStart(autoReloadReceiverTimer, 0);

  vTaskStartScheduler();

  return 0;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

void vApplicationMallocFailedHook(void)
{
  /* Called if a call to pvPortMalloc() fails because there is insufficient
   free memory available in the FreeRTOS heap.  pvPortMalloc() is called
   internally by FreeRTOS API functions that create tasks, queues, software
   timers, and semaphores.  The size of the FreeRTOS heap is set by the
   configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
  for (;;)
    ;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
  (void)pcTaskName;
  (void)pxTask;

  /* Run time stack overflow checking is performed if
   configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
   function is called if a stack overflow is detected. */
  for (;;)
    ;
}

void vApplicationIdleHook(void)
{
  volatile size_t xFreeStackSpace;

  /* This function is called on each cycle of the idle task.  In this case it
   does nothing useful, other than report the amout of FreeRTOS heap that
   remains unallocated. */
  xFreeStackSpace = xPortGetFreeHeapSize();

  if (xFreeStackSpace > 100)
  {
    /* By now, the kernel has allocated everything it is going to, so
     if there is a lot of heap remaining unallocated then
     the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
     reduced accordingly. */
  }
}

void vApplicationTickHook(void)
{
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
   state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
   Note that, as the array is necessarily of type StackType_t,
   configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 application must provide an implementation of vApplicationGetTimerTaskMemory()
 to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
