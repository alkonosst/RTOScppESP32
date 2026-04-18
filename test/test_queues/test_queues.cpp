#include <unity.h>

#include "RTOScppQueue.h"

using namespace RTOS::Queues;

/* ------------------------------------------- Queues ------------------------------------------- */
static constexpr uint32_t queue_size  = 3;
static constexpr uint32_t item_to_add = 123456789;

QueueDynamic<uint32_t, queue_size> q_dyn;
QueueStatic<uint32_t, queue_size> q_st;
QueueExternalStorage<uint32_t, queue_size> q_ext;

// Queue of length 1 to test overwrite
QueueStatic<uint32_t, 1> q_overwrite;
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_queues_creation() {
  TEST_ASSERT_TRUE(q_dyn);
  TEST_ASSERT_TRUE(q_st);

  static uint8_t* ext_buffer = static_cast<uint8_t*>(malloc(q_ext.REQUIRED_SIZE));
  TEST_ASSERT_NOT_NULL(ext_buffer);
  TEST_ASSERT_TRUE(q_ext.create(ext_buffer));

  TEST_ASSERT_TRUE(q_overwrite);

  TEST_ASSERT_EQUAL_STRING("RtosQueue", q_dyn.getName());
  TEST_ASSERT_EQUAL_STRING("RtosQueue", q_st.getName());
  TEST_ASSERT_EQUAL_STRING("RtosQueue", q_ext.getName());
  TEST_ASSERT_EQUAL_STRING("RtosQueue", q_overwrite.getName());
}

void test_queues_full_empty() {
  TEST_ASSERT_TRUE(q_dyn.isEmpty());
  TEST_ASSERT_TRUE(q_st.isEmpty());
  TEST_ASSERT_TRUE(q_ext.isEmpty());

  TEST_ASSERT_FALSE(q_dyn.isFull());
  TEST_ASSERT_FALSE(q_st.isFull());
  TEST_ASSERT_FALSE(q_ext.isFull());

  for (uint32_t i = 0; i < queue_size; i++) {
    TEST_ASSERT_TRUE(q_dyn.add(item_to_add));
    TEST_ASSERT_TRUE(q_st.add(item_to_add));
    TEST_ASSERT_TRUE(q_ext.add(item_to_add));
  }

  TEST_ASSERT_FALSE(q_dyn.isEmpty());
  TEST_ASSERT_FALSE(q_st.isEmpty());
  TEST_ASSERT_FALSE(q_ext.isEmpty());

  TEST_ASSERT_TRUE(q_dyn.isFull());
  TEST_ASSERT_TRUE(q_st.isFull());
  TEST_ASSERT_TRUE(q_ext.isFull());

  q_dyn.reset();
  q_st.reset();
  q_ext.reset();

  TEST_ASSERT_TRUE(q_dyn.isEmpty());
  TEST_ASSERT_TRUE(q_st.isEmpty());
  TEST_ASSERT_TRUE(q_ext.isEmpty());

  TEST_ASSERT_FALSE(q_dyn.isFull());
  TEST_ASSERT_FALSE(q_st.isFull());
  TEST_ASSERT_FALSE(q_ext.isFull());
}

