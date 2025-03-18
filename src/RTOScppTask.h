/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>

#include <freertos/task.h>

namespace RTOS::Tasks {

// Interface for Task objects, useful when using pointers
class ITask {
  protected:
  ITask() = default;

  public:
  ITask(const ITask&)            = delete;
  ITask& operator=(const ITask&) = delete;
  ITask(ITask&&)                 = delete;
  ITask& operator=(ITask&&)      = delete;
  virtual ~ITask()               = default;

  /**
   * @brief Get the low-level handle of the task. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return TaskHandle_t Task handle, nullptr if the task is not created.
   */
  virtual TaskHandle_t getHandle() const = 0;

  /**
   * @brief Create the task with the already set parameters.
   * @return true Task created.
   */
  virtual bool create() = 0;

  /**
   * @brief Create the task with the specified parameters.
   * @param name Task name
   * @param function Task function
   * @param priority Task priority (0 to configMAX_PRIORITIES - 1)
   * @param parameters Task parameters
   * @param running_core Core where the task will run (0 or 1)
   * @return true Task created.
   */
  virtual bool create(const char* name, TaskFunction_t function, uint8_t priority,
    void* parameters = nullptr, uint8_t running_core = ARDUINO_RUNNING_CORE) = 0;

  /**
   * @brief Check if the task is created.
   * @return true Task is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Suspend the task.
   * @return true Task suspended successfully, false if the task is not created.
   */
  virtual bool suspend() const = 0;

  /**
   * @brief Resume the task.
   * @return true Task resumed successfully, false if the task is not created.
   */
  virtual bool resume() const = 0;

  /**
   * @brief Get the state of the task.
   * @return eTaskState Task state, eTaskState::eInvalid if the task is not created.
   */
  virtual eTaskState getState() const = 0;

  /**
   * @brief Abort the delay of the task.
   * @return true Delay aborted successfully, false if the task is not created or the task is not in
   * the Blocked state.
   */
  virtual bool abortDelay() const = 0;

  /**
   * @brief Get the name of the task.
   * @return const char* Task name, nullptr if the task is not created.
   */
  virtual const char* getName() const = 0;

  /**
   * @brief Get the parameters of the task.
   * @return void* Task parameters, nullptr if the task is not created or no parameters were set.
   */
  virtual void* getParameters() const = 0;

  /**
   * @brief Get the core where the task is running.
   * @return uint8_t Core number, 0xFF if the task is not created.
   */
  virtual uint8_t getCore() const = 0;

  /**
   * @brief Set the priority of the task.
   * @param priority Task priority (0 to configMAX_PRIORITIES - 1)
   * @return true Priority set successfully, false if the task is not created.
   */
  virtual bool setPriority(const uint8_t priority) = 0;

  /**
   * @brief Get the priority of the task.
   * @return uint8_t Task priority, 0xFF if the task is not created.
   */
  virtual uint8_t getPriority() const = 0;

  /**
   * @brief Get the priority of the task from an ISR.
   * @return uint8_t Task priority, 0xFF if the task is not created.
   */
  virtual uint8_t getPriorityFromISR() const = 0;

  /**
   * @brief Get the stack size of the task.
   * @return uint32_t Stack size.
   */
  virtual uint32_t getStackSize() const = 0;

  /**
   * @brief Notify the task.
   * @param value Value to notify.
   * @param action Action to take.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  virtual bool notify(const uint32_t value, const eNotifyAction action) const = 0;

  /**
   * @brief Notify the task from an ISR.
   * @param value Value to notify.
   * @param action Action to take.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  virtual bool notifyFromISR(const uint32_t value, const eNotifyAction action,
    BaseType_t& task_woken) const = 0;

  /**
   * @brief Notify the task and query the old value.
   * @param value Value to notify.
   * @param action Action to take.
   * @param old_value Old value.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  virtual bool notifyAndQuery(const uint32_t value, const eNotifyAction action,
    uint32_t& old_value) const = 0;

  /**
   * @brief Notify the task and query the old value from an ISR.
   * @param value Value to notify.
   * @param action Action to take.
   * @param old_value Old value.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  virtual bool notifyAndQueryFromISR(const uint32_t value, const eNotifyAction action,
    uint32_t& old_value, BaseType_t& task_woken) const = 0;

  /**
   * @brief Notify the task. This function acts as a counting semaphore, it will increment the
   * notification value by 1. The task can wait for the notification using notifyTake().
   * @return true Notification sent successfully, false if the task is not created.
   */
  virtual bool notifyGive() const = 0;

