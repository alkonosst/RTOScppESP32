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
  TaskBase(const TaskBase&)       = delete; // Delete copy constructor
  void operator=(const TaskBase&) = delete; // Delete copy assignment operator

  protected:
  TaskBase(const char* name, TaskFunction_t function, uint8_t priority)
      : _name(name)
      , _function(function)
      , _priority(priority)
      , _handle(nullptr) {}

  virtual ~TaskBase() {
    if (_handle) {
      vTaskDelete(_handle);
    }
  }

  const char* _name;
  TaskFunction_t _function;
  uint8_t _priority;
  TaskHandle_t _handle;

  public:
  virtual bool init() = 0;

  void suspend() { vTaskSuspend(_handle); }
  void resume() { vTaskResume(_handle); }
  bool abortDelay() { return xTaskAbortDelay(_handle); }

  void setPriority(uint8_t priority) {
    _priority = priority;
    vTaskPrioritySet(_handle, _priority);
  }

  uint8_t getPriority() { return uxTaskPriorityGet(_handle); }
  uint8_t getPriorityFromISR() { return uxTaskPriorityGetFromISR(_handle); }
  const char* getName() { return _name; }
  TaskHandle_t getHandle() { return _handle; }

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

class TaskDynamic : public TaskBase {
  public:
  TaskDynamic(const char* name, TaskFunction_t function, uint8_t priority, uint32_t stack_size)
      : TaskBase(name, function, priority)
      , _stack_size(stack_size) {}

  bool init() { return xTaskCreate(_function, _name, _stack_size, nullptr, _priority, &_handle); }

  private:
  uint32_t _stack_size;
};

class TaskDynamicPinnedToCore : public TaskBase {
  public:
  TaskDynamicPinnedToCore(const char* name, TaskFunction_t function, uint8_t priority,
                          uint32_t stack_size, BaseType_t running_core)
      : TaskBase(name, function, priority)
      , _stack_size(stack_size)
      , _running_core(running_core) {}

  bool init() {
    return xTaskCreatePinnedToCore(
      _function, _name, _stack_size, nullptr, _priority, &_handle, _running_core);
  }

  private:
  uint32_t _stack_size;
  BaseType_t _running_core;
};

template <uint32_t STACK_SIZE>
class TaskStatic : public TaskBase {
  public:
  TaskStatic(const char* name, TaskFunction_t function, uint8_t priority)
      : TaskBase(name, function, priority) {}

  bool init() {
    _handle = xTaskCreateStatic(_function, _name, STACK_SIZE, nullptr, _priority, _stack, &_tcb);
    return _handle != nullptr ? true : false;
  }

  private:
  StaticTask_t _tcb;
  StackType_t _stack[STACK_SIZE];
};

template <uint32_t STACK_SIZE>
class TaskStaticPinnedToCore : public TaskBase {
  public:
  TaskStaticPinnedToCore(const char* name, TaskFunction_t function, uint8_t priority,
                         BaseType_t running_core)
      : TaskBase(name, function, priority)
      , _running_core(running_core) {}

  bool init() {
    _handle = xTaskCreateStaticPinnedToCore(
      _function, _name, STACK_SIZE, nullptr, _priority, _stack, &_tcb, _running_core);

    return _handle != nullptr ? true : false;
  }

  private:
  BaseType_t _running_core;
  StaticTask_t _tcb;
  StackType_t _stack[STACK_SIZE];
};

#endif // RTOS_CPP_TASK_H