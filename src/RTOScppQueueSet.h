/**
 * SPDX-FileCopyrightText: 2025 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <Arduino.h>

#include "RTOScppLock.h"
#include "RTOScppQueue.h"
#include "RTOScppRingBuffer.h"

namespace RTOS::QueueSets {

class QueueSet {
  public:
  QueueSet(const uint32_t queue_length)
      : _handle(xQueueCreateSet(queue_length)) {}

  ~QueueSet() {
    if (_handle) vQueueDelete(_handle);
  }

  QueueSet(const QueueSet&)            = delete;
  QueueSet& operator=(const QueueSet&) = delete;
  QueueSet(QueueSet&&)                 = delete;
  QueueSet& operator=(QueueSet&&)      = delete;

  bool add(RTOS::Locks::ILock& lock) const { return xQueueAddToSet(lock.getHandle(), _handle); }
  bool add(RTOS::Queues::IQueue& queue) const { return xQueueAddToSet(queue.getHandle(), _handle); }
  bool add(RTOS::RingBuffers::IRingBuffer& ring_buffer) const {
    return xRingbufferAddToQueueSetRead(ring_buffer.getHandle(), _handle);
  }

  bool remove(RTOS::Locks::ILock& lock) const {
    return xQueueRemoveFromSet(lock.getHandle(), _handle);
  }

  bool remove(RTOS::Queues::IQueue& queue) const {
    return xQueueRemoveFromSet(queue.getHandle(), _handle);
  }

  bool remove(RTOS::RingBuffers::IRingBuffer& ring_buffer) const {
    return xRingbufferRemoveFromQueueSetRead(ring_buffer.getHandle(), _handle);
  }

  QueueSetMemberHandle_t select(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSelectFromSet(_handle, ticks_to_wait);
  }
  QueueSetMemberHandle_t selectFromISR() const { return xQueueSelectFromSetFromISR(_handle); }

  explicit operator bool() const { return _handle != nullptr; }

  private:
  QueueSetHandle_t _handle;
};

} // namespace RTOS::QueueSets