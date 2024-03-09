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
  Lock(Lock const&)           = delete; // Delete copy constructor
  void operator=(Lock const&) = delete; // Delete copy assignment operator

  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const Lock& semaphore);

  protected:
  Lock(SemaphoreHandle_t handle)
      : _handle(handle) {}
  virtual ~Lock() { vSemaphoreDelete(_handle); }

  SemaphoreHandle_t _handle;

  public:
  // SemaphoreHandle_t getHandle() { return _handle; }

  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(_handle, ticks_to_wait);
  }

  virtual bool give() { return xSemaphoreGive(_handle); }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const Lock& semaphore) {
  return queue_set_member == semaphore._handle;
}

class MutexDynamic : public Lock {
  public:
  MutexDynamic()
      : Lock(xSemaphoreCreateMutex()) {}
};

class MutexStatic : public Lock {
  public:
  MutexStatic()
      : Lock(xSemaphoreCreateMutexStatic(&_tcb)) {}

  private:
  StaticSemaphore_t _tcb;
};

class MutexRecursiveDynamic : public Lock {
  public:
  MutexRecursiveDynamic()
      : Lock(xSemaphoreCreateRecursiveMutex()) {}

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return xSemaphoreTakeRecursive(_handle, ticks_to_wait);
  }

  bool give() override { return xSemaphoreGiveRecursive(_handle); }
};

class MutexRecursiveStatic : public Lock {
  public:
  MutexRecursiveStatic()
      : Lock(xSemaphoreCreateRecursiveMutexStatic(&_tcb)) {}

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return xSemaphoreTakeRecursive(_handle, ticks_to_wait);
  }

  bool give() override { return xSemaphoreGiveRecursive(_handle); }

  private:
  StaticSemaphore_t _tcb;
};

class Semaphore : public Lock {
  protected:
  Semaphore(SemaphoreHandle_t handle)
      : Lock(handle) {}

  public:
  bool takeFromISR(BaseType_t& task_woken) { return xSemaphoreTakeFromISR(_handle, &task_woken); }
  bool giveFromISR(BaseType_t& task_woken) { return xSemaphoreGiveFromISR(_handle, &task_woken); }
  uint8_t getCount() { return uxSemaphoreGetCount(_handle); }
};

class SemaphoreBinaryDynamic : public Semaphore {
  public:
  SemaphoreBinaryDynamic()
      : Semaphore(xSemaphoreCreateBinary()) {}
};

class SemaphoreBinaryStatic : public Semaphore {
  public:
  SemaphoreBinaryStatic()
      : Semaphore(xSemaphoreCreateBinaryStatic(&_tcb)) {}

  private:
  StaticSemaphore_t _tcb;
};

class SemaphoreCountingDynamic : public Semaphore {
  public:
  SemaphoreCountingDynamic(const uint8_t max_count, const uint8_t initial_count = 0)
      : Semaphore(xSemaphoreCreateCounting(max_count, initial_count)) {}
};

class SemaphoreCountingStatic : public Semaphore {
  public:
  SemaphoreCountingStatic(const uint8_t max_count, const uint8_t initial_count = 0)
      : Semaphore(xSemaphoreCreateCountingStatic(max_count, initial_count, &_tcb)) {}

  private:
  StaticSemaphore_t _tcb;
};

#endif // RTOS_CPP_LOCK_H