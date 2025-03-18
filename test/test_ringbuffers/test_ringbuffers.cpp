#include <Arduino.h>
#include <unity.h>

#include "RTOScppRingBuffer.h"

using namespace RTOS::RingBuffers;
static constexpr const char* tag = "test_ringbufs";

/* ----------------------------------------- RingBuffers ---------------------------------------- */
static constexpr uint8_t rb_len = 64;

RingBufferNoSplitDynamic<char, rb_len> rb_nosp_dyn;
RingBufferNoSplitStatic<char, rb_len> rb_nosp_st;
RingBufferNoSplitExternalStorage<char, rb_len> rb_nosp_ext;

RingBufferSplitDynamic<char, rb_len> rb_sp_dyn;
RingBufferSplitStatic<char, rb_len> rb_sp_st;
RingBufferSplitExternalStorage<char, rb_len> rb_sp_ext;

RingBufferByteDynamic<rb_len> rb_byte_dyn;
RingBufferByteStatic<rb_len> rb_byte_st;
RingBufferByteExternalStorage<rb_len> rb_byte_ext;
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_rb_creation() {
  static uint8_t* rb_nosp_ext_buffer = nullptr;
  static uint8_t* rb_sp_ext_buffer   = nullptr;
  static uint8_t* rb_byte_ext_buffer = nullptr;

  TEST_ASSERT_TRUE(rb_nosp_dyn);
  TEST_ASSERT_TRUE(rb_nosp_st);

  rb_nosp_ext_buffer = static_cast<uint8_t*>(malloc(rb_nosp_ext.REQUIRED_SIZE));
  TEST_ASSERT_NOT_NULL(rb_nosp_ext_buffer);
  TEST_ASSERT_TRUE(rb_nosp_ext.create(rb_nosp_ext_buffer));

  TEST_ASSERT_TRUE(rb_sp_dyn);
  TEST_ASSERT_TRUE(rb_sp_st);

  rb_sp_ext_buffer = static_cast<uint8_t*>(malloc(rb_sp_ext.REQUIRED_SIZE));
  TEST_ASSERT_NOT_NULL(rb_sp_ext_buffer);
  TEST_ASSERT_TRUE(rb_sp_ext.create(rb_sp_ext_buffer));

  TEST_ASSERT_TRUE(rb_byte_dyn);
  TEST_ASSERT_TRUE(rb_byte_st);

  rb_byte_ext_buffer = static_cast<uint8_t*>(malloc(rb_byte_ext.REQUIRED_SIZE));
  TEST_ASSERT_NOT_NULL(rb_byte_ext_buffer);
  TEST_ASSERT_TRUE(rb_byte_ext.create(rb_byte_ext_buffer));
}

