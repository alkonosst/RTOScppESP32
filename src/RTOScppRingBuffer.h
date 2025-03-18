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
  virtual ~IRingBuffer()                     = default;

  /**
   * @brief Get the low-level handle of the ringbuffer. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return RingbufHandle_t Ringbuffer handle, nullptr if the Ringbuffer is not created.
   */
  virtual RingbufHandle_t getHandle() const = 0;

  /**
   * @brief Check if the ringbuffer is created.
   * @return true Ringbuffer is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Get the maximum size of an item that can be placed in the ring buffer.
   * @return size_t Maximum size, in bytes, of an item that can be placed in a ring buffer.
   */
  virtual size_t getMaxItemSize() const = 0;

  /**
   * @brief Get the current free size available for an item/data in the buffer.
   * @return size_t Free size available for an item/data in the buffer.
   */
  virtual size_t getFreeSize() const = 0;

  /**
   * @brief Check if the ringbuffer is created.
   * @return true Ringbuffer is created.
   */
  virtual explicit operator bool() const = 0;

  friend bool operator==(const QueueSetMemberHandle_t& queue_set_member,
    const IRingBuffer& ringbuf);
};

// Comparison operator for QueueSet
inline bool operator==(const QueueSetMemberHandle_t& queue_set_member, const IRingBuffer& ringbuf) {
  return xRingbufferCanRead(ringbuf.getHandle(), queue_set_member);
}

namespace Internal {

// CRTP base policy class
template <typename Derived, typename T>
class Policy {
  public:
  /**
   * @brief Get the low-level handle of the ringbuffer. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return RingbufHandle_t Ringbuffer handle, nullptr if the Ringbuffer is not created.
   */
  RingbufHandle_t getHandle() const { return _handle; }

  /**
   * @brief Check if the ringbuffer is created.
   * @return true Ringbuffer is created.
   */
  bool isCreated() const { return _handle != nullptr; }

  /**
   * @brief Send an item to the ring buffer.
   * @param item Item to send.
   * @param item_size Size of the item to send.
   * @param ticks_to_wait Maximum time to wait for the item to be sent.
   * @return true Item sent successfully, false if the ring buffer is not created or failed to send
   * the item.
   */
  bool send(const T* const item, const size_t item_size,
    const TickType_t ticks_to_wait = portMAX_DELAY) const {
    if (!isCreated()) return false;
    return xRingbufferSend(this->_handle, item, item_size, ticks_to_wait);
  }

  /**
   * @brief Send an item to the ring buffer from an ISR.
   * @param item Item to send.
   * @param item_size Size of the item to send.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item sent successfully, false if the ring buffer is not created or failed to send
   * the item.
   */
  bool sendFromISR(const T* const item, const size_t item_size, BaseType_t& task_woken) const {
    return xRingbufferSendFromISR(this->_handle, item, item_size, &task_woken);
  }

  /**
   * @brief Return an item to the ring buffer after using it.
   * @param item Item to return.
   * @return true Item returned successfully, false if the ring buffer is not created.
   */
  bool returnItem(T* const item) const {
    if (!isCreated()) return false;
    vRingbufferReturnItem(this->_handle, item);
    return true;
  }

