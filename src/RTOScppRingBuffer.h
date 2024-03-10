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

class RingBufferInterface {
  private:
  RingBufferInterface(const RingBufferInterface&)            = delete; // Delete copy constructor
  RingBufferInterface& operator=(const RingBufferInterface&) = delete; // Delete copy assignment op.

  friend class QueueSet;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                         const RingBufferInterface& ring_buffer);

  protected:
  RingBufferInterface(RingbufHandle_t handle)
      : _handle(handle) {}
  virtual ~RingBufferInterface() {
    if (_handle) vRingbufferDelete(_handle);
  }

  RingbufHandle_t _handle;

  public:
  explicit operator bool() const { return _handle != nullptr; }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const RingBufferInterface& ring_buffer) {
  return xRingbufferCanRead(ring_buffer._handle, queue_set_member);
}

template <typename T>
class RingBufferBase : public RingBufferInterface {
  protected:
  RingBufferBase(RingbufHandle_t handle)
      : RingBufferInterface(handle) {}
  virtual ~RingBufferBase() {}

  public:
  bool send(const T* item, uint32_t item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferSend(_handle, (void*)item, item_size, ticks_to_wait);
  }

  bool sendFromISR(const T* item, uint32_t item_size, BaseType_t& higher_priority_task_woken) {
    return xRingbufferSendFromISR(_handle, (void*)item, item_size, &higher_priority_task_woken);
  }

  void returnItem(T* item) { vRingbufferReturnItem(_handle, (void*)item); }
  void returnItemFromISR(T* item, BaseType_t& higher_priority_task_woken) {
    vRingbufferReturnItemFromISR(_handle, (void*)item, &higher_priority_task_woken);
  }
};

template <typename T>
class RingBufferNoSplitBase : public RingBufferBase<T> {
  protected:
  RingBufferNoSplitBase(RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferNoSplitBase() {}

  public:
  T* receive(uint32_t& item_size, TickType_t ticks_to_wait = portMAX_DELAY) {
    return (T*)xRingbufferReceive(this->_handle, &item_size, ticks_to_wait);
  }

  T* receiveFromISR(uint32_t& item_size) {
    return (T*)xRingbufferReceiveFromISR(this->_handle, &item_size);
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

template <typename T>
class RingBufferSplitBase : public RingBufferBase<T> {
  protected:
  RingBufferSplitBase(RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferSplitBase() {}

  public:
  bool receive(T** head, T** tail, uint32_t& head_item_size, uint32_t& tail_item_size,
               TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferReceiveSplit(
      this->_handle, head, tail, &head_item_size, &tail_item_size, ticks_to_wait);
  }

  bool receiveFromISR(T** head, T** tail, uint32_t& head_item_size, uint32_t& tail_item_size) {
    return xRingbufferReceiveSplitFromISR(
      this->_handle, head, tail, &head_item_size, &tail_item_size);
  }
};

template <typename T>
class RingBufferSplitDynamic : public RingBufferSplitBase<T> {
  public:
  RingBufferSplitDynamic(uint32_t buffer_size)
      : RingBufferSplitBase<T>(xRingbufferCreate(buffer_size, RINGBUF_TYPE_ALLOWSPLIT)) {}
};

template <typename T, uint32_t BUFFER_SIZE>
class RingBufferSplitStatic : public RingBufferSplitBase<T> {
  public:
  RingBufferSplitStatic()
      : RingBufferSplitBase<T>(
          xRingbufferCreateStatic(BUFFER_SIZE, RINGBUF_TYPE_ALLOWSPLIT, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE];
};

template <typename T>
class RingBufferSplitExternalStorage : public RingBufferSplitBase<T> {
  public:
  RingBufferSplitExternalStorage()
      : RingBufferSplitBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* tcb, uint8_t* buffer_storage, uint32_t buffer_size) {
    this->_handle =
      xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_ALLOWSPLIT, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};

template <typename T>
class RingBufferByteBase : public RingBufferBase<T> {
  protected:
  RingBufferByteBase(RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferByteBase() {}

  public:
  T* receiveUpTo(uint32_t max_item_size, uint32_t& item_size,
                 TickType_t ticks_to_wait = portMAX_DELAY) {
    return (T*)xRingbufferReceiveUpTo(this->_handle, &item_size, ticks_to_wait, max_item_size);
  }

  T* receiveUpToFromISR(uint32_t max_item_size, uint32_t& item_size) {
    return (T*)xRingbufferReceiveUpToFromISR(this->_handle, &item_size, max_item_size);
  }
};

template <typename T>
class RingBufferByteDynamic : public RingBufferByteBase<T> {
  public:
  RingBufferByteDynamic(uint32_t buffer_size)
      : RingBufferByteBase<T>(xRingbufferCreate(buffer_size, RINGBUF_TYPE_BYTEBUF)) {}
};

template <typename T, uint32_t BUFFER_SIZE>
class RingBufferByteStatic : public RingBufferByteBase<T> {
  public:
  RingBufferByteStatic()
      : RingBufferByteBase<T>(
          xRingbufferCreateStatic(BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE];
};

template <typename T>
class RingBufferByteExternalStorage : public RingBufferByteBase<T> {
  public:
  RingBufferByteExternalStorage()
      : RingBufferByteBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* tcb, uint8_t* buffer_storage, uint32_t buffer_size) {
    this->_handle = xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_BYTEBUF, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};

#endif // RTOS_CPP_RINGBUFFER_H