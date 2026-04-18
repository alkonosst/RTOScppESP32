/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Arduino.h>

#include <freertos/timers.h>

namespace RTOS::Timers {

// Interface for Timer objects, useful when using pointers
class ITimer {
  protected:
  ITimer() = default;

  public:
  ITimer(const ITimer&)            = delete;
  ITimer& operator=(const ITimer&) = delete;
  ITimer(ITimer&&)                 = delete;
  ITimer& operator=(ITimer&&)      = delete;
  virtual ~ITimer()                = default;

  /**
   * @brief Get the low-level handle of the timer. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return TimerHandle_t Timer handle, nullptr if the timer is not created.
   */
  virtual TimerHandle_t getHandle() const = 0;

  /**
   * @brief Get the name of the timer. Useful for debugging and logging purposes.
   * @return const char* Name of the timer, "RtosTimer" if the timer has no name.
   */
  virtual const char* getName() const = 0;

  /**
   * @brief Create the timer with the specified parameters.
   * @param name Timer name. If nullptr, the timer will be named "RtosTimer".
   * @param callback Timer callback function
   * @param period Timer period in ticks (can't be 0)
   * @param id Timer ID (nullptr if not used)
   * @param auto_reload Timer auto reload mode
   * @param start Start the timer after creation
   * @return true Timer created.
   */
  virtual bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period,
    void* id, const bool auto_reload, const bool start) = 0;

