/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
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
  virtual ~IBuffer()                 = default;

  /**
   * @brief Get the low-level handle of the buffer. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return StreamBufferHandle_t Buffer handle, nullptr if the buffer is not created.
   */
  virtual StreamBufferHandle_t getHandle() const = 0;

  /**
   * @brief Check if the buffer is created.
   * @return true Buffer is created.
   */
  virtual bool isCreated() const = 0;

  /**
   * @brief Send data to the buffer.
   * @param tx_buffer Data to send.
   * @param bytes Number of bytes to send.
   * @param ticks_to_wait Maximum time to wait for the buffer to be available.
   * @return uint32_t Number of bytes sent, 0 if the buffer is not created.
   */
  virtual uint32_t send(const void* tx_buffer, const uint32_t bytes,
    const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Send data to the buffer from an ISR.
   * @param tx_buffer Data to send.
   * @param bytes Number of bytes to send.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return uint32_t Number of bytes sent, 0 if the buffer is not created.
   */
  virtual uint32_t sendFromISR(const void* tx_buffer, const uint32_t bytes,
    BaseType_t& task_woken) const = 0;

  /**
   * @brief Receive data from the buffer.
   * @param rx_buffer Buffer to store the received data.
   * @param bytes Number of bytes to receive.
   * @param ticks_to_wait Maximum time to wait for the buffer to have enough data.
   * @return uint32_t Number of bytes received, 0 if the buffer is not created or failed to receive
   * the data.
   */
  virtual uint32_t receive(void* rx_buffer, const uint32_t bytes,
    const TickType_t ticks_to_wait = portMAX_DELAY) const = 0;

  /**
   * @brief Receive data from the buffer from an ISR.
   * @param rx_buffer Buffer to store the received data.
   * @param bytes Number of bytes to receive.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return uint32_t Number of bytes received, 0 if the buffer is not created or failed to receive
   * the data.
   */
  virtual uint32_t receiveFromISR(void* rx_buffer, const uint32_t bytes,
    BaseType_t& task_woken) const = 0;

  /**
   * @brief Reset the buffer.
   * @return true Buffer reset successfully, false if the buffer is not created or failed to reset.
   */
  virtual bool reset() const = 0;

  /**
   * @brief Check if the buffer is empty.
   * @return true Buffer is empty, false if the buffer is not created or has data.
   */
  virtual bool isEmpty() const = 0;

  /**
   * @brief Check if the buffer is full.
   * @return true Buffer is full, false if the buffer is not created or has available space.
   */
  virtual bool isFull() const = 0;

  /**
   * @brief Get the available spaces in the buffer.
   * @return uint32_t Available spaces, 0 if the buffer is not created.
   */
  virtual uint32_t getAvailableSpaces() const = 0;

  /**
   * @brief Get the available bytes in the buffer.
   * @return uint32_t Available bytes, 0 if the buffer is not created.
   */
  virtual uint32_t getAvailableBytes() const = 0;

  /**
   * @brief Check if the buffer is created.
   * @return true Buffer is created.
   */
  virtual explicit operator bool() const = 0;
};

namespace Internal {

// CRTP base policy class
template <typename Derived>
class Policy {
  public:
  StreamBufferHandle_t getHandle() const { return _handle; }

  bool isCreated() const { return _handle != nullptr; }

  protected:
  Policy()
      : _handle(nullptr) {}

