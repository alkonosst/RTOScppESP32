<h1 align="center">
  <a><img src=".img/logo.png" alt="Logo" width="300"></a>
  <br>
  RTOScppESP32
</h1>

<p align="center">
  <b>C++ abstraction layer for FreeRTOS on ESP32.</b>
</p>

<p align="center">
  <a href="https://www.ardu-badge.com/RTOScppESP32">
    <img src="https://www.ardu-badge.com/badge/RTOScppESP32.svg?" alt="Arduino Library Badge">
  </a>
  <a href="https://registry.platformio.org/libraries/alkonosst/RTOScppESP32">
    <img src="https://badges.registry.platformio.org/packages/alkonosst/library/RTOScppESP32.svg" alt="PlatformIO Registry">
  </a>
  <br><br>
  <a href="https://opensource.org/licenses/MIT">
    <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge&color=blue" alt="License">
  </a>
  <br><br>
  <a href="https://ko-fi.com/alkonosst">
    <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Ko-fi">
  </a>
</p>

---

# Table of contents <!-- omit in toc -->

- [Description](#description)
- [Key Features](#key-features)
- [Quick Example](#quick-example)
- [Installation](#installation)
  - [PlatformIO](#platformio)
  - [Arduino IDE](#arduino-ide)
- [Usage](#usage)
  - [Including the library](#including-the-library)
  - [Namespaces](#namespaces)
  - [Memory allocation strategies](#memory-allocation-strategies)
  - [Tasks](#tasks)
  - [Timers](#timers)
  - [Locks](#locks)
    - [Mutexes](#mutexes)
    - [Binary semaphores](#binary-semaphores)
    - [Counting semaphores](#counting-semaphores)
    - [ISR methods](#isr-methods)
    - [Interfaces](#interfaces)
  - [Queues](#queues)
    - [Queue behavior on full](#queue-behavior-on-full)
  - [Buffers](#buffers)
    - [Stream buffers](#stream-buffers)
    - [Message buffers](#message-buffers)
  - [Ring buffers](#ring-buffers)
    - [No-split](#no-split)
    - [Split](#split)
    - [Byte](#byte)
  - [Queue sets](#queue-sets)
- [Release Status](#release-status)
- [License](#license)

---

# Description

**RTOScppESP32** is a C++ abstraction library for the ESP32 that wraps FreeRTOS and ESP-IDF RTOS primitives behind a clean, object-oriented API. Instead of dealing with raw handles, manual resource management, and scattered function calls, every primitive - tasks, timers, queues, buffers, locks, and ring buffers - is encapsulated in a typed C++ class with consistent naming and IDE-friendly code completion.

The library is ESP32-only and is designed for both the **Arduino** and **PlatformIO** ecosystems. All source is documented via Doxygen comments in the header files. For FreeRTOS concepts, refer to the [FreeRTOS documentation](https://www.freertos.org/Documentation/01-FreeRTOS-quick-start/01-Beginners-guide/01-RTOS-fundamentals) and the [ESP-IDF FreeRTOS reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html). Ring buffers are an ESP-IDF extension - see the [Ring Buffer API reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_additions.html#ring-buffers).

# Key Features

- **Full RTOS coverage** - Tasks, software timers, mutexes, semaphores, queues, stream/message buffers, ring buffers, and queue sets.
- **Three memory strategies** - Every object comes in `Dynamic` (heap), `Static` (preallocated), and where applicable `ExternalStorage` (user-supplied buffer, e.g. PSRAM) variants.
- **ISR-safe API** - Semaphores, queues, ring buffers, timers, tasks, and buffers all expose dedicated `*FromISR()` methods with a consistent `BaseType_t& task_woken` signature.
- **Polymorphic interfaces** - Each primitive family exposes an `IXxx` interface (`ITask`, `ITimer`, `ILock`, `ISemaphore`, `IQueue`, `IBuffer`) so objects can be used through pointers without knowing the concrete type.
- **Consistent naming** - All types follow the `<Type><Dynamic|Static|ExternalStorage>` pattern with uniform method names across all primitives.
- **IDE-friendly** - Full Doxygen documentation on every public method; all public types live in dedicated sub-namespaces under `RTOS`.

# Quick Example

Two tasks share an LED, protected by a mutex to avoid concurrent access:

```cpp
#include <Arduino.h>

#include "RTOScppLock.h"
#include "RTOScppTask.h"
using namespace RTOS::Locks;
using namespace RTOS::Tasks;

MutexStatic mutex;

void task1Fn(void* params);
void task2Fn(void* params);
TaskStatic</*stack bytes*/ 4 * 1024> task1(/*name*/ "Task1", /*function*/ task1Fn, /*priority*/ 1);
TaskStatic</*stack bytes*/ 4 * 1024> task2(/*name*/ "Task2", /*function*/ task2Fn, /*priority*/ 1);

void blinkLED(const uint8_t times, const uint32_t delay_ms) {
  if (!mutex.take()) return;

  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }

  mutex.give();
}

void task1Fn(void*) {
  while (true) {
    blinkLED(3, 100);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task2Fn(void*) {
  while (true) {
    blinkLED(7, 50);
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  task1.create();
  task2.create();
}

void loop() {}
```

# Installation

## PlatformIO

Add to your `platformio.ini`:

```ini
[env:your_env]
; Most recent changes
lib_deps =
  https://github.com/alkonosst/RTOScppESP32.git

; Pinned release (recommended for production)
lib_deps =
  https://github.com/alkonosst/RTOScppESP32.git#vx.y.z
```

## Arduino IDE

1. Open Arduino IDE.
2. Go to **Sketch > Manage Libraries...**
3. Search for **"RTOScppESP32"**.
4. Click **Install**.

# Usage

## Including the library

Each primitive has its own header. Include only what you need:

| Header                | Primitives                             |
| --------------------- | -------------------------------------- |
| `RTOScppTask.h`       | `TaskDynamic`, `TaskStatic`            |
| `RTOScppTimer.h`      | `TimerDynamic`, `TimerStatic`          |
| `RTOScppLock.h`       | Mutexes and semaphores                 |
| `RTOScppQueue.h`      | `QueueDynamic`, `QueueStatic`, ...     |
| `RTOScppBuffer.h`     | Stream and message buffers             |
| `RTOScppRingBuffer.h` | No-split, split, and byte ring buffers |
| `RTOScppQueueSet.h`   | `QueueSet`                             |

## Namespaces

All types live in the `RTOS` namespace, with a sub-namespace per primitive family:

| Sub-namespace       | Contents                                         |
| ------------------- | ------------------------------------------------ |
| `RTOS::Tasks`       | Task types and `ITask`                           |
| `RTOS::Timers`      | Timer types and `ITimer`                         |
| `RTOS::Locks`       | Mutex and semaphore types, `ILock`, `ISemaphore` |
| `RTOS::Queues`      | Queue types and `IQueue`                         |
| `RTOS::Buffers`     | Buffer types and `IBuffer`                       |
| `RTOS::RingBuffers` | Ring buffer types                                |
| `RTOS::QueueSets`   | `QueueSet`                                       |

Use `using namespace` to avoid repeating the full path:

```cpp
#include "RTOScppTask.h"
using namespace RTOS::Tasks;

TaskStatic<4 * 1024> task;
```

Or alias the namespace for a shorter prefix:

```cpp
namespace RT = RTOS::Tasks;
RT::TaskStatic<4 * 1024> task;
```

## Memory allocation strategies

Every primitive is available in up to three variants:

| Variant           | Memory                  | RAM footprint at compile time | Notes                                      |
| ----------------- | ----------------------- | ----------------------------- | ------------------------------------------ |
| `Dynamic`         | FreeRTOS heap           | Not counted                   | Flexible, but adds heap fragmentation risk |
| `Static`          | Statically preallocated | Counted                       | Recommended for most cases                 |
| `ExternalStorage` | User-supplied buffer    | Not counted (user owns it)    | Ideal for PSRAM or custom memory pools     |

`ExternalStorage` objects require the user to allocate a buffer of at least `REQUIRED_SIZE` bytes and pass it to `create()`.

> [!NOTE]
> Prefer `Static` objects whenever possible. Static allocation avoids heap fragmentation and makes the total RAM usage visible at compile time - a good practice on resource-constrained microcontrollers.

All objects accept an optional `name` parameter for FreeRTOS tracing and debugging. A default name is used when no name is provided.

## Tasks

```cpp
#include "RTOScppTask.h"
using namespace RTOS::Tasks;

void taskFn(void* params);

// Dynamic: task created later via create(name, fn, priority, params, core)
TaskDynamic<4 * 1024> task1;

// Static: parameters supplied at construction; create() is called separately
TaskStatic<8 * 1024> task2("Task2", taskFn, /*priority*/ 1, /*params*/ nullptr, /*core*/ 1);

void setup() {
  task1.create("Task1", taskFn, 1);
  task2.create();
}
```

Key methods: `create()`, `suspend()`, `resume()`, `getState()`, `setPriority()`, `getPriority()`, `getPriorityFromISR()`, `notify()`, `notifyFromISR()`, `notifyGive()`, `notifyGiveFromISR()`, `notifyTake()`, `notifyWait()`, `updateStackStats()`.

## Timers

```cpp
#include "RTOScppTimer.h"
using namespace RTOS::Timers;

void timerFn(TimerHandle_t timer);

// Dynamic: timer created later via create(...)
TimerDynamic timer1;

// Static: all parameters at construction; optionally start immediately
TimerStatic timer2(
  /*name*/ "Timer2",
  /*callback*/ timerFn,
  /*period*/ pdMS_TO_TICKS(1000),
  /*id*/ nullptr,
  /*auto-reload*/ true,
  /*start*/ false);

void setup() {
  timer1.create("Timer1", timerFn, pdMS_TO_TICKS(500), nullptr, true, true);
  timer2.start();
}
```

Key methods: `create()`, `start()`, `stop()`, `reset()`, `isActive()`, `setPeriod()`, `getPeriod()`, `getExpiryTime()`, `getID()`, `setID()`. All have ISR counterparts: `startFromISR()`, `stopFromISR()`, `resetFromISR()`, `setPeriodFromISR()`.

## Locks

### Mutexes

Standard and recursive mutexes. They implement `ILock`.

```cpp
#include "RTOScppLock.h"
using namespace RTOS::Locks;

MutexDynamic mutex1;           // standard, dynamic
MutexStatic mutex2("MyMutex"); // standard, static, named

MutexRecursiveDynamic rec_mutex1; // recursive, dynamic
MutexRecursiveStatic rec_mutex2;  // recursive, static
```

```cpp
if (mutex2.take()) {
  // critical section
  mutex2.give();
}
```

> [!IMPORTANT]
> FreeRTOS does not allow mutexes to be taken or given from an ISR. Use binary or counting semaphores for ISR synchronization instead.

### Binary semaphores

Binary semaphores implement `ISemaphore`, which extends `ILock` with ISR methods.

```cpp
SemBinaryDynamic sem1;
SemBinaryStatic sem2("MySem");
```

### Counting semaphores

Template parameters set the maximum count and optional initial count.

```cpp
SemCountingDynamic</*max*/ 5> sem3; // initial count defaults to 0
SemCountingStatic</*max*/ 5, /*init*/ 3> sem4("CountSem");
```

Additional method: `getCount()` returns the current semaphore count.

### ISR methods

Binary and counting semaphores expose `takeFromISR()` and `giveFromISR()`:

```cpp
void IRAM_ATTR myISR() {
  BaseType_t task_woken = pdFALSE;
  sem2.giveFromISR(task_woken);
  portYIELD_FROM_ISR(task_woken);
}
```

### Interfaces

Use `ILock*` for polymorphic access to any lock type, or `ISemaphore*` when ISR methods are needed:

```cpp
ILock* lock = &mutex2;
lock->take();
lock->give();

ISemaphore* sem = &sem2;
BaseType_t woken = pdFALSE;
sem->giveFromISR(woken);
```

## Queues

```cpp
#include "RTOScppQueue.h"
using namespace RTOS::Queues;

QueueDynamic<uint32_t, 10> queue1;
QueueStatic<uint32_t, 10> queue2("MyQueue");
QueueExternalStorage<uint32_t, 10> queue3;

void setup() {
  static uint8_t buf[queue3.REQUIRED_SIZE];
  queue3.create(buf);
}

void loop() {
  uint32_t value = 42;
  queue2.add(value);        // add to back (FIFO)
  queue2.push(value);       // add to front (LIFO)

  uint32_t received;
  queue2.pop(received);     // remove from front
  queue2.peek(received);    // read front without removing
}
```

Key methods: `add()`, `addFromISR()`, `push()`, `pushFromISR()`, `pop()`, `popFromISR()`, `peek()`, `peekFromISR()`, `overwrite()`, `overwriteFromISR()`, `isFull()`, `isEmpty()`, `isFullFromISR()`, `isEmptyFromISR()`, `getAvailableMessages()`, `getAvailableMessagesFromISR()`, `getAvailableSpaces()`, `reset()`.

### Queue behavior on full

The third template parameter controls what happens when the queue is full during a non-ISR `add()` or `push()`. The default is `FullBehavior::Block` (block for `ticks_to_wait`). Use `FullBehavior::Fail` to return `false` immediately instead:

```cpp
#include "RTOScppQueue.h"
using namespace RTOS::Queues;

QueueStatic<uint32_t, 10, FullBehavior::Fail> queue_no_block;
```

## Buffers

### Stream buffers

Byte streams with a configurable trigger level: the receiver unblocks only when at least `TriggerBytes` bytes are available.

```cpp
#include "RTOScppBuffer.h"
using namespace RTOS::Buffers;

StreamBufferDynamic</*trigger*/ 5, /*length*/ 64> sb1;
StreamBufferStatic</*trigger*/ 5, /*length*/ 64> sb2("StreamBuf");
StreamBufferExternalStorage</*trigger*/ 5, /*length*/ 64> sb3;

void setup() {
  static uint8_t buf[sb3.REQUIRED_SIZE];
  sb3.create(buf);
}

void loop() {
  const char* msg = "hello";
  sb2.send(msg, strlen(msg));

  char rx[64];
  uint32_t received = sb2.receive(rx, sizeof(rx));
}
```

### Message buffers

Message buffers are stream buffers with implicit length framing - each `send()` stores the byte count alongside the payload so `receive()` always returns exactly one message.

```cpp
MessageBufferDynamic</*length*/ 128> mb1;
MessageBufferStatic</*length*/ 128> mb2("MsgBuf");
MessageBufferExternalStorage</*length*/ 128> mb3;

void setup() {
  static uint8_t buf[mb3.REQUIRED_SIZE];
  mb3.create(buf);
}
```

Key methods (both types): `send()`, `sendFromISR()`, `receive()`, `receiveFromISR()`, `reset()`, `isEmpty()`, `isFull()`, `getAvailableBytes()`, `getMaxAvailableBytes()`.

## Ring buffers

ESP-IDF ring buffers, exposed through the `RTOS::RingBuffers` namespace. Three buffer types are available.

### No-split

Items are always stored contiguously. A receive returns a single pointer to the item.

```cpp
#include "RTOScppRingBuffer.h"
using namespace RTOS::RingBuffers;

RingBufferNoSplitDynamic<char, 128> rb1;
RingBufferNoSplitStatic<char, 128> rb2("RingBuf");
RingBufferNoSplitExternalStorage<char, 128> rb3;

void setup() {
  static uint8_t buf[rb3.REQUIRED_SIZE];
  rb3.create(buf);
}

void loop() {
  const char* msg = "hello";
  rb2.send(msg, strlen(msg));

  size_t size;
  char* item = rb2.receive(size);
  if (item) {
    // use item...
    rb2.returnItem(item);
  }
}
```

### Split

Items may wrap around the end of the buffer and be split into two contiguous fragments. A receive returns head and tail pointers.

```cpp
RingBufferSplitDynamic<char, 128> split1;
RingBufferSplitStatic<char, 128> split2("SplitBuf");
RingBufferSplitExternalStorage<char, 128> split3;

void setup() {
  static uint8_t buf[split3.REQUIRED_SIZE];
  split3.create(buf);
}

void loop() {
  char* head;
  char* tail;
  size_t head_size, tail_size;

  if (split2.receive(head, tail, head_size, tail_size)) {
    // process head (head_size bytes) and tail (tail_size bytes)
    split2.returnItem(head);
    if (tail) split2.returnItem(tail);
  }
}
```

### Byte

Unframed byte stream. `receiveUpTo()` reads up to a given number of bytes in a single call.

```cpp
RingBufferByteDynamic<128> byte1;
RingBufferByteStatic<128> byte2("ByteBuf");
RingBufferByteExternalStorage<128> byte3;

void setup() {
  static uint8_t buf[byte3.REQUIRED_SIZE];
  byte3.create(buf);
}

void loop() {
  const uint8_t data[] = {0x01, 0x02, 0x03};
  byte2.send(data, sizeof(data));

  size_t received_size;
  uint8_t* item = byte2.receiveUpTo(/*max_bytes*/ 64, received_size);
  if (item) {
    // use item...
    byte2.returnItem(item);
  }
}
```

All ring buffer types share: `send()`, `sendFromISR()`, `returnItem()`, `returnItemFromISR()`, `isCreated()`, `getHandle()`, `getName()`.

## Queue sets

A queue set blocks on multiple queues and semaphores simultaneously, returning whichever member becomes ready first.

```cpp
#include "RTOScppQueueSet.h"
#include "RTOScppLock.h"
#include "RTOScppQueue.h"
using namespace RTOS::QueueSets;
using namespace RTOS::Locks;
using namespace RTOS::Queues;

SemBinaryStatic sem;
QueueStatic<uint32_t, 10> queue;

// Total event capacity = 1 (semaphore) + 10 (queue)
QueueSet queue_set(1 + 10);

void setup() {
  queue_set.add(sem);
  queue_set.add(queue);
}

void loop() {
  QueueSetMemberHandle_t member = queue_set.select(); // blocks indefinitely

  if (member == sem) {
    sem.take(0);
    // handle semaphore event
  } else if (member == queue) {
    uint32_t value;
    queue.pop(value, 0);
    // handle queue event
  }
}
```

Key methods: `add()`, `remove()`, `select()`, `selectFromISR()`.

# Release Status

This project is actively maintained. Report bugs or suggestions on the [GitHub Issues](https://github.com/alkonosst/RTOScppESP32/issues) page.

# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
