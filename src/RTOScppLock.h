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

  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) = 0;
  virtual bool give()                                               = 0;

  virtual explicit operator bool() const = 0;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const ILock& lock);
};

// Comparison operator for QueueSet
inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const ILock& lock) {
  return queue_set_member == lock.getHandle();
}

namespace Internal {

// CRTP base policy class
template <typename Derived>
class Policy {
  public:
  SemaphoreHandle_t getHandle() const { return _handle; }

  bool create() { return static_cast<Derived*>(this)->createImpl(); }

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return static_cast<Derived*>(this)->takeImpl(ticks_to_wait);
  }

  bool give() { return static_cast<Derived*>(this)->giveImpl(); }

  protected:
  SemaphoreHandle_t _handle;
};

// CRTP mutex base policy class
template <typename Derived>
class MutexPolicy : public Policy<MutexPolicy<Derived>> {
  public:
  bool createImpl() { return static_cast<Derived*>(this)->createImpl(); }

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }
};

// Policy for mutex with dynamic memory allocation
class MutexDynamicPolicy : public MutexPolicy<MutexDynamicPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateMutex();
    return this->_handle != nullptr;
  }
};

// Policy for mutex with static memory allocation
class MutexStaticPolicy : public MutexPolicy<MutexStaticPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateMutexStatic(&_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticSemaphore_t _tcb;
};

// CRTP recursive mutex policy base class
template <typename Derived>
class MutexRecursivePolicy : public Policy<MutexRecursivePolicy<Derived>> {
  public:
  bool createImpl() { return static_cast<Derived*>(this)->createImpl(); }

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTakeRecursive(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGiveRecursive(this->_handle); }
};

// Policy for recursive mutex with dynamic memory allocation
class MutexRecursiveDynamicPolicy : public MutexRecursivePolicy<MutexRecursiveDynamicPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateRecursiveMutex();
    return this->_handle != nullptr;
  }
};

// Policy for recursive mutex with static memory allocation
class MutexRecursiveStaticPolicy : public MutexRecursivePolicy<MutexRecursiveStaticPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateRecursiveMutexStatic(&_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticSemaphore_t _tcb;
};

// CRTP binary semaphore policy base class
template <typename Derived>
class SemaphoreBinaryPolicy : public Policy<SemaphoreBinaryPolicy<Derived>> {
  public:
  bool createImpl() { return static_cast<Derived*>(this)->createImpl(); }

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }
};

// Policy for binary semaphore with dynamic memory allocation
class SemaphoreBinaryDynamicPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryDynamicPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateBinary();
    return this->_handle != nullptr;
  }
};

// Policy for binary semaphore with static memory allocation
class SemaphoreBinaryStaticPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryStaticPolicy> {
  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateBinaryStatic(&_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticSemaphore_t _tcb;
};

// CRTP counting semaphore policy base class
template <typename Derived>
class SemaphoreCountingPolicy : public Policy<SemaphoreCountingPolicy<Derived>> {
  public:
  bool createImpl() { return static_cast<Derived*>(this)->createImpl(); }

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }

  uint32_t getCount() const { return uxSemaphoreGetCount(this->_handle); }
};

// Policy for counting semaphore with dynamic memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingDynamicPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>> {

  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateCounting(MaxCount, InitialCount);
    return this->_handle != nullptr;
  }
};

// Policy for counting semaphore with static memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingStaticPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingStaticPolicy<MaxCount, InitialCount>> {

  public:
  bool createImpl() {
    this->_handle = xSemaphoreCreateCountingStatic(MaxCount, InitialCount, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticSemaphore_t _tcb;
};

// Main Lock class. You need to specify the policy used
template <typename Policy>
class Lock : public ILock {
  public:
  Lock() { _policy.create(); }

  ~Lock() {
    if (getHandle()) vSemaphoreDelete(getHandle());
  }

  SemaphoreHandle_t getHandle() const override { return _policy.getHandle(); }

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return _policy.take(ticks_to_wait);
  }

  bool give() override { return _policy.give(); }

  template <typename P = Policy>
  typename std::enable_if<std::is_base_of<SemaphoreCountingPolicy<P>, P>::value, uint32_t>::type
  getCount() const {
    return _policy.getCount();
  }

  explicit operator bool() const override { return _policy.getHandle() != nullptr; }

  private:
  Policy _policy;
};

} // namespace Internal

using MutexDynamic           = Internal::Lock<Internal::MutexDynamicPolicy>;
using MutexStatic            = Internal::Lock<Internal::MutexStaticPolicy>;
using MutexRecursiveDynamic  = Internal::Lock<Internal::MutexRecursiveDynamicPolicy>;
using MutexRecursiveStatic   = Internal::Lock<Internal::MutexRecursiveStaticPolicy>;
using SemaphoreBinaryDynamic = Internal::Lock<Internal::SemaphoreBinaryDynamicPolicy>;
using SemaphoreBinaryStatic  = Internal::Lock<Internal::SemaphoreBinaryStaticPolicy>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemaphoreCountingDynamic =
  Internal::Lock<Internal::SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemaphoreCountingStatic =
  Internal::Lock<Internal::SemaphoreCountingStaticPolicy<MaxCount, InitialCount>>;

} // namespace RTOS::Locks