  StreamBufferHandle_t _handle;
};

// CRTP stream buffer base policy class
template <typename Derived>
class StreamBufferPolicy : public Policy<StreamBufferPolicy<Derived>> {
  public:
  /**
   * @brief Set the trigger level of the stream buffer. The level must be less than or equal to the
   * buffer size.
   * @param trigger_bytes Number of bytes to trigger the buffer.
   * @return true Trigger level set successfully, false if the buffer is not created.
   */
  bool setTriggerLevel(const uint32_t trigger_bytes) {
    if (!this->isCreated()) return false;
    return xStreamBufferSetTriggerLevel(this->_handle, trigger_bytes);
  }
};

// Policy for stream buffer with dynamic memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferDynamicPolicy
    : public StreamBufferPolicy<StreamBufferDynamicPolicy<BufferSize, TriggerBytes>> {
  public:
  StreamBufferDynamicPolicy() {
    this->_handle = xStreamBufferGenericCreate(BufferSize, TriggerBytes, false, nullptr, nullptr);
  }
};

// Policy for data buffer with static memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferStaticPolicy
    : public StreamBufferPolicy<StreamBufferStaticPolicy<BufferSize, TriggerBytes>> {
  public:
  StreamBufferStaticPolicy() {
    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1,
      TriggerBytes,
      false,
      _storage,
      &_buf_buffer,
      nullptr,
      nullptr);
  }

  private:
  StaticStreamBuffer_t _buf_buffer;
  uint8_t _storage[BufferSize + 2];
};

// Policy for data buffer with external memory allocation
template <uint32_t BufferSize, uint32_t TriggerBytes>
class StreamBufferExternalStoragePolicy
    : public StreamBufferPolicy<StreamBufferExternalStoragePolicy<BufferSize, TriggerBytes>> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = BufferSize + 2;

  /**
   * @brief Create the stream buffer with an external memory allocation.
   * @param buffer External memory buffer.
   * @return true Buffer created successfully, false if the buffer is nullptr or failed to create.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;

    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1,
      TriggerBytes,
      false,
      buffer,
      &_buf_buffer,
      nullptr,
      nullptr);

    return this->_handle != nullptr;
  }

  private:
  StaticStreamBuffer_t _buf_buffer;
};

// Policy for message buffer with dynamic memory allocation
template <uint32_t BufferSize>
class MessageBufferDynamicPolicy : public Policy<MessageBufferDynamicPolicy<BufferSize>> {
  public:
  MessageBufferDynamicPolicy() {
    this->_handle = xStreamBufferGenericCreate(BufferSize, 0, true, nullptr, nullptr);
  }
};

// Policy for message buffer with static memory allocation
template <uint32_t BufferSize>
class MessageBufferStaticPolicy : public Policy<MessageBufferStaticPolicy<BufferSize>> {
  public:
  MessageBufferStaticPolicy() {
    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1,
      0,
      true,
      _storage,
      &_buf_buffer,
      nullptr,
      nullptr);
  }

  private:
  StaticStreamBuffer_t _buf_buffer;
  uint8_t _storage[BufferSize + 2];
};

// Policy for message buffer with external memory allocation
template <uint32_t BufferSize>
class MessageBufferExternalStoragePolicy
    : public Policy<MessageBufferExternalStoragePolicy<BufferSize>> {
  public:
  static constexpr uint32_t REQUIRED_SIZE = BufferSize + 2;

  /**
   * @brief Create the message buffer with an external memory allocation.
   * @param buffer External memory buffer.
   * @return true Buffer created successfully, false if the buffer is nullptr or failed to create.
   */
  bool create(uint8_t* const buffer) {
    if (buffer == nullptr) return false;

    this->_handle = xStreamBufferGenericCreateStatic(BufferSize + 1,
      0,
      true,
      buffer,
      &_buf_buffer,
      nullptr,
      nullptr);

    return this->_handle != nullptr;
  }

  private:
  StaticStreamBuffer_t _buf_buffer;
};

// Main DataBuffer class. You need to specify the policy used
template <typename Policy>
class DataBuffer : public IBuffer, public Policy {
  public:
  using Policy::Policy; // Inherit constructor

  ~DataBuffer() {
    if (Policy::isCreated()) vStreamBufferDelete(Policy::getHandle());
  }

  /**
   * @brief Get the low-level handle of the buffer. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return StreamBufferHandle_t Buffer handle, nullptr if the buffer is not created.
   */
  StreamBufferHandle_t getHandle() const override { return Policy::getHandle(); }

