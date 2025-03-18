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
  /**
   * @brief Instantiate a QueueSet object.
   * @param queue_length Max number of events that the queue set can hold. Take precautions to avoid
   * an assert from the FreeRTOS kernel if the queue set length is less than the number of events
   * that are going to be added. Each lock will use 1 event. Each queue will use events equal up to
   * the queue length. Each ringbuffer will use the number of messages capable of holding at a
   * certain time, not the length of it.
   */
  QueueSet(const UBaseType_t queue_length)
      : _handle(xQueueCreateSet(queue_length)) {}

  ~QueueSet() {
    if (_handle) vQueueDelete(_handle);
  }

  QueueSet(const QueueSet&)            = delete;
  QueueSet& operator=(const QueueSet&) = delete;
  QueueSet(QueueSet&&)                 = delete;
  QueueSet& operator=(QueueSet&&)      = delete;

  /**
   * @brief Get the low-level handle of the queue set. Useful for direct FreeRTOS API calls. Use it
   * with caution.
   * @return QueueSetHandle_t Queue set handle, nullptr if the queue set is not created.
   */
  QueueSetHandle_t getHandle() const { return _handle; }

  /**
   * @brief Check if the queue set is created.
   * @return true Queue set is created.
   */
  bool isCreated() const { return _handle != nullptr; }

  /**
   * @brief Add a lock to the queue set.
   * @param lock Lock object.
   * @return true Lock added successfully, false if the queue set is not created or the lock is not
   * created.
   */
  bool add(RTOS::Locks::ILock& lock) const {
    if (!isCreated() || !lock) return false;
    return xQueueAddToSet(lock.getHandle(), _handle);
  }

  /**
   * @brief Add a queue to the queue set.
   * @param queue Queue object.
   * @return true Queue added successfully, false if the queue set is not created or the queue is
   * not created.
   */
  bool add(RTOS::Queues::IQueue& queue) const {
    if (!isCreated() || !queue) return false;
    return xQueueAddToSet(queue.getHandle(), _handle);
  }

  /**
   * @brief Add a ring buffer to the queue set.
   * @param ring_buffer Ring buffer object.
   * @return true Ring buffer added successfully, false if the queue set is not created or the ring
   * buffer is not created.
   */
  bool add(RTOS::RingBuffers::IRingBuffer& ring_buffer) const {
    if (!isCreated() || !ring_buffer) return false;
    return xRingbufferAddToQueueSetRead(ring_buffer.getHandle(), _handle);
  }

  /**
   * @brief Remove a lock from the queue set.
   * @param lock Lock object.
   * @return true Lock removed successfully, false if the queue set is not created or the lock is
   * not created.
   */
  bool remove(RTOS::Locks::ILock& lock) const {
    if (!isCreated() || !lock) return false;
    return xQueueRemoveFromSet(lock.getHandle(), _handle);
  }

  /**
   * @brief Remove a queue from the queue set.
   * @param queue Queue object.
   * @return true Queue removed successfully, false if the queue set is not created or the queue is
   * not created.
   */
  bool remove(RTOS::Queues::IQueue& queue) const {
    if (!isCreated() || !queue) return false;
    return xQueueRemoveFromSet(queue.getHandle(), _handle);
  }

  /**
   * @brief Remove a ring buffer from the queue set.
   * @param ring_buffer Ring buffer object.
   * @return true Ring buffer removed successfully, false if the queue set is not created or the
   * ring buffer is not created.
   */
  bool remove(RTOS::RingBuffers::IRingBuffer& ring_buffer) const {
    if (!isCreated() || !ring_buffer) return false;
    return xRingbufferRemoveFromQueueSetRead(ring_buffer.getHandle(), _handle);
  }

  /**
   * @brief Select a member of the queue set which holds an event.
   * @param ticks_to_wait Number of ticks to wait for a member to be available.
   * @return QueueSetMemberHandle_t Queue set member handle, nullptr if the queue set is not
   * created.
   */
  QueueSetMemberHandle_t select(const TickType_t ticks_to_wait = portMAX_DELAY) const {
    if (!isCreated()) return nullptr;
    return xQueueSelectFromSet(_handle, ticks_to_wait);
  }

  /**
   * @brief Select a member of the queue set which holds an event from an ISR.
   * @return QueueSetMemberHandle_t Queue set member handle, nullptr if the queue set is not
   * created.
   */
  QueueSetMemberHandle_t selectFromISR() const {
    if (!isCreated()) return nullptr;
    return xQueueSelectFromSetFromISR(_handle);
  }

  /**
   * @brief Check if the queue set is created.
   * @return true Queue set is created.
   */
  explicit operator bool() const { return isCreated(); }

  private:
  QueueSetHandle_t _handle;
};

} // namespace RTOS::QueueSets