  /**
   * @brief Notify the task from an ISR. This function acts as a counting semaphore, it will
   * increment the notification value by 1. The task can wait for the notification using
   * notifyTake().
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created.
   */
  virtual bool notifyGiveFromISR(BaseType_t& task_woken) const = 0;

  /**
   * @brief Wait for a notification. This function will block the task until a notification is
   * received or the timeout expires.
   * @param clear Clear the notification. If true, the notification value will be cleared, acting as
   * a binary semaphore. If false, the notification value will be decremented by 1.
   * @param ticks_to_wait Maximum time to wait for the notification.
   * @return uint32_t Value of the notification.
   */
  virtual uint32_t notifyTake(const bool clear,
    const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Wait for a notification. This function will block the task until a notification is
   * received or the timeout expires.
   * @param clear_on_entry Bit mask to clear the notification on entry.
   * @param clear_on_exit Bit mask to clear the notification on exit.
   * @param value Value of the notification.
   * @param ticks_to_wait Maximum time to wait for the notification.
   * @return true Notification received successfully, false if the task is not created or failed to
   * receive the notification.
   */
  virtual bool notifyWait(const uint32_t clear_on_entry, const uint32_t clear_on_exit,
    uint32_t& value, const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Update the stack statistics of the task. Call this function periodically to get the
   * stack usage.
   * @return true Stack statistics updated successfully, false if the task is not created.
   */
  virtual bool updateStackStats() = 0;

  /**
   * @brief Get the stack used by the task.
   * @return uint32_t Stack used.
   */
  virtual uint32_t getStackUsed() const = 0;

  /**
   * @brief Get the minimum stack used by the task.
   * @return uint32_t Minimum stack used.
   */
  virtual uint32_t getStackMinUsed() const = 0;

  /**
   * @brief Get the maximum stack used by the task.
   * @return uint32_t Maximum stack used.
   */
  virtual uint32_t getStackMaxUsed() const = 0;

  /**
   * @brief Check if the task is created.
   * @return true Task is created.
   */
  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base policy class
template <typename Derived>
class Policy {
  public:
  Policy()
      : _handle(nullptr)
      , _name(nullptr)
      , _function(nullptr)
      , _priority(0)
      , _parameters(nullptr)
      , _core(0) {}

  TaskHandle_t getHandle() const { return _handle; }
  const char* getName() const { return _name; }
  void* getParameters() const { return _parameters; }
  uint8_t getCore() const { return _core; }

  bool setup(const char* name, TaskFunction_t function, uint8_t priority,
    void* parameters = nullptr, BaseType_t core = ARDUINO_RUNNING_CORE) {
    _name       = name;
    _parameters = parameters;
    _function   = function;
    _priority   = priority;
    _core       = core;

    return isValid();
  }

  bool isValid() const {
    if (_name == nullptr || _function == nullptr || _priority >= configMAX_PRIORITIES ||
        !taskVALID_CORE_ID(_core))
      return false;

    return true;
  }

  bool create() {
    if (!isValid()) return false;

    if (isCreated()) return true;

    return static_cast<Derived*>(this)->createImpl();
  }

  bool isCreated() const { return _handle != nullptr; }

  protected:
  TaskHandle_t _handle;
  const char* _name;
  TaskFunction_t _function;
  uint8_t _priority;
  void* _parameters;
  uint8_t _core;
};

// Policy for task with dynamic memory allocation
template <uint32_t StackSize>
class DynamicPolicy : public Policy<DynamicPolicy<StackSize>> {
  public:
  static constexpr uint32_t _stack_size = StackSize;

  bool createImpl() {
    if (!this->isValid()) return false;

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
class StaticPolicy : public Policy<StaticPolicy<StackSize>> {
  public:
  static constexpr uint32_t _stack_size = StackSize;

  bool createImpl() {
    if (!this->isValid()) return false;

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

  private:
  StackType_t _stack[StackSize];
  StaticTask_t _tcb;
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

  Task(const char* name, TaskFunction_t function, uint8_t priority, void* parameters = nullptr,
    uint8_t running_core = ARDUINO_RUNNING_CORE, bool create = false)
      : _policy()
      , _stack_used(0)
      , _stack_min(0xffffffff)
      , _stack_max(0) {
    if (!_policy.setup(name, function, priority, parameters, running_core)) {
      return;
    }

    if (create) {
      _policy.create();
    }
  }

  ~Task() {
    if (getHandle()) {
      vTaskDelete(getHandle());
    }
  }

  /**
   * @brief Get the low-level handle of the task. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return TaskHandle_t Task handle, nullptr if the task is not created.
   */
  TaskHandle_t getHandle() const { return _policy.getHandle(); }

  /**
   * @brief Create the task with the already set parameters.
   * @return true Task created.
   */
  bool create() { return _policy.create(); }

  /**
   * @brief Create the task with the specified parameters.
   * @param name Task name
   * @param function Task function
   * @param priority Task priority (0 to configMAX_PRIORITIES - 1)
   * @param parameters Task parameters
   * @param running_core Core where the task will run (0 or 1)
   * @return true Task created.
   */
  bool create(const char* name, TaskFunction_t function, uint8_t priority,
    void* parameters = nullptr, uint8_t running_core = ARDUINO_RUNNING_CORE) {
    if (!_policy.setup(name, function, priority, parameters, running_core)) {
      return false;
    }

    return _policy.create();
  }

  /**
   * @brief Check if the task is created.
   * @return true Task is created.
   */
  bool isCreated() const { return _policy.isCreated(); }

  /**
   * @brief Suspend the task.
   * @return true Task suspended successfully, false if the task is not created.
   */
  bool suspend() const {
    if (!isCreated()) return false;
    vTaskSuspend(getHandle());
    return true;
  }

  /**
   * @brief Resume the task.
   * @return true Task resumed successfully, false if the task is not created.
   */
  bool resume() const {
    if (!isCreated()) return false;
    vTaskResume(getHandle());
    return true;
  }

  /**
   * @brief Get the state of the task.
   * @return eTaskState Task state, eTaskState::eInvalid if the task is not created.
   */
  eTaskState getState() const {
    if (!isCreated()) return eTaskState::eInvalid;
    return eTaskGetState(getHandle());
  }

  /**
   * @brief Abort the delay of the task.
   * @return true Delay aborted successfully, false if the task is not created or the task is not in
   * the Blocked state.
   */
  bool abortDelay() const {
    if (!isCreated()) return false;
    return xTaskAbortDelay(getHandle());
  }

  /**
   * @brief Get the name of the task.
   * @return const char* Task name, nullptr if the task is not created.
   */
  const char* getName() const {
    if (!isCreated()) return nullptr;
    return _policy.getName();
  }

  /**
   * @brief Get the parameters of the task.
   * @return void* Task parameters, nullptr if the task is not created or no parameters were set.
   */
  void* getParameters() const {
    if (!isCreated()) return nullptr;
    return _policy.getParameters();
  }

  /**
   * @brief Get the core where the task is running.
   * @return uint8_t Core number, 0xFF if the task is not created.
   */
  uint8_t getCore() const {
    if (!isCreated()) return 0xFF;
    return _policy.getCore();
  }

  /**
   * @brief Set the priority of the task.
   * @param priority Task priority (0 to configMAX_PRIORITIES - 1)
   * @return true Priority set successfully, false if the task is not created.
   */
  bool setPriority(const uint8_t priority) {
    if (!isCreated()) return false;
    vTaskPrioritySet(getHandle(), priority);
    return true;
  }

  /**
   * @brief Get the priority of the task.
   * @return uint8_t Task priority, 0xFF if the task is not created.
   */
  uint8_t getPriority() const {
    if (!isCreated()) return 0xFF;
    return uxTaskPriorityGet(getHandle());
  }

  /**
   * @brief Get the priority of the task from an ISR.
   * @return uint8_t Task priority, 0xFF if the task is not created.
   */
  uint8_t getPriorityFromISR() const {
    if (!isCreated()) return 0xFF;
    return uxTaskPriorityGetFromISR(getHandle());
  }

  /**
   * @brief Get the stack size of the task.
   * @return uint32_t Stack size.
   */
  uint32_t getStackSize() const { return Policy::_stack_size; }

  /**
   * @brief Notify the task.
   * @param value Value to notify.
   * @param action Action to take.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  bool notify(const uint32_t value, const eNotifyAction action) const {
    if (!isCreated()) return false;
    return xTaskNotify(getHandle(), value, action);
  }

  /**
   * @brief Notify the task from an ISR.
   * @param value Value to notify.
   * @param action Action to take.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  bool notifyFromISR(const uint32_t value, const eNotifyAction action,
    BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xTaskNotifyFromISR(getHandle(), value, action, &task_woken);
  }

  /**
   * @brief Notify the task and query the old value.
   * @param value Value to notify.
   * @param action Action to take.
   * @param old_value Old value.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  bool notifyAndQuery(const uint32_t value, const eNotifyAction action, uint32_t& old_value) const {
    if (!isCreated()) return false;
    return xTaskNotifyAndQuery(getHandle(), value, action, &old_value);
  }

  /**
   * @brief Notify the task and query the old value from an ISR.
   * @param value Value to notify.
   * @param action Action to take.
   * @param old_value Old value.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created or failed to send
   * the notification.
   */
  bool notifyAndQueryFromISR(const uint32_t value, const eNotifyAction action, uint32_t& old_value,
    BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xTaskNotifyAndQueryFromISR(getHandle(), value, action, &old_value, &task_woken);
  }

  /**
   * @brief Notify the task. This function acts as a counting semaphore, it will increment the
   * notification value by 1. The task can wait for the notification using notifyTake().
   * @return true Notification sent successfully, false if the task is not created.
   */
  bool notifyGive() const {
    if (!isCreated()) return false;
    return xTaskNotifyGive(getHandle());
  }

  /**
   * @brief Notify the task from an ISR. This function acts as a counting semaphore, it will
   * increment the notification value by 1. The task can wait for the notification using
   * notifyTake().
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Notification sent successfully, false if the task is not created.
   */
  bool notifyGiveFromISR(BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    vTaskNotifyGiveFromISR(getHandle(), &task_woken);
    return true;
  }

  /**
   * @brief Wait for a notification. This function will block the task until a notification is
   * received or the timeout expires.
   * @param clear Clear the notification. If true, the notification value will be cleared, acting as
   * a binary semaphore. If false, the notification value will be decremented by 1.
   * @param ticks_to_wait Maximum time to wait for the notification.
   * @return uint32_t Value of the notification.
   */
  uint32_t notifyTake(const bool clear, const TickType_t ticks_to_wait) const {
    if (!isCreated()) return 0;
    return ulTaskNotifyTake(clear, ticks_to_wait);
  }

  /**
   * @brief Wait for a notification. This function will block the task until a notification is
   * received or the timeout expires.
   * @param clear_on_entry Bit mask to clear the notification on entry.
   * @param clear_on_exit Bit mask to clear the notification on exit.
   * @param value Value of the notification.
   * @param ticks_to_wait Maximum time to wait for the notification.
   * @return true Notification received successfully, false if the task is not created or failed to
   * receive the notification.
   */
  bool notifyWait(const uint32_t clear_on_entry, const uint32_t clear_on_exit, uint32_t& value,
    const TickType_t ticks_to_wait) const {
    if (!isCreated()) return false;
    return xTaskNotifyWait(clear_on_entry, clear_on_exit, &value, ticks_to_wait);
  }

  /**
   * @brief Update the stack statistics of the task. Call this function periodically to get the
   * stack usage.
   * @return true Stack statistics updated successfully, false if the task is not created.
   */
  bool updateStackStats() {
    if (!isCreated()) return false;

    _stack_used = Policy::_stack_size - uxTaskGetStackHighWaterMark(getHandle());
    _stack_min  = min(_stack_min, _stack_used);
    _stack_max  = max(_stack_max, _stack_used);

    return true;
  }

  /**
   * @brief Get the stack used by the task.
   * @return uint32_t Stack used.
   */
  uint32_t getStackUsed() const {
    if (!isCreated()) return 0;
    return _stack_used;
  }

  /**
   * @brief Get the minimum stack used by the task.
   * @return uint32_t Minimum stack used.
   */
  uint32_t getStackMinUsed() const {
    if (!isCreated()) return 0;
    return _stack_min;
  }

  /**
   * @brief Get the maximum stack used by the task.
   * @return uint32_t Maximum stack used.
   */
  uint32_t getStackMaxUsed() const {
    if (!isCreated()) return 0;
    return _stack_max;
  }

  /**
   * @brief Check if the task is created.
   * @return true Task is created.
   */
  explicit operator bool() const { return isCreated(); }

  private:
  Policy _policy;
  uint32_t _stack_used;
  uint32_t _stack_min;
  uint32_t _stack_max;
};
}; // namespace Internal

template <uint32_t StackSize>
using TaskDynamic = Internal::Task<Internal::DynamicPolicy<StackSize>>;

template <uint32_t StackSize>
using TaskStatic = Internal::Task<Internal::StaticPolicy<StackSize>>;
} // namespace RTOS::Tasks