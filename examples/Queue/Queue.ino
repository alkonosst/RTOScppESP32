/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/** Explanation of the example:
 * - This example shows how to use a queue to send data between two tasks.
 * - Two tasks are created to illustrate the use of the queue. The producer task sends data to the
 *   consumer task every second.
 * - The consumer task waits indefinitely for data to be received and blinks the LED when it
 * receives the data.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, prints
 * the number of messages in the queue when 'p' is received, and sends data to the queue when 'a' is
 *   received.
 */

#include "RTOScppQueue.h"
#include "RTOScppTask.h"
using namespace RTOS::Queues;
using namespace RTOS::Tasks;

// Custom data to send through the queue
struct CustomData {
  uint8_t a;
  uint8_t b;
};

// Queue object size 10
QueueStatic<CustomData, 10> queue;

// Tasks to test the queue
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

  // Check if the queue was created
  if (!queue) {
    Serial.println("Queue not created");
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

      // Print the queue status
      case 'p': Serial.printf("Serial: Queue messages: %u\n", queue.getAvailableMessages()); break;

      // Add a message to the queue with a timeout of 100ms
      case 'a':
      {
        static CustomData data{255, 255};

        if (!queue.add(data, pdMS_TO_TICKS(100))) {
          Serial.println("Serial: Queue full!");
        } else {
          Serial.printf("Serial: Data sent: %u, %u\n", data.a, data.b);
        }
      } break;
    }
  }
}

void producerFn(void* params) {
  // Custom data to send
  CustomData data{};

  for (;;) {
    // Fill the data
    data.a++;
    data.b += 2;

    // Send the data to the queue with a timeout of 100ms
    if (!queue.add(data, pdMS_TO_TICKS(100))) {
      Serial.println("Producer: Queue full!");
    } else {
      Serial.printf("Producer: Data sent: %u, %u\n", data.a, data.b);
    }

    // Delay to allow the other task to run
    delay(1000);
  }
}

void consumerFn(void* params) {
  // Custom data to receive
  CustomData data{};

  for (;;) {
    // Block until data is received, then pop it from the queue
    queue.pop(data);

    // Print the data
    Serial.printf("\tConsumer: Data received: %u, %u\n", data.a, data.b);

    // Blink the LED
    blinkLED();
  }
}

void blinkLED() {
  Serial.printf("\t\tBlinking LED...\n");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}