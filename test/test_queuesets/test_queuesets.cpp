#include <Arduino.h>
#include <unity.h>

#include "RTOScppQueueSet.h"

using namespace RTOS::QueueSets;
using namespace RTOS::Locks;
using namespace RTOS::Queues;
using namespace RTOS::RingBuffers;
static constexpr const char* tag = "test_queue_set";

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
/* ---------------------------------------------------------------------------------------------- */

/* ------------------------------------------ USB Logs ------------------------------------------ */
SemaphoreHandle_t log_mutex = nullptr;

int redirectLogs(const char* str, va_list list) {
  if (log_mutex != nullptr) xSemaphoreTake(log_mutex, portMAX_DELAY);

  static char buffer[2048];
  int ret = vsnprintf(buffer, sizeof(buffer), str, list);
  Serial.write(buffer);

  if (log_mutex != nullptr) xSemaphoreGive(log_mutex);

  return ret;
}
/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------- */
void setup() {
  log_mutex = xSemaphoreCreateMutex();
  esp_log_set_vprintf(redirectLogs);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  Serial.begin(115200);
  delay(2000);

  ESP_LOGI(tag, "Running tests...");
  UNITY_BEGIN();

  RUN_TEST(test_objects_creation);

  RUN_TEST(test_add_objects_to_queueset);
  RUN_TEST(test_select_queue_from_queueset);
  RUN_TEST(test_select_ringbuffer_from_queueset);
  RUN_TEST(test_select_semaphore_from_queueset);
  RUN_TEST(test_remove_objects_from_queueset);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }
/* ---------------------------------------------------------------------------------------------- */