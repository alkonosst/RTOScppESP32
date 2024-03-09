/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_TIMER_H
#define RTOS_CPP_TIMER_H

#include <Arduino.h>
#include <freertos/timers.h>

class TimerBase {
  private:
  TimerBase(const TimerBase&)      = delete; // Delete copy constructor
  void operator=(const TimerBase&) = delete; // Delete copy assignment operator

  protected:
  TimerBase(TimerHandle_t handle)
      : _handle(handle) {}
  virtual ~TimerBase() {
    if (_handle) xTimerDelete(_handle, portMAX_DELAY);
  }

  TimerHandle_t _handle;

  public:
  bool start(TickType_t ticks_to_wait = portMAX_DELAY) {
    return xTimerStart(_handle, ticks_to_wait);
  }

  bool startFromISR(BaseType_t& task_woken) { return xTimerStartFromISR(_handle, &task_woken); }
  bool stop(TickType_t ticks_to_wait = portMAX_DELAY) { return xTimerStop(_handle, ticks_to_wait); }
  bool stopFromISR(BaseType_t& task_woken) { return xTimerStopFromISR(_handle, &task_woken); }

  bool isActive() { return xTimerIsTimerActive(_handle); }
  bool reset(TickType_t ticks_to_wait = portMAX_DELAY) {
    return xTimerReset(_handle, ticks_to_wait);
  }

  bool resetFromISR(BaseType_t& task_woken) { return xTimerResetFromISR(_handle, &task_woken); }

  const char* getName() { return pcTimerGetName(_handle); }
  TickType_t getExpiryTime() { return xTimerGetExpiryTime(_handle); }

  bool setPeriod(TickType_t period, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xTimerChangePeriod(_handle, period, ticks_to_wait);
  }

  bool setPeriodFromISR(TickType_t period, BaseType_t& task_woken) {
    return xTimerChangePeriodFromISR(_handle, period, &task_woken);
  }

  TickType_t getPeriod() { return xTimerGetPeriod(_handle); }

  void setReloadMode(bool auto_reload) { vTimerSetReloadMode(_handle, auto_reload); }
  bool getReloadMode() { return uxTimerGetReloadMode(_handle); }
};

class TimerDynamic : public TimerBase {
  public:
  TimerDynamic(const char* name, TimerCallbackFunction_t callback, TickType_t period,
               bool auto_reload, bool start)
      : TimerBase(xTimerCreate(name, period, auto_reload, 0, callback)) {
    if (start) this->start();
  }
};

class TimerStatic : public TimerBase {
  public:
  TimerStatic(const char* name, TimerCallbackFunction_t callback, TickType_t period,
              bool auto_reload, bool start)
      : TimerBase(xTimerCreateStatic(name, period, auto_reload, 0, callback, &_tcb)) {
    if (start) this->start();
  }

  private:
  StaticTimer_t _tcb;
};

#endif // RTOS_CPP_TIMER_H