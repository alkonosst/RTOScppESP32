/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/ringbuf.h>

namespace RTOS::RingBuffers {

// Interface for RingBuffer objects, useful when using pointers
class IRingBuffer {
  protected:
  IRingBuffer() = default;

  public:
  IRingBuffer(const IRingBuffer&)            = delete;
  IRingBuffer& operator=(const IRingBuffer&) = delete;
  IRingBuffer(IRingBuffer&&)                 = delete;
  IRingBuffer& operator=(IRingBuffer&&)      = delete;

  virtual ~IRingBuffer() = default;

  virtual RingbufHandle_t getHandle() const = 0;

  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base policy class
template <typename Derived, typename T>
class Policy {
  public:
  RingbufHandle_t getHandle() const { return _handle; }

  bool send(const T* const item, const uint32_t item_size,
            const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xRingbufferSend(this->_handle, (void*)item, item_size, ticks_to_wait);
  }

  bool sendFromISR(const T* const item, const uint32_t item_size, BaseType_t& task_woken) const {
    return xRingbufferSendFromISR(this->_handle, (void*)item, item_size, &task_woken);
  }

  void returnItem(const T* const item) const { vRingbufferReturnItem(this->_handle, (void*)item); }

  void returnItemFromISR(const T* const item, BaseType_t& task_woken) const {
    vRingbufferReturnItemFromISR(this->_handle, (void*)item, &task_woken);
  }

  protected:
  Policy()
      : _handle(nullptr) {}

  RingbufHandle_t _handle;
};

// CRTP no-split base policy class
template <typename Derived, typename T>
class NoSplitPolicy : public Policy<Derived, T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  T* receive(uint32_t& item_size, const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferReceive(this->_handle, &item_size, ticks_to_wait);
  }

