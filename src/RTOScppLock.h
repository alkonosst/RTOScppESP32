/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <freertos/FreeRTOS.h>
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
   * @brief Get the name of the lock. Useful for debugging and logging purposes.
   * @return const char* Name of the lock. Default is "RtosLock" if no name is provided.
   */
  virtual const char* getName() const = 0;

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

// Interface for Semaphore objects, extends ILock with ISR support
class ISemaphore : public ILock {
  protected:
  ISemaphore() = default;

  public:
  /**
   * @brief Give the semaphore from an ISR.
   * @param task_woken Higher priority task woken flag. You need to use
   * portYIELD_FROM_ISR(task_woken) at the end of the ISR.
   * @return true Semaphore given successfully, false if the semaphore is not created or failed to
   * give.
   */
  virtual bool giveFromISR(BaseType_t& task_woken) = 0;

  /**
   * @brief Take the semaphore from an ISR.
   * @param task_woken Higher priority task woken flag. You need to use
   * portYIELD_FROM_ISR(task_woken) at the end of the ISR.
   * @return true Semaphore taken successfully, false if the semaphore is not created or failed to
   * take.
   */
  virtual bool takeFromISR(BaseType_t& task_woken) = 0;
};

namespace Internal {

// CRTP base policy class
template <typename Derived>
class Policy {
  protected:
  Policy()
      : _handle(nullptr)
      , _name("RtosLock") {}

  public:
  SemaphoreHandle_t getHandle() const { return _handle; }

  const char* getName() const { return _name; }

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
  const char* _name;
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
  MutexDynamicPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateMutex();
    if (name) this->_name = name;
  }
};

// Policy for mutex with static memory allocation
template <uint8_t Dummy = 0>
class MutexStaticPolicy : public MutexPolicy<MutexStaticPolicy<>> {
  public:
  MutexStaticPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateMutexStatic(&_mutex_buffer);
    if (name) this->_name = name;
  }

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
  MutexRecursiveDynamicPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateRecursiveMutex();
    if (name) this->_name = name;
  }
};

// Policy for recursive mutex with static memory allocation
template <uint8_t Dummy = 0>
class MutexRecursiveStaticPolicy : public MutexRecursivePolicy<MutexRecursiveStaticPolicy<>> {
  public:
  MutexRecursiveStaticPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateRecursiveMutexStatic(&_mutex_recursive_buffer);
    if (name) this->_name = name;
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

  public:
  bool takeFromISR(BaseType_t& task_woken) {
    if (!this->isCreated()) return false;
    return xSemaphoreTakeFromISR(this->_handle, &task_woken);
  }

  bool giveFromISR(BaseType_t& task_woken) {
    if (!this->isCreated()) return false;
    return xSemaphoreGiveFromISR(this->_handle, &task_woken);
  }
};

// Policy for binary semaphore with dynamic memory allocation
template <uint8_t Dummy = 0>
class SemaphoreBinaryDynamicPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryDynamicPolicy<>> {
  public:
  SemaphoreBinaryDynamicPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateBinary();
    if (name) this->_name = name;
  }
};

// Policy for binary semaphore with static memory allocation
template <uint8_t Dummy = 0>
class SemaphoreBinaryStaticPolicy : public SemaphoreBinaryPolicy<SemaphoreBinaryStaticPolicy<>> {
  public:
  SemaphoreBinaryStaticPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateBinaryStatic(&_sem_binary_buffer);
    if (name) this->_name = name;
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

  bool takeFromISR(BaseType_t& task_woken) {
    if (!this->isCreated()) return false;
    return xSemaphoreTakeFromISR(this->_handle, &task_woken);
  }

  bool giveFromISR(BaseType_t& task_woken) {
    if (!this->isCreated()) return false;
    return xSemaphoreGiveFromISR(this->_handle, &task_woken);
  }
};

// Policy for counting semaphore with dynamic memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingDynamicPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>> {

  public:
  SemaphoreCountingDynamicPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateCounting(MaxCount, InitialCount);
    if (name) this->_name = name;
  }
};