  /**
   * @brief Return an item to the ring buffer after using it from an ISR.
   * @param item Item to return.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return true Item returned successfully, false if the ring buffer is not created.
   */
  bool returnItemFromISR(T* const item, BaseType_t& task_woken) const {
    if (!isCreated()) return false;
    vRingbufferReturnItemFromISR(this->_handle, item, &task_woken);
    return true;
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
  /**
   * @brief Receive an item from the ring buffer.
   * @param item_size Size of the item received.
   * @param ticks_to_wait Maximum time to wait for the item to be received.
   * @return T* Item received, nullptr if the ring buffer is not created or failed to receive the
   * item.
   */
  T* receive(size_t& item_size, const TickType_t ticks_to_wait = portMAX_DELAY) {
    if (!this->isCreated()) return nullptr;
    return static_cast<T*>(xRingbufferReceive(this->_handle, &item_size, ticks_to_wait));
  }

  /**
   * @brief Receive an item from the ring buffer from an ISR.
   * @param item_size Size of the item received.
   * @return T* Item received, nullptr if the ring buffer is not created or failed to receive the
   * item.
   */
  T* receiveFromISR(size_t& item_size) {
    if (!this->isCreated()) return nullptr;
    return static_cast<T*>(xRingbufferReceiveFromISR(this->_handle, &item_size));
  }
};

// Policy for no-split ring buffer with dynamic memory allocation
template <typename T, size_t Length>
class NoSplitDynamicPolicy : public NoSplitPolicy<NoSplitDynamicPolicy<T, Length>, T> {
  public:
  NoSplitDynamicPolicy() { this->_handle = xRingbufferCreate(Length, RINGBUF_TYPE_NOSPLIT); }
};

// Policy for no-split ring buffer with static memory allocation
template <typename T, size_t Length>
class NoSplitStaticPolicy : public NoSplitPolicy<NoSplitStaticPolicy<T, Length>, T> {
  private:
  // Size aligned to nearest 4 bytes
  static constexpr size_t REQUIRED_SIZE = 4 * ((Length + 3) / 4);

  public:
  NoSplitStaticPolicy() {
    this->_handle =
      xRingbufferCreateStatic(REQUIRED_SIZE, RINGBUF_TYPE_NOSPLIT, _storage, &_ringbuf_buffer);
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
  uint8_t _storage[REQUIRED_SIZE];
};

// Policy for no-split ring buffer with external storage
template <typename T, size_t Length>
class NoSplitExternalStoragePolicy
    : public NoSplitPolicy<NoSplitExternalStoragePolicy<T, Length>, T> {
  public:
  // Size aligned to nearest 4 bytes
  static constexpr size_t REQUIRED_SIZE = 4 * ((Length + 3) / 4);

  /**
   * @brief Create the ring buffer with external storage.
   * @param buffer External storage buffer.
   * @return true Ring buffer created successfully, false if the buffer is nullptr or failed to
   * create it.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;

    this->_handle =
      xRingbufferCreateStatic(REQUIRED_SIZE, RINGBUF_TYPE_NOSPLIT, buffer, &_ringbuf_buffer);

    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
};

// CRTP split base policy class
template <typename Derived, typename T>
class SplitPolicy : public Policy<Derived, T> {
  public:
  /**
   * @brief Receive an item from the ring buffer.
   * @param head Pointer to the head item received.
   * @param tail Pointer to the tail item received.
   * @param head_item_size Size of the head item received.
   * @param tail_item_size Size of the tail item received.
   * @param ticks_to_wait Maximum time to wait for the item to be received.
   * @return true Items received successfully, false if the ring buffer is not created or failed to
   * receive the items.
   */
  bool receive(T*& head, T*& tail, size_t& head_item_size, size_t& tail_item_size,
    const TickType_t ticks_to_wait = portMAX_DELAY) {
    if (!this->isCreated()) return false;

    return xRingbufferReceiveSplit(this->_handle,
      reinterpret_cast<void**>(&head),
      reinterpret_cast<void**>(&tail),
      &head_item_size,
      &tail_item_size,
      ticks_to_wait);
  }

  /**
   * @brief Receive an item from the ring buffer from an ISR.
   * @param head Pointer to the head item received.
   * @param tail Pointer to the tail item received.
   * @param head_item_size Size of the head item received.
   * @param tail_item_size Size of the tail item received.
   * @return true Items received successfully, false if the ring buffer is not created or failed to
   * receive the items.
   */
  bool receiveFromISR(T*& head, T*& tail, size_t& head_item_size, size_t& tail_item_size) {
    if (!this->isCreated()) return false;

    return xRingbufferReceiveSplitFromISR(this->_handle,
      reinterpret_cast<void**>(&head),
      reinterpret_cast<void**>(&tail),
      &head_item_size,
      &tail_item_size);
  }
};

// Policy for split ring buffer with dynamic memory allocation
template <typename T, size_t Length>
class SplitDynamicPolicy : public SplitPolicy<SplitDynamicPolicy<T, Length>, T> {
  private:
  // Size aligned to nearest 4 bytes
  static constexpr size_t REQUIRED_SIZE = 4 * ((Length + 3) / 4);

