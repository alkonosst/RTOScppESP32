#include <unity.h>

#include "RTOScppBuffer.h"

using namespace RTOS::Buffers;

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

  TEST_ASSERT_EQUAL_STRING("RtosBuffer", sb_dyn.getName());
  TEST_ASSERT_EQUAL_STRING("RtosBuffer", sb_st.getName());
  TEST_ASSERT_EQUAL_STRING("RtosBuffer", sb_ext.getName());
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

  TEST_ASSERT_EQUAL_STRING("RtosBuffer", mb_dyn.getName());
  TEST_ASSERT_EQUAL_STRING("RtosBuffer", mb_st.getName());
  TEST_ASSERT_EQUAL_STRING("RtosBuffer", mb_ext.getName());
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

void test_custom_names() {
  static constexpr const char* name = "CustomName";

  static StreamBufferDynamic<buffer_size, trigger_bytes> sb_dyn_named(name);
  static StreamBufferStatic<buffer_size, trigger_bytes> sb_st_named(name);
  static MessageBufferDynamic<buffer_size> mb_dyn_named(name);
  static MessageBufferStatic<buffer_size> mb_st_named(name);

  TEST_ASSERT_EQUAL_STRING(name, sb_dyn_named.getName());
  TEST_ASSERT_EQUAL_STRING(name, sb_st_named.getName());
  TEST_ASSERT_EQUAL_STRING(name, mb_dyn_named.getName());
  TEST_ASSERT_EQUAL_STRING(name, mb_st_named.getName());
}
/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  UNITY_BEGIN();

  RUN_TEST(test_sb_creation);
  RUN_TEST(test_sb_send_receive);
  RUN_TEST(test_sb_send_receive_less_than_trigger);
  RUN_TEST(test_sb_change_trigger_level);
  RUN_TEST(test_mb_creation);
  RUN_TEST(test_mb_send_receive);
  RUN_TEST(test_custom_names);

  UNITY_END();
}

void loop() {}
/* ---------------------------------------------------------------------------------------------- */