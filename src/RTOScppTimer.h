/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

  virtual ~ITimer() = default;

  virtual TimerHandle_t getHandle() const = 0;

  virtual bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period,
                      void* id, const bool auto_reload, const bool start) = 0;

  virtual bool start(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;
  virtual bool startFromISR(BaseType_t& task_woken) const                  = 0;
  virtual bool stop(const TickType_t ticks_to_wait = portMAX_DELAY) const  = 0;
  virtual bool stopFromISR(BaseType_t& task_woken) const                   = 0;

  virtual bool isActive() const = 0;

  virtual bool reset(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;
  virtual bool resetFromISR(BaseType_t& task_woken) const                  = 0;

  virtual const char* getName() const      = 0;
  virtual TickType_t getExpiryTime() const = 0;

  virtual bool setPeriod(const TickType_t period,
                         const TickType_t ticks_to_wait = portMAX_DELAY) const         = 0;
  virtual bool setPeriodFromISR(const TickType_t period, BaseType_t& task_woken) const = 0;
  virtual TickType_t getPeriod() const                                                 = 0;

  virtual void setTimerID(void* id) const = 0;
  virtual void* getTimerID() const        = 0;

  virtual void setReloadMode(const bool auto_reload) const = 0;
  virtual bool getReloadMode() const                       = 0;

  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base class
template <typename Derived>
class Policy {
  public:
  TimerHandle_t getHandle() const { return _handle; }

  bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
              const bool auto_reload, const bool start) {
    return static_cast<Derived*>(this)->createImpl(name, callback, period, id, auto_reload, start);
  }

  protected:
  TimerHandle_t _handle;
};

// Policy for timer with dynamic memory allocation
class DynamicPolicy : public Policy<DynamicPolicy> {
  public:
  bool createImpl(const char* name, TimerCallbackFunction_t callback, const TickType_t period,
                  void* id, const bool auto_reload, const bool start) {
    this->_handle = xTimerCreate(name, period, auto_reload, id, callback);
    if (this->_handle == nullptr) return false;
    if (start) xTimerStart(this->_handle, 0);
    return true;
  }
};

// Policy for timer with static memory allocation
class StaticPolicy : public Policy<StaticPolicy> {
  public:
  bool createImpl(const char* name, TimerCallbackFunction_t callback, const TickType_t period,
                  void* id, const bool auto_reload, const bool start) {
    this->_handle = xTimerCreateStatic(name, period, auto_reload, id, callback, &_tcb);
    if (this->_handle == nullptr) return false;
    if (start) xTimerStart(this->_handle, 0);
    return true;
  }

  private:
  StaticTimer_t _tcb;
};

// Main Timer class. You need to specify the policy used
template <typename Policy>
class Timer : public ITimer {
  public:
  Timer() = default;

  Timer(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
        const bool auto_reload, const bool start) {
    _policy.createImpl(name, callback, period, id, auto_reload, start);
  }

  ~Timer() {
    if (getHandle()) xTimerDelete(_policy.getHandle(), portMAX_DELAY);
  }

  TimerHandle_t getHandle() const override { return _policy.getHandle(); }

  bool create(const char* name, TimerCallbackFunction_t callback, const TickType_t period, void* id,
              const bool auto_reload, const bool start) override {
    return _policy.createImpl(name, callback, period, id, auto_reload, start);
  }

  bool start(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xTimerStart(getHandle(), ticks_to_wait);
  }

  bool startFromISR(BaseType_t& task_woken) const override {
    return xTimerStartFromISR(getHandle(), &task_woken);
  }

  bool stop(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xTimerStop(getHandle(), ticks_to_wait);
  }

  bool stopFromISR(BaseType_t& task_woken) const override {
    return xTimerStopFromISR(getHandle(), &task_woken);
  }

  bool isActive() const override { return xTimerIsTimerActive(getHandle()); }

  bool reset(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xTimerReset(getHandle(), ticks_to_wait);
  }

  bool resetFromISR(BaseType_t& task_woken) const override {
    return xTimerResetFromISR(getHandle(), &task_woken);
  }

  const char* getName() const override { return pcTimerGetName(getHandle()); }

  TickType_t getExpiryTime() const override {
    return xTimerGetExpiryTime(getHandle()) - xTaskGetTickCount();
  }

  bool setPeriod(const TickType_t period,
                 const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xTimerChangePeriod(getHandle(), period, ticks_to_wait);
  }

  bool setPeriodFromISR(const TickType_t period, BaseType_t& task_woken) const override {
    return xTimerChangePeriodFromISR(getHandle(), period, &task_woken);
  }

  TickType_t getPeriod() const override { return xTimerGetPeriod(getHandle()); }

  void setTimerID(void* id) const override { vTimerSetTimerID(getHandle(), id); }
  void* getTimerID() const override { return pvTimerGetTimerID(getHandle()); }

  void setReloadMode(const bool auto_reload) const override {
    vTimerSetReloadMode(getHandle(), auto_reload);
  }
  bool getReloadMode() const override { return uxTimerGetReloadMode(getHandle()); }

  explicit operator bool() const override { return getHandle() != nullptr; }

  private:
  Policy _policy;
};

} // namespace Internal

using TimerDynamic = Internal::Timer<Internal::DynamicPolicy>;
using TimerStatic  = Internal::Timer<Internal::StaticPolicy>;

} // namespace RTOS::Timers