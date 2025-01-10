/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/task.h>

namespace RTOS::Task {

// Interface for Task objects, useful when using pointers
class ITask {
  protected:
  ITask() = default;

  public:
  ITask(const ITask&)            = delete;
  ITask& operator=(const ITask&) = delete;
  ITask(ITask&&)                 = delete;
  ITask& operator=(ITask&&)      = delete;

  virtual ~ITask() = default;

  virtual bool create()                                            = 0;
  virtual bool create(const char* name, TaskFunction_t function, uint8_t priority, void* parameters,
                      uint8_t running_core = ARDUINO_RUNNING_CORE) = 0;
  virtual bool isCreated() const                                   = 0;
  virtual void suspend() const                                     = 0;
  virtual void resume() const                                      = 0;
  virtual bool abortDelay() const                                  = 0;

  virtual TaskHandle_t getHandle() const = 0;
  virtual const char* getName() const    = 0;

  virtual void setPriority(const uint8_t priority) = 0;
  virtual uint8_t getPriority() const              = 0;
  virtual uint8_t getPriorityFromISR() const       = 0;

  virtual uint32_t getStackSize() const = 0;

  virtual bool notify(const uint32_t value, const eNotifyAction action)                 = 0;
  virtual bool notifyFromISR(const uint32_t value, const eNotifyAction action,
                             BaseType_t& task_woken) const                              = 0;
  virtual bool notifyAndQuery(const uint32_t value, const eNotifyAction action,
                              uint32_t& old_value) const                                = 0;
  virtual bool notifyAndQueryFromISR(const uint32_t value, const eNotifyAction action,
                                     uint32_t& old_value, BaseType_t& task_woken) const = 0;

  virtual bool notifyGive() const                                                     = 0;
  virtual void notifyGiveFromISR(BaseType_t& task_woken) const                        = 0;
  virtual uint32_t notifyTake(const bool clear, const TickType_t ticks_to_wait) const = 0;

  virtual bool notifyWait(const uint32_t clear_on_entry, const uint32_t clear_on_exit,
                          uint32_t* const value, const TickType_t ticks_to_wait) const = 0;

  virtual void updateStackStats()          = 0;
  virtual uint32_t getStackUsed() const    = 0;
  virtual uint32_t getStackMinUsed() const = 0;
  virtual uint32_t getStackMaxUsed() const = 0;

  virtual explicit operator bool() const = 0;
};

namespace Internal {
// Policy base class (CRTP)
template <typename Derived>
struct Policy {
  TaskHandle_t _handle;
  const char* _name;
  TaskFunction_t _function;
  uint8_t _priority;
  void* _parameters;
  BaseType_t _core;

  bool create() { return static_cast<Derived*>(this)->createImpl(); }

  bool create(const char* name, TaskFunction_t function, void* parameters, uint8_t priority,
              BaseType_t core) {
    _name       = name;
    _function   = function;
    _parameters = parameters;
    _priority   = priority;
    _core       = core;

    return static_cast<Derived*>(this)->createImpl();
  }
};

// Policy for task with dynamic memory allocation
template <uint32_t StackSize>
struct DynamicPolicy : public Policy<DynamicPolicy<StackSize>> {
  static constexpr uint32_t _stack_size = StackSize;

  bool createImpl() {
    return xTaskCreatePinnedToCore(this->_function,
                                   this->_name,
                                   StackSize,
                                   this->_parameters,
                                   this->_priority,
                                   &this->_handle,
                                   this->_core);
  }
};

// Policy for task with static memory allocation
template <uint32_t StackSize>
struct StaticPolicy : public Policy<StaticPolicy<StackSize>> {
  static constexpr uint32_t _stack_size = StackSize;
  StackType_t _stack[StackSize];
  StaticTask_t _tcb;

  bool createImpl() {
    this->_handle = xTaskCreateStaticPinnedToCore(this->_function,
                                                  this->_name,
                                                  StackSize,
                                                  this->_parameters,
                                                  this->_priority,
                                                  _stack,
                                                  &_tcb,
                                                  this->_core);

    return this->_handle != nullptr;
  }
};

// Main task class. You need to specify the policy used
template <typename Policy>
class Task : public ITask {
  public:
  Task()
      : _policy()
      , _stack_used(0)
      , _stack_min(0xffffffff)
      , _stack_max(0) {}

