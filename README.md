# COUNTER_OLED_STM32_FreeRTOS

> **FreeRTOS Mutex Demo — Shared OLED Display with Counter Task**  
> STM32F103RB (Nucleo-F103RB) | STM32CubeIDE | HAL | FreeRTOS (CMSIS-RTOS v1)

---

## Overview

This project demonstrates **mutex-protected shared resource access** using FreeRTOS on the STM32F103RB microcontroller. Two concurrent tasks share a single SSD1306 OLED display over I2C. A mutex ensures only one task accesses the display bus at a time, preventing data corruption and garbled output.

This is **Module 7E2** of the *STM32 FreeRTOS Professional Workshop* by Altrobyte Automation Pvt Ltd.

---

## Hardware

| Component | Details |
|---|---|
| MCU Board | STM32 Nucleo-F103RB |
| Microcontroller | STM32F103RBT6 (Cortex-M3, 72MHz) |
| Display | SSD1306 OLED 128×64 (I2C) |
| Interface | I2C1 — PB6 (SCL), PB7 (SDA) |
| Clock Speed | 8MHz HSI (no PLL) |

**Wiring:**

```
SSD1306 VCC  →  3.3V
SSD1306 GND  →  GND
SSD1306 SCL  →  PB6 (I2C1_SCL)
SSD1306 SDA  →  PB7 (I2C1_SDA)
```

---

## Software Architecture

### RTOS Tasks

| Task | Priority | Stack | Period | Role |
|---|---|---|---|---|
| `DisplayTask` | Normal | 256 words | 500ms | Reads counter, updates OLED |
| `CounterTask` | Normal | 128 words | 1000ms | Increments shared counter |
| `DefaultTask` | Normal | 128 words | — | Idle placeholder |

### Shared Resources

| Resource | Type | Protected By |
|---|---|---|
| SSD1306 OLED (I2C bus) | Hardware peripheral | `oledMutex` |
| `g_counter` (uint32_t) | Global variable | `oledMutex` |

### Mutex Design

```
DisplayTask                        CounterTask
    │                                   │
    ├─ xSemaphoreTake(oledMutex, 100ms) ├─ xSemaphoreTake(oledMutex, 200ms)
    ├─ ssd1306_Fill()                   ├─ g_counter++
    ├─ ssd1306_WriteString()            ├─ xSemaphoreGive(oledMutex)
    ├─ ssd1306_UpdateScreen()           │
    ├─ xSemaphoreGive(oledMutex)        │
    └─ vTaskDelay(500ms)                └─ vTaskDelay(1000ms)
```

**Key design decisions:**
- Mutex is **given before `vTaskDelay()`** — never held during sleep
- Both tasks use **timeout-based `xSemaphoreTake()`** to avoid deadlock
- Mutex created in `main()` before scheduler starts, in `RTOS_MUTEX` section

---

## OLED Display Output

```
┌────────────────────────┐
│ Counter: 42            │
│                        │
│ Up: 21s                │
│                        │
│ Altrobyte FreeRTOS     │
└────────────────────────┘
```

---

## Project Structure

```
COUNTER_OLED_STM32_FreeRTOS/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── ssd1306.h
│   │   ├── ssd1306_fonts.h
│   │   ├── ssd1306_conf.h          # I2C mode, resolution, handle
│   │   └── stm32f1xx_hal_conf.h
│   └── Src/
│       ├── main.c                  # Init, mutex creation, task registration
│       ├── freertos.c              # DisplayTask, CounterTask implementations
│       ├── ssd1306.c               # OLED driver
│       ├── ssd1306_fonts.c         # Font bitmaps
│       └── stm32f1xx_hal_timebase_tim.c
├── Drivers/
│   ├── STM32F1xx_HAL_Driver/
│   └── CMSIS/
├── Middlewares/
│   └── Third_Party/FreeRTOS/
└── README.md
```

---

## Key Concepts Demonstrated

- **Mutex vs Binary Semaphore** — mutex carries ownership; prevents priority inversion
- **Shared peripheral protection** — I2C bus accessed by only one task at a time
- **Shared variable protection** — `g_counter` guarded even though `uint32_t` is atomic on Cortex-M3 (good practice for portability)
- **Timeout-based acquire** — `pdMS_TO_TICKS(100)` prevents permanent blocking
- **Never hold mutex during sleep** — give before `vTaskDelay()`, not after

---

## Build Environment

| Tool | Version |
|---|---|
| STM32CubeIDE | 2.1.0 |
| GCC (arm-none-eabi) | 14.3.1 |
| STM32CubeF1 HAL | Latest |
| FreeRTOS | CMSIS-RTOS v1 wrapper |
| SSD1306 Driver | Alexey Dynda / community port |

**Build configuration:** Debug (`-O0 -g3`)

---

## How to Build and Flash

1. Clone the repo:
```bash
git clone git@github.com:Mpraveenkumar61/COUNTER_OLED_STM32_FreeRTOS.git
```

2. Open STM32CubeIDE → **File → Open Projects from File System**

3. Select the cloned folder → **Finish**

4. **Project → Build Project** (or `Ctrl+B`)

5. Connect Nucleo board via USB → **Run → Debug** (or `F11`)

---

## Serial Output (SWV / UART)

Connect a serial monitor at **115200 baud** to observe task logs:

```
[MAIN] Mutex created OK. Starting scheduler...
[DISPLAY] OLED initialized. Starting display loop.
[COUNTER] Started. Incrementing every 1000ms.
[COUNTER] Count: 1
[DISPLAY] Counter: 1 | Uptime: 1s
[COUNTER] Count: 2
[DISPLAY] Counter: 2 | Uptime: 2s
```

---

## Course Reference

| Field | Detail |
|---|---|
| Module | 7E2 — Mutex: Shared OLED Display |
| Course | STM32 FreeRTOS Professional Workshop |
| Instructor | Pawan Meena |
| Organization | Altrobyte Automation Pvt Ltd |

---

## Author

**Praveen Kumar Mudigonda**  
Embedded Systems & IoT Engineer (in transition)  
GitHub: [@Mpraveenkumar61](https://github.com/Mpraveenkumar61)

---

## License

This project is for educational purposes as part of a structured embedded systems workshop.
