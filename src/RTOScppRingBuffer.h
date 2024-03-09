/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_RINGBUFFER_H
#define RTOS_CPP_RINGBUFFER_H

#include <Arduino.h>
#include <freertos/ringbuf.h>

// Forward declaration of QueueSet
class QueueSet;

class RingBufferBase {
  private:
  RingBufferBase(const RingBufferBase&)            = delete; // Delete copy constructor
  RingBufferBase& operator=(const RingBufferBase&) = delete; // Delete copy assignment operator

  friend class QueueSet;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                         const RingBufferBase& ring_buffer);

  protected:
  RingBufferBase(RingbufHandle_t handle)
      : _handle(handle) {}
  virtual ~RingBufferBase() {
    if (_handle) vRingbufferDelete(_handle);
  }

  RingbufHandle_t _handle;
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const RingBufferBase& ring_buffer) {
  return xRingbufferCanRead(ring_buffer._handle, queue_set_member);
}

template <typename T>
class RingBufferNoSplitBase : public RingBufferBase {
  protected:
  RingBufferNoSplitBase(RingbufHandle_t handle)
      : RingBufferBase(handle) {}

  public:
  bool send(const T* item, uint32_t item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferSend(_handle, (void*)item, item_size, ticks_to_wait);
  }

  bool sendFromISR(const T* item, uint32_t item_size, BaseType_t& higher_priority_task_woken) {
    return xRingbufferSendFromISR(_handle, (void*)item, item_size, &higher_priority_task_woken);
  }

  T* receive(uint32_t& item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return (T*)xRingbufferReceive(_handle, &item_size, ticks_to_wait);
  }

  T* receiveFromISR(uint32_t& item_size) {
    return (T*)xRingbufferReceiveFromISR(_handle, &item_size);
  }

  void returnItem(T* item) { vRingbufferReturnItem(_handle, (void*)item); }
  void returnItemFromISR(T* item, BaseType_t& higher_priority_task_woken) {
    vRingbufferReturnItemFromISR(_handle, (void*)item, &higher_priority_task_woken);
  }
};

template <typename T>
class RingBufferNoSplitDynamic : public RingBufferNoSplitBase<T> {
  public:
  RingBufferNoSplitDynamic(uint32_t buffer_size)
      : RingBufferNoSplitBase<T>(xRingbufferCreate(buffer_size, RINGBUF_TYPE_NOSPLIT)) {}
};

template <typename T, uint32_t BUFFER_SIZE>
class RingBufferNoSplitStatic : public RingBufferNoSplitBase<T> {
  public:
  RingBufferNoSplitStatic()
      : RingBufferNoSplitBase<T>(
          xRingbufferCreateStatic(BUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE];
};

template <typename T>
class RingBufferNoSplitExternalStorage : public RingBufferNoSplitBase<T> {
  public:
  RingBufferNoSplitExternalStorage()
      : RingBufferNoSplitBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* tcb, uint8_t* buffer_storage, uint32_t buffer_size) {
    this->_handle = xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_NOSPLIT, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};

#endif // RTOS_CPP_RINGBUFFER_H