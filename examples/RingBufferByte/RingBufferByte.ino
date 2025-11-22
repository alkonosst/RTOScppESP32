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
 * - This example shows how to use a byte ring buffer to send data between two tasks.
 * - Two tasks are created to illustrate the use of the byte ring buffer. The producer task sends
 * data to the consumer task every second.
 * - The consumer task waits for data to be received and blinks the LED when it receives the data.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, prints
 * the buffer information when 'i' is received, and sends a message to the buffer when 'a' is
 * received.
 * - The consumer task will receive up to 4 bytes from the buffer and print them. So, you will see
 *   in the serial monitor that the message is received in chunks.
 */

// Byte Ring Buffer with a size of 32 bytes
// - The max item size is 32 bytes and the buffer is aligned to the next 4 bytes multiple
RingBufferByteStatic<32> buffer;

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
        const uint8_t message[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};

        Serial.printf("Serial: Sending message\n");

        // Send the message to the buffer, with a timeout of 100ms
        if (!buffer.send(message, sizeof(message), pdMS_TO_TICKS(100))) {
          Serial.println("Failed to send message to buffer");
        }
        break;
      }
    }
  }
}

void producerFn(void* params) {
  const uint8_t message[] = {'1', '2', '3', '4', '5', '6', '7', '8'};

  for (;;) {
    // Delay to simulate work being done
    delay(1000);

    Serial.printf("Producer: Sending message\n");

    // Send the message to the buffer
    buffer.send(message, sizeof(message));
  }
}

void consumerFn(void* params) {
  char chunk[5]; // Buffer to receive the message in chunks

  for (;;) {
    // Receive up to 4 bytes from the buffer
    size_t bytes_recv;
    uint8_t* msg = buffer.receiveUpTo(4, bytes_recv);

    if (msg == nullptr) {
      Serial.println("\tConsumer: Failed to receive!");
      continue;
    }

    // Store the message in a buffer and null-terminate it
    memcpy(chunk, msg, bytes_recv);
    chunk[bytes_recv] = '\0';

    // Print the message received and blink the LED
    Serial.printf("\tConsumer: Message received (%u): %s\n", bytes_recv, chunk);
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