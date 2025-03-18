/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/** Explanation of the example:
 * - Two tasks are created with different stack sizes, one with dynamic memory allocation and the
 *   other with static memory allocation, having a total of 3 tasks (including the loop task).
 * - The tasks are created and started in the setup function using pointers. They have the same
 *   priority and a custom struct as a parameter.
 * - The loop function reads the serial input and performs the following actions:
 *    - Restart the ESP32 when '.' is received.
 *    - Notify task 1 when '1' is received.
 *    - Notify task 2 when '2' is received.
 *    - Notify both tasks when '3' is received.
 *    - Suspend all tasks when 's' is received.
 *    - Resume all tasks when 'r' is received.
 *    - The LED blinks every 100 ms.
 * - Task 1 waits for a notification for 1 second and increments the 'a' parameter.
 * - Task 2 waits for a notification for 5 seconds and increments the 'b' parameter.
 */

#include "RTOScppTask.h"
using namespace RTOS::Tasks;

// Task functions
void task1Fn(void* params);
void task2Fn(void* params);

// Custom task parameters
struct TaskParams {
  uint8_t a;
  uint8_t b;
} task_params{};

// Task objects
TaskDynamic<4 * 1024> task1("Task 1", task1Fn, 1, &task_params);
TaskStatic<8 * 1024> task2("Task 2", task2Fn, 1, &task_params);

// Task pointers
ITask* tasks[]              = {&task1, &task2};
constexpr uint8_t tasks_num = sizeof(tasks) / sizeof(tasks[0]);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting tasks...");

  pinMode(LED_BUILTIN, OUTPUT);

  // Create tasks
  for (uint8_t i = 0; i < tasks_num; i++) {
    if (!tasks[i]->create()) {
      Serial.printf("Task %u not created!\n", i + 1);
    }
  }
}

void loop() {
  // Read serial input
  if (Serial.available()) {
    char c = Serial.read();

    switch (c) {
      // Restart ESP32
      case '.': ESP.restart(); break;

      // Notify tasks
      case '1': task1.notify(0, eNotifyAction::eNoAction); break;
      case '2': task2.notify(0, eNotifyAction::eNoAction); break;

      // Notify both tasks
      case '3':
        task1.notify(0, eNotifyAction::eNoAction);
        task2.notify(0, eNotifyAction::eNoAction);
        break;

      // Suspend and resume tasks
      case 's':
      {
        for (uint8_t i = 0; i < tasks_num; i++) {
          tasks[i]->suspend();
        }
        Serial.println("-> Tasks suspended!");
      } break;

      case 'r':
      {
        for (uint8_t i = 0; i < tasks_num; i++) {
          tasks[i]->resume();
        }
        Serial.println("-> Tasks resumed!");
      } break;
    }
  }

  // Blink LED
  static uint32_t last_blink = 0;

  if (millis() - last_blink > 100) {
    last_blink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void task1Fn(void* params) {
  TaskParams* task_params = static_cast<TaskParams*>(params);
  uint32_t notif;

  while (true) {
    // Wait a notification for 1 second
    if (task1.notifyWait(0, 0, notif, pdMS_TO_TICKS(1000))) {
      Serial.println("Task 1 notified!");
    }

    // Print task parameters and increment a
    Serial.printf("Task 1 (a: %u, b: %u)\n", task_params->a, task_params->b);
    task_params->a++;
  }
}

void task2Fn(void* params) {
  TaskParams* task_params = static_cast<TaskParams*>(params);
  uint32_t notif;

  while (true) {
    // Wait a notification for 5 seconds
    if (task2.notifyWait(0, 0, notif, pdMS_TO_TICKS(5000))) {
      Serial.println("\tTask 2 notified!");
    }

    // Print task parameters and increment b
    Serial.printf("\tTask 2 (a: %u, b: %u)\n", task_params->a, task_params->b);
    task_params->b++;
  }
}