void test_queues_add() {
  uint32_t values[queue_size] = {1, 2, 3};

  for (uint32_t i = 0; i < queue_size; i++) {
    TEST_ASSERT_TRUE(q_dyn.add(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_dyn.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_dyn.getAvailableSpaces());

    TEST_ASSERT_TRUE(q_st.add(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_st.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_st.getAvailableSpaces());

    TEST_ASSERT_TRUE(q_ext.add(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_ext.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_ext.getAvailableSpaces());
  }

  TEST_ASSERT_TRUE(q_dyn.isFull());
  TEST_ASSERT_TRUE(q_st.isFull());
  TEST_ASSERT_TRUE(q_ext.isFull());

  for (uint32_t i = 0; i < queue_size; i++) {
    uint32_t item_to_pop = 0;
    TEST_ASSERT_TRUE(q_dyn.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_dyn.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_dyn.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[i], item_to_pop);
    item_to_pop = 0;

    TEST_ASSERT_TRUE(q_st.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_st.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_st.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[i], item_to_pop);
    item_to_pop = 0;

    TEST_ASSERT_TRUE(q_ext.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_ext.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_ext.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[i], item_to_pop);
  }
}

void test_queues_push() {
  uint32_t values[queue_size] = {1, 2, 3};

  for (uint32_t i = 0; i < queue_size; i++) {
    TEST_ASSERT_TRUE(q_dyn.push(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_dyn.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_dyn.getAvailableSpaces());

    TEST_ASSERT_TRUE(q_st.push(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_st.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_st.getAvailableSpaces());

    TEST_ASSERT_TRUE(q_ext.push(values[i]));
    TEST_ASSERT_EQUAL(i + 1, q_ext.getAvailableMessages());
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_ext.getAvailableSpaces());
  }

  TEST_ASSERT_TRUE(q_dyn.isFull());
  TEST_ASSERT_TRUE(q_st.isFull());
  TEST_ASSERT_TRUE(q_ext.isFull());

  for (uint32_t i = 0; i < queue_size; i++) {
    uint32_t item_to_pop = 0;
    TEST_ASSERT_TRUE(q_dyn.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_dyn.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_dyn.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[queue_size - i - 1], item_to_pop);
    item_to_pop = 0;

    TEST_ASSERT_TRUE(q_st.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_st.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_st.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[queue_size - i - 1], item_to_pop);
    item_to_pop = 0;

    TEST_ASSERT_TRUE(q_ext.pop(item_to_pop));
    TEST_ASSERT_EQUAL(queue_size - i - 1, q_ext.getAvailableMessages());
    TEST_ASSERT_EQUAL(i + 1, q_ext.getAvailableSpaces());
    TEST_ASSERT_EQUAL(values[queue_size - i - 1], item_to_pop);
  }
}

void test_queues_peek() {
  uint32_t item_to_peek = 0;

  TEST_ASSERT_TRUE(q_dyn.add(item_to_add));
  TEST_ASSERT_TRUE(q_dyn.peek(item_to_peek));
  TEST_ASSERT_EQUAL(1, q_dyn.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size - 1, q_dyn.getAvailableSpaces());
  TEST_ASSERT_EQUAL(item_to_add, item_to_peek);
  TEST_ASSERT_TRUE(q_dyn.pop(item_to_peek));
  TEST_ASSERT_EQUAL(0, q_dyn.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size, q_dyn.getAvailableSpaces());
  item_to_peek = 0;

  TEST_ASSERT_TRUE(q_st.add(item_to_add));
  TEST_ASSERT_TRUE(q_st.peek(item_to_peek));
  TEST_ASSERT_EQUAL(1, q_st.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size - 1, q_st.getAvailableSpaces());
  TEST_ASSERT_EQUAL(item_to_add, item_to_peek);
  TEST_ASSERT_TRUE(q_st.pop(item_to_peek));
  TEST_ASSERT_EQUAL(0, q_st.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size, q_st.getAvailableSpaces());
  item_to_peek = 0;

  TEST_ASSERT_TRUE(q_ext.add(item_to_add));
  TEST_ASSERT_TRUE(q_ext.peek(item_to_peek));
  TEST_ASSERT_EQUAL(1, q_ext.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size - 1, q_ext.getAvailableSpaces());
  TEST_ASSERT_EQUAL(item_to_add, item_to_peek);
  TEST_ASSERT_TRUE(q_ext.pop(item_to_peek));
  TEST_ASSERT_EQUAL(0, q_ext.getAvailableMessages());
  TEST_ASSERT_EQUAL(queue_size, q_ext.getAvailableSpaces());
  item_to_peek = 0;
}

void test_queue_overwrite() {
  uint32_t item_to_peek = 0;

  TEST_ASSERT_TRUE(q_overwrite.overwrite(1));
  TEST_ASSERT_TRUE(q_overwrite.peek(item_to_peek));
  TEST_ASSERT_EQUAL(1, q_overwrite.getAvailableMessages());
  TEST_ASSERT_EQUAL(0, q_overwrite.getAvailableSpaces());
  TEST_ASSERT_EQUAL(1, item_to_peek);

  TEST_ASSERT_TRUE(q_overwrite.overwrite(2));
  TEST_ASSERT_TRUE(q_overwrite.peek(item_to_peek));
  TEST_ASSERT_EQUAL(1, q_overwrite.getAvailableMessages());
  TEST_ASSERT_EQUAL(0, q_overwrite.getAvailableSpaces());
  TEST_ASSERT_EQUAL(2, item_to_peek);
}

void test_queue_full_behavior_fail() {
  static QueueStatic<uint8_t, 2, FullBehavior::Fail> q_fail;
  TEST_ASSERT_TRUE(q_fail);

  TEST_ASSERT_TRUE(q_fail.add(1));
  TEST_ASSERT_TRUE(q_fail.add(2));
  TEST_ASSERT_FALSE(q_fail.add(3));
  TEST_ASSERT_FALSE(q_fail.push(3));

  uint8_t item = 0;
  TEST_ASSERT_TRUE(q_fail.pop(item));
  TEST_ASSERT_TRUE(q_fail.add(3));
  TEST_ASSERT_FALSE(q_fail.add(4));
  TEST_ASSERT_FALSE(q_fail.push(4));
}

void test_custom_names() {
  static constexpr const char* name = "CustomQueue";

  static QueueDynamic<uint32_t, queue_size> q_dyn_named(name);
  static QueueStatic<uint32_t, queue_size> q_st_named(name);

  static uint8_t* ext_buffer =
    static_cast<uint8_t*>(malloc(QueueExternalStorage<uint32_t, queue_size>::REQUIRED_SIZE));
  TEST_ASSERT_NOT_NULL(ext_buffer);

  static QueueExternalStorage<uint32_t, queue_size> q_ext_named(name);
  TEST_ASSERT_TRUE(q_ext_named.create(ext_buffer));

  TEST_ASSERT_EQUAL_STRING(name, q_dyn_named.getName());
  TEST_ASSERT_EQUAL_STRING(name, q_st_named.getName());
  TEST_ASSERT_EQUAL_STRING(name, q_ext_named.getName());
}
/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(1000);

  UNITY_BEGIN();

  RUN_TEST(test_queues_creation);
  RUN_TEST(test_queues_full_empty);
  RUN_TEST(test_queues_add);
  RUN_TEST(test_queues_push);
  RUN_TEST(test_queues_peek);
  RUN_TEST(test_queue_overwrite);
  RUN_TEST(test_queue_full_behavior_fail);
  RUN_TEST(test_custom_names);

  UNITY_END();
}

void loop() {}
/* ---------------------------------------------------------------------------------------------- */