  Task(const char* name, TaskFunction_t function, uint8_t priority, void* parameters,
       uint8_t running_core = ARDUINO_RUNNING_CORE) {
    _policy.create(name, function, parameters, priority, running_core);
  }

  ~Task() {
    if (_policy._handle) vTaskDelete(_policy._handle);
  }

  bool create() { return _policy.create(); }

  bool create(const char* name, TaskFunction_t function, uint8_t priority, void* parameters,
              uint8_t running_core = ARDUINO_RUNNING_CORE) {
    return _policy.create(name, function, parameters, priority, running_core);
  }

  bool isCreated() const { return _policy._handle != nullptr; }

  void suspend() const { vTaskSuspend(_policy._handle); }

  void resume() const { vTaskResume(_policy._handle); }

  bool abortDelay() const { return xTaskAbortDelay(_policy._handle); }

  TaskHandle_t getHandle() const { return _policy._handle; }

  const char* getName() const { return _policy._name; }

  void* getParameters() const { return _policy._parameters; }

  void setPriority(const uint8_t priority) {
    vTaskPrioritySet(_policy._handle, priority);
    _policy._priority = priority;
  }

  uint8_t getPriority() const { return uxTaskPriorityGet(_policy._handle); }

  uint8_t getPriorityFromISR() const { return uxTaskPriorityGetFromISR(_policy._handle); }

  uint32_t getStackSize() const { return Policy::_stack_size; }

  bool notify(const uint32_t value, const eNotifyAction action) {
    return xTaskNotify(_policy._handle, value, action);
  }

  bool notifyFromISR(const uint32_t value, const eNotifyAction action,
                     BaseType_t& task_woken) const {
    return xTaskNotifyFromISR(_policy._handle, value, action, &task_woken);
  }

  bool notifyAndQuery(const uint32_t value, const eNotifyAction action, uint32_t& old_value) const {
    return xTaskNotifyAndQuery(_policy._handle, value, action, &old_value);
  }

  bool notifyAndQueryFromISR(const uint32_t value, const eNotifyAction action, uint32_t& old_value,
                             BaseType_t& task_woken) const {
    return xTaskNotifyAndQueryFromISR(_policy._handle, value, action, &old_value, &task_woken);
  }

  bool notifyGive() const { return xTaskNotifyGive(_policy._handle); }

  void notifyGiveFromISR(BaseType_t& task_woken) const {
    return vTaskNotifyGiveFromISR(_policy._handle, &task_woken);
  }

  uint32_t notifyTake(const bool clear, const TickType_t ticks_to_wait) const {
    return ulTaskNotifyTake(clear, ticks_to_wait);
  }

  bool notifyWait(const uint32_t clear_on_entry, const uint32_t clear_on_exit,
                  uint32_t* const value, const TickType_t ticks_to_wait) const {
    return xTaskNotifyWait(clear_on_entry, clear_on_exit, value, ticks_to_wait);
  }

  void updateStackStats() {
    _stack_used = Policy::_stack_size - uxTaskGetStackHighWaterMark(_policy._handle);
    _stack_min  = min(_stack_min, _stack_used);
    _stack_max  = max(_stack_max, _stack_used);
  }

  uint32_t getStackUsed() const { return _stack_used; }

  uint32_t getStackMinUsed() const { return _stack_min; }

  uint32_t getStackMaxUsed() const { return _stack_max; }

  explicit operator bool() const { return _policy._handle != nullptr; }

  private:
  Policy _policy;
  uint32_t _stack_used;
  uint32_t _stack_min;
  uint32_t _stack_max;
};
}; // namespace Internal

template <uint32_t STACK_SIZE>
using TaskDynamic = Internal::Task<Internal::DynamicPolicy<STACK_SIZE>>;

template <uint32_t STACK_SIZE>
using TaskStatic = Internal::Task<Internal::StaticPolicy<STACK_SIZE>>;
} // namespace RTOS::Task