/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "RTOScppRingBuffer.h"
#include "RTOScppTask.h"
using namespace RTOS::RingBuffers;
using namespace RTOS::Tasks;

/** Explanation of the example:
 * - This example shows how to use a no-split ring buffer to send data between two tasks.
 * - Two tasks are created to illustrate the use of the no-split ring buffer. The producer task
 * sends data to the consumer task every second.
 * - The consumer task waits indefinitely for data to be received and blinks the LED when it
 * receives the data.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, prints
 * the buffer information when 'i' is received, and sends a message to the buffer when 'a' is
 * received.
 */

// No-split Ring Buffer with a size of 32 bytes
// - The max item size is ((bufferSize / 2) - headerSize), headerSize being 8 bytes
// - In this case, the item is (32 / 2) - 8 = 8 bytes
// - Remember that internally the buffer will be aligned to the next 4 bytes multiple, so if you use
//   a size of 30, it will be aligned to 32 bytes
RingBufferNoSplitStatic<char, 32> buffer;

// Tasks to test the buffer
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

  // Check if the buffer was created
  if (!buffer) {
    Serial.println("Buffer not created");
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

      // Print buffer information
      case 'i':
      {
        size_t max_item_size = buffer.getMaxItemSize();
        size_t free_size     = buffer.getFreeSize();

        Serial.printf("Serial: Buffer size: %u/%u\n", free_size, max_item_size);
        break;
      }

      // Add a message to the buffer
      case 'a':
      {
        const char* message = "Hello!";
        const uint8_t len   = strlen(message);

        Serial.printf("Serial: Sending message: %s\n", message);

        // Send the message to the buffer including the null terminator, with a timeout of 100ms
        if (!buffer.send(message, len + 1, pdMS_TO_TICKS(100))) {
          Serial.println("Failed to send message to buffer");
        }
        break;
      }
    }
  }
}

void producerFn(void* params) {
  const char* message = "1234";
  const uint8_t len   = strlen(message);

  for (;;) {
    // Delay to simulate work being done
    delay(1000);

    Serial.printf("Producer: Sending message: %s\n", message);

    // Send the message to the buffer, including the null terminator
    buffer.send(message, len + 1);
  }
}

void consumerFn(void* params) {
  for (;;) {
    size_t bytes_recv;
    char* msg = buffer.receive(bytes_recv);

    if (msg == nullptr) {
      Serial.println("\tConsumer: Failed to receive!");
      continue;
    }

    // Print the message received and blink the LED
    Serial.printf("\tConsumer: Message received (%u): %s\n", bytes_recv, msg);
    blinkLED();

    // Return the item once it's no longer needed
    buffer.returnItem(msg);
  }
}

void blinkLED() {
  Serial.printf("\t\tBlinking LED...\n");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}