  /**
   * @brief Check if the buffer is created.
   * @return true Buffer is created.
   */
  bool isCreated() const override { return Policy::isCreated(); }

  /**
   * @brief Send data to the buffer.
   * @param tx_buffer Data to send.
   * @param bytes Number of bytes to send.
   * @param ticks_to_wait Maximum time to wait for the buffer to be available.
   * @return uint32_t Number of bytes sent, 0 if the buffer is not created.
   */
  uint32_t send(const void* tx_buffer, const uint32_t bytes,
    const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return 0;
    return xStreamBufferSend(getHandle(), tx_buffer, bytes, ticks_to_wait);
  }

  /**
   * @brief Send data to the buffer from an ISR.
   * @param tx_buffer Data to send.
   * @param bytes Number of bytes to send.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return uint32_t Number of bytes sent, 0 if the buffer is not created.
   */
  uint32_t sendFromISR(const void* tx_buffer, const uint32_t bytes,
    BaseType_t& task_woken) const override {
    if (!isCreated()) return 0;
    return xStreamBufferSendFromISR(getHandle(), tx_buffer, bytes, &task_woken);
  }

  /**
   * @brief Receive data from the buffer.
   * @param rx_buffer Buffer to store the received data.
   * @param bytes Number of bytes to receive.
   * @param ticks_to_wait Maximum time to wait for the buffer to have enough data.
   * @return uint32_t Number of bytes received, 0 if the buffer is not created or failed to receive
   * the data.
   */
  uint32_t receive(void* rx_buffer, const uint32_t bytes,
    const TickType_t ticks_to_wait = portMAX_DELAY) const override {
    if (!isCreated()) return 0;
    return xStreamBufferReceive(getHandle(), rx_buffer, bytes, ticks_to_wait);
  }

  /**
   * @brief Receive data from the buffer from an ISR.
   * @param rx_buffer Buffer to store the received data.
   * @param bytes Number of bytes to receive.
   * @param task_woken Task woken flag. If true, you need to use portYIELD_FROM_ISR() at the end of
   * the ISR.
   * @return uint32_t Number of bytes received, 0 if the buffer is not created or failed to receive
   * the data.
   */
  uint32_t receiveFromISR(void* rx_buffer, const uint32_t bytes,
    BaseType_t& task_woken) const override {
    if (!isCreated()) return 0;
    return xStreamBufferReceiveFromISR(getHandle(), rx_buffer, bytes, &task_woken);
  }

  /**
   * @brief Reset the buffer.
   * @return true Buffer reset successfully, false if the buffer is not created or failed to reset.
   */
  bool reset() const override {
    if (!isCreated()) return false;
    return xStreamBufferReset(getHandle());
  }

  /**
   * @brief Check if the buffer is empty.
   * @return true Buffer is empty, false if the buffer is not created or has data.
   */
  bool isEmpty() const override {
    if (!isCreated()) return false;
    return xStreamBufferIsEmpty(getHandle());
  }

  /**
   * @brief Check if the buffer is full.
   * @return true Buffer is full, false if the buffer is not created or has available space.
   */
  bool isFull() const override {
    if (!isCreated()) return false;
    return xStreamBufferIsFull(getHandle());
  }

  /**
   * @brief Get the available spaces in the buffer.
   * @return uint32_t Available spaces, 0 if the buffer is not created.
   */
  uint32_t getAvailableSpaces() const override {
    if (!isCreated()) return 0;
    return xStreamBufferSpacesAvailable(getHandle());
  }

  /**
   * @brief Get the available bytes in the buffer.
   * @return uint32_t Available bytes, 0 if the buffer is not created.
   */
  uint32_t getAvailableBytes() const override {
    if (!isCreated()) return 0;
    return xStreamBufferBytesAvailable(getHandle());
  }

  /**
   * @brief Check if the buffer is created.
   * @return true Buffer is created.
   */
  explicit operator bool() const override { return isCreated(); }
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