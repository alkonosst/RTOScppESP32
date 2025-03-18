#include <unity.h>

#include "RTOScppTimer.h"

using namespace RTOS::Timers;
static constexpr const char* tag = "test_timers";

/* ------------------------------------------- Timers ------------------------------------------- */
static constexpr uint32_t timer_period = pdMS_TO_TICKS(1000);

// Dynamic timers
TimerDynamic timer_dyn_ctor(
  "TimerDynCtor", [](TimerHandle_t) {}, timer_period, nullptr, false, false);
TimerDynamic timer_dyn;

// Static timers
TimerStatic timer_st_ctor(
  "TimerStCtor", [](TimerHandle_t) {}, timer_period, nullptr, false, false);
TimerStatic timer_st;

// Invalid timer
TimerStatic timer_invalid;

// Timer and parameters for testing methods
static bool timer_expired = false;

// Custom struct to pass as timer id
struct MyTimerId {
  uint32_t value;
} my_timer_id{};

void timerCb(TimerHandle_t timer) {
  ESP_LOGI(tag, "Timer expired");
  timer_expired = true;

  // Change timer id value
  MyTimerId* id = static_cast<MyTimerId*>(pvTimerGetTimerID(timer));
  id->value     = 123;
}

TimerStatic timer("Timer", timerCb, timer_period, nullptr, false, false);
/* ---------------------------------------------------------------------------------------------- */

/* -------------------------------------------- Tests ------------------------------------------- */
void test_timers_creation() {
  // Dynamic timers
  TEST_ASSERT_TRUE(timer_dyn_ctor);
  TEST_ASSERT_TRUE(timer_dyn_ctor.isCreated());

  TEST_ASSERT_FALSE(timer_dyn);
  TEST_ASSERT_FALSE(timer_dyn.isCreated());
  TEST_ASSERT_TRUE(timer_dyn.create(
    "TimerSt",
    [](TimerHandle_t) {},
    timer_period,
    nullptr,
    false,
    false));
  TEST_ASSERT_TRUE(timer_dyn);
  TEST_ASSERT_TRUE(timer_dyn.isCreated());

  // Static timers
  TEST_ASSERT_TRUE(timer_st_ctor);
  TEST_ASSERT_TRUE(timer_st_ctor.isCreated());

  TEST_ASSERT_FALSE(timer_st);
  TEST_ASSERT_FALSE(timer_st.isCreated());
  TEST_ASSERT_TRUE(timer_st.create(
    "TimerSt",
    [](TimerHandle_t) {},
    timer_period,
    nullptr,
    false,
    false));
  TEST_ASSERT_TRUE(timer_st);
  TEST_ASSERT_TRUE(timer_st.isCreated());

  // Testing timer
  TEST_ASSERT_TRUE(timer);
  TEST_ASSERT_TRUE(timer.isCreated());
}

void test_invalid_timer() {
  TEST_ASSERT_FALSE(timer_invalid.isCreated());
  TEST_ASSERT_FALSE(timer_invalid.create(nullptr, nullptr, 0, nullptr, false, false));
  TEST_ASSERT_FALSE(timer_invalid.isCreated());
}

void test_get_timer_info() {
  const char* name = timer.getName();
  TEST_ASSERT_EQUAL_STRING("Timer", name);

  TickType_t period = timer.getPeriod();
  TEST_ASSERT_EQUAL(timer_period, period);

  bool reload_mode = timer.getReloadMode();
  TEST_ASSERT_EQUAL(false, reload_mode);

  void* timer_id = timer.getTimerID();
  TEST_ASSERT_EQUAL(nullptr, timer_id);

  TEST_ASSERT_TRUE(timer.setTimerID(&my_timer_id));
  timer_id = timer.getTimerID();
  TEST_ASSERT_EQUAL(&my_timer_id, timer_id);
}

void test_control() {
  TEST_ASSERT_TRUE(timer.start());
  vTaskDelay(pdMS_TO_TICKS(10));

  TEST_ASSERT_TRUE(timer.isActive());

  TickType_t expiry_time = timer.getExpiryTime();
  TEST_ASSERT_LESS_THAN_UINT32(timer_period, expiry_time);

  TEST_ASSERT_EQUAL(false, timer_expired);
  vTaskDelay(pdMS_TO_TICKS(10));

  TEST_ASSERT_TRUE(timer.stop());
  vTaskDelay(pdMS_TO_TICKS(10));
  TEST_ASSERT_FALSE(timer.isActive());
  TEST_ASSERT_EQUAL(false, timer_expired);

  TEST_ASSERT_TRUE(timer.reset());
  vTaskDelay(pdMS_TO_TICKS(10));
  TEST_ASSERT_TRUE(timer.isActive());
  vTaskDelay(timer_period + pdMS_TO_TICKS(100));

  TEST_ASSERT_TRUE(timer_expired);
  TEST_ASSERT_EQUAL(123, my_timer_id.value);
  timer_expired = false;
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

  RUN_TEST(test_timers_creation);
  RUN_TEST(test_invalid_timer);
  RUN_TEST(test_get_timer_info);
  RUN_TEST(test_control);

  ESP_LOGI(tag, "Finishing tests...");
  UNITY_END();
}

void loop() { vTaskDelete(nullptr); }
/* ---------------------------------------------------------------------------------------------- */