  public:
  SplitDynamicPolicy() {
    this->_handle = xRingbufferCreate(REQUIRED_SIZE, RINGBUF_TYPE_ALLOWSPLIT);
  }
};

// Policy for split ring buffer with static memory allocation
template <typename T, size_t Length>
class SplitStaticPolicy : public SplitPolicy<SplitStaticPolicy<T, Length>, T> {
  private:
  // Size aligned to nearest 4 bytes
  static constexpr size_t REQUIRED_SIZE = 4 * ((Length + 3) / 4);

  public:
  SplitStaticPolicy() {
    this->_handle =
      xRingbufferCreateStatic(REQUIRED_SIZE, RINGBUF_TYPE_ALLOWSPLIT, _storage, &_ringbuf_buffer);
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
  uint8_t _storage[REQUIRED_SIZE];
};

// Policy for split ring buffer with external storage
template <typename T, size_t Length>
class SplitExternalStoragePolicy : public SplitPolicy<SplitExternalStoragePolicy<T, Length>, T> {
  public:
  // Size aligned to nearest 4 bytes
  static constexpr size_t REQUIRED_SIZE = 4 * ((Length + 3) / 4);

  /**
   * @brief Create the ring buffer with external storage.
   * @param buffer External storage buffer.
   * @return true Ring buffer created successfully, false if the buffer is nullptr or failed to
   * create it.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;

    this->_handle =
      xRingbufferCreateStatic(REQUIRED_SIZE, RINGBUF_TYPE_ALLOWSPLIT, buffer, &_ringbuf_buffer);

    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
};

// CRTP byte base policy class
template <typename Derived>
class BytePolicy : public Policy<Derived, uint8_t> {
  public:
  /**
   * @brief Receive an item from the ring buffer.
   * @param max_item_size Maximum size of the item to receive.
   * @param item_size Size of the item received.
   * @param ticks_to_wait Maximum time to wait for the item to be received.
   * @return uint8_t* Item received, nullptr if the ring buffer is not created or failed to receive
   * the item.
   */
  uint8_t* receiveUpTo(const size_t max_item_size, size_t& item_size,
    const TickType_t ticks_to_wait = portMAX_DELAY) {
    if (!this->isCreated()) return nullptr;

    return static_cast<uint8_t*>(
      xRingbufferReceiveUpTo(this->_handle, &item_size, ticks_to_wait, max_item_size));
  }

  /**
   * @brief Receive an item from the ring buffer from an ISR.
   * @param max_item_size Maximum size of the item to receive.
   * @param item_size Size of the item received.
   * @return uint8_t* Item received, nullptr if the ring buffer is not created or failed to receive
   * the item.
   */
  uint8_t* receiveUpToFromISR(const size_t max_item_size, size_t& item_size) {
    if (!this->isCreated()) return nullptr;

    return static_cast<uint8_t*>(
      xRingbufferReceiveUpToFromISR(this->_handle, &item_size, max_item_size));
  }
};

// Policy for byte ring buffer with dynamic memory allocation
template <size_t Length>
class ByteDynamicPolicy : public BytePolicy<ByteDynamicPolicy<Length>> {
  public:
  ByteDynamicPolicy() { this->_handle = xRingbufferCreate(Length, RINGBUF_TYPE_BYTEBUF); }
};

// Policy for byte ring buffer with static memory allocation
template <size_t Length>
class ByteStaticPolicy : public BytePolicy<ByteStaticPolicy<Length>> {
  public:
  ByteStaticPolicy() {
    this->_handle =
      xRingbufferCreateStatic(Length, RINGBUF_TYPE_BYTEBUF, _storage, &_ringbuf_buffer);
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
  uint8_t _storage[Length];
};

// Policy for byte ring buffer with external storage
template <size_t Length>
class ByteExternalStoragePolicy : public BytePolicy<ByteExternalStoragePolicy<Length>> {
  public:
  static constexpr size_t REQUIRED_SIZE = Length;

