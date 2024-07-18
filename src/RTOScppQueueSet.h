/**
 * SPDX-FileCopyrightText: 2024 Maximiliano Ramirez
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "RTOScppLock.h"
#include "RTOScppQueue.h"
#include "RTOScppRingBuffer.h"
#include <Arduino.h>

class QueueSet {
  public:
  QueueSet(const uint32_t queue_length)
      : _handle(xQueueCreateSet(queue_length)) {}

  ~QueueSet() {
    if (_handle) vQueueDelete(_handle);
  }

  QueueSet(const QueueSet&)                = delete;
  QueueSet& operator=(const QueueSet&)     = delete;
  QueueSet(QueueSet&&) noexcept            = delete;
  QueueSet& operator=(QueueSet&&) noexcept = delete;

  bool add(LockInterface& lock) const { return xQueueAddToSet(lock._handle, _handle); }
  bool add(QueueInterface& queue) const { return xQueueAddToSet(queue._handle, _handle); }
  bool add(RingBufferInterface& ring_buffer) const {
    return xRingbufferAddToQueueSetRead(ring_buffer._handle, _handle);
  }

  bool remove(LockInterface& lock) const { return xQueueRemoveFromSet(lock._handle, _handle); }
  bool remove(QueueInterface& queue) const { return xQueueRemoveFromSet(queue._handle, _handle); }
  bool remove(RingBufferInterface& ring_buffer) const {
    return xRingbufferRemoveFromQueueSetRead(ring_buffer._handle, _handle);
  }

  QueueSetMemberHandle_t select(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    return xQueueSelectFromSet(_handle, ticks_to_wait);
  }
  QueueSetMemberHandle_t selectFromISR() const { return xQueueSelectFromSetFromISR(_handle); }

  explicit operator bool() const { return _handle != nullptr; }

  private:
  QueueSetHandle_t _handle;
};