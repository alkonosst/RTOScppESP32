#include <Arduino.h>
#include <unity.h>

#include "RTOScppLock.h"

#include <memory>

using namespace RTOS::Locks;
static constexpr const char* tag = "test_locks";

/* -------------------------------------------- Locks ------------------------------------------- */
MutexDynamic mutex_dyn;
MutexStatic mutex_st;
MutexRecursiveDynamic mutex_rec_dyn;
MutexRecursiveStatic mutex_rec_st;
SemBinaryDynamic sem_bin_dyn;
SemBinaryStatic sem_bin_st;

static constexpr uint8_t sem_count = 2;
SemCountingDynamic<sem_count> sem_count_dyn;
SemCountingStatic<sem_count> sem_count_st;

static constexpr uint8_t take_wait = 10;
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_locks_creation() {
  TEST_ASSERT_TRUE(mutex_dyn);
  TEST_ASSERT_TRUE(mutex_dyn.isCreated());

  TEST_ASSERT_TRUE(mutex_st);
  TEST_ASSERT_TRUE(mutex_st.isCreated());

  TEST_ASSERT_TRUE(mutex_rec_dyn);
  TEST_ASSERT_TRUE(mutex_rec_dyn.isCreated());

  TEST_ASSERT_TRUE(mutex_rec_st);
  TEST_ASSERT_TRUE(mutex_rec_st.isCreated());

  TEST_ASSERT_TRUE(sem_bin_dyn);
  TEST_ASSERT_TRUE(sem_bin_dyn.isCreated());

  TEST_ASSERT_TRUE(sem_bin_st);
  TEST_ASSERT_TRUE(sem_bin_st.isCreated());

  TEST_ASSERT_TRUE(sem_count_dyn);
  TEST_ASSERT_TRUE(sem_count_dyn.isCreated());

  TEST_ASSERT_TRUE(sem_count_st);
  TEST_ASSERT_TRUE(sem_count_st.isCreated());
}

void test_mutex() {
  TEST_ASSERT_TRUE(mutex_dyn.take(take_wait));
  TEST_ASSERT_TRUE(mutex_st.take(take_wait));

  TEST_ASSERT_TRUE(mutex_dyn.give());
  TEST_ASSERT_TRUE(mutex_st.give());
}

void test_mutex_recursive() {
  TEST_ASSERT_TRUE(mutex_rec_dyn.take(take_wait));
  TEST_ASSERT_TRUE(mutex_rec_st.take(take_wait));

  TEST_ASSERT_TRUE(mutex_rec_dyn.take());
  TEST_ASSERT_TRUE(mutex_rec_st.take());

  TEST_ASSERT_TRUE(mutex_rec_dyn.give());
  TEST_ASSERT_TRUE(mutex_rec_st.give());

  TEST_ASSERT_TRUE(mutex_rec_dyn.give());
  TEST_ASSERT_TRUE(mutex_rec_st.give());
}

void test_semaphore_binary() {
  TEST_ASSERT_TRUE(sem_bin_dyn.give());
  TEST_ASSERT_TRUE(sem_bin_st.give());

  TEST_ASSERT_TRUE(sem_bin_dyn.take(take_wait));
  TEST_ASSERT_TRUE(sem_bin_st.take(take_wait));

  TEST_ASSERT_TRUE(sem_bin_dyn.give());
  TEST_ASSERT_TRUE(sem_bin_st.give());
}

void test_semaphore_counting() {
  UBaseType_t count_dyn = sem_count_dyn.getCount();
  UBaseType_t count_st  = sem_count_dyn.getCount();

  TEST_ASSERT_EQUAL(0, count_dyn);
  TEST_ASSERT_EQUAL(0, count_st);

  for (uint8_t i = 0; i < sem_count; i++) {
    TEST_ASSERT_TRUE(sem_count_dyn.give());
    TEST_ASSERT_TRUE(sem_count_st.give());
  }

  TEST_ASSERT_TRUE(sem_count_dyn.take(take_wait));
  TEST_ASSERT_TRUE(sem_count_st.take(take_wait));

  count_dyn = sem_count_dyn.getCount();
  count_st  = sem_count_st.getCount();

  TEST_ASSERT_EQUAL(sem_count - 1, count_dyn);
  TEST_ASSERT_EQUAL(sem_count - 1, count_st);

  TEST_ASSERT_TRUE(sem_count_dyn.give());
  TEST_ASSERT_TRUE(sem_count_st.give());

  count_dyn = sem_count_dyn.getCount();
  count_st  = sem_count_st.getCount();

  TEST_ASSERT_EQUAL(sem_count, count_dyn);
  TEST_ASSERT_EQUAL(sem_count, count_st);
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
  delay(3000);

  ESP_LOGI(tag, "Running tests...");
  UNITY_BEGIN();

  RUN_TEST(test_locks_creation);
  RUN_TEST(test_mutex);
  RUN_TEST(test_mutex_recursive);
  RUN_TEST(test_semaphore_binary);
  RUN_TEST(test_semaphore_counting);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }
/* ---------------------------------------------------------------------------------------------- */