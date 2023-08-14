/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_TASK_H
#define RTOS_CPP_TASK_H

#include <Arduino.h>
#include <freertos/task.h>

class TaskBase {
  private:
  TaskBase(const TaskBase&)       = delete;
  void operator=(const TaskBase&) = delete;

  protected:
  TaskBase(const char* name, TaskFunction_t function, uint8_t priority)
      : _name(name)
      , _function(function)
      , _priority(priority)
      , _handle(nullptr) {}
  ~TaskBase() { vTaskDelete(_handle); }

  const char* _name;
  TaskFunction_t _function;
  uint8_t _priority;
  TaskHandle_t _handle;

  public:
  virtual bool init() = 0;

  bool notify(uint32_t value, eNotifyAction action) { return xTaskNotify(_handle, value, action); }

  bool notifyFromISR(uint32_t value, eNotifyAction action, BaseType_t& task_woken) {
    return xTaskNotifyFromISR(_handle, value, action, &task_woken);
  }

  bool notifyAndQuery(uint32_t value, eNotifyAction action, uint32_t& old_value) {
    return xTaskNotifyAndQuery(_handle, value, action, &old_value);
  }

  bool notifyAndQueryFromISR(uint32_t value, eNotifyAction action, uint32_t& old_value,
                             BaseType_t& task_woken) {
    return xTaskNotifyAndQueryFromISR(_handle, value, action, &old_value, &task_woken);
  }

  bool notifyWait(uint32_t clear_on_entry, uint32_t clear_on_exit, uint32_t* value,
                  TickType_t ticks_to_wait = portMAX_DELAY) {
    return xTaskNotifyWait(clear_on_entry, clear_on_exit, value, ticks_to_wait);
  }

  bool notifyGive() { return xTaskNotifyGive(_handle); }
  void notifyGiveFromISR(BaseType_t& task_woken) { vTaskNotifyGiveFromISR(_handle, &task_woken); }

  uint32_t notifyTake(bool clear, TickType_t ticks_to_wait = portMAX_DELAY) {
    return ulTaskNotifyTake(clear, ticks_to_wait);
  }
};

template <uint32_t STACK_SIZE>
class TaskStaticPinnedToCore : public TaskBase {
  public:
  TaskStaticPinnedToCore(const char* name, TaskFunction_t function, uint8_t priority,
                         BaseType_t running_core)
      : TaskBase(name, function, priority)
      , _running_core(running_core) {}

  bool init() {
    _handle = xTaskCreateStaticPinnedToCore(_function,      // Función
                                            _name,          // Nombre
                                            STACK_SIZE,     // Tamaño stack
                                            NULL,           // Parámetros
                                            _priority,      // Prioridad
                                            _stack,         // Buffer stack
                                            &_tcb,          // Buffer tarea
                                            _running_core); // Core

    return _handle != nullptr ? true : false;
  }

  private:
  BaseType_t _running_core;
  StaticTask_t _tcb;
  StackType_t _stack[STACK_SIZE];
};

#endif // RTOS_CPP_TASK_H