/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_LOCK_H
#define RTOS_CPP_LOCK_H

#include <Arduino.h>
#include <freertos/semphr.h>

class Lock {
  private:
  Lock(Lock const&)           = delete;
  void operator=(Lock const&) = delete;

  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const Lock& semaphore);

  protected:
  Lock(SemaphoreHandle_t handle)
      : _handle(handle) {}
  ~Lock() { vSemaphoreDelete(_handle); }

  SemaphoreHandle_t _handle;
  StaticSemaphore_t _tcb;

  public:
  SemaphoreHandle_t getHandle() { return _handle; }

  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(_handle, ticks_to_wait);
  }

  virtual bool give() { return xSemaphoreGive(_handle); }
};

bool operator==(const QueueSetMemberHandle_t& queue_set_member, const Lock& semaphore) {
  return queue_set_member == semaphore._handle;
}

class Mutex : public Lock {
  public:
  Mutex()
      : Lock(xSemaphoreCreateMutexStatic(&_tcb)) {}
};

class MutexRecursive : public Lock {
  public:
  MutexRecursive()
      : Lock(xSemaphoreCreateRecursiveMutexStatic(&_tcb)) {}

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return xSemaphoreTakeRecursive(_handle, ticks_to_wait);
  }

  bool give() override { return xSemaphoreGiveRecursive(_handle); }
};

class Semaphore : public Lock {
  protected:
  Semaphore(SemaphoreHandle_t handle)
      : Lock(handle) {}

  public:
  bool takeFromISR(BaseType_t& task_woken) { return xSemaphoreTakeFromISR(_handle, &task_woken); }
  bool giveFromISR(BaseType_t& task_woken) { return xSemaphoreGiveFromISR(_handle, &task_woken); }
};

class SemaphoreBinary : public Semaphore {
  public:
  SemaphoreBinary()
      : Semaphore(xSemaphoreCreateBinaryStatic(&_tcb)) {}
};

class SemaphoreCounting : public Semaphore {
  public:
  SemaphoreCounting(const uint8_t max_count, const uint8_t initial_count = 0)
      : Semaphore(xSemaphoreCreateCountingStatic(max_count, initial_count, &_tcb)) {}
};

#endif // RTOS_CPP_LOCK_H