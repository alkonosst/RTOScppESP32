/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_QUEUE_H
#define RTOS_CPP_QUEUE_H

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
  virtual ~QueueInterface() {
    if (_handle) vQueueDelete(_handle);
  }

  QueueHandle_t _handle;
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const QueueInterface& queue) {
  return queue_set_member == queue._handle;
}

template <typename T>
class QueueBase : public QueueInterface {
  private:
  QueueBase(QueueBase const&)      = delete; // Delete copy constructor
  void operator=(QueueBase const&) = delete; // Delete copy assignment operator

  protected:
  QueueBase(QueueHandle_t handle)
      : QueueInterface(handle) {}
  virtual ~QueueBase() {}

  public:
  uint32_t getAvailableMessages() { return uxQueueMessagesWaiting(_handle); }
  uint32_t getAvailableMessagesFromISR() { return uxQueueMessagesWaitingFromISR(_handle); }
  uint32_t getAvailableSpaces() { return uxQueueSpacesAvailable(_handle); }
  void reset() { xQueueReset(_handle); }
  bool isFull() { return uxQueueSpacesAvailable(_handle) == 0; }
  bool isEmpty() { return uxQueueMessagesWaiting(_handle) == 0; }
  bool isFullFromISR() { return xQueueIsQueueFullFromISR(_handle); }
  bool isEmptyFromISR() { return xQueueIsQueueEmptyFromISR(_handle); }

  bool push(const T& item, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xQueueSendToFront(_handle, &item, ticks_to_wait);
  }

  bool add(const T& item, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xQueueSendToBack(_handle, &item, ticks_to_wait);
  }

  bool pop(T& var, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xQueueReceive(_handle, &var, ticks_to_wait);
  }

  bool peek(T& var, TickType_t ticks_to_wait = 0) {
    return xQueuePeek(_handle, &var, ticks_to_wait);
  }

  bool pushFromISR(const T& item, BaseType_t& task_woken) {
    return xQueueSendToFrontFromISR(_handle, &item, &task_woken);
  }

  bool addFromISR(const T& item, BaseType_t& task_woken) {
    return xQueueSendToBackFromISR(_handle, &item, &task_woken);
  }

  bool popFromISR(T& var, BaseType_t& task_woken) {
    return xQueueReceiveFromISR(_handle, &var, &task_woken);
  }

  bool peekFromISR(T& var) { return xQueuePeekFromISR(_handle, &var); }
};

template <typename T>
class QueueDynamic : public QueueBase<T> {
  public:
  QueueDynamic(uint32_t length)
      : QueueBase<T>(xQueueCreate(length, sizeof(T))) {}
};

template <typename T, uint32_t LENGTH>
class QueueStatic : public QueueBase<T> {
  public:
  QueueStatic()
      : QueueBase<T>(xQueueCreateStatic(LENGTH, sizeof(T), _storage, &this->_tcb)) {}

  private:
  StaticQueue_t _tcb;
  uint8_t _storage[LENGTH * sizeof(T)];
};

template <typename T>
class QueueStaticExternalStorage : public QueueBase<T> {
  public:
  QueueStaticExternalStorage()
      : QueueBase<T>(nullptr) {}

  bool init(uint8_t* buffer, uint32_t buffer_size) {
    this->_handle = xQueueCreateStatic(buffer_size, sizeof(T), buffer, &this->_tcb);
    return this->_handle != nullptr ? true : false;
  }

  private:
  StaticQueue_t _tcb;
};

#endif // RTOS_CPP_QUEUE_H