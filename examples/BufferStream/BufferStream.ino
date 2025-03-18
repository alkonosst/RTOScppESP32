/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/** Explanation of the example:
 * - This example shows how to use a stream buffer to send data between two tasks.
 * - Two tasks are created to illustrate the use of the stream buffer. The producer task sends data
 * to the consumer task every second.
 * - The consumer task waits indefinitely for data to be received and blinks the LED when it
 * receives the data.
 * - The consumer task will not receive the data until the trigger level is reached (5 bytes).
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, and
 * changes the trigger level of the buffer when '1' or '5' is received.
 */

#include "RTOScppBuffer.h"
#include "RTOScppTask.h"
using namespace RTOS::Buffers;
using namespace RTOS::Tasks;

// Stream buffer with a size of 10 bytes and a trigger level of 5 bytes
StreamBufferStatic<10, 5> buffer;

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

      // Change the trigger level of the buffer
      case '1':
      {
        if (buffer.setTriggerLevel(1)) {
          Serial.println("Trigger level set to 1");
        }
      } break;

      case '5':
      {
        if (buffer.setTriggerLevel(5)) {
          Serial.println("Trigger level set to 5");
        }
      } break;
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

    // Send the message to the buffer
    buffer.send(message, len);
  }
}

void consumerFn(void* params) {
  char message[20];
  const size_t len = sizeof(message);

  for (;;) {
    // Receive the message from the buffer when the trigger level (5) is reached
    uint32_t bytes_recv = buffer.receive(message, len);

    if (bytes_recv == 0) {
      Serial.println("\tConsumer: Failed to receive!");
      continue;
    }

    // Add the null terminator to the message
    message[bytes_recv] = '\0';

    // Print the message received and blink the LED
    Serial.printf("\tConsumer: Message received: %s\n", message);
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