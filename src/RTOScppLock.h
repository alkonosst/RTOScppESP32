/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

namespace RTOS::Locks {

// Interface for Lock objects, useful when using pointers
class ILock {
  protected:
  ILock() = default;

  public:
  ILock(const ILock&)            = delete;
  ILock& operator=(const ILock&) = delete;
  ILock(ILock&&)                 = delete;
  ILock& operator=(ILock&&)      = delete;

  virtual ~ILock() = default;

  virtual SemaphoreHandle_t getHandle() const = 0;

  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;
  virtual bool give() const                                               = 0;

  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base class template
template <typename Derived>
class Policy {
  public:
  SemaphoreHandle_t getHandle() const { return _handle; }

  protected:
  Policy()
      : _handle(nullptr) {}

  SemaphoreHandle_t _handle;
};

// Policy base class for mutex
template <typename Derived>
class MutexPolicy : public Policy<Derived> {
  public:
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool give() const { return xSemaphoreGive(this->_handle); }
};

// Policy for mutex with dynamic memory allocation
class MutexDynamicPolicy : public MutexPolicy<MutexDynamicPolicy> {
  public:
  MutexDynamicPolicy() { _handle = xSemaphoreCreateMutex(); }
};

// Policy for mutex with static memory allocation
class MutexStaticPolicy : public MutexPolicy<MutexStaticPolicy> {
  public:
  MutexStaticPolicy() { _handle = xSemaphoreCreateMutexStatic(&_tcb); }

  private:
  StaticSemaphore_t _tcb;
};

// Policy base class for recursive mutex
template <typename Derived>
class MutexRecursivePolicy : public Policy<Derived> {
  public:
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xSemaphoreTakeRecursive(this->_handle, ticks_to_wait);
  }

  bool give() const { return xSemaphoreGiveRecursive(this->_handle); }
};

// Policy for recursive mutex with dynamic memory allocation
class MutexRecursiveDynamicPolicy : public MutexRecursivePolicy<MutexRecursiveDynamicPolicy> {
  public:
  MutexRecursiveDynamicPolicy() { _handle = xSemaphoreCreateRecursiveMutex(); }
};

// Policy for recursive mutex with static memory allocation
class MutexRecursiveStaticPolicy : public MutexRecursivePolicy<MutexRecursiveStaticPolicy> {
  public:
  MutexRecursiveStaticPolicy() { _handle = xSemaphoreCreateRecursiveMutexStatic(&_tcb); }

  private:
  StaticSemaphore_t _tcb;
};

// Policy base class for semaphores
template <typename Derived>
class SemaphorePolicy : public Policy<Derived> {
  public:
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool give() const { return xSemaphoreGive(this->_handle); }
};

// Policy for binary semaphore with dynamic memory allocation
class SemaphoreBinaryDynamicPolicy : public SemaphorePolicy<SemaphoreBinaryDynamicPolicy> {
  public:
  SemaphoreBinaryDynamicPolicy() { _handle = xSemaphoreCreateBinary(); }
};

// Policy for binary semaphore with static memory allocation
class SemaphoreBinaryStaticPolicy : public SemaphorePolicy<SemaphoreBinaryStaticPolicy> {
  public:
  SemaphoreBinaryStaticPolicy() { _handle = xSemaphoreCreateBinaryStatic(&_tcb); }

  private:
  StaticSemaphore_t _tcb;
};

// Policy for counting semaphore with dynamic memory allocation
class SemaphoreCountingDynamicPolicy : public SemaphorePolicy<SemaphoreCountingDynamicPolicy> {
  public:
  SemaphoreCountingDynamicPolicy(const uint8_t max_count, const uint8_t initial_count = 0) {
    _handle = xSemaphoreCreateCounting(max_count, initial_count);
  }
};

// Policy for counting semaphore with static memory allocation
class SemaphoreCountingStaticPolicy : public SemaphorePolicy<SemaphoreCountingStaticPolicy> {
  public:
  SemaphoreCountingStaticPolicy(const uint8_t max_count, const uint8_t initial_count = 0) {
    _handle = xSemaphoreCreateCountingStatic(max_count, initial_count, &_tcb);
  }

  private:
  StaticSemaphore_t _tcb;
};

// Main Lock class. You need to specify the policy used
template <typename Policy>
class Lock : public ILock, private Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~Lock() {
    if (getHandle()) vSemaphoreDelete(getHandle());
  }

  SemaphoreHandle_t getHandle() const override { return Policy::getHandle(); }

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return Policy::take(ticks_to_wait);
  }

  bool give() const override { return Policy::give(); }

  explicit operator bool() const override { return Policy::getHandle() != nullptr; }
};

} // namespace Internal

using MutexDynamic             = Internal::Lock<Internal::MutexDynamicPolicy>;
using MutexStatic              = Internal::Lock<Internal::MutexStaticPolicy>;
using MutexRecursiveDynamic    = Internal::Lock<Internal::MutexRecursiveDynamicPolicy>;
using MutexRecursiveStatic     = Internal::Lock<Internal::MutexRecursiveStaticPolicy>;
using SemaphoreBinaryDynamic   = Internal::Lock<Internal::SemaphoreBinaryDynamicPolicy>;
using SemaphoreBinaryStatic    = Internal::Lock<Internal::SemaphoreBinaryStaticPolicy>;
using SemaphoreCountingDynamic = Internal::Lock<Internal::SemaphoreCountingDynamicPolicy>;
using SemaphoreCountingStatic  = Internal::Lock<Internal::SemaphoreCountingStaticPolicy>;

} // namespace RTOS::Locks