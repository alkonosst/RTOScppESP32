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

  bool add(RingbufHandle_t ring_buffer) {
    return xRingbufferAddToQueueSetRead(ring_buffer, _handle);
  }

  QueueSetMemberHandle_t select(TickType_t ticks_to_wait = portMAX_DELAY) {
    return xQueueSelectFromSet(_handle, ticks_to_wait);
  }

  private:
  QueueSetHandle_t _handle;

  QueueSet(const QueueSet&)            = delete;
  QueueSet& operator=(const QueueSet&) = delete;
};

#endif // RTOS_CPP_QUEUE_SET_H