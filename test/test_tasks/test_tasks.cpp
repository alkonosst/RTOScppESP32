#include <unity.h>

#include "RTOScppTask.h"

using namespace RTOS::Tasks;
static constexpr const char* tag = "test_tasks";

/* -------------------------------------------- Tasks ------------------------------------------- */
static constexpr uint32_t STACK_SIZE = 4096;

// Task function prototypes
void taskDynamicCtor(void* params);
void taskDynamic(void* params);
void taskStaticCtor(void* params);
void taskStatic(void* params);

// Dynamic tasks
TaskDynamic<STACK_SIZE> task_dyn_ctor("TaskDynCtor", taskDynamicCtor, 1, nullptr);
TaskDynamic<STACK_SIZE> task_dyn;

// Static tasks
TaskStatic<STACK_SIZE> task_st_ctor("TaskStCtor", taskStaticCtor, 1, nullptr);
TaskStatic<STACK_SIZE> task_st;

// Invalid task (uninitialized)
TaskStatic<STACK_SIZE> task_invalid;

// Custom struct to pass as parameter
struct MyParams {
  uint32_t value;
} my_params{};

// Testing task
void taskFunction(void* params);
TaskStatic<STACK_SIZE> task("task", taskFunction, 1, &my_params, ARDUINO_RUNNING_CORE);
static bool notify_received           = false;
static uint32_t notify_value_received = 0;
static uint32_t notify_old_value      = 0;
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_create_dynamic_ctor();
void test_create_dynamic();
void test_create_static_ctor();
void test_create_static();
void test_create_testing_task();

void test_invalid_task();

void test_get_task_info();
void test_change_priority();
void test_block_state();
void test_suspend();
void test_resume();
void test_abort_delay();
void test_notify();
void test_check_notify();
void test_notify_and_query();
void test_check_notify_and_query();
void test_notify_give();
void test_check_notify_take();
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

  RUN_TEST(test_create_dynamic_ctor);
  RUN_TEST(test_create_dynamic);
  RUN_TEST(test_create_static_ctor);
  RUN_TEST(test_create_static);
  RUN_TEST(test_create_testing_task);

  RUN_TEST(test_invalid_task);

  RUN_TEST(test_get_task_info);
  RUN_TEST(test_change_priority);
  RUN_TEST(test_block_state);
  RUN_TEST(test_abort_delay);
  RUN_TEST(test_suspend);
  RUN_TEST(test_resume);
  RUN_TEST(test_notify);
  RUN_TEST(test_check_notify);
  RUN_TEST(test_notify_and_query);
  RUN_TEST(test_check_notify_and_query);
  RUN_TEST(test_notify_give);
  RUN_TEST(test_check_notify_take);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }

void taskDynamicCtor(void* params) {
  for (;;) {
    vTaskDelay(portMAX_DELAY);
  }
}

void taskDynamic(void* params) {
  for (;;) {
    vTaskDelay(portMAX_DELAY);
  }
}

void taskStaticCtor(void* params) {
  for (;;) {
    vTaskDelay(portMAX_DELAY);
  }
}

void taskStatic(void* params) {
  for (;;) {
    vTaskDelay(portMAX_DELAY);
  }
}

void taskFunction(void* params) {
  // Change parameter value to check it later
  MyParams* p = static_cast<MyParams*>(params);
  p->value    = 123;

  for (;;) {
    // Check blocked state test
    ESP_LOGD(tag, "Task is going to block for 10s");
    vTaskDelay(10000);

    // Suspend and resume test
    ESP_LOGD(tag, "Task unblocked, blocking again for 10s");
    vTaskDelay(10000);

    ESP_LOGD(tag, "Task resumed, waiting for notify");

    // Notify test
    notify_received = task.notifyWait(0, 0, notify_value_received, portMAX_DELAY);
    ESP_LOGD(tag,
      "Notify received (%s): %u",
      notify_received ? "true" : "false",
      notify_value_received);

    // Notify and query test
    notify_received = task.notifyWait(0, 0, notify_value_received, portMAX_DELAY);
    ESP_LOGD(tag,
      "Notify received (%s): %u",
      notify_received ? "true" : "false",
      notify_value_received);

    // Notify take test
    notify_value_received = task.notifyTake(true, portMAX_DELAY);
    ESP_LOGD(tag, "Notify take passed");

    ESP_LOGD(tag, "Blocking indefinitely");
    vTaskDelay(portMAX_DELAY);
  }
}
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_create_dynamic_ctor() {
  TEST_ASSERT_FALSE(task_dyn_ctor.isCreated());
  TEST_ASSERT_TRUE(task_dyn_ctor.create());
  TEST_ASSERT_TRUE(task_dyn_ctor.isCreated());
}

