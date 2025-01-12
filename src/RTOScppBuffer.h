/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/message_buffer.h>
#include <freertos/stream_buffer.h>

namespace RTOS::Buffers {

// Interface for Buffer objects, useful when using pointers
class IBuffer {
  protected:
  IBuffer() = default;

  public:
  IBuffer(const IBuffer&)            = delete;
  IBuffer& operator=(const IBuffer&) = delete;
  IBuffer(IBuffer&&)                 = delete;
  IBuffer& operator=(IBuffer&&)      = delete;

  virtual ~IBuffer() = default;

  virtual StreamBufferHandle_t getHandle() const = 0;

  virtual uint32_t send(const void* tx_buffer, const uint32_t bytes,
                        const TickType_t ticks_to_wait = portMAX_DELAY) const    = 0;
  virtual uint32_t sendFromISR(const void* tx_buffer, const uint32_t bytes,
                               BaseType_t& task_woken) const                     = 0;
  virtual uint32_t receive(void* rx_buffer, const uint32_t bytes,
                           const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;
  virtual uint32_t receiveFromISR(void* rx_buffer, const uint32_t bytes,
                                  BaseType_t& task_woken) const                  = 0;

  virtual bool reset() const               = 0;
  virtual bool isEmpty() const             = 0;
  virtual bool isFull() const              = 0;
  virtual uint32_t availableSpaces() const = 0;
  virtual uint32_t availableBytes() const  = 0;

  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base policy class
template <typename Derived>
class Policy {
  public:
  StreamBufferHandle_t getHandle() const { return _handle; }

  protected:
  Policy()
      : _handle(nullptr) {}

  StreamBufferHandle_t _handle;
};

// CRTP stream buffer base policy class
template <typename Derived>
class StreamBufferPolicy : public Policy<StreamBufferPolicy<Derived>> {
  public:
  bool setTriggerLevel(const uint32_t trigger_bytes) {
    return xStreamBufferSetTriggerLevel(this->_handle, trigger_bytes);
  }
};

// Policy for stream buffer with dynamic memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferDynamicPolicy
    : public StreamBufferPolicy<StreamBufferDynamicPolicy<BufferSize, TriggerBytes>> {
  public:
  StreamBufferDynamicPolicy() {
    this->_handle = xStreamBufferGenericCreate(BufferSize, TriggerBytes, false);
  }
};

// Policy for data buffer with static memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferStaticPolicy
    : public StreamBufferPolicy<StreamBufferStaticPolicy<BufferSize, TriggerBytes>> {
  public:
  StreamBufferStaticPolicy() {
    this->_handle =
      xStreamBufferGenericCreateStatic(BufferSize + 1, TriggerBytes, false, _storage, &_tcb);
  }

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BufferSize + 2];
};

// Policy for data buffer with external memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferExternalStoragePolicy
    : public StreamBufferPolicy<StreamBufferExternalStoragePolicy<BufferSize, TriggerBytes>> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = BufferSize + 2;

  bool create(uint8_t* const buffer) {
    this->_handle =
      xStreamBufferGenericCreateStatic(BufferSize + 1, TriggerBytes, false, buffer, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticStreamBuffer_t _tcb;
};

// Policy for message buffer with dynamic memory allocation
template <uint32_t BufferSize>
class MessageBufferDynamicPolicy : public Policy<MessageBufferDynamicPolicy<BufferSize>> {
  public:
  MessageBufferDynamicPolicy() { this->_handle = xStreamBufferGenericCreate(BufferSize, 0, true); }
};

// Policy for message buffer with static memory allocation
template <uint32_t BufferSize>
class MessageBufferStaticPolicy : public Policy<MessageBufferStaticPolicy<BufferSize>> {
  public:
  MessageBufferStaticPolicy() {
    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1, 0, true, _storage, &_tcb);
  }

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BufferSize + 2];
};

// Policy for message buffer with external memory allocation
template <uint32_t BufferSize>
class MessageBufferExternalStoragePolicy
    : public Policy<MessageBufferExternalStoragePolicy<BufferSize>> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = BufferSize + 2;

  bool create(uint8_t* const buffer) {
    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1, 0, true, buffer, &_tcb);
    return this->_handle != nullptr;
  }

  private:
  StaticStreamBuffer_t _tcb;
};

// Main DataBuffer class. You need to specify the policy used
template <typename Policy>
class DataBuffer : public IBuffer, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~DataBuffer() {
    if (Policy::getHandle()) vStreamBufferDelete(Policy::getHandle());
  }

  StreamBufferHandle_t getHandle() const override { return Policy::getHandle(); }

  uint32_t send(const void* tx_buffer, const uint32_t bytes,
                const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xStreamBufferSend(getHandle(), tx_buffer, bytes, ticks_to_wait);
  }

  uint32_t sendFromISR(const void* tx_buffer, const uint32_t bytes,
                       BaseType_t& task_woken) const override {
    return xStreamBufferSendFromISR(getHandle(), tx_buffer, bytes, &task_woken);
  }

  uint32_t receive(void* rx_buffer, const uint32_t bytes,
                   const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    return xStreamBufferReceive(getHandle(), rx_buffer, bytes, ticks_to_wait);
  }

  uint32_t receiveFromISR(void* rx_buffer, const uint32_t bytes,
                          BaseType_t& task_woken) const override {
    return xStreamBufferReceiveFromISR(getHandle(), rx_buffer, bytes, &task_woken);
  }

  bool reset() const override { return xStreamBufferReset(getHandle()); }

  bool isEmpty() const override { return xStreamBufferIsEmpty(getHandle()); }

  bool isFull() const override { return xStreamBufferIsFull(getHandle()); }

  uint32_t availableSpaces() const override { return xStreamBufferSpacesAvailable(getHandle()); }

  uint32_t availableBytes() const override { return xStreamBufferBytesAvailable(getHandle()); }

  explicit operator bool() const override { return getHandle() != nullptr; }
};

} // namespace Internal

template <uint32_t BufferSize, uint32_t TriggerBytes>
using StreamBufferDynamic =
  Internal::DataBuffer<Internal::StreamBufferDynamicPolicy<BufferSize, TriggerBytes>>;

template <uint32_t BufferSize, uint32_t TriggerBytes>
using StreamBufferStatic =
  Internal::DataBuffer<Internal::StreamBufferStaticPolicy<BufferSize, TriggerBytes>>;

template <uint32_t BufferSize, uint32_t TriggerBytes>
using StreamBufferExternalStorage =
  Internal::DataBuffer<Internal::StreamBufferExternalStoragePolicy<BufferSize, TriggerBytes>>;

template <uint32_t BufferSize>
using MessageBufferDynamic = Internal::DataBuffer<Internal::MessageBufferDynamicPolicy<BufferSize>>;

template <uint32_t BufferSize>
using MessageBufferStatic = Internal::DataBuffer<Internal::MessageBufferStaticPolicy<BufferSize>>;

template <uint32_t BufferSize>
using MessageBufferExternalStorage =
  Internal::DataBuffer<Internal::MessageBufferExternalStoragePolicy<BufferSize>>;

} // namespace RTOS::Buffers