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

class DataBufferBase {
  private:
  DataBufferBase(const DataBufferBase&) = delete; // Delete copy constructor
  void operator=(const DataBufferBase&) = delete; // Delete copy assignment operator

  protected:
  DataBufferBase(StreamBufferHandle_t handle)
      : _handle(handle) {}
  virtual ~DataBufferBase() { vStreamBufferDelete(_handle); }

  StreamBufferHandle_t _handle;

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
  uint32_t availableBytes() { return xStreamBufferBytesAvailable(_handle); }

  bool setTriggerLevel(uint32_t trigger_bytes) {
    return xStreamBufferSetTriggerLevel(_handle, trigger_bytes);
  }

  explicit operator bool() const { return _handle != nullptr; }
};

class StreamBufferDynamic : public DataBufferBase {
  public:
  StreamBufferDynamic(uint32_t buffer_size, uint32_t trigger_bytes)
      : DataBufferBase(xStreamBufferGenericCreate(buffer_size + 1, trigger_bytes, false)) {}
};

template <uint32_t BUFFER_SIZE>
class StreamBufferStatic : public DataBufferBase {
  public:
  StreamBufferStatic(uint32_t trigger_bytes)
      : DataBufferBase(xStreamBufferGenericCreateStatic(BUFFER_SIZE + 1, trigger_bytes, false,
                                                        _storage, &_tcb)) {}

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE + 1];
};

class StreamBufferExternalStorage : public DataBufferBase {
  public:
  StreamBufferExternalStorage(uint32_t trigger_bytes)
      : DataBufferBase(nullptr) {}

  bool init(uint32_t trigger_bytes, uint8_t* buffer, uint32_t buffer_size) {
    _handle =
      xStreamBufferGenericCreateStatic(buffer_size + 1, trigger_bytes, pdFALSE, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }

  private:
  StaticStreamBuffer_t _tcb;
};

class MessageBufferDynamic : public DataBufferBase {
  public:
  MessageBufferDynamic(uint32_t buffer_size)
      : DataBufferBase(xStreamBufferGenericCreate(buffer_size + 1, 0, false)) {}
};

template <uint32_t BUFFER_SIZE>
class MessageBufferStatic : public DataBufferBase {
  public:
  MessageBufferStatic()
      : DataBufferBase(
          xStreamBufferGenericCreateStatic(BUFFER_SIZE + 1, 0, false, _storage, &_tcb)) {}

  private:
  StaticStreamBuffer_t _tcb;
  uint8_t _storage[BUFFER_SIZE + 1];
};

class MessageBufferExternalStorage : public DataBufferBase {
  public:
  MessageBufferExternalStorage()
      : DataBufferBase(nullptr) {}

  bool init(uint32_t trigger_bytes, uint8_t* buffer, uint32_t buffer_size) {
    _handle = xStreamBufferGenericCreateStatic(buffer_size + 1, 0, true, buffer, &_tcb);
    return _handle != nullptr ? true : false;
  }

  private:
  StaticStreamBuffer_t _tcb;
};

#endif // RTOS_CPP_BUFFER_H
