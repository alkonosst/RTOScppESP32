/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>
#include <freertos/message_buffer.h>
#include <freertos/stream_buffer.h>

class DataBufferInterface {
  protected:
  DataBufferInterface(const StreamBufferHandle_t handle)
      : _handle(handle) {}

  StreamBufferHandle_t _handle;

  public:
  virtual ~DataBufferInterface() {
    if (_handle) vStreamBufferDelete(_handle);
  }

  DataBufferInterface(const DataBufferInterface&)                = delete;
  DataBufferInterface& operator=(const DataBufferInterface&)     = delete;
  DataBufferInterface(DataBufferInterface&&) noexcept            = delete;
  DataBufferInterface& operator=(DataBufferInterface&&) noexcept = delete;

  uint32_t send(const void* tx_buffer, const uint32_t bytes,
                const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xStreamBufferSend(_handle, tx_buffer, bytes, ticks_to_wait);
  }

  uint32_t sendFromISR(const void* tx_buffer, const uint32_t bytes, BaseType_t& task_woken) const {
    return xStreamBufferSendFromISR(_handle, tx_buffer, bytes, &task_woken);
  }

  uint32_t receive(void* rx_buffer, const uint32_t bytes,
                   const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xStreamBufferReceive(_handle, rx_buffer, bytes, ticks_to_wait);
  }

  uint32_t receiveFromISR(void* rx_buffer, const uint32_t bytes, BaseType_t& task_woken) const {
    return xStreamBufferReceiveFromISR(_handle, rx_buffer, bytes, &task_woken);
  }

  bool reset() const { return xStreamBufferReset(_handle); }
  bool isEmpty() const { return xStreamBufferIsEmpty(_handle); }
  bool isFull() const { return xStreamBufferIsFull(_handle); }
  uint32_t availableSpaces() const { return xStreamBufferSpacesAvailable(_handle); }
  uint32_t availableBytes() const { return xStreamBufferBytesAvailable(_handle); }

  bool setTriggerLevel(const uint32_t trigger_bytes) {
    return xStreamBufferSetTriggerLevel(_handle, trigger_bytes);
  }

  explicit operator bool() const { return _handle != nullptr; }
};

class StreamBufferDynamic : public DataBufferInterface {
  public:
  StreamBufferDynamic(const uint32_t buffer_size, const uint32_t trigger_bytes)
      : DataBufferInterface(xStreamBufferGenericCreate(buffer_size + 1, trigger_bytes, false)) {}
};

template <uint32_t BUFFER_SIZE>
class StreamBufferStatic : public DataBufferInterface {
  public:
  StreamBufferStatic(const uint32_t trigger_bytes)
      : DataBufferInterface(xStreamBufferGenericCreateStatic(BUFFER_SIZE + 1, trigger_bytes, false,
                                                             _storage, &_tcb)) {}

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE + 1];
};

class StreamBufferExternalStorage : public DataBufferInterface {
  public:
  StreamBufferExternalStorage(const uint32_t trigger_bytes)
      : DataBufferInterface(nullptr) {}

  bool init(const uint32_t trigger_bytes, uint8_t* const buffer, const uint32_t buffer_size) {
    _handle =
      xStreamBufferGenericCreateStatic(buffer_size + 1, trigger_bytes, pdFALSE, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }

  private:
  StaticStreamBuffer_t _tcb;
};

class MessageBufferDynamic : public DataBufferInterface {
  public:
  MessageBufferDynamic(const uint32_t buffer_size)
      : DataBufferInterface(xStreamBufferGenericCreate(buffer_size + 1, 0, false)) {}
};

template <uint32_t BUFFER_SIZE>
class MessageBufferStatic : public DataBufferInterface {
  public:
  MessageBufferStatic()
      : DataBufferInterface(
          xStreamBufferGenericCreateStatic(BUFFER_SIZE + 1, 0, false, _storage, &_tcb)) {}

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE + 1];
};

class MessageBufferExternalStorage : public DataBufferInterface {
  public:
  MessageBufferExternalStorage()
      : DataBufferInterface(nullptr) {}

  bool init(const uint32_t trigger_bytes, uint8_t* const buffer, const uint32_t buffer_size) {
    _handle = xStreamBufferGenericCreateStatic(buffer_size + 1, 0, true, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }

  private:
  StaticStreamBuffer_t _tcb;
};
