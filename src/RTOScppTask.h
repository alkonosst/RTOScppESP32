/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/task.h>

class TaskInterface {
  protected:
  TaskInterface(const char* name, const TaskFunction_t function, const uint8_t priority,
                const uint32_t stack_size, const BaseType_t running_core)
      : _name(name)
      , _function(function)
      , _priority(priority)
      , _stack_size(stack_size)
      , _running_core(running_core)
      , _handle(nullptr)
      , _stack_used(0)
      , _stack_min(0xffffffff)
      , _stack_max(0) {}

  const char* _name;
  const TaskFunction_t _function;
  uint8_t _priority;
  const uint32_t _stack_size;
  const BaseType_t _running_core;
  TaskHandle_t _handle;
  uint32_t _stack_used;
  uint32_t _stack_min;
  uint32_t _stack_max;

  public:
  virtual ~TaskInterface() {
    if (_handle) vTaskDelete(_handle);
  }

  TaskInterface(const TaskInterface&)                = delete;
  TaskInterface& operator=(const TaskInterface&)     = delete;
  TaskInterface(TaskInterface&&) noexcept            = delete;
  TaskInterface& operator=(TaskInterface&&) noexcept = delete;

  virtual bool init() = 0;

  void suspend() const { vTaskSuspend(_handle); }
  void resume() const { vTaskResume(_handle); }
  bool abortDelay() const { return xTaskAbortDelay(_handle); }

  TaskHandle_t getHandle() const { return _handle; }
  const char* getName() const { return _name; }

  void setPriority(const uint8_t priority) {
    _priority = priority;
    vTaskPrioritySet(_handle, _priority);
  }

  uint8_t getPriority() const { return uxTaskPriorityGet(_handle); }
  uint8_t getPriorityFromISR() const { return uxTaskPriorityGetFromISR(_handle); }

  uint32_t getStackSize() const { return _stack_size; }

  bool notify(const uint32_t value, const eNotifyAction action) {
    return xTaskNotify(_handle, value, action);
  }

  bool notifyFromISR(const uint32_t value, const eNotifyAction action,
                     BaseType_t& task_woken) const {
    return xTaskNotifyFromISR(_handle, value, action, &task_woken);
  }

  bool notifyAndQuery(const uint32_t value, const eNotifyAction action, uint32_t& old_value) const {
    return xTaskNotifyAndQuery(_handle, value, action, &old_value);
  }

  bool notifyAndQueryFromISR(const uint32_t value, const eNotifyAction action, uint32_t& old_value,
                             BaseType_t& task_woken) const {
    return xTaskNotifyAndQueryFromISR(_handle, value, action, &old_value, &task_woken);
  }

  bool notifyWait(const uint32_t clear_on_entry, const uint32_t clear_on_exit,
                  uint32_t* const value, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xTaskNotifyWait(clear_on_entry, clear_on_exit, value, ticks_to_wait);
  }

  bool notifyGive() const { return xTaskNotifyGive(_handle); }

  void notifyGiveFromISR(BaseType_t& task_woken) const {
    vTaskNotifyGiveFromISR(_handle, &task_woken);
  }

  uint32_t notifyTake(const bool clear, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return ulTaskNotifyTake(clear, ticks_to_wait);
  }

  void updateStackStats() {
    _stack_used = _stack_size - uxTaskGetStackHighWaterMark(_handle);
    _stack_min  = min(_stack_min, _stack_used);
    _stack_max  = max(_stack_max, _stack_used);
  }

  uint32_t getStackUsed() const { return _stack_used; }
  uint32_t getStackMinUsed() const { return _stack_min; }
  uint32_t getStackMaxUsed() const { return _stack_max; }

  explicit operator bool() const { return _handle != nullptr; }
};

class TaskDynamic : public TaskInterface {
  public:
  TaskDynamic(const char* name, TaskFunction_t function, uint8_t priority, uint32_t stack_size,
              BaseType_t running_core = ARDUINO_RUNNING_CORE)
      : TaskInterface(name, function, priority, stack_size, running_core) {}

  bool init() {
    return xTaskCreatePinnedToCore(
             _function, _name, _stack_size, nullptr, _priority, &_handle, _running_core) == pdPASS;
  }
};

template <uint32_t STACK_SIZE>
class TaskStatic : public TaskInterface {
  public:
  TaskStatic(const char* name, TaskFunction_t function, uint8_t priority,
             BaseType_t running_core = ARDUINO_RUNNING_CORE)
      : TaskInterface(name, function, priority, STACK_SIZE, running_core) {}

  bool init() {
    _handle = xTaskCreateStaticPinnedToCore(
      _function, _name, _stack_size, nullptr, _priority, _stack, &_tcb, _running_core);

    return _handle != nullptr;
  }

  private:
  StaticTask_t _tcb;
  StackType_t _stack[STACK_SIZE];
};