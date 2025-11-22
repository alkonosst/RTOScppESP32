/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of the example:
 * - This example shows how to use a binary semaphore to signal an event between two tasks.
 * - Two tasks are created to illustrate the use of the semaphore. The producer task sends an event
 *   to the consumer task every second.
 * - The consumer task waits indefinitely for the semaphore to be signaled and blinks the LED when
 * it receives the event.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, and
 * signals the event to the consumer task when 's' is received.
 */

#include "RTOScppLock.h"
#include "RTOScppTask.h"
using namespace RTOS::Locks;
using namespace RTOS::Tasks;

// Binary Semaphore object
SemBinaryStatic sem;

// Tasks to test the lock
void producerFn(void* params);
void consumerFn(void* params);
TaskStatic<4 * 1024> producer("Producer", producerFn, 1);
TaskStatic<4 * 1024> consumer("Consumer", consumerFn, 1);

// Blink the internal LED one time
void blinkLED();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(LED_BUILTIN, OUTPUT);

  // Check if the mutex was created
  if (!sem) {
    Serial.println("Semaphore not created");
    while (true)
      ;
  }

  // Create the tasks
  if (!producer.create()) {
    Serial.println("Producer task not created");
    while (true)
      ;
  }

  if (!consumer.create()) {
    Serial.println("Consumer task not created");
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

      // Signal a event to the receiver task using the semaphore
      case 's':
        Serial.println("Serial: Producing event...");
        sem.give();
        break;
    }
  }
}

void producerFn(void* params) {
  for (;;) {
    // Delay to simulate some work (e.g. reading a sensor)
    delay(1000);

    Serial.println("Producer: Producing event...");

    // Signal a event to the consumer task using the semaphore
    sem.give();
  }
}

void consumerFn(void* params) {
  for (;;) {
    // Wait indefinitely for the semaphore to be signaled
    sem.take();

    Serial.println("\tConsumer: Received event!");

    // Blink the LED to indicate the event
    blinkLED();
  }
}

void blinkLED() {
  Serial.println("\t\tBlinking LED...");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}