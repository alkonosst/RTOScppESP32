/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Forward declaration of QueueSet
class QueueSet;

class LockInterface {
  private:
  friend class QueueSet;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const LockInterface& lock);

  protected:
  LockInterface(SemaphoreHandle_t handle)
      : _handle(handle) {}

  SemaphoreHandle_t _handle;

  public:
  virtual ~LockInterface() {
    if (_handle) vSemaphoreDelete(_handle);
  }

  LockInterface(const LockInterface&)                = delete;
  LockInterface& operator=(const LockInterface&)     = delete;
  LockInterface(LockInterface&&) noexcept            = delete;
  LockInterface& operator=(LockInterface&&) noexcept = delete;

  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xSemaphoreTake(_handle, ticks_to_wait);
  }

  virtual bool give() const { return xSemaphoreGive(_handle); }

  explicit operator bool() const { return _handle != nullptr; }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const LockInterface& lock) {
  return queue_set_member == lock._handle;
}

class MutexDynamic : public LockInterface {
  public:
  MutexDynamic()
      : LockInterface(xSemaphoreCreateMutex()) {}
};

class MutexStatic : public LockInterface {
  public:
  MutexStatic()
      : LockInterface(xSemaphoreCreateMutexStatic(&_tcb)) {}

  private:
  StaticSemaphore_t _tcb;
};

class MutexRecursiveDynamic : public LockInterface {
  public:
  MutexRecursiveDynamic()
      : LockInterface(xSemaphoreCreateRecursiveMutex()) {}

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xSemaphoreTakeRecursive(_handle, ticks_to_wait);
  }

  bool give() const override { return xSemaphoreGiveRecursive(_handle); }
};

class MutexRecursiveStatic : public LockInterface {
  public:
  MutexRecursiveStatic()
      : LockInterface(xSemaphoreCreateRecursiveMutexStatic(&_tcb)) {}

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xSemaphoreTakeRecursive(_handle, ticks_to_wait);
  }

  bool give() const override { return xSemaphoreGiveRecursive(_handle); }

  private:
  StaticSemaphore_t _tcb;
};

class Semaphore : public LockInterface {
  protected:
  Semaphore(const SemaphoreHandle_t handle)
      : LockInterface(handle) {}

  public:
  bool takeFromISR(BaseType_t& task_woken) const {
    return xSemaphoreTakeFromISR(_handle, &task_woken);
  }
  bool giveFromISR(BaseType_t& task_woken) const {
    return xSemaphoreGiveFromISR(_handle, &task_woken);
  }
  uint8_t getCount() const { return uxSemaphoreGetCount(_handle); }
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