void test_rb_nosplit_send_recv() {
  static char item_to_send               = 'a';
  static const uint8_t item_to_send_size = sizeof(item_to_send);

  char* item_recv;
  size_t item_recv_size;

  TEST_ASSERT_TRUE(rb_nosp_dyn.send(&item_to_send, item_to_send_size));
  item_recv = rb_nosp_dyn.receive(item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(item_to_send, *item_recv);
  rb_nosp_dyn.returnItem(item_recv);

  TEST_ASSERT_TRUE(rb_nosp_st.send(&item_to_send, item_to_send_size));
  item_recv = rb_nosp_st.receive(item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(item_to_send, *item_recv);
  rb_nosp_st.returnItem(item_recv);

  TEST_ASSERT_TRUE(rb_nosp_ext.send(&item_to_send, item_to_send_size));
  item_recv = rb_nosp_ext.receive(item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(item_to_send, *item_recv);
  rb_nosp_ext.returnItem(item_recv);
}

void test_rb_split_send_recv() {
  // Here we will leave a gap on the middle of the buffer, to force the split.
  static char small_item[8];
  memset(small_item, 's', sizeof(small_item));

  static char large_item[20];
  memset(large_item, 'l', sizeof(large_item));

  static char split_item[20];
  memset(split_item, 'g', sizeof(split_item));

  char* item_small_head       = nullptr;
  char* item_small_tail       = nullptr;
  size_t item_small_head_size = 0;
  size_t item_small_tail_size = 0;

  char* item_large_head       = nullptr;
  char* item_large_tail       = nullptr;
  size_t item_large_head_size = 0;
  size_t item_large_tail_size = 0;

  char* item_split_head       = nullptr;
  char* item_split_tail       = nullptr;
  size_t item_split_head_size = 0;
  size_t item_split_tail_size = 0;

  // Dynamic version ----------------------------------------------------------
  // Send items to the buffer
  TEST_ASSERT_TRUE(rb_sp_dyn.send(small_item, sizeof(small_item)));
  TEST_ASSERT_TRUE(rb_sp_dyn.send(large_item, sizeof(large_item)));

  // Receive them
  TEST_ASSERT_TRUE(rb_sp_dyn.receive(item_small_head,
    item_small_tail,
    item_small_head_size,
    item_small_tail_size));
  TEST_ASSERT_TRUE(rb_sp_dyn.receive(item_large_head,
    item_large_tail,
    item_large_head_size,
    item_large_tail_size));

  // Check the small item
  TEST_ASSERT_NOT_NULL(item_small_head);
  TEST_ASSERT_NULL(item_small_tail);
  TEST_ASSERT_EQUAL_MEMORY(small_item, item_small_head, item_small_head_size);

  // Check the large item
  TEST_ASSERT_NOT_NULL(item_large_head);
  TEST_ASSERT_NULL(item_large_tail);
  TEST_ASSERT_EQUAL_MEMORY(large_item, item_large_head, item_large_head_size);

  // Return them
  rb_sp_dyn.returnItem(item_small_head);
  rb_sp_dyn.returnItem(item_large_head);

  // Add a large item to force the split and fill the ends
  TEST_ASSERT_TRUE(rb_sp_dyn.send(split_item, sizeof(split_item)));

  // Receive the split item
  TEST_ASSERT_TRUE(rb_sp_dyn.receive(item_split_head,
    item_split_tail,
    item_split_head_size,
    item_split_tail_size));

  // Check the split item
  TEST_ASSERT_NOT_NULL(item_split_head);
  TEST_ASSERT_NOT_NULL(item_split_tail);

  for (uint32_t i = 0; i < item_split_head_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i], item_split_head[i]);
  }

  for (uint32_t i = 0; i < item_split_tail_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i + item_split_head_size], item_split_tail[i]);
  }

  // Return the split item
  rb_sp_dyn.returnItem(item_split_head);
  rb_sp_dyn.returnItem(item_split_tail);

  // Static version -----------------------------------------------------------
  item_small_head      = nullptr;
  item_small_tail      = nullptr;
  item_small_head_size = 0;
  item_small_tail_size = 0;

  item_large_head      = nullptr;
  item_large_tail      = nullptr;
  item_large_head_size = 0;
  item_large_tail_size = 0;

  item_split_head      = nullptr;
  item_split_tail      = nullptr;
  item_split_head_size = 0;
  item_split_tail_size = 0;

  // Send items to the buffer
  TEST_ASSERT_TRUE(rb_sp_st.send(small_item, sizeof(small_item)));
  TEST_ASSERT_TRUE(rb_sp_st.send(large_item, sizeof(large_item)));

  // Receive them
  TEST_ASSERT_TRUE(
    rb_sp_st.receive(item_small_head, item_small_tail, item_small_head_size, item_small_tail_size));
  TEST_ASSERT_TRUE(
    rb_sp_st.receive(item_large_head, item_large_tail, item_large_head_size, item_large_tail_size));

  // Check the small item
  TEST_ASSERT_NOT_NULL(item_small_head);
  TEST_ASSERT_NULL(item_small_tail);
  TEST_ASSERT_EQUAL_MEMORY(small_item, item_small_head, item_small_head_size);

  // Check the large item
  TEST_ASSERT_NOT_NULL(item_large_head);
  TEST_ASSERT_NULL(item_large_tail);
  TEST_ASSERT_EQUAL_MEMORY(large_item, item_large_head, item_large_head_size);

  // Return them
  rb_sp_st.returnItem(item_small_head);
  rb_sp_st.returnItem(item_large_head);

  // Add a large item to force the split and fill the ends
  TEST_ASSERT_TRUE(rb_sp_st.send(split_item, sizeof(split_item)));

  // Receive the split item
  TEST_ASSERT_TRUE(
    rb_sp_st.receive(item_split_head, item_split_tail, item_split_head_size, item_split_tail_size));

  // Check the split item
  TEST_ASSERT_NOT_NULL(item_split_head);
  TEST_ASSERT_NOT_NULL(item_split_tail);

  for (uint32_t i = 0; i < item_split_head_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i], item_split_head[i]);
  }

  for (uint32_t i = 0; i < item_split_tail_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i + item_split_head_size], item_split_tail[i]);
  }

  // Return the split item
  rb_sp_st.returnItem(item_split_head);
  rb_sp_st.returnItem(item_split_tail);

  // External storage version -------------------------------------------------
  item_small_head      = nullptr;
  item_small_tail      = nullptr;
  item_small_head_size = 0;
  item_small_tail_size = 0;

  item_large_head      = nullptr;
  item_large_tail      = nullptr;
  item_large_head_size = 0;
  item_large_tail_size = 0;

  item_split_head      = nullptr;
  item_split_tail      = nullptr;
  item_split_head_size = 0;
  item_split_tail_size = 0;

  // Send items to the buffer
  TEST_ASSERT_TRUE(rb_sp_ext.send(small_item, sizeof(small_item)));
  TEST_ASSERT_TRUE(rb_sp_ext.send(large_item, sizeof(large_item)));

  // Receive them
  TEST_ASSERT_TRUE(rb_sp_ext.receive(item_small_head,
    item_small_tail,
    item_small_head_size,
    item_small_tail_size));
  TEST_ASSERT_TRUE(rb_sp_ext.receive(item_large_head,
    item_large_tail,
    item_large_head_size,
    item_large_tail_size));

  // Check the small item
  TEST_ASSERT_NOT_NULL(item_small_head);
  TEST_ASSERT_NULL(item_small_tail);
  TEST_ASSERT_EQUAL_MEMORY(small_item, item_small_head, item_small_head_size);

  // Check the large item
  TEST_ASSERT_NOT_NULL(item_large_head);
  TEST_ASSERT_NULL(item_large_tail);
  TEST_ASSERT_EQUAL_MEMORY(large_item, item_large_head, item_large_head_size);

  // Return them
  rb_sp_ext.returnItem(item_small_head);
  rb_sp_ext.returnItem(item_large_head);

  // Add a large item to force the split and fill the ends
  TEST_ASSERT_TRUE(rb_sp_ext.send(split_item, sizeof(split_item)));

  // Receive the split item
  TEST_ASSERT_TRUE(rb_sp_ext.receive(item_split_head,
    item_split_tail,
    item_split_head_size,
    item_split_tail_size));

  // Check the split item
  TEST_ASSERT_NOT_NULL(item_split_head);
  TEST_ASSERT_NOT_NULL(item_split_tail);

  for (uint32_t i = 0; i < item_split_head_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i], item_split_head[i]);
  }

  for (uint32_t i = 0; i < item_split_tail_size; i++) {
    TEST_ASSERT_EQUAL(split_item[i + item_split_head_size], item_split_tail[i]);
  }

  // Return the split item
  rb_sp_ext.returnItem(item_split_head);
  rb_sp_ext.returnItem(item_split_tail);
}

