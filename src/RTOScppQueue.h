/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
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
  virtual ~IQueue()                = default;

  /**
   * @brief Get the low-level handle of the queue. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return QueueHandle_t Queue handle, nullptr if the queue is not created.
   */
  virtual QueueHandle_t getHandle() const = 0;

  /**
   * @brief Check if the queue is created.
   * @return true Queue is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Get the number of available messages to read from the queue.
   * @return uint32_t Number of available messages.
   */
  virtual uint32_t getAvailableMessages() const = 0;

  /**
   * @brief Get the number of available messages to read from the queue from an ISR.
   * @return uint32_t Number of available messages.
   */
  virtual uint32_t getAvailableMessagesFromISR() const = 0;

  /**
   * @brief Get the number of available spaces to write to the queue.
   * @return uint32_t Number of available spaces.
   */
  virtual uint32_t getAvailableSpaces() const = 0;

  /**
   * @brief Reset the queue.
   * @return true Queue reset successfully, false if the queue is not created.
   */
  virtual bool reset() const = 0;

  /**
   * @brief Check if the queue is full.
   * @return true Queue is full, false if the queue is not created.
   */
  virtual bool isFull() const = 0;

  /**
   * @brief Check if the queue is empty.
   * @return true Queue is empty, false if the queue is not created.
   */
  virtual bool isEmpty() const = 0;

  /**
   * @brief Check if the queue is full from an ISR.
   * @return true Queue is full, false if the queue is not created.
   */
  virtual bool isFullFromISR() const = 0;

  /**
   * @brief Check if the queue is empty from an ISR.
   * @return true Queue is empty, false if the queue is not created.
   */
  virtual bool isEmptyFromISR() const = 0;

  /**
   * @brief Check if the queue is created.
   * @return true Queue is created.
   */
  virtual explicit operator bool() const = 0;

  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member, const IQueue& queue);
};

// Comparison operator for QueueSet
inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const IQueue& queue) {
  return queue_set_member == queue.getHandle();
}

namespace Internal {

// CRTP base class
template <typename Derived>
class Policy {
  protected:
  Policy()
      : _handle(nullptr) {}

  QueueHandle_t getHandle() const { return _handle; }

  bool isCreated() const { return _handle != nullptr; }

  protected:
  QueueHandle_t _handle;
};

// Policy for queue with dynamic memory allocation
template <typename T, uint32_t Length>
class DynamicPolicy : public Policy<DynamicPolicy<T, Length>> {
  public:
  DynamicPolicy() { this->_handle = xQueueCreate(Length, sizeof(T)); }
};

// Policy for queue with static memory allocation
template <typename T, uint32_t Length>
class StaticPolicy : public Policy<StaticPolicy<T, Length>> {
  public:
  StaticPolicy() {
    this->_handle = xQueueCreateStatic(Length, sizeof(T), _storage, &_queue_buffer);
  }

  private:
  StaticQueue_t _queue_buffer;
  uint8_t _storage[Length * sizeof(T)];
};

// Policy for queue with external storage
template <typename T, uint32_t Length>
class ExternalStoragePolicy : public Policy<ExternalStoragePolicy<T, Length>> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = Length * sizeof(T);

  ExternalStoragePolicy() { this->_handle = nullptr; }

  /**
   * @brief Create the queue with the specified buffer.
   * @param buffer Buffer to use.
   * @return true Queue created, false if the buffer is nullptr or failed to create the queue.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;
    this->_handle = xQueueCreateStatic(Length, sizeof(T), buffer, &_queue_buffer);
    return (this->_handle != nullptr);
  }

  private:
  StaticQueue_t _queue_buffer;
};

// Main Queue class. You need to specify the policy used
template <typename Policy, typename T>
class Queue : public IQueue, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~Queue() {
    if (Policy::isCreated()) vQueueDelete(Policy::getHandle());
  }

  /**
   * @brief Get the low-level handle of the queue. Useful for direct FreeRTOS API calls. Use it with
   * caution.
   * @return QueueHandle_t Queue handle, nullptr if the queue is not created.
   */
  QueueHandle_t getHandle() const override { return Policy::getHandle(); }

  /**
   * @brief Check if the queue is created.
   * @return true Queue is created.
   */
  bool isCreated() const override { return Policy::isCreated(); }

  /**
   * @brief Get the number of available messages to read from the queue.
   * @return uint32_t Number of available messages.
   */
  uint32_t getAvailableMessages() const override {
    if (!isCreated()) return 0;
    return uxQueueMessagesWaiting(getHandle());
  }

  /**
   * @brief Get the number of available messages to read from the queue from an ISR.
   * @return uint32_t Number of available messages.
   */
  uint32_t getAvailableMessagesFromISR() const override {
    if (!isCreated()) return 0;
    return uxQueueMessagesWaitingFromISR(getHandle());
  }

  /**
   * @brief Get the number of available spaces to write to the queue.
   * @return uint32_t Number of available spaces.
   */
  uint32_t getAvailableSpaces() const override {
    if (!isCreated()) return 0;
    return uxQueueSpacesAvailable(getHandle());
  }

  /**
   * @brief Reset the queue.
   * @return true Queue reset successfully, false if the queue is not created.
   */
  bool reset() const override {
    if (!isCreated()) return false;
    xQueueReset(getHandle());
  }

