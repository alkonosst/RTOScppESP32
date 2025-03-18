/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "RTOScppQueueSet.h"
#include "RTOScppTask.h"
using namespace RTOS::QueueSets;
using namespace RTOS::Locks;
using namespace RTOS::Queues;
using namespace RTOS::RingBuffers;
using namespace RTOS::Tasks;

/** Explanation of the example:
 * - This example shows how to use a queue set to receive events from a semaphore, a queue, and a
 * ring buffer.
 * - A task is created to illustrate the use of the queue set. The task waits indefinitely for an
 *   event to be received and blinks the LED when it receives the event.
 * - The loop function reads the serial input and restarts the ESP32 when '.' is received, gives the
 *   semaphore when 's' is received, sends data to the queue when 'q' is received, and sends data to
 *   the ring buffer when 'b' is received.
 * - The queue set is created with a length of 1 + 10 + 3 = 14, which is the sum of the maximum
 *   number of events that can be stored in the semaphore, queue, and buffer.
 * - The queue set is subscribed to the semaphore, queue, and buffer.
 * - The task blocks indefinitely until an event is received from the queue set.
 * - The task checks which event was received and blinks the LED accordingly.
 * - Try to spam the serial input with 's', 'q', and 'b' to see the LED blink and the messages being
 * received.
 */

// Binary Semaphore
SemBinaryStatic sem;

// Queue, type uint32_t, size 10
QueueStatic<uint32_t, 10> queue;

// No-split Ring Buffer with a size of 32 bytes
// - The max item size is ((bufferSize / 2) - headerSize), headerSize being 8 bytes
// - In this case, the max item size is (32 / 2) - 8 = 16 bytes
// - Remember that internally the buffer will be aligned to the next 4 bytes multiple, so if you use
//   a size of 30, it will be aligned to 32 bytes
RingBufferNoSplitStatic<char, 32> buffer;

// Queue set to hold events from the semaphore, queue and buffer
// - The size of the queue set must be the maximum number of events that can be stored
// - In this case, the maximum number of events is 1:
//   - 1 for the semaphore
//   - 10 for the queue
//   - 3 for the buffer ( (8+4)*3 = 36 bytes ), 3 items of 4 bytes with a header of 8 bytes each. In
//     theory it could hold 2 items, but 3 is selected for safety
// - Note that an assert will be triggered if the queue set length is less than the sum of the
// member events
// - Assert message: (pxQueueSetContainer->uxMessagesWaiting < pxQueueSetContainer->uxLength)
QueueSet queue_set(1 + 10 + 3);

// Tasks to test the locks
void taskFn(void* params);
TaskStatic<4 * 1024> task("Task", taskFn, 1);

// Blink the internal LED one time
void blinkLED();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  pinMode(LED_BUILTIN, OUTPUT);

  // Check if the objects were created
  if (!sem || !queue || !buffer) {
    Serial.println("One or more objects not created");
    while (true)
      ;
  }

  // Create the task
  if (!task.create()) {
    Serial.println("Task not created");
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

      // Give the semaphore
      case 's':
        Serial.println("Serial: Giving semaphore...");
        sem.give();
        break;

      // Send a message to the queue
      case 'q':
      {
        const uint32_t message = 1234;
        Serial.printf("Serial: Sending message to queue: %u\n", message);

        if (!queue.add(message, pdMS_TO_TICKS(100))) {
          Serial.println("Serial: Failed to send message to queue");
        }
      } break;

      // Send a message to the buffer
      case 'b':
      {
        const char* message = "1234";
        const uint8_t len   = strlen(message);

        Serial.printf("Serial: Sending message: %s\n", message);

        // Send the message to the buffer, including the null terminator, with a timeout of 100ms
        if (!buffer.send(message, len + 1, pdMS_TO_TICKS(100))) {
          Serial.println("Serial: Failed to send message to buffer");
        }
      } break;
    }
  }
}

void taskFn(void* params) {
  // Subscribe the semaphore, queue and buffer to the queue set
  queue_set.add(sem);
  queue_set.add(queue);
  queue_set.add(buffer);

  uint32_t queue_msg;

  for (;;) {
    // Block indefinitely until an event is received
    auto event = queue_set.select();

    if (!event) {
      Serial.println("\tTask: Queue set or member not created");
      while (true)
        ;
    }

    // Check which event was received

    // Semaphore
    if (event == sem) {
      Serial.println("\tTask: Received semaphore event");
      sem.take();
      blinkLED();
    }

    // Queue
    else if (event == queue) {
      queue.pop(queue_msg);
      Serial.printf("\tTask: Received message from queue: %u\n", queue_msg);
      blinkLED();
    }

    // Ring buffer
    else if (event == buffer) {
      size_t len;
      char* msg = buffer.receive(len);

      Serial.printf("\tTask: Received message from buffer: %s\n", msg);
      blinkLED();

      // Return the item once it's no longer needed
      buffer.returnItem(msg);
    }
  }
}

void blinkLED() {
  Serial.printf("\t\tBlinking LED...\n");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}