/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of the example:
 * - Two timers are created with different periods, one to blink the LED every 100 ms and the other
 *   to print a message every 1000 ms.
 * - The timers are created and started in the setup function using static memory allocation.
 * - The loop function reads the serial input and performs the following actions:
 *    - Restart the ESP32 when '.' is received.
 *    - Reset the blink timer when '1' is received.
 *    - Reset the print timer when '2' is received.
 *    - Change the blink timer period to 100 ms when '3' is received.
 *    - Change the blink timer period to 1000 ms when '4' is received.
 * - The LED blinks if the blink timer expired and restarts it.
 * - A message is printed if the print timer expired and restarts it.
 */

#include "RTOScppTimer.h"
using namespace RTOS::Timers;

// Flags to indicate if the timer expired
static bool timer_blink_expired = false;
static bool timer_print_expired = false;

// Timer callbacks
void timerBlinkFn(TimerHandle_t timer) { timer_blink_expired = true; }
void timerPrintFn(TimerHandle_t timer) { timer_print_expired = true; }

TimerStatic timer_blink(
  /*name*/ "TimerBlink",
  /*callback*/ timerBlinkFn,
  /*period*/ pdMS_TO_TICKS(100),
  /*id*/ nullptr,
  /*auto-reload*/ false,
  /*start*/ false);

TimerStatic timer_print(
  /*name*/ "TimerPrint",
  /*callback*/ timerPrintFn,
  /*period*/ pdMS_TO_TICKS(1000),
  /*id*/ nullptr,
  /*auto-reload*/ false,
  /*start*/ false);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(LED_BUILTIN, OUTPUT);

  // Start timers
  timer_blink.start();
  timer_print.start();
}

void loop() {
  // Read serial input
  if (Serial.available()) {
    char c = Serial.read();

    switch (c) {
      // Restart ESP32
      case '.': ESP.restart(); break;

      // Reset timers
      case '1': timer_blink.reset(); break;
      case '2': timer_print.reset(); break;

      // Change blink timer period
      case '3': timer_blink.setPeriod(pdMS_TO_TICKS(100)); break;
      case '4': timer_blink.setPeriod(pdMS_TO_TICKS(1000)); break;
    }
  }

  // Blink LED if the timer expired and restart it
  if (timer_blink_expired) {
    timer_blink_expired = false;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    timer_blink.start();
  }

  // Print message if the timer expired and restart it
  if (timer_print_expired) {
    static uint32_t counter = 0;
    timer_print_expired     = false;
    Serial.printf("Timer print expired! Counter: %u\n", counter++);
    timer_print.start();
  }
}