  /**
   * @brief Check if the queue is full.
   * @return true Queue is full, false if the queue is not created.
   */
  bool isFull() const override {
    if (!isCreated()) return false;
    return uxQueueSpacesAvailable(getHandle()) == 0;
  }

  /**
   * @brief Check if the queue is empty.
   * @return true Queue is empty, false if the queue is not created.
   */
  bool isEmpty() const override {
    if (!isCreated()) return false;
    return uxQueueMessagesWaiting(getHandle()) == 0;
  }

  /**
   * @brief Check if the queue is full from an ISR.
   * @return true Queue is full, false if the queue is not created.
   */
  bool isFullFromISR() const override {
    if (!isCreated()) return false;
    return xQueueIsQueueFullFromISR(getHandle());
  }

  /**
   * @brief Check if the queue is empty from an ISR.
   * @return true Queue is empty, false if the queue is not created.
   */
  bool isEmptyFromISR() const override {
    if (!isCreated()) return false;
    return xQueueIsQueueEmptyFromISR(getHandle());
  }

  /**
   * @brief Check if the queue is created.
   * @return true Queue is created.
   */
  explicit operator bool() const override { return isCreated(); }

  /**
   * @brief Push an item to the front of the queue (LIFO order).
   * @param item Item to push.
   * @param ticks_to_wait Maximum time to wait for the queue to be available.
   * @return true Item pushed successfully, false if the queue is not created or the queue is full.
   */
  bool push(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    if (!isCreated()) return false;
    return xQueueSendToFront(getHandle(), &item, ticks_to_wait);
  }

  /**
   * @brief Push an item to the front of the queue (LIFO order) from an ISR.
   * @param item Item to push.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item pushed successfully, false if the queue is not created or the queue is full.
   */
  bool pushFromISR(const T& item, BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xQueueSendToFrontFromISR(getHandle(), &item, &task_woken);
  }

  /**
   * @brief Add an item to the back of the queue (FIFO order).
   * @param item Item to add.
   * @param ticks_to_wait Maximum time to wait for the queue to be available.
   * @return true Item added successfully, false if the queue is not created or the queue is full.
   */
  bool add(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    if (!isCreated()) return false;
    return xQueueSendToBack(getHandle(), &item, ticks_to_wait);
  }

  /**
   * @brief Add an item to the back of the queue (FIFO order) from an ISR.
   * @param item Item to add.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item added successfully, false if the queue is not created or the queue is full.
   */
  bool addFromISR(const T& item, BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xQueueSendToBackFromISR(getHandle(), &item, &task_woken);
  }

  /**
   * @brief Pop (remove) an item from the queue.
   * @param var Variable to store the item.
   * @param ticks_to_wait Maximum time to wait for the queue to be available.
   * @return true Item popped successfully, false if the queue is not created or the queue is empty.
   */
  bool pop(T& var, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    if (!isCreated()) return false;
    return xQueueReceive(getHandle(), &var, ticks_to_wait);
  }

  /**
   * @brief Pop (remove) an item from the queue from an ISR.
   * @param var Variable to store the item.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item popped successfully, false if the queue is not created or the queue is empty.
   */
  bool popFromISR(T& var, BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xQueueReceiveFromISR(getHandle(), &var, &task_woken);
  }

  /**
   * @brief Peek (read) an item from the queue without removing it.
   * @param var Variable to store the item.
   * @param ticks_to_wait Maximum time to wait for the queue to be available.
   * @return true Item read successfully, false if the queue is not created or the queue is empty.
   */
  bool peek(T& var, const TickType_t ticks_to_wait = 0) const {
    if (!isCreated()) return false;
    return xQueuePeek(getHandle(), &var, ticks_to_wait);
  }

  /**
   * @brief Peek (read) an item from the queue without removing it from an ISR.
   * @param var Variable to store the item.
   * @return true Item read successfully, false if the queue is not created or the queue is empty.
   */
  bool peekFromISR(T& var) const {
    if (!isCreated()) return false;
    return xQueuePeekFromISR(getHandle(), &var);
  }

  /**
   * @brief Overwrite an item in the queue. Use it only with queues of length 1.
   * @param item Item to overwrite.
   * @param ticks_to_wait Maximum time to wait for the queue to be available.
   * @return true Item overwritten successfully, false if the queue is not created.
   */
  bool overwrite(const T& item, const TickType_t ticks_to_wait = 0) const {
    if (!isCreated()) return false;
    return xQueueOverwrite(getHandle(), &item);
  }

  /**
   * @brief Overwrite an item in the queue from an ISR. Use it only with queues of length 1.
   * @param item Item to overwrite.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item overwritten successfully, false if the queue is not created.
   */
  bool overwriteFromISR(const T& item, BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    return xQueueOverwriteFromISR(getHandle(), &item, &task_woken);
  }
};

} // namespace Internal

template <typename T, uint32_t Length>
using QueueDynamic = Internal::Queue<Internal::DynamicPolicy<T, Length>, T>;

template <typename T, uint32_t Length>
using QueueStatic = Internal::Queue<Internal::StaticPolicy<T, Length>, T>;

template <typename T, uint32_t Length>
using QueueExternalStorage = Internal::Queue<Internal::ExternalStoragePolicy<T, Length>, T>;

} // namespace RTOS::Queues