void test_create_dynamic() {
  TEST_ASSERT_FALSE(task_dyn.isCreated());
  TEST_ASSERT_TRUE(task_dyn.create("TaskDyn", taskDynamic, 1, nullptr, ARDUINO_RUNNING_CORE));
  TEST_ASSERT_TRUE(task_dyn.isCreated());
}

void test_create_static_ctor() {
  TEST_ASSERT_FALSE(task_st_ctor.isCreated());
  TEST_ASSERT_TRUE(task_st_ctor.create());
  TEST_ASSERT_TRUE(task_st_ctor.isCreated());
}

void test_create_static() {
  TEST_ASSERT_FALSE(task_st.isCreated());
  TEST_ASSERT_TRUE(task_st.create("TaskSt", taskStatic, 1, nullptr, ARDUINO_RUNNING_CORE));
  TEST_ASSERT_TRUE(task_st.isCreated());
}

void test_create_testing_task() {
  TEST_ASSERT_FALSE(task.isCreated());
  TEST_ASSERT_TRUE(task.create());
  TEST_ASSERT_TRUE(task.isCreated());
}

void test_invalid_task() {
  TEST_ASSERT_FALSE(task_invalid.isCreated());
  TEST_ASSERT_FALSE(task_invalid.create(nullptr, nullptr, 0));
  TEST_ASSERT_FALSE(task_invalid.isCreated());

  const char* name = task_invalid.getName();
  void* params     = task_invalid.getParameters();
  uint8_t core     = task_invalid.getCore();
  uint8_t priority = task_invalid.getPriority();

  TEST_ASSERT_NULL(name);
  TEST_ASSERT_NULL(params);
  TEST_ASSERT_EQUAL(0xFF, core);
  TEST_ASSERT_EQUAL(0xFF, priority);
}

void test_get_task_info() {
  const char* name = task.getName();
  TEST_ASSERT_NOT_NULL(name);
  TEST_ASSERT_EQUAL_STRING("task", name);

  void* params = task.getParameters();
  TEST_ASSERT_NOT_NULL(params);
  TEST_ASSERT_EQUAL(123, static_cast<MyParams*>(params)->value);

  uint8_t core = task.getCore();
  TEST_ASSERT_EQUAL(1, task.getCore());

  uint8_t priority = task.getPriority();
  TEST_ASSERT_EQUAL(1, priority);

  TEST_ASSERT_EQUAL(STACK_SIZE, task.getStackSize());

  TEST_ASSERT_TRUE(task.updateStackStats());

  uint32_t stack_used = task.getStackUsed();
  TEST_ASSERT_GREATER_THAN_UINT32(0, stack_used);

  uint32_t stack_min = task.getStackMinUsed();
  TEST_ASSERT_LESS_THAN_UINT32(0xffffffff, stack_min);

  uint32_t stack_max = task.getStackMaxUsed();
  TEST_ASSERT_GREATER_THAN_UINT32(0, stack_max);
}

void test_change_priority() {
  TEST_ASSERT_TRUE(task.setPriority(2));

  uint8_t priority = task.getPriority();
  TEST_ASSERT_EQUAL(2, priority);
}

void test_block_state() { TEST_ASSERT_EQUAL(eTaskState::eBlocked, task.getState()); }

void test_abort_delay() {
  TEST_ASSERT_TRUE(task.abortDelay());
  TEST_ASSERT_EQUAL(eTaskState::eReady, task.getState());
}

void test_suspend() {
  TEST_ASSERT_TRUE(task.suspend());
  TEST_ASSERT_EQUAL(eTaskState::eSuspended, task.getState());
}

void test_resume() {
  TEST_ASSERT_TRUE(task.resume());
  TEST_ASSERT_NOT_EQUAL(eTaskState::eSuspended, task.getState());
}

void test_notify() { TEST_ASSERT_TRUE(task.notify(1, eNotifyAction::eSetValueWithOverwrite)); }

void test_check_notify() {
  TEST_ASSERT_TRUE(notify_received);
  notify_received = false;

  TEST_ASSERT_EQUAL(1, notify_value_received);
  notify_value_received = 0;
}

void test_notify_and_query() {
  TEST_ASSERT_TRUE(task.notifyAndQuery(2, eNotifyAction::eSetValueWithOverwrite, notify_old_value));
}

void test_check_notify_and_query() {
  TEST_ASSERT_TRUE(notify_received);
  notify_received = false;

  TEST_ASSERT_EQUAL(1, notify_old_value);
  notify_old_value = 0;

  TEST_ASSERT_EQUAL(2, notify_value_received);
  notify_value_received = 0;
}

void test_notify_give() { TEST_ASSERT_TRUE(task.notifyGive()); }

void test_check_notify_take() {
  TEST_ASSERT_EQUAL(0, notify_value_received);
  notify_value_received = 0;
}
/* ---------------------------------------------------------------------------------------------- */