/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_RINGBUFFER_H
#define RTOS_CPP_RINGBUFFER_H

#include <Arduino.h>
#include <freertos/ringbuf.h>

class RingBufBase {
  private:
  RingBufBase(const RingBufBase&)            = delete;
  RingBufBase& operator=(const RingBufBase&) = delete;

  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                         const RingBufBase& ring_buffer);

  protected:
  RingBufBase(RingbufHandle_t handle)
      : _handle(handle) {}
  ~RingBufBase() { vRingbufferDelete(_handle); }

  RingbufHandle_t _handle;

  public:
  RingbufHandle_t getHandle() { return _handle; }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const RingBufBase& ring_buffer) {
  return xRingbufferCanRead(ring_buffer._handle, queue_set_member);
}

template <typename T>
class RingBufNoSplitBase : public RingBufBase {
  protected:
  RingBufNoSplitBase(RingbufHandle_t handle)
      : RingBufBase(handle) {}

  public:
  bool send(const T* item, uint32_t item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferSend(_handle, (void*)item, item_size, ticks_to_wait);
  }

  T* receive(uint32_t& item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return (T*)xRingbufferReceive(_handle, &item_size, ticks_to_wait);
  }

  void returnItem(T* item) { vRingbufferReturnItem(_handle, (void*)item); }
};

template <typename T, uint32_t BUFFER_SIZE>
class RingBufNoSplit : public RingBufNoSplitBase<T> {
  public:
  RingBufNoSplit()
      : RingBufNoSplitBase<T>(
          xRingbufferCreateStatic(BUFFER_SIZE, RINGBUF_TYPE_NOSPLIT, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE];
};

template <typename T>
class RingBufNoSplitExtStorage : public RingBufNoSplitBase<T> {
  public:
  RingBufNoSplitExtStorage()
      : RingBufNoSplitBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* tcb, uint8_t* buffer_storage, uint32_t buffer_size) {
    this->_handle = xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_NOSPLIT, buffer_storage, tcb);

    return this->_handle != nullptr ? true : false;
  }
};

#endif // RTOS_CPP_RINGBUFFER_H