  /**
   * @brief Create the ring buffer with external storage.
   * @param buffer External storage buffer.
   * @return true Ring buffer created successfully, false if the buffer is nullptr or failed to
   * create it.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;

    this->_handle = xRingbufferCreateStatic(Length, RINGBUF_TYPE_BYTEBUF, buffer, &_ringbuf_buffer);
    return this->_handle != nullptr;
  }

  private:
  StaticRingbuffer_t _ringbuf_buffer;
};

// Main class for RingBuffer objects
template <typename Policy>
class RingBuffer : public IRingBuffer, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~RingBuffer() {
    if (Policy::getHandle()) vRingbufferDelete(Policy::getHandle());
  }

  /**
   * @brief Get the low-level handle of the ringbuffer. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return RingbufHandle_t Ringbuffer handle, nullptr if the Ringbuffer is not created.
   */
  RingbufHandle_t getHandle() const override { return Policy::getHandle(); }

  /**
   * @brief Check if the ringbuffer is created.
   * @return true Ringbuffer is created.
   */
  bool isCreated() const override { return Policy::isCreated(); }

  /**
   * @brief Get the maximum size of an item that can be placed in the ring buffer.
   * @return size_t Maximum size, in bytes, of an item that can be placed in a ring buffer.
   */
  size_t getMaxItemSize() const override {
    if (!isCreated()) return 0;
    return xRingbufferGetMaxItemSize(getHandle());
  }

  /**
   * @brief Get the current free size available for an item/data in the buffer.
   * @return size_t Free size available for an item/data in the buffer.
   */
  size_t getFreeSize() const override {
    if (!isCreated()) return 0;
    return xRingbufferGetCurFreeSize(getHandle());
  }

  /**
   * @brief Check if the ringbuffer is created.
   * @return true Ringbuffer is created.
   */
  explicit operator bool() const override { return isCreated(); }
};

} // namespace Internal

template <typename T, size_t Length>
using RingBufferNoSplitDynamic = Internal::RingBuffer<Internal::NoSplitDynamicPolicy<T, Length>>;

template <typename T, size_t Length>
using RingBufferNoSplitStatic = Internal::RingBuffer<Internal::NoSplitStaticPolicy<T, Length>>;

template <typename T, size_t Length>
using RingBufferNoSplitExternalStorage =
  Internal::RingBuffer<Internal::NoSplitExternalStoragePolicy<T, Length>>;

template <typename T, size_t Length>
using RingBufferSplitDynamic = Internal::RingBuffer<Internal::SplitDynamicPolicy<T, Length>>;

template <typename T, size_t Length>
using RingBufferSplitStatic = Internal::RingBuffer<Internal::SplitStaticPolicy<T, Length>>;

template <typename T, size_t Length>
using RingBufferSplitExternalStorage =
  Internal::RingBuffer<Internal::SplitExternalStoragePolicy<T, Length>>;

template <size_t Length>
using RingBufferByteDynamic = Internal::RingBuffer<Internal::ByteDynamicPolicy<Length>>;

template <size_t Length>
using RingBufferByteStatic = Internal::RingBuffer<Internal::ByteStaticPolicy<Length>>;

template <size_t Length>
using RingBufferByteExternalStorage =
  Internal::RingBuffer<Internal::ByteExternalStoragePolicy<Length>>;

} // namespace RTOS::RingBuffers