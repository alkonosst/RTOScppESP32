/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/** Explanation of the example:
 * - This example shows how to use a recursive mutex to protect a shared resource, in this case, the
 * LED.
 * - Two tasks are created to illustrate the use of the mutex. Both tasks blink the LED,
 *  but one blinks it 5 times and the other 10 times.
 * - The function blinkLED is used to blink the LED and is protected by the mutex, meaning that only
 *   one task can access the LED at a time.
 * - When a mutex is taken, another task trying to take it will be blocked until the mutex is given.
 * - A recursive mutex allows the same task to take the mutex multiple times without blocking
 *   itself. To release the mutex, the mutex must be given the same number of times it was taken.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received.
 */

#include "RTOScppLock.h"
#include "RTOScppTask.h"
using namespace RTOS::Locks;
using namespace RTOS::Tasks;

// Mutex recursive object
MutexRecursiveStatic mutex_recursive;

// Tasks to test the lock
void task1Fn(void* params);
void task2Fn(void* params);
TaskStatic<4 * 1024> task1("Task1", task1Fn, 1);
TaskStatic<4 * 1024> task2("Task2", task2Fn, 1);

// Shared resource example: blinking LED
void blinkLED(const uint8_t times);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(LED_BUILTIN, OUTPUT);

  // Check if the mutex was created
  if (!mutex_recursive) {
    Serial.println("Recursive Mutex not created");
    while (true)
      ;
  }

  // Create the tasks
  if (!task1.create()) {
    Serial.println("Task 1 not created");
    while (true)
      ;
  }

  if (!task2.create()) {
    Serial.println("Task 2 not created");
    while (true)
      ;
  }
}

void loop() {
  // Read serial input
  if (Serial.available()) {
    char c = Serial.read();

    switch (c) {
      // Restart ESP32
      case '.': ESP.restart(); break;
    }
  }
}

void task1Fn(void* params) {
  for (;;) {
    // Blink the LED 5 times
    blinkLED(5);

    // Release the mutex for the second time
    mutex_recursive.give();

    // Small delay to allow the other task to run
    // If there weren't any delay, the task2 would never run (starvation effect)
    delay(10);
  }
}

void task2Fn(void* params) {
  for (;;) {
    // Blink the LED 10 times
    blinkLED(10);

    // Release the mutex for the second time
    mutex_recursive.give();

    // Small delay to allow the other task to run
    delay(10);
  }
}

void blinkLED(const uint8_t times) {
  // Take the mutex for the first time
  mutex_recursive.take();

  Serial.printf("Blinking LED %u times... ", times);

  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  Serial.print("waiting... ");
  delay(2000);
  Serial.println("done!");

  // Take the mutex for the second time
  mutex_recursive.take();

  // Give the mutex one time
  // To release the mutex, the mutex must be given the same number of times it was taken
  // So, the mutex will be released after the second call to give
  mutex_recursive.give();
}