  T* receiveFromISR(uint32_t& item_size) {
    return xRingbufferReceiveFromISR(this->_handle, &item_size);
  }
};

// Policy for no-split ring buffer with dynamic memory allocation
template <typename T, uint32_t Length>
class NoSplitDynamicPolicy : public NoSplitPolicy<NoSplitDynamicPolicy<T, Length>, T> {
  public:
  NoSplitDynamicPolicy() {
    this->_handle =
      xRingbufferCreate(Length * NoSplitPolicy<NoSplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE,
                        RINGBUF_TYPE_NOSPLIT);
  }
};

// Policy for no-split ring buffer with static memory allocation
template <typename T, uint32_t Length>
class NoSplitStaticPolicy : public NoSplitPolicy<NoSplitStaticPolicy<T, Length>, T> {
  public:
  NoSplitStaticPolicy() {
    this->_handle = xRingbufferCreateStatic(
      Length * NoSplitPolicy<NoSplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE,
      RINGBUF_TYPE_NOSPLIT,
      _storage,
      &_tcb);
  }

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[Length * NoSplitPolicy<NoSplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE];
};

// Policy for no-split ring buffer with external storage
template <typename T>
class NoSplitExternalStoragePolicy : public NoSplitPolicy<NoSplitExternalStoragePolicy<T>, T> {
  public:
  bool create(uint8_t* const buffer, const uint32_t length) {
    this->_handle = xRingbufferCreateStatic(length, RINGBUF_TYPE_NOSPLIT, buffer, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _tcb;
};

// CRTP split base policy class
template <typename Derived, typename T>
class SplitPolicy : public Policy<Derived, T> {
  public:
  // Size aligned to nearest 4 bytes + 8 bytes per item
  static constexpr uint32_t ALIGNED_SIZE = 4 * ((sizeof(T) + 3) / 4) + 8;

  bool receive(T** head, T** tail, uint32_t& head_item_size, uint32_t& tail_item_size,
               const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferReceiveSplit(
      this->_handle, head, tail, &head_item_size, &tail_item_size, ticks_to_wait);
  }

  bool receiveFromISR(T** head, T** tail, uint32_t& head_item_size, uint32_t& tail_item_size) {
    return xRingbufferReceiveSplitFromISR(
      this->_handle, head, tail, &head_item_size, &tail_item_size);
  }
};

// Policy for split ring buffer with dynamic memory allocation
template <typename T, uint32_t Length>
class SplitDynamicPolicy : public SplitPolicy<SplitDynamicPolicy<T, Length>, T> {

  public:
  SplitDynamicPolicy() {
    this->_handle =
      xRingbufferCreate(Length * SplitPolicy<SplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE,
                        RINGBUF_TYPE_ALLOWSPLIT);
  }
};

// Policy for split ring buffer with static memory allocation
template <typename T, uint32_t Length>
class SplitStaticPolicy : public SplitPolicy<SplitStaticPolicy<T, Length>, T> {
  public:
  SplitStaticPolicy() {
    this->_handle =
      xRingbufferCreateStatic(Length * SplitPolicy<SplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE,
                              RINGBUF_TYPE_ALLOWSPLIT,
                              _storage,
                              &_tcb);
  }

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[Length * SplitPolicy<SplitDynamicPolicy<T, Length>, T>::ALIGNED_SIZE];
};

// Policy for split ring buffer with external storage
template <typename T>
class SplitExternalStoragePolicy : public SplitPolicy<SplitExternalStoragePolicy<T>, T> {
  public:
  bool create(uint8_t* const buffer, const uint32_t length) {
    this->_handle = xRingbufferCreateStatic(length, RINGBUF_TYPE_ALLOWSPLIT, buffer, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _tcb;
};

// CRTP byte base policy class
template <typename Derived>
class BytePolicy : public Policy<Derived, uint8_t> {
  public:
  uint8_t* receiveUpTo(const uint32_t max_item_size, uint32_t& item_size,
                       const TickType_t ticks_to_wait = portMAX_DELAY) {
    return xRingbufferReceiveUpTo(this->_handle, &item_size, ticks_to_wait, max_item_size);
  }

  uint8_t* receiveUpToFromISR(const uint32_t max_item_size, uint32_t& item_size) {
    return xRingbufferReceiveUpToFromISR(this->_handle, &item_size, max_item_size);
  }
};

// Policy for byte ring buffer with dynamic memory allocation
template <uint32_t Length>
class ByteDynamicPolicy : public BytePolicy<ByteDynamicPolicy<Length>> {
  public:
  ByteDynamicPolicy() { this->_handle = xRingbufferCreate(Length, RINGBUF_TYPE_BYTEBUF); }
};

// Policy for byte ring buffer with static memory allocation
template <uint32_t Length>
class ByteStaticPolicy : public BytePolicy<ByteStaticPolicy<Length>> {
  public:
  ByteStaticPolicy() {
    this->_handle = xRingbufferCreateStatic(Length, RINGBUF_TYPE_BYTEBUF, _storage, &_tcb);
  }

  private:
  StaticRingbuffer_t _tcb;
  uint8_t _storage[Length];
};

// Policy for byte ring buffer with external storage
template <uint8_t Dummy = 0>
class ByteExternalStoragePolicy : public BytePolicy<ByteExternalStoragePolicy<Dummy>> {
  public:
  bool create(uint8_t* const buffer, const uint32_t length) {
    this->_handle = xRingbufferCreateStatic(length, RINGBUF_TYPE_BYTEBUF, buffer, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _tcb;
};

// Main class for RingBuffer objects
template <typename Policy>
class RingBuffer : public IRingBuffer, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~RingBuffer() {
    if (Policy::getHandle()) vRingbufferDelete(Policy::getHandle());
  }

  RingbufHandle_t getHandle() const override { return Policy::getHandle(); }

  explicit operator bool() const override { return Policy::getHandle() != nullptr; }
};

} // namespace Internal

template <typename T, uint32_t Length>
using RingBufferNoSplitDynamic = Internal::RingBuffer<Internal::NoSplitDynamicPolicy<T, Length>>;

template <typename T, uint32_t Length>
using RingBufferNoSplitStatic = Internal::RingBuffer<Internal::NoSplitStaticPolicy<T, Length>>;

template <typename T>
using RingBufferNoSplitExternalStorage =
  Internal::RingBuffer<Internal::NoSplitExternalStoragePolicy<T>>;

template <typename T, uint32_t Length>
using RingBufferSplitDynamic = Internal::RingBuffer<Internal::SplitDynamicPolicy<T, Length>>;

template <typename T, uint32_t Length>
using RingBufferSplitStatic = Internal::RingBuffer<Internal::SplitStaticPolicy<T, Length>>;

template <typename T>
using RingBufferSplitExternalStorage =
  Internal::RingBuffer<Internal::SplitExternalStoragePolicy<T>>;

template <uint32_t Length>
using RingBufferByteDynamic = Internal::RingBuffer<Internal::ByteDynamicPolicy<Length>>;

template <uint32_t Length>
using RingBufferByteStatic = Internal::RingBuffer<Internal::ByteStaticPolicy<Length>>;

using RingBufferByteExternalStorage = Internal::RingBuffer<Internal::ByteExternalStoragePolicy<>>;

} // namespace RTOS::RingBuffers