  /**
   * @brief Check if the timer is created.
   * @return true Timer is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Start the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer started successfully, false if the timer is not created or failed to start.
   */
  virtual bool start(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Start the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer started successfully, false if the timer is not created or failed to start.
   */
  virtual bool startFromISR(BaseType_t& task_woken) const = 0;

  /**
   * @brief Stop the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer stopped successfully, false if the timer is not created or failed to stop.
   */
  virtual bool stop(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Stop the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer stopped successfully, false if the timer is not created or failed to stop.
   */
  virtual bool stopFromISR(BaseType_t& task_woken) const = 0;

  /**
   * @brief Check if the timer is active.
   * @return true Timer is active.
   */
  virtual bool isActive() const = 0;

  /**
   * @brief Reset the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer reset successfully, false if the timer is not created or failed to reset.
   */
  virtual bool reset(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Reset the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer reset successfully, false if the timer is not created or failed to reset.
   */
  virtual bool resetFromISR(BaseType_t& task_woken) const = 0;

  /**
   * @brief Get the time left for the timer to expire.
   * @return TickType_t Time left for the timer to expire, 0 if the timer is not created.
   */
  virtual TickType_t getExpiryTime() const = 0;

  /**
   * @brief Set the timer period.
   * @param period Timer period in ticks (can't be 0)
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer period set successfully, false if the timer is not created or failed to set
   * the period.
   */
  virtual bool setPeriod(const TickType_t period,
    const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Set the timer period from an ISR.
   * @param period Timer period in ticks (can't be 0)
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer period set successfully, false if the timer is not created or failed to set
   * the period.
   */
  virtual bool setPeriodFromISR(const TickType_t period, BaseType_t& task_woken) const = 0;

  /**
   * @brief Get the timer period.
   * @return TickType_t Timer period in ticks, 0 if the timer is not created.
   */
  virtual TickType_t getPeriod() const = 0;

  /**
   * @brief Set the timer ID.
   * @param id Timer ID.
   * @return true Timer ID set successfully, false if the timer is not created.
   */
  virtual bool setTimerID(void* id) const = 0;

  /**
   * @brief Get the timer ID.
   * @return void* Timer ID, nullptr if the timer is not created.
   */
  virtual void* getTimerID() const = 0;

  /**
   * @brief Set the timer reload mode.
   * @param auto_reload Timer auto reload mode.
   * @return true Timer reload mode set successfully, false if the timer is not created.
   */
  virtual bool setReloadMode(const bool auto_reload) const = 0;

  /**
   * @brief Get the timer reload mode.
   * @return bool Timer auto reload mode, false if the timer is not created.
   */
  virtual bool getReloadMode() const = 0;

  /**
   * @brief Check if the timer is created.
   * @return true Timer is created.
   */
  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base class
template <typename Derived>
class Policy {
  public:
  Policy()
      : _handle(nullptr)
      , _name("RtosTimer") {}

  TimerHandle_t getHandle() const { return _handle; }
  const char* getName() const { return _name; }

  bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
    const bool auto_reload, const bool start) {
    if (_handle != nullptr) return true;
    if (callback == nullptr || period == 0) return false;
    if (name) _name = name; // Update name if provided

    return static_cast<Derived*>(this)->createImpl(callback, period, id, auto_reload, start);
  }

  protected:
  TimerHandle_t _handle;
  const char* _name;
};

// Policy for timer with dynamic memory allocation
class DynamicPolicy : public Policy<DynamicPolicy> {
  public:
  DynamicPolicy(const char* name = nullptr) {
    if (name) this->_name = name;
  }

  bool createImpl(TimerCallbackFunction_t callback, const TickType_t period, void* id,
    const bool auto_reload, const bool start) {
    this->_handle = xTimerCreate(this->_name, period, auto_reload, id, callback);

    if (this->_handle == nullptr) return false;

    if (start) xTimerStart(this->_handle, 0);

    return true;
  }
};

// Policy for timer with static memory allocation
class StaticPolicy : public Policy<StaticPolicy> {
  public:
  StaticPolicy(const char* name = nullptr) {
    if (name) this->_name = name;
  }

  bool createImpl(TimerCallbackFunction_t callback, const TickType_t period, void* id,
    const bool auto_reload, const bool start) {
    this->_handle =
      xTimerCreateStatic(this->_name, period, auto_reload, id, callback, &_timer_buffer);

    if (this->_handle == nullptr) return false;

    if (start) xTimerStart(this->_handle, 0);

    return true;
  }

  private:
  StaticTimer_t _timer_buffer;
};

// Main Timer class. You need to specify the policy used
template <typename Policy>
class Timer : public ITimer {
  public:
  /**
   * @brief Instantiate a empty Timer object. You need to call create(parameters) before using it.
   */
  Timer(const char* name = nullptr)
      : _policy(name) {}

  /**
   * @brief Instantiate a Timer object with the specified parameters.
   * @param name Timer name. If nullptr, the timer will be named "RtosTimer".
   * @param callback Timer callback function
   * @param period Timer period in ticks (can't be 0)
   * @param id Timer ID (nullptr if not used)
   * @param auto_reload Timer auto reload mode
   * @param start Start the timer after creation
   */
  Timer(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
    const bool auto_reload, const bool start) {
    _policy.create(name, callback, period, id, auto_reload, start);
  }

  ~Timer() {
    if (getHandle()) xTimerDelete(getHandle(), portMAX_DELAY);
  }

  /**
   * @brief Get the low-level handle of the timer. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return TimerHandle_t Timer handle, nullptr if the timer is not created.
   */
  TimerHandle_t getHandle() const override { return _policy.getHandle(); }

  /**
   * @brief Get the name of the timer. Useful for debugging and logging purposes.
   * @return const char* Name of the timer, "RtosTimer" if the timer has no name.
   */
  const char* getName() const override { return _policy.getName(); }

  /**
   * @brief Create the timer with the specified parameters.
   * @param name Timer name
   * @param callback Timer callback function
   * @param period Timer period in ticks (can't be 0)
   * @param id Timer ID (nullptr if not used)
   * @param auto_reload Timer auto reload mode
   * @param start Start the timer after creation
   * @return true Timer created.
   */
  bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
    const bool auto_reload, const bool start) override {
    return _policy.create(name, callback, period, id, auto_reload, start);
  }

  /**
   * @brief Check if the timer is created.
   * @return true Timer is created.
   */
  bool isCreated() const override { return getHandle() != nullptr; }

  /**
   * @brief Start the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer started successfully, false if the timer is not created or failed to start.
   */
  bool start(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return false;
    return xTimerStart(getHandle(), ticks_to_wait);
  }

  /**
   * @brief Start the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer started successfully, false if the timer is not created or failed to start.
   */
  bool startFromISR(BaseType_t& task_woken) const override {
    if (!isCreated()) return false;
    return xTimerStartFromISR(getHandle(), &task_woken);
  }

  /**
   * @brief Stop the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer stopped successfully, false if the timer is not created or failed to stop.
   */
  bool stop(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return false;
    return xTimerStop(getHandle(), ticks_to_wait);
  }

  /**
   * @brief Stop the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer stopped successfully, false if the timer is not created or failed to stop.
   */
  bool stopFromISR(BaseType_t& task_woken) const override {
    if (!isCreated()) return false;
    return xTimerStopFromISR(getHandle(), &task_woken);
  }

  /**
   * @brief Check if the timer is active.
   * @return true Timer is active.
   */
  bool isActive() const override {
    if (!isCreated()) return false;
    return xTimerIsTimerActive(getHandle());
  }

  /**
   * @brief Reset the timer.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer reset successfully, false if the timer is not created or failed to reset.
   */
  bool reset(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return false;
    return xTimerReset(getHandle(), ticks_to_wait);
  }

  /**
   * @brief Reset the timer from an ISR.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer reset successfully, false if the timer is not created or failed to reset.
   */
  bool resetFromISR(BaseType_t& task_woken) const override {
    if (!isCreated()) return false;
    return xTimerResetFromISR(getHandle(), &task_woken);
  }

  /**
   * @brief Get the time left for the timer to expire.
   * @return TickType_t Time left for the timer to expire, 0 if the timer is not created.
   */
  TickType_t getExpiryTime() const override {
    if (!isCreated()) return 0;
    return xTimerGetExpiryTime(getHandle()) - xTaskGetTickCount();
  }

  /**
   * @brief Set the timer period.
   * @param period Timer period in ticks (can't be 0)
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Timer period set successfully, false if the timer is not created or failed to set
   * the period.
   */
  bool setPeriod(const TickType_t period,
    const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return false;
    return xTimerChangePeriod(getHandle(), period, ticks_to_wait);
  }

  /**
   * @brief Set the timer period from an ISR.
   * @param period Timer period in ticks (can't be 0)
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Timer period set successfully, false if the timer is not created or failed to set
   * the period.
   */
  bool setPeriodFromISR(const TickType_t period, BaseType_t& task_woken) const override {
    if (!isCreated()) return false;
    return xTimerChangePeriodFromISR(getHandle(), period, &task_woken);
  }

  /**
   * @brief Get the timer period.
   * @return TickType_t Timer period in ticks, 0 if the timer is not created.
   */
  TickType_t getPeriod() const override {
    if (!isCreated()) return 0;
    return xTimerGetPeriod(getHandle());
  }

  /**
   * @brief Set the timer ID.
   * @param id Timer ID.
   * @return true Timer ID set successfully, false if the timer is not created.
   */
  bool setTimerID(void* id) const override {
    if (!isCreated()) return false;
    vTimerSetTimerID(getHandle(), id);
    return true;
  }

  /**
   * @brief Get the timer ID.
   * @return void* Timer ID, nullptr if the timer is not created.
   */
  void* getTimerID() const override {
    if (!isCreated()) return nullptr;
    return pvTimerGetTimerID(getHandle());
  }

  /**
   * @brief Set the timer reload mode.
   * @param auto_reload Timer auto reload mode.
   * @return true Timer reload mode set successfully, false if the timer is not created.
   */
  bool setReloadMode(const bool auto_reload) const override {
    if (!isCreated()) return false;
    vTimerSetReloadMode(getHandle(), auto_reload);
    return true;
  }

  /**
   * @brief Get the timer reload mode.
   * @return bool Timer auto reload mode, false if the timer is not created.
   */
  bool getReloadMode() const override {
    if (!isCreated()) return false;
    return uxTimerGetReloadMode(getHandle());
  }

  /**
   * @brief Check if the timer is created.
   * @return true Timer is created.
   */
  explicit operator bool() const override { return isCreated(); }

  private:
  Policy _policy;
};

} // namespace Internal

using TimerDynamic = Internal::Timer<Internal::DynamicPolicy>;
using TimerStatic  = Internal::Timer<Internal::StaticPolicy>;

} // namespace RTOS::Timers