#include <Arduino.h>
#include <unity.h>

#include "RTOScppQueueSet.h"

using namespace RTOS::QueueSets;
using namespace RTOS::Locks;
using namespace RTOS::Queues;
using namespace RTOS::RingBuffers;

/* ------------------------------------- Items for Queue Set ------------------------------------ */
QueueStatic<uint32_t, 3> queue;
RingBufferNoSplitStatic<uint32_t, 32> rbuffer; // Header + 1 uint32_t element
SemBinaryStatic sem;

// 3 queue items + 1 ring buffer item + 1 semaphore item
QueueSet q_set(3 + 1 + 1);
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_objects_creation() {
  TEST_ASSERT_TRUE(queue);
  TEST_ASSERT_TRUE(rbuffer);
  TEST_ASSERT_TRUE(sem);
  TEST_ASSERT_TRUE(q_set);

  TEST_ASSERT_EQUAL_STRING("RtosQueueSet", q_set.getName());
}

void test_add_objects_to_queueset() {
  TEST_ASSERT_TRUE(q_set.add(queue));
  TEST_ASSERT_TRUE(q_set.add(rbuffer));
  TEST_ASSERT_TRUE(q_set.add(sem));
}

void test_select_queue_from_queueset() {
  TEST_ASSERT_TRUE(queue.add(1));

  auto member = q_set.select();
  TEST_ASSERT_NOT_NULL(member);
  TEST_ASSERT_TRUE(member == queue);
}

void test_select_ringbuffer_from_queueset() {
  uint32_t data = 1;
  TEST_ASSERT_TRUE(rbuffer.send(&data, sizeof(data)));

  auto member = q_set.select();
  TEST_ASSERT_NOT_NULL(member);
  TEST_ASSERT_TRUE(member == rbuffer);
}

void test_select_semaphore_from_queueset() {
  TEST_ASSERT_TRUE(sem.give());

  auto member = q_set.select();
  TEST_ASSERT_NOT_NULL(member);
  TEST_ASSERT_TRUE(member == sem);
}

void test_remove_objects_from_queueset() {
  // Need to empty the queue and ring buffer
  queue.reset();

  size_t size;
  uint32_t* data = rbuffer.receive(size);

  TEST_ASSERT_NOT_NULL(data);
  rbuffer.returnItem(data);

  TEST_ASSERT_TRUE(sem.take());

  TEST_ASSERT_TRUE(q_set.remove(queue));
  TEST_ASSERT_TRUE(q_set.remove(rbuffer));
  TEST_ASSERT_TRUE(q_set.remove(sem));
}

void test_custom_names() {
  static constexpr const char* name = "CustomQueueSet";

  static QueueSet q_set_named(3, name);
  TEST_ASSERT_TRUE(q_set_named);
  TEST_ASSERT_EQUAL_STRING(name, q_set_named.getName());
}
/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  UNITY_BEGIN();

  RUN_TEST(test_objects_creation);
  RUN_TEST(test_add_objects_to_queueset);
  RUN_TEST(test_select_queue_from_queueset);
  RUN_TEST(test_select_ringbuffer_from_queueset);
  RUN_TEST(test_select_semaphore_from_queueset);
  RUN_TEST(test_remove_objects_from_queueset);
  RUN_TEST(test_custom_names);

  UNITY_END();
}

void loop() {}
/* ---------------------------------------------------------------------------------------------- */