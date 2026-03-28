/* USER CODE BEGIN Header */
/**
 * @brief   : Module 7E2 - Mutex Shared OLED Display
 * @course  : STM32 Professional - Altrobyte Automation
 * @author  : Pawan Meena
 * @company : Altrobyte Automation Pvt Ltd
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "semphr.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern I2C_HandleTypeDef hi2c1;

SemaphoreHandle_t oledMutex = NULL;

/* Shared counter — written by CounterTask, read by DisplayTask */
volatile uint32_t g_counter = 0;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartDisplayTask(void const * argument);
void StartCounterTask(void const * argument);
/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ================================================================
 * DISPLAY TASK
 * - Owns the OLED display
 * - Takes oledMutex before every write, gives after
 * - Updates display every 500ms
 * ================================================================ */
void StartDisplayTask(void const * argument)
{
    /* Mutex already created in main(). Just verify it here. */
    if (oledMutex == NULL)
    {
        printf("[DISPLAY] ERROR: oledMutex is NULL. Exiting task.\r\n");
        vTaskDelete(NULL);
        return;
    }

    /* Initialize OLED — safe here as only DisplayTask owns the display */
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    printf("[DISPLAY] OLED initialized. Starting display loop.\r\n");

    char line1[20];
    char line2[20];

    for (;;)
    {
        /* Take mutex — wait max 100ms
         * If CounterTask holds it, this task sleeps (no busy-wait)
         * 100ms timeout prevents deadlock hang               */
        if (xSemaphoreTake(oledMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* ===== CRITICAL SECTION: Only one task executes here ===== */

            ssd1306_Fill(Black);

            /* Line 1: Counter value (read inside mutex — consistent value) */
            snprintf(line1, sizeof(line1), "Counter: %lu", (unsigned long)g_counter);
            ssd1306_SetCursor(0, 0);
            ssd1306_WriteString(line1, Font_7x10, White);

            /* Line 2: Uptime in seconds */
            uint32_t uptime = xTaskGetTickCount() / 1000;
            snprintf(line2, sizeof(line2), "Up: %lus", (unsigned long)uptime);
            ssd1306_SetCursor(0, 20);
            ssd1306_WriteString(line2, Font_7x10, White);

            /* Line 3: Static branding label */
            ssd1306_SetCursor(0, 40);
            ssd1306_WriteString("Altrobyte FreeRTOS", Font_6x8, White);

            /* Push frame buffer to physical display */
            ssd1306_UpdateScreen();

            /* ===== END CRITICAL SECTION ===== */

            printf("[DISPLAY] Counter: %lu | Uptime: %lus\r\n",
                   (unsigned long)g_counter, (unsigned long)uptime);

            /* FIX #3: Give mutex BEFORE vTaskDelay — never hold mutex during sleep */
            xSemaphoreGive(oledMutex);
        }
        else
        {
            /* Mutex timeout — another task held it too long, log the event */
            printf("[DISPLAY] WARNING: Mutex timeout! OLED update skipped.\r\n");
        }

        /* Sleep 500ms — display refresh rate */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ================================================================
 * COUNTER TASK
 * - Increments g_counter every 1000ms
 * - FIX #2: Takes mutex before modifying shared variable g_counter
 *   This ensures DisplayTask never reads a half-updated value
 *   (critical for multi-byte types; good practice for uint32_t too)
 * ================================================================ */
void StartCounterTask(void const * argument)
{
    printf("[COUNTER] Started. Incrementing every 1000ms.\r\n");

    for (;;)
    {
        /* FIX #2: Protect shared variable with mutex */
        if (oledMutex != NULL)
        {
            if (xSemaphoreTake(oledMutex, pdMS_TO_TICKS(200)) == pdTRUE)
            {
                g_counter++;
                printf("[COUNTER] Count: %lu\r\n", (unsigned long)g_counter);
                xSemaphoreGive(oledMutex);
            }
            else
            {
                printf("[COUNTER] WARNING: Mutex timeout! Counter not incremented.\r\n");
            }
        }

        /* Increment every 1000ms — independent of DisplayTask's 500ms cycle */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* USER CODE END Application */