// Policy for counting semaphore with static memory allocation
template <uint32_t MaxCount, uint32_t InitialCount = 0>
class SemaphoreCountingStaticPolicy
    : public SemaphoreCountingPolicy<SemaphoreCountingStaticPolicy<MaxCount, InitialCount>> {

  public:
  SemaphoreCountingStaticPolicy(const char* name = nullptr) {
    this->_handle = xSemaphoreCreateCountingStatic(MaxCount, InitialCount, &_sem_counting_buffer);
    if (name) this->_name = name;
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
  SemaphoreHandle_t getHandle() const override { return Policy::getHandle(); }

  /**
   * @brief Get the name of the lock. Useful for debugging and logging purposes.
   * @return const char* Name of the lock. Default is "RtosLock" if no name is provided.
   */
  const char* getName() const override { return Policy::getName(); }

  /**
   * @brief Check if the lock is created.
   * @return true Lock is created.
   */
  bool isCreated() const override { return Policy::isCreated(); }

  /**
   * @brief Take the lock.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Lock taken successfully, false if the lock is not created or failed to take.
   */
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return Policy::take(ticks_to_wait);
  }

  /**
   * @brief Give the lock.
   * @return true Lock given successfully, false if the lock is not created or failed to give.
   */
  bool give() override { return Policy::give(); }

  /**
   * @brief Check if the lock is taken.
   * @return true Lock is taken.
   */
  explicit operator bool() const override { return isCreated(); }
};

// Main SemaphoreLock class with ISR support. You need to specify the policy used
template <typename Policy>
class SemaphoreLock : public ISemaphore, public Policy {
  public:
  using Policy::Policy;

  ~SemaphoreLock() {
    if (Policy::isCreated()) vSemaphoreDelete(Policy::getHandle());
  }

  /**
   * @brief Get the low-level handle of the semaphore. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return SemaphoreHandle_t Semaphore handle, nullptr if the semaphore is not created.
   */
  SemaphoreHandle_t getHandle() const override { return Policy::getHandle(); }

  /**
   * @brief Get the name of the semaphore. Useful for debugging and logging purposes.
   * @return const char* Name of the semaphore. Default is "RtosLock" if no name is provided.
   */
  const char* getName() const override { return Policy::getName(); }

  /**
   * @brief Check if the semaphore is created.
   * @return true Semaphore is created.
   */
  bool isCreated() const override { return Policy::isCreated(); }

  /**
   * @brief Take the semaphore.
   * @param ticks_to_wait Maximum time to wait for the operation to complete.
   * @return true Semaphore taken successfully, false if the semaphore is not created or failed to
   * take.
   */
  bool take(const TickType_t ticks_to_wait = portMAX_DELAY) override {
    return Policy::take(ticks_to_wait);
  }

  /**
   * @brief Give the semaphore.
   * @return true Semaphore given successfully, false if the semaphore is not created or failed to
   * give.
   */
  bool give() override { return Policy::give(); }

  /**
   * @brief Give the semaphore from an ISR.
   * @param task_woken Higher priority task woken flag. You need to use
   * portYIELD_FROM_ISR(task_woken) at the end of the ISR.
   * @return true Semaphore given successfully, false if the semaphore is not created or failed to
   * give.
   */
  bool giveFromISR(BaseType_t& task_woken) override { return Policy::giveFromISR(task_woken); }

  /**
   * @brief Take the semaphore from an ISR.
   * @param task_woken Higher priority task woken flag. You need to use
   * portYIELD_FROM_ISR(task_woken) at the end of the ISR.
   * @return true Semaphore taken successfully, false if the semaphore is not created or failed to
   * take.
   */
  bool takeFromISR(BaseType_t& task_woken) override { return Policy::takeFromISR(task_woken); }

  /**
   * @brief Check if the semaphore is created.
   * @return true Semaphore is created.
   */
  explicit operator bool() const override { return isCreated(); }
};

} // namespace Internal

using MutexDynamic          = Internal::Lock<Internal::MutexDynamicPolicy<>>;
using MutexStatic           = Internal::Lock<Internal::MutexStaticPolicy<>>;
using MutexRecursiveDynamic = Internal::Lock<Internal::MutexRecursiveDynamicPolicy<>>;
using MutexRecursiveStatic  = Internal::Lock<Internal::MutexRecursiveStaticPolicy<>>;
using SemBinaryDynamic      = Internal::SemaphoreLock<Internal::SemaphoreBinaryDynamicPolicy<>>;
using SemBinaryStatic       = Internal::SemaphoreLock<Internal::SemaphoreBinaryStaticPolicy<>>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemCountingDynamic =
  Internal::SemaphoreLock<Internal::SemaphoreCountingDynamicPolicy<MaxCount, InitialCount>>;

template <uint32_t MaxCount, uint32_t InitialCount = 0>
using SemCountingStatic =
  Internal::SemaphoreLock<Internal::SemaphoreCountingStaticPolicy<MaxCount, InitialCount>>;

} // namespace RTOS::Locks