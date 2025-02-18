/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/queue.h>

// Forward declaration of QueueSet
class QueueSet;

class QueueInterface {
  private:
  friend class QueueSet;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                         const QueueInterface& queue);

  protected:
  QueueInterface(QueueHandle_t handle)
      : _handle(handle) {}

  QueueHandle_t _handle;

  public:
  virtual ~QueueInterface() {
    if (_handle) vTaskDelete(_handle);
  }

  QueueInterface(const QueueInterface&)                = delete;
  QueueInterface& operator=(const QueueInterface&)     = delete;
  QueueInterface(QueueInterface&&) noexcept            = delete;
  QueueInterface& operator=(QueueInterface&&) noexcept = delete;

  QueueHandle_t getHandle() const { return _handle; }

  uint32_t getAvailableMessages() const { return uxQueueMessagesWaiting(_handle); }
  uint32_t getAvailableMessagesFromISR() const { return uxQueueMessagesWaitingFromISR(_handle); }
  uint32_t getAvailableSpaces() const { return uxQueueSpacesAvailable(_handle); }

  void reset() const { xQueueReset(_handle); }
  bool isFull() const { return uxQueueSpacesAvailable(_handle) == 0; }
  bool isEmpty() const { return uxQueueMessagesWaiting(_handle) == 0; }
  bool isFullFromISR() const { return xQueueIsQueueFullFromISR(_handle); }
  bool isEmptyFromISR() const { return xQueueIsQueueEmptyFromISR(_handle); }

  explicit operator bool() const { return _handle != nullptr; }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const QueueInterface& queue) {
  return queue_set_member == queue._handle;
}

template <typename T>
class _QueueBase : public QueueInterface {
  protected:
  _QueueBase(QueueHandle_t handle)
      : QueueInterface(handle) {}

  public:
  void overwrite(const T& item) const { xQueueOverwrite(_handle, &item); }

  bool push(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSendToFront(_handle, &item, ticks_to_wait);
  }

  bool add(const T& item, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSendToBack(_handle, &item, ticks_to_wait);
  }

  bool pop(T& var, const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueReceive(_handle, &var, ticks_to_wait);
  }

  bool peek(T& var, const TickType_t ticks_to_wait = 0) const {
    return xQueuePeek(_handle, &var, ticks_to_wait);
  }

  bool pushFromISR(const T& item, BaseType_t& task_woken) const {
    return xQueueSendToFrontFromISR(_handle, &item, &task_woken);
  }

  bool addFromISR(const T& item, BaseType_t& task_woken) const {
    return xQueueSendToBackFromISR(_handle, &item, &task_woken);
  }

  bool popFromISR(T& var, BaseType_t& task_woken) const {
    return xQueueReceiveFromISR(_handle, &var, &task_woken);
  }

  bool peekFromISR(T& var) const { return xQueuePeekFromISR(_handle, &var); }
};

template <typename T>
class QueueDynamic : public _QueueBase<T> {
  public:
  QueueDynamic(const uint32_t length)
      : _QueueBase<T>(xQueueCreate(length, sizeof(T))) {}
};

template <typename T, uint32_t LENGTH>
class QueueStatic : public _QueueBase<T> {
  public:
  QueueStatic()
      : _QueueBase<T>(xQueueCreateStatic(LENGTH, sizeof(T), _storage, &this->_tcb)) {}

  private:
  StaticQueue_t _tcb;
  uint8_t _storage[LENGTH * sizeof(T)];
};

template <typename T>
class QueueExternalStorage : public _QueueBase<T> {
  public:
  QueueExternalStorage()
      : _QueueBase<T>(nullptr) {}

  bool init(uint8_t* const buffer, const uint32_t length) {
    this->_handle = xQueueCreateStatic(length, sizeof(T), buffer, &this->_tcb);
    return (this->_handle != nullptr);
  }

  private:
  StaticQueue_t _tcb;
};