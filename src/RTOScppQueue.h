/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_QUEUE_H
#define RTOS_CPP_QUEUE_H

#include <Arduino.h>
#include <freertos/queue.h>

template <typename T>
class QueueBase {
  private:
  QueueBase(QueueBase const&)      = delete;
  void operator=(QueueBase const&) = delete;

  protected:
  QueueBase(QueueHandle_t handle)
      : _handle(handle) {}
  ~QueueBase() { vQueueDelete(_handle); }

  StaticQueue_t _tcb;
  QueueHandle_t _handle;

  public:
  uint32_t availableMessages() { return uxQueueMessagesWaiting(_handle); }
  uint32_t availableMessagesFromISR() { return uxQueueMessagesWaitingFromISR(_handle); }
  uint32_t availableSpaces() { return uxQueueSpacesAvailable(_handle); }
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

template <typename T, uint32_t LENGTH>
class Queue : public QueueBase<T> {
  public:
  Queue()
      : QueueBase<T>(xQueueCreateStatic(LENGTH, sizeof(T), _storage, &this->_tcb)) {}

  private:
  uint8_t _storage[LENGTH * sizeof(T)];
};

template <typename T>
class QueueExternalStorage : public QueueBase<T> {
  public:
  QueueExternalStorage()
      : QueueBase<T>(nullptr) {}

  bool init(uint8_t* buffer, uint32_t buffer_size) {
    this->_handle = xQueueCreateStatic(buffer_size, sizeof(T), buffer, &this->_tcb);
    return this->_handle != nullptr ? true : false;
  }
};

#endif // RTOS_CPP_QUEUE_H