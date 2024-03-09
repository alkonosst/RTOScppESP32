/**
 * SPDX-FileCopyrightText: 2023 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTOS_CPP_QUEUE_SET_H
#define RTOS_CPP_QUEUE_SET_H

#include "RTOScppLock.h"
#include "RTOScppQueue.h"
#include "RTOScppRingBuffer.h"
#include <Arduino.h>

class QueueSet {
  public:
  QueueSet(const uint8_t queue_length)
      : _handle(xQueueCreateSet(queue_length)) {}
  ~QueueSet() {
    if (_handle) vQueueDelete(_handle);
  }

  bool add(LockBase& lock) { return xQueueAddToSet(lock._handle, _handle); }
  bool add(QueueInterface& queue) { return xQueueAddToSet(queue._handle, _handle); }
  bool add(RingBufferBase& ring_buffer) {
    return xRingbufferAddToQueueSetRead(ring_buffer._handle, _handle);
  }

  bool remove(LockBase& lock) { return xQueueRemoveFromSet(lock._handle, _handle); }
  bool remove(QueueInterface& queue) { return xQueueRemoveFromSet(queue._handle, _handle); }
  bool remove(RingBufferBase& ring_buffer) {
    return xRingbufferRemoveFromQueueSetRead(ring_buffer._handle, _handle);
  }

  QueueSetMemberHandle_t select(TickType_t ticks_to_wait = portMAX_DELAY) {
    return xQueueSelectFromSet(_handle, ticks_to_wait);
  }
  QueueSetMemberHandle_t selectFromISR() { return xQueueSelectFromSetFromISR(_handle); }

  private:
  QueueSetHandle_t _handle;

  QueueSet(const QueueSet&)       = delete; // Delete copy constructor
  void operator=(const QueueSet&) = delete; // Delete copy assignment operator
};

#endif // RTOS_CPP_QUEUE_SET_H