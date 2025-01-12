/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/queue.h>

namespace RTOS::Queues {

// Interface for Queue objects, useful when using pointers
class IQueue {
  protected:
  IQueue() = default;

  public:
  IQueue(const IQueue&)            = delete;
  IQueue& operator=(const IQueue&) = delete;
  IQueue(IQueue&&)                 = delete;
  IQueue& operator=(IQueue&&)      = delete;

  virtual ~IQueue() = default;

  virtual QueueHandle_t getHandle() const = 0;

  virtual uint32_t getAvailableMessages() const        = 0;
  virtual uint32_t getAvailableMessagesFromISR() const = 0;
  virtual uint32_t getAvailableSpaces() const          = 0;

  virtual void reset() const          = 0;
  virtual bool isFull() const         = 0;
  virtual bool isEmpty() const        = 0;
  virtual bool isFullFromISR() const  = 0;
  virtual bool isEmptyFromISR() const = 0;

  virtual explicit operator bool() const = 0;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const IQueue& queue);
};

// Comparison operator for QueueSet
inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const IQueue& queue) {
  return queue_set_member == queue.getHandle();
}

namespace Internal {

// CRTP base class
template <typename Derived, typename T>
class Policy {
  public:
  QueueHandle_t getHandle() const { return _handle; }

  protected:
  Policy()
      : _handle(nullptr) {}

  QueueHandle_t _handle;
};

// Policy for queue with dynamic memory allocation
template <typename T, uint32_t Length>
class DynamicPolicy : public Policy<DynamicPolicy<T, Length>, T> {
  public:
  DynamicPolicy() { this->_handle = xQueueCreate(Length, sizeof(T)); }
};

// Policy for queue with static memory allocation
template <typename T, uint32_t Length>
class StaticPolicy : public Policy<StaticPolicy<T, Length>, T> {
  public:
  StaticPolicy() { this->_handle = xQueueCreateStatic(Length, sizeof(T), _storage, &_tcb); }

  private:
  StaticQueue_t _tcb;
  uint8_t _storage[Length * sizeof(T)];
};

// Policy for queue with external storage
template <typename T, uint32_t Length>
class ExternalStoragePolicy : public Policy<ExternalStoragePolicy<T, Length>, T> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = Length * sizeof(T);

  ExternalStoragePolicy() { this->_handle = nullptr; }

  bool create(uint8_t* const buffer) {
    this->_handle = xQueueCreateStatic(Length, sizeof(T), buffer, &_tcb);
    return (this->_handle != nullptr);
  }

  private:
  StaticQueue_t _tcb;
};

// Main Queue class. You need to specify the policy used
template <typename Policy, typename T>
class Queue : public IQueue, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~Queue() {
    if (this->getHandle()) vQueueDelete(this->getHandle());
  }

  QueueHandle_t getHandle() const override { return Policy::getHandle(); }

  uint32_t getAvailableMessages() const override {
    return uxQueueMessagesWaiting(Policy::getHandle());
  }

  uint32_t getAvailableMessagesFromISR() const override {
    return uxQueueMessagesWaitingFromISR(Policy::getHandle());
  }

  uint32_t getAvailableSpaces() const override {
    return uxQueueSpacesAvailable(Policy::getHandle());
  }

  void reset() const override { xQueueReset(Policy::getHandle()); }

  bool isFull() const override { return uxQueueSpacesAvailable(Policy::getHandle()) == 0; }

  bool isEmpty() const override { return uxQueueMessagesWaiting(Policy::getHandle()) == 0; }

  bool isFullFromISR() const override { return xQueueIsQueueFullFromISR(Policy::getHandle()); }

  bool isEmptyFromISR() const override { return xQueueIsQueueEmptyFromISR(Policy::getHandle()); }

  explicit operator bool() const override { return Policy::getHandle() != nullptr; }

  bool push(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSendToFront(Policy::getHandle(), &item, ticks_to_wait);
  }

  bool pushFromISR(const T& item, BaseType_t& task_woken) const {
    return xQueueSendToFrontFromISR(Policy::getHandle(), &item, &task_woken);
  }

  bool add(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSendToBack(Policy::getHandle(), &item, ticks_to_wait);
  }

  bool addFromISR(const T& item, BaseType_t& task_woken) const {
    return xQueueSendToBackFromISR(Policy::getHandle(), &item, &task_woken);
  }

  bool pop(T& var, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueReceive(Policy::getHandle(), &var, ticks_to_wait);
  }

  bool popFromISR(T& var, BaseType_t& task_woken) const {
    return xQueueReceiveFromISR(Policy::getHandle(), &var, &task_woken);
  }

  bool peek(T& var, const TickType_t ticks_to_wait = 0) const {
    return xQueuePeek(Policy::getHandle(), &var, ticks_to_wait);
  }

  bool peekFromISR(T& var) const { return xQueuePeekFromISR(Policy::getHandle(), &var); }
};

} // namespace Internal

template <typename T, uint32_t Length>
using QueueDynamic = Internal::Queue<Internal::DynamicPolicy<T, Length>, T>;

template <typename T, uint32_t Length>
using QueueStatic = Internal::Queue<Internal::StaticPolicy<T, Length>, T>;

template <typename T, uint32_t Length>
using QueueExternalStorage = Internal::Queue<Internal::ExternalStoragePolicy<T, Length>, T>;

} // namespace RTOS::Queues