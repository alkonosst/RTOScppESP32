/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/ringbuf.h>

// Forward declaration of QueueSet
class QueueSet;

class RingBufferInterface {
  private:
  friend class QueueSet;
  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                         const RingBufferInterface& ring_buffer);

  protected:
  RingBufferInterface(const RingbufHandle_t handle)
      : _handle(handle) {}

  RingbufHandle_t _handle;

  public:
  virtual ~RingBufferInterface() {
    if (_handle) vRingbufferDelete(_handle);
  }

  RingBufferInterface(const RingBufferInterface&)                = delete;
  RingBufferInterface& operator=(const RingBufferInterface&)     = delete;
  RingBufferInterface(RingBufferInterface&&) noexcept            = delete;
  RingBufferInterface& operator=(RingBufferInterface&&) noexcept = delete;

  explicit operator bool() const { return _handle != nullptr; }
};

inline bool operator==(const QueueSetMemberHandle_t& queue_set_member,
                       const RingBufferInterface& ring_buffer) {
  return xRingbufferCanRead(ring_buffer._handle, queue_set_member);
}

template <typename T>
class RingBufferBase : public RingBufferInterface {
  protected:
  RingBufferBase(const RingbufHandle_t handle)
      : RingBufferInterface(handle) {}
  virtual ~RingBufferBase() {}

  public:
  bool send(const T* const item, const uint32_t item_size,
            const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xRingbufferSend(_handle, (void*)item, item_size, ticks_to_wait);
  }

  bool sendFromISR(const T* const item, const uint32_t item_size,
                   BaseType_t& higher_priority_task_woken) const {
    return xRingbufferSendFromISR(_handle, (void*)item, item_size, &higher_priority_task_woken);
  }

  void returnItem(const T* const item) const { vRingbufferReturnItem(_handle, (void*)item); }

  void returnItemFromISR(const T* const item, BaseType_t& higher_priority_task_woken) const {
    vRingbufferReturnItemFromISR(_handle, (void*)item, &higher_priority_task_woken);
  }
};

template <typename T>
class RingBufferNoSplitBase : public RingBufferBase<T> {
  protected:
  RingBufferNoSplitBase(const RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferNoSplitBase() {}

  public:
  T* receive(uint32_t& item_size, const TickType_t ticks_to_wait = portMAX_DELAY) {
    return (T*)xRingbufferReceive(this->_handle, &item_size, ticks_to_wait);
  }

  T* receiveFromISR(uint32_t& item_size) {
    return (T*)xRingbufferReceiveFromISR(this->_handle, &item_size);
  }
};

template <typename T>
class RingBufferNoSplitDynamic : public RingBufferNoSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferNoSplitDynamic(const uint32_t length)
      : RingBufferNoSplitBase<T>(xRingbufferCreate(length * ALIGNED_SIZE, RINGBUF_TYPE_NOSPLIT)) {}
};

template <typename T, uint32_t LENGTH>
class RingBufferNoSplitStatic : public RingBufferNoSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferNoSplitStatic()
      : RingBufferNoSplitBase<T>(
          xRingbufferCreateStatic(LENGTH * ALIGNED_SIZE, RINGBUF_TYPE_NOSPLIT, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[LENGTH * ALIGNED_SIZE];
};

template <typename T>
class RingBufferNoSplitExternalStorage : public RingBufferNoSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferNoSplitExternalStorage()
      : RingBufferNoSplitBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* const tcb, uint8_t* const buffer_storage,
              const uint32_t buffer_size) const {
    this->_handle = xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_NOSPLIT, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};

template <typename T>
class RingBufferSplitBase : public RingBufferBase<T> {
  protected:
  RingBufferSplitBase(const RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferSplitBase() {}

  public:
  bool receive(T** head, T** tail, uint32_t& head_item_size, uint32_t& tail_item_size,
               const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xRingbufferReceiveSplit(
      this->_handle, head, tail, &head_item_size, &tail_item_size, ticks_to_wait);
  }

  bool receiveFromISR(T** head, T** tail, uint32_t& head_item_size,
                      uint32_t& tail_item_size) const {
    return xRingbufferReceiveSplitFromISR(
      this->_handle, head, tail, &head_item_size, &tail_item_size);
  }
};

template <typename T>
class RingBufferSplitDynamic : public RingBufferSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferSplitDynamic(const uint32_t length)
      : RingBufferSplitBase<T>(xRingbufferCreate(length * ALIGNED_SIZE, RINGBUF_TYPE_ALLOWSPLIT)) {}
};

template <typename T, uint32_t LENGTH>
class RingBufferSplitStatic : public RingBufferSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferSplitStatic()
      : RingBufferSplitBase<T>(xRingbufferCreateStatic(LENGTH * ALIGNED_SIZE,
                                                       RINGBUF_TYPE_ALLOWSPLIT, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[LENGTH * ALIGNED_SIZE];
};

template <typename T>
class RingBufferSplitExternalStorage : public RingBufferSplitBase<T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  RingBufferSplitExternalStorage()
      : RingBufferSplitBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* tcb, uint8_t* buffer_storage, uint32_t buffer_size) const {
    this->_handle =
      xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_ALLOWSPLIT, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};

template <typename T>
class RingBufferByteBase : public RingBufferBase<T> {
  protected:
  RingBufferByteBase(const RingbufHandle_t handle)
      : RingBufferBase<T>(handle) {}
  virtual ~RingBufferByteBase() {}

  public:
  T* receiveUpTo(const uint32_t max_item_size, uint32_t& item_size,
                 const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return (T*)xRingbufferReceiveUpTo(this->_handle, &item_size, ticks_to_wait, max_item_size);
  }

  T* receiveUpToFromISR(const uint32_t max_item_size, uint32_t& item_size) const {
    return (T*)xRingbufferReceiveUpToFromISR(this->_handle, &item_size, max_item_size);
  }
};

template <typename T>
class RingBufferByteDynamic : public RingBufferByteBase<T> {
  public:
  RingBufferByteDynamic(const uint32_t length)
      : RingBufferByteBase<T>(xRingbufferCreate(length, RINGBUF_TYPE_BYTEBUF)) {}
};

template <typename T, uint32_t LENGTH>
class RingBufferByteStatic : public RingBufferByteBase<T> {
  public:
  RingBufferByteStatic()
      : RingBufferByteBase<T>(
          xRingbufferCreateStatic(LENGTH, RINGBUF_TYPE_BYTEBUF, _storage, &_tcb)) {}

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[LENGTH];
};

template <typename T>
class RingBufferByteExternalStorage : public RingBufferByteBase<T> {
  public:
  RingBufferByteExternalStorage()
      : RingBufferByteBase<T>(nullptr) {}

  bool create(StaticRingbuffer_t* const tcb, uint8_t* const buffer_storage,
              const uint32_t buffer_size) const {
    this->_handle = xRingbufferCreateStatic(buffer_size, RINGBUF_TYPE_BYTEBUF, buffer_storage, tcb);
    return this->_handle != nullptr ? true : false;
  }
};