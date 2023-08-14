/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_BUFFER_H
#define RTOS_CPP_BUFFER_H

#include <Arduino.h>
#include <freertos/message_buffer.h>
#include <freertos/stream_buffer.h>

class DataBuffer {
  private:
  DataBuffer(const DataBuffer&)     = delete;
  void operator=(const DataBuffer&) = delete;

  protected:
  DataBuffer(StreamBufferHandle_t handle)
      : _handle(handle) {}
  ~DataBuffer() { vStreamBufferDelete(_handle); }

  StreamBufferHandle_t _handle;
  StaticStreamBuffer_t _tcb;

  public:
  uint32_t send(const void* tx_buffer, uint32_t bytes, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xStreamBufferSend(_handle, tx_buffer, bytes, ticks_to_wait);
  }

  uint32_t sendFromISR(const void* tx_buffer, uint32_t bytes, BaseType_t& task_woken) {
    return xStreamBufferSendFromISR(_handle, tx_buffer, bytes, &task_woken);
  }

  uint32_t receive(void* rx_buffer, uint32_t bytes, TickType_t ticks_to_wait = portMAX_DELAY) {
    return xStreamBufferReceive(_handle, rx_buffer, bytes, ticks_to_wait);
  }

  uint32_t receiveFromISR(void* rx_buffer, uint32_t bytes, BaseType_t& task_woken) {
    return xStreamBufferReceiveFromISR(_handle, rx_buffer, bytes, &task_woken);
  }

  bool reset() { return xStreamBufferReset(_handle); }
  bool isEmpty() { return xStreamBufferIsEmpty(_handle); }
  bool isFull() { return xStreamBufferIsFull(_handle); }
  uint32_t availableSpaces() { return xStreamBufferSpacesAvailable(_handle); }
};

template <uint32_t LENGTH>
class StreamBuffer : public DataBuffer {
  public:
  StreamBuffer(uint32_t trigger_bytes)
      : DataBuffer(
          xStreamBufferGenericCreateStatic(LENGTH + 1, trigger_bytes, pdFALSE, _storage, &_tcb)) {}

  uint32_t availableBytes() { return xStreamBufferBytesAvailable(_handle); }

  bool setTriggerLevel(uint32_t trigger_bytes) {
    return xStreamBufferSetTriggerLevel(_handle, trigger_bytes);
  }

  private:
  uint8_t _storage[LENGTH + 1];
};

class StreamBufferExternalStorage : public DataBuffer {
  public:
  StreamBufferExternalStorage()
      : DataBuffer(nullptr) {}

  bool init(uint32_t trigger_bytes, uint8_t* buffer, uint32_t buffer_size) {
    _handle =
      xStreamBufferGenericCreateStatic(buffer_size + 1, trigger_bytes, pdFALSE, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }

  uint32_t availableBytes() { return xStreamBufferBytesAvailable(_handle); }

  bool setTriggerLevel(uint32_t trigger_bytes) {
    return xStreamBufferSetTriggerLevel(_handle, trigger_bytes);
  }
};

template <uint32_t LENGTH>
class MessageBuffer : public DataBuffer {
  public:
  MessageBuffer()
      : DataBuffer(xStreamBufferGenericCreateStatic(LENGTH + 1, 0, pdTRUE, _storage, &_tcb)) {}

  private:
  uint8_t _storage[LENGTH + 1];
};

class MessageBufferExternalStorage : public DataBuffer {
  public:
  MessageBufferExternalStorage()
      : DataBuffer(nullptr) {}

  bool init(uint8_t* buffer, uint32_t buffer_size) {
    _handle = xStreamBufferGenericCreateStatic(buffer_size + 1, 0, pdTRUE, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }
};

#endif // RTOS_CPP_BUFFER_H
