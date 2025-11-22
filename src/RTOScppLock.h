/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
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
  virtual ~ILock()               = default;

  /**
   * @brief Get the low-level handle of the lock. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return SemaphoreHandle_t Lock handle, nullptr if the lock is not created.
   */
  virtual SemaphoreHandle_t getHandle() const = 0;

  /**
   * @brief Check if the lock is created.
   * @return true Lock is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Take the lock.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Lock taken successfully, false if the lock is not created or failed to take.
   */
  virtual bool take(const TickType_t ticks_to_wait = portMAX_DELAY) = 0;

  /**
   * @brief Give the lock.
   * @return true Lock given successfully, false if the lock is not created or failed to give.
   */
  virtual bool give() = 0;

  /**
   * @brief Check if the lock is taken.
   * @return true Lock is taken.
   */
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
  protected:
  Policy()
      : _handle(nullptr) {}

  public:
  SemaphoreHandle_t getHandle() const { return _handle; }

  bool isCreated() const { return _handle != nullptr; }

  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) {
    if (!isCreated()) return false;
    return static_cast<Derived*>(this)->takeImpl(ticks_to_wait);
  }

  bool give() {
    if (!isCreated()) return false;
    return static_cast<Derived*>(this)->giveImpl();
  }

  protected:
  SemaphoreHandle_t _handle;
};

// CRTP mutex base policy class
template <typename Derived>
class MutexPolicy : public Policy<MutexPolicy<Derived>> {
  protected:
  friend class Policy<MutexPolicy<Derived>>;

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }
};

// Policy for mutex with dynamic memory allocation
template <uint8_t Dummy = 0>
class MutexDynamicPolicy : public MutexPolicy<MutexDynamicPolicy<>> {
  public:
  MutexDynamicPolicy() { this->_handle = xSemaphoreCreateMutex(); }
};

// Policy for mutex with static memory allocation
template <uint8_t Dummy = 0>
class MutexStaticPolicy : public MutexPolicy<MutexStaticPolicy<>> {
  public:
  MutexStaticPolicy() { this->_handle = xSemaphoreCreateMutexStatic(&_mutex_buffer); }

  private:
  StaticSemaphore_t _mutex_buffer;
};

// CRTP recursive mutex policy base class
template <typename Derived>
class MutexRecursivePolicy : public Policy<MutexRecursivePolicy<Derived>> {
  protected:
  friend class Policy<MutexRecursivePolicy<Derived>>;

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTakeRecursive(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGiveRecursive(this->_handle); }
};

// Policy for recursive mutex with dynamic memory allocation
template <uint8_t Dummy = 0>
class MutexRecursiveDynamicPolicy : public MutexRecursivePolicy<MutexRecursiveDynamicPolicy<>> {
  public:
  MutexRecursiveDynamicPolicy() { this->_handle = xSemaphoreCreateRecursiveMutex(); }
};

// Policy for recursive mutex with static memory allocation
template <uint8_t Dummy = 0>
class MutexRecursiveStaticPolicy : public MutexRecursivePolicy<MutexRecursiveStaticPolicy<>> {
  public:
  MutexRecursiveStaticPolicy() {
    this->_handle = xSemaphoreCreateRecursiveMutexStatic(&_mutex_recursive_buffer);
  }

  private:
  StaticSemaphore_t _mutex_recursive_buffer;
};

// CRTP binary semaphore policy base class
template <typename Derived>
class SemaphoreBinaryPolicy : public Policy<SemaphoreBinaryPolicy<Derived>> {
  protected:
  friend class Policy<SemaphoreBinaryPolicy<Derived>>;

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }
};

// Policy for binary semaphore with dynamic memory allocation
template <uint8_t Dummy = 0>
class SemaphoreBinaryDynamicPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryDynamicPolicy<>> {
  public:
  SemaphoreBinaryDynamicPolicy() { this->_handle = xSemaphoreCreateBinary(); }
};

// Policy for binary semaphore with static memory allocation
template <uint8_t Dummy = 0>
class SemaphoreBinaryStaticPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryStaticPolicy<>> {
  public:
  SemaphoreBinaryStaticPolicy() {
    this->_handle = xSemaphoreCreateBinaryStatic(&_sem_binary_buffer);
  }

  private:
  StaticSemaphore_t _sem_binary_buffer;
};

// CRTP counting semaphore policy base class
template <typename Derived>
class SemaphoreCountingPolicy : public Policy<SemaphoreCountingPolicy<Derived>> {
  protected:
  friend class Policy<SemaphoreCountingPolicy<Derived>>;

  bool takeImpl(const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xSemaphoreTake(this->_handle, ticks_to_wait);
  }

  bool giveImpl() { return xSemaphoreGive(this->_handle); }

  public:
  /**
   * @brief Get the count of the counting semaphore.
   * @return UBaseType_t Count of the counting semaphore, 0 if the semaphore is not created.
   */
  UBaseType_t getCount() const {
    if (!this->isCreated()) return 0;
    return uxSemaphoreGetCount(this->_handle);
  }
};

// Policy for counting semaphore with dynamic memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingDynamicPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>> {

  public:
  SemaphoreCountingDynamicPolicy() {
    this->_handle = xSemaphoreCreateCounting(MaxCount, InitialCount);
  }
};

// Policy for counting semaphore with static memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingStaticPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingStaticPolicy<MaxCount, InitialCount>> {

  public:
  SemaphoreCountingStaticPolicy() {
    this->_handle = xSemaphoreCreateCountingStatic(MaxCount, InitialCount, &_sem_counting_buffer);
  }

  private:
  StaticSemaphore_t _sem_counting_buffer;
};

// Main Lock class. You need to specify the policy used
template <typename Policy>
class Lock : public ILock, public Policy {
  public:
  using Policy::Policy;

  ~Lock() {
    if (Policy::isCreated()) vSemaphoreDelete(Policy::getHandle());
  }

  /**
   * @brief Get the low-level handle of the lock. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return SemaphoreHandle_t Lock handle, nullptr if the lock is not created.
   */
  SemaphoreHandle_t getHandle() const { return Policy::getHandle(); }

  /**
   * @brief Check if the lock is created.
   * @return true Lock is created.
   */
  bool isCreated() const { return Policy::isCreated(); }

  /**
   * @brief Take the lock.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Lock taken successfully, false if the lock is not created or failed to take.
   */
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) { return Policy::take(ticks_to_wait); }

  /**
   * @brief Give the lock.
   * @return true Lock given successfully, false if the lock is not created or failed to give.
   */
  bool give() { return Policy::give(); }

  /**
   * @brief Check if the lock is taken.
   * @return true Lock is taken.
   */
  explicit operator bool() const { return isCreated(); }
};

} // namespace Internal

using MutexDynamic          = Internal::Lock<Internal::MutexDynamicPolicy<>>;
using MutexStatic           = Internal::Lock<Internal::MutexStaticPolicy<>>;
using MutexRecursiveDynamic = Internal::Lock<Internal::MutexRecursiveDynamicPolicy<>>;
using MutexRecursiveStatic  = Internal::Lock<Internal::MutexRecursiveStaticPolicy<>>;
using SemBinaryDynamic      = Internal::Lock<Internal::SemaphoreBinaryDynamicPolicy<>>;
using SemBinaryStatic       = Internal::Lock<Internal::SemaphoreBinaryStaticPolicy<>>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemCountingDynamic =
  Internal::Lock<Internal::SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemCountingStatic =
  Internal::Lock<Internal::SemaphoreCountingStaticPolicy<MaxCount, InitialCount>>;

} // namespace RTOS::Locks