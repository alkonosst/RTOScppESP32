/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "RTOScppRingBuffer.h"
#include "RTOScppTask.h"
using namespace RTOS::RingBuffers;
using namespace RTOS::Tasks;

/** Explanation of the example:
 * - This example shows how to use a split ring buffer to send data between two tasks.
 * - Two tasks are created to illustrate the use of the split ring buffer. The producer task sends
 * data to the consumer task every second.
 * - The consumer task waits indefinitely for data to be received and blinks the LED when it
 * receives the data.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, prints
 * the buffer information when 'i' is received, and sends a message to the buffer when 'a' is
 * received.
 * - Try to send a message with serial. Suddenly you will see a split message received. This is
 * because the message was split into two parts to fit in the buffer. When this happens, the
 * consumer task will concatenate the two parts and print the complete message.
 */

// Split Ring Buffer with a size of 32 bytes
// - The max item size is (bufferSize / 2)
// - In this case, the max item size is (32 / 2) = 16 bytes
// - Remember that internally the buffer will be aligned to the next 4 bytes multiple, so if you use
//   a size of 30, it will be aligned to 32 bytes
RingBufferSplitStatic<char, 32> buffer;

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
  const char* message = "12345678";
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
  char msg[32]; // Buffer to receive the message if it was split

  for (;;) {
    char* head_msg   = nullptr;
    char* tail_msg   = nullptr;
    size_t head_size = 0;
    size_t tail_size = 0;

    bool receive = buffer.receive(head_msg, tail_msg, head_size, tail_size);

    if (!receive) {
      Serial.println("\tConsumer: Failed to receive!");
      continue;
    }

    // Message not split
    if (!tail_msg) {
      // Print the message received and blink the LED
      Serial.printf("\tConsumer: Message received (%u): %s\n", head_size, head_msg);
      blinkLED();

      // Return the item once it's no longer needed
      buffer.returnItem(head_msg);
      continue;
    }

    // Message split
    // Ensure that the message fits in the buffer
    if (head_size + tail_size > sizeof(msg)) {
      Serial.println("\tConsumer: Message too large to fit in the buffer");
      continue;
    }

    // Copy the head and tail messages to the buffer
    memcpy(msg, head_msg, head_size);
    memcpy(msg + head_size, tail_msg, tail_size);

    // Print the message received and blink the LED
    Serial.printf("\tConsumer: Message received [SPLIT - head: %u, tail: %u]: %s\n",
      head_size,
      tail_size,
      msg);

    blinkLED();

    // Return the items once they're no longer needed
    buffer.returnItem(head_msg);
    buffer.returnItem(tail_msg);
  }
}

void blinkLED() {
  Serial.printf("\t\tBlinking LED...\n");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}