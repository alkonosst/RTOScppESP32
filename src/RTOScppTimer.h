/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/timers.h>

class TimerInterface {
  protected:
  TimerInterface(TimerHandle_t handle)
      : _handle(handle) {}

  TimerHandle_t _handle;
  const char* _name;
  TimerCallbackFunction_t _callback;
  TickType_t _period;
  void* _id;
  bool _auto_reload;
  bool _start;

  public:
  virtual ~TimerInterface() {
    if (_handle) vTaskDelete(_handle);
  }

  TimerInterface(const TimerInterface&)                = delete;
  TimerInterface& operator=(const TimerInterface&)     = delete;
  TimerInterface(TimerInterface&&) noexcept            = delete;
  TimerInterface& operator=(TimerInterface&&) noexcept = delete;

  void setup(const char* name, const TimerCallbackFunction_t callback, const TickType_t period,
             void* id, const bool auto_reload, const bool start) {
    _name        = name;
    _callback    = callback;
    _period      = period;
    _id          = id;
    _auto_reload = auto_reload;
    _start       = start;
  }

  virtual bool create() = 0;

  bool start(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xTimerStart(_handle, ticks_to_wait);
  }

  bool startFromISR(BaseType_t& task_woken) const {
    return xTimerStartFromISR(_handle, &task_woken);
  }
  bool stop(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xTimerStop(_handle, ticks_to_wait);
  }
  bool stopFromISR(BaseType_t& task_woken) const { return xTimerStopFromISR(_handle, &task_woken); }

  bool isActive() const { return xTimerIsTimerActive(_handle); }

  bool reset(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xTimerReset(_handle, ticks_to_wait);
  }

  bool resetFromISR(BaseType_t& task_woken) const {
    return xTimerResetFromISR(_handle, &task_woken);
  }

  const char* getName() const { return pcTimerGetName(_handle); }
  TickType_t getExpiryTime() const { return xTimerGetExpiryTime(_handle); }

  bool setPeriod(const TickType_t period, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xTimerChangePeriod(_handle, period, ticks_to_wait);
  }

  bool setPeriodFromISR(const TickType_t period, BaseType_t& task_woken) const {
    return xTimerChangePeriodFromISR(_handle, period, &task_woken);
  }

  TickType_t getPeriod() const { return xTimerGetPeriod(_handle); }

  void setTimerID() const { vTimerSetTimerID(_handle, _id); }
  void* getTimerID() const { return pvTimerGetTimerID(_handle); }

  void setReloadMode(const bool auto_reload) const { vTimerSetReloadMode(_handle, auto_reload); }
  bool getReloadMode() const { return uxTimerGetReloadMode(_handle); }

  explicit operator bool() const { return _handle != nullptr; }
};

class TimerDynamic : public TimerInterface {
  public:
  TimerDynamic()
      : TimerInterface(nullptr) {}

  TimerDynamic(const char* name, const TimerCallbackFunction_t callback, const TickType_t period,
               void* id, const bool auto_reload, const bool start)
      : TimerInterface(xTimerCreate(name, period, auto_reload, id, callback)) {
    this->setup(name, callback, period, id, auto_reload, start);

    if ((_handle != nullptr) && start) {
      this->start();
    }
  }

  bool create() {
    _handle = xTimerCreate(_name, _period, _auto_reload, _id, _callback);
    if (_handle == nullptr) return false;
    return _start ? this->start() : true;
  }
};

class TimerStatic : public TimerInterface {
  public:
  TimerStatic()
      : TimerInterface(nullptr) {}

  TimerStatic(const char* name, const TimerCallbackFunction_t callback, const TickType_t period,
              void* id, const bool auto_reload, const bool start)
      : TimerInterface(xTimerCreateStatic(name, period, auto_reload, id, callback, &_tcb)) {
    this->setup(name, callback, period, id, auto_reload, start);

    if ((_handle != nullptr) && start) {
      this->start();
    }
  }

  bool create() {
    _handle = xTimerCreateStatic(_name, _period, _auto_reload, _id, _callback, &_tcb);
    if (_handle == nullptr) return false;
    return _start ? this->start() : true;
  }

  private:
  StaticTimer_t _tcb;
};