void test_rb_byte_send_recv() {
  static uint8_t item_to_send[16];
  memset(item_to_send, 'b', sizeof(item_to_send));
  static uint8_t recv_up_to = 8;

  uint8_t* item_recv;
  size_t item_recv_size;

  TEST_ASSERT_TRUE(rb_byte_dyn.send(item_to_send, sizeof(item_to_send)));
  item_recv = rb_byte_dyn.receiveUpTo(recv_up_to, item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(recv_up_to, item_recv_size);
  TEST_ASSERT_EQUAL_MEMORY(item_to_send, item_recv, item_recv_size);
  rb_byte_dyn.returnItem(item_recv);

  TEST_ASSERT_TRUE(rb_byte_st.send(item_to_send, sizeof(item_to_send)));
  item_recv = rb_byte_st.receiveUpTo(recv_up_to, item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(recv_up_to, item_recv_size);
  TEST_ASSERT_EQUAL_MEMORY(item_to_send, item_recv, item_recv_size);
  rb_byte_st.returnItem(item_recv);

  TEST_ASSERT_TRUE(rb_byte_ext.send(item_to_send, sizeof(item_to_send)));
  item_recv = rb_byte_ext.receiveUpTo(recv_up_to, item_recv_size);
  TEST_ASSERT_NOT_NULL(item_recv);
  TEST_ASSERT_EQUAL(recv_up_to, item_recv_size);
  TEST_ASSERT_EQUAL_MEMORY(item_to_send, item_recv, item_recv_size);
  rb_byte_ext.returnItem(item_recv);
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

  RUN_TEST(test_rb_creation);
  RUN_TEST(test_rb_nosplit_send_recv);
  RUN_TEST(test_rb_split_send_recv);
  RUN_TEST(test_rb_byte_send_recv);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }
/* ---------------------------------------------------------------------------------------------- */