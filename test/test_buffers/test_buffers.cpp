#include <unity.h>

#include "RTOScppBuffer.h"

using namespace RTOS::Buffers;
static constexpr const char* tag = "test_buffers";

/* ---------------------------------------- Data Buffers ---------------------------------------- */
static char tx_buffer[]         = "123456789";
static constexpr uint8_t tx_len = sizeof(tx_buffer);
static char rx_buffer[tx_len]; // Extra byte for null terminator

static constexpr uint32_t buffer_size   = 100;
static constexpr uint32_t trigger_bytes = 5;

StreamBufferDynamic<buffer_size, trigger_bytes> sb_dyn;
StreamBufferStatic<buffer_size, trigger_bytes> sb_st;
StreamBufferExternalStorage<buffer_size, trigger_bytes> sb_ext;

MessageBufferDynamic<buffer_size> mb_dyn;
MessageBufferStatic<buffer_size> mb_st;
MessageBufferExternalStorage<buffer_size> mb_ext;
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_sb_creation() {
  TEST_ASSERT_TRUE(sb_dyn);
  TEST_ASSERT_TRUE(sb_st);

  static uint8_t* ext_buffer = static_cast<uint8_t*>(malloc(sb_ext.REQUIRED_SIZE));

  TEST_ASSERT_NOT_NULL(ext_buffer);
  TEST_ASSERT_TRUE(sb_ext.create(ext_buffer));
}

void test_sb_send_receive() {
  TEST_ASSERT_EQUAL(tx_len, sb_dyn.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(sb_dyn.isFull());
  TEST_ASSERT_FALSE(sb_dyn.isEmpty());
  TEST_ASSERT_EQUAL(tx_len, sb_dyn.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len, sb_dyn.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, sb_dyn.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(sb_dyn.isEmpty());

  TEST_ASSERT_EQUAL(tx_len, sb_st.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(sb_st.isFull());
  TEST_ASSERT_FALSE(sb_st.isEmpty());
  TEST_ASSERT_EQUAL(tx_len, sb_st.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len, sb_st.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, sb_st.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(sb_st.isEmpty());

  TEST_ASSERT_EQUAL(tx_len, sb_ext.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(sb_ext.isFull());
  TEST_ASSERT_FALSE(sb_ext.isEmpty());
  TEST_ASSERT_EQUAL(tx_len, sb_ext.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len, sb_ext.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, sb_ext.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(sb_ext.isEmpty());
}

void test_sb_send_receive_less_than_trigger() {
  static constexpr uint8_t bytes = 2;

  TEST_ASSERT_TRUE(sb_dyn.reset());
  TEST_ASSERT_EQUAL(0, sb_dyn.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_dyn.send(tx_buffer, bytes));
  TEST_ASSERT_EQUAL(bytes, sb_dyn.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_dyn.receive(rx_buffer, tx_len));

  TEST_ASSERT_TRUE(sb_st.reset());
  TEST_ASSERT_EQUAL(0, sb_st.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_st.send(tx_buffer, bytes));
  TEST_ASSERT_EQUAL(bytes, sb_st.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_st.receive(rx_buffer, tx_len));

  TEST_ASSERT_TRUE(sb_ext.reset());
  TEST_ASSERT_EQUAL(0, sb_ext.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_ext.send(tx_buffer, bytes));
  TEST_ASSERT_EQUAL(bytes, sb_ext.getAvailableBytes());
  TEST_ASSERT_EQUAL(bytes, sb_ext.receive(rx_buffer, tx_len));
}

void test_sb_change_trigger_level() {
  static constexpr uint8_t new_trigger = 2;

  TEST_ASSERT_TRUE(sb_dyn.setTriggerLevel(new_trigger));
  TEST_ASSERT_TRUE(sb_st.setTriggerLevel(new_trigger));
  TEST_ASSERT_TRUE(sb_ext.setTriggerLevel(new_trigger));

  static constexpr uint32_t bad_trigger = buffer_size * 2;

  TEST_ASSERT_FALSE(sb_dyn.setTriggerLevel(bad_trigger));
  TEST_ASSERT_FALSE(sb_st.setTriggerLevel(bad_trigger));
  TEST_ASSERT_FALSE(sb_ext.setTriggerLevel(bad_trigger));
}

void test_mb_creation() {
  TEST_ASSERT_TRUE(mb_dyn);
  TEST_ASSERT_TRUE(mb_st);

  static uint8_t* ext_buffer = static_cast<uint8_t*>(malloc(mb_ext.REQUIRED_SIZE));

  TEST_ASSERT_NOT_NULL(ext_buffer);
  TEST_ASSERT_TRUE(mb_ext.create(ext_buffer));
}

void test_mb_send_receive() {
  static constexpr uint32_t tx_len_plus_size = tx_len + sizeof(uint32_t);

  TEST_ASSERT_EQUAL(tx_len, mb_dyn.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(mb_dyn.isFull());
  TEST_ASSERT_FALSE(mb_dyn.isEmpty());
  TEST_ASSERT_EQUAL(tx_len_plus_size, mb_dyn.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len_plus_size, mb_dyn.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, mb_dyn.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(mb_dyn.isEmpty());

  TEST_ASSERT_EQUAL(tx_len, mb_st.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(mb_st.isFull());
  TEST_ASSERT_FALSE(mb_st.isEmpty());
  TEST_ASSERT_EQUAL(tx_len_plus_size, mb_st.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len_plus_size, mb_st.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, mb_st.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(mb_st.isEmpty());

  TEST_ASSERT_EQUAL(tx_len, mb_ext.send(tx_buffer, tx_len));
  TEST_ASSERT_FALSE(mb_ext.isFull());
  TEST_ASSERT_FALSE(mb_ext.isEmpty());
  TEST_ASSERT_EQUAL(tx_len_plus_size, mb_ext.getAvailableBytes());
  TEST_ASSERT_EQUAL(buffer_size - tx_len_plus_size, mb_ext.getAvailableSpaces());
  TEST_ASSERT_EQUAL(tx_len, mb_ext.receive(rx_buffer, tx_len));
  TEST_ASSERT_EQUAL_STRING(tx_buffer, rx_buffer);
  TEST_ASSERT_TRUE(mb_ext.isEmpty());
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

  RUN_TEST(test_sb_creation);
  RUN_TEST(test_sb_send_receive);
  RUN_TEST(test_sb_send_receive_less_than_trigger);
  RUN_TEST(test_sb_change_trigger_level);
  RUN_TEST(test_mb_creation);
  RUN_TEST(test_mb_send_receive);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }
/* ---------------------------------------------------------------------------------------------- */