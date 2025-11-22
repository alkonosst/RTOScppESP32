<h1 align="center">
  <a><img src=".img/logo.png" alt="Logo" width="300"></a>
  <br>
  RTOScppESP32
</h1>

<p align="center">
  <b>Make your real-time application development a breeze!</b>
</p>

<p align="center">
  <a href="https://www.ardu-badge.com/RTOScppESP32">
    <img src="https://www.ardu-badge.com/badge/RTOScppESP32.svg?" alt="Arduino Library Badge">
  </a>
  <a href="https://registry.platformio.org/libraries/alkonosst/RTOScppESP32">
    <img src="https://badges.registry.platformio.org/packages/alkonosst/library/RTOScppESP32.svg" alt="PlatformIO Registry">
  </a>
  <br><br>
  <a href="https://ko-fi.com/alkonosst">
    <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Ko-fi">
    </a>
</p>

---

# Table of Contents <!-- omit in toc -->

- [Description](#description)
- [Motivation](#motivation)
- [Documentation](#documentation)
- [Usage](#usage)
  - [Adding library to Arduino IDE](#adding-library-to-arduino-ide)
  - [Adding library to platformio.ini (PlatformIO)](#adding-library-to-platformioini-platformio)
  - [Using the library](#using-the-library)
    - [Including the library](#including-the-library)
    - [Namespaces](#namespaces)
    - [Dynamic, static and external memory objects](#dynamic-static-and-external-memory-objects)
  - [RTOS objects](#rtos-objects)
    - [Using tasks](#using-tasks)
    - [Using timers](#using-timers)
    - [Using locks](#using-locks)
    - [Using queues](#using-queues)
    - [Using FreeRTOS buffers](#using-freertos-buffers)
    - [Using ESP-IDF Ring Buffers](#using-esp-idf-ring-buffers)
    - [Using queue sets](#using-queue-sets)
- [License](#license)

# Description

**RTOScppESP32** is your go-to C++ library for the ESP32 platform, designed to make real-time application development fun and easy. With a user-friendly API, it brings the power of FreeRTOS to your fingertips, allowing you to effortlessly manage tasks, timers, queues, buffers, and locks. Key features include:

- Task Management: Simplify task creation and management with intuitive functions.
- Timers: Use dynamic and static timers for precise timing operations.
- Queues: Enhance inter-task communication with versatile queue options.
- Buffers: Efficiently handle data with various buffer types.
- Locks: Ensure thread safety with robust locking mechanisms.

# Motivation

I've been working with the ESP32 for a while now, and I've always found the FreeRTOS API to be a bit
_cumbersome_. Maybe it's the fact that I prefer C++ over C. I like the idea that I can create classes
and objects to encapsulate functionality and make my code more readable and maintainable, and take
advantage of the IDE features like code completion to see what methods and properties are available.
I don't like the idea of having to remember function names for all the FreeRTOS API calls, or having
to look them up in the documentation every time I need to use them.

I wanted to create a library that would simplify the process of working with
FreeRTOS, making it more intuitive and user-friendly. That's how **RTOScppESP32** was born. I hope
this library will help you streamline your real-time application development and make your projects
more enjoyable.

# Documentation

If you are new to FreeRTOS, I recommend you to read the [official
documentation](https://www.freertos.org/Documentation/01-FreeRTOS-quick-start/01-Beginners-guide/01-RTOS-fundamentals).
Also you can check the [API
Reference](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/00-TaskHandle)
for more information about the FreeRTOS API.

And if you want to know how the FreeRTOS is implemented in the ESP32, you can check the [ESP-IDF
documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html).
It is worth reading the [Ring Buffers API
Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_additions.html#ring-buffers),
because that's an additional feature that is not present in the FreeRTOS API and is implemented in
this library.

Besides that, all the classes and methods are documented in the source code, so you can check the
comments in the header files to see what each method does and how to use it. Also you can check the
examples in the `examples` folder to see how to use the library in practice.

# Usage

## Adding library to Arduino IDE

Search for the library in the Library Manager.

## Adding library to platformio.ini (PlatformIO)

```ini
; Most recent changes
lib_deps =
  https://github.com/alkonosst/RTOScppESP32.git

; Release vx.y.z (using an exact version is recommended)
lib_deps =
  https://github.com/alkonosst/RTOScppESP32.git#v1.0.0
```

## Using the library

### Including the library

To use a feature, you need to include the corresponding header file. For example, to use **Tasks**
you need to include the `RTOScppESP32.h` file:

```cpp
#include "RTOScppTask.h"
```

### Namespaces

All the features are inside the `RTOS` namespace. Inside this namespace, you will find another one
for each feature. For example, the `RTOS::Tasks` includes all the classes related to tasks. So, if you
want to use the `TaskStatic` class, you need to use the full name:

```cpp
#include "RTOScppTask.h"
RTOS::Tasks::TaskStatic<4096> task;
```

If you prefer, you can rename the namespace to something shorter:

```cpp
#include "RTOScppTask.h"
namespace RT = RTOS::Tasks;
RT::TaskStatic<4096> task;
```

Or you can use the `using` directive to avoid using the full name:

```cpp
#include "RTOScppTask.h"
using namespace RTOS::Tasks;
TaskStatic<4096> task;
```

### Dynamic, static and external memory objects

The library provides three types of objects: **Dynamic**, **Static** and some of them have **External**:

- **Dynamic**: The object will be created dynamically in the heap memory. When you compile your
  program, the size of the object will not be included in the total RAM usage of the program.
- **Static**: The object will be created statically in the stack memory. When you compile your
  program, the size of the object will be included in the total RAM usage of the program.
- **External**: The object will be created externally by the user. The user is responsible for
  allocating memory for the object and passing the pointer to the corresponding method for creating
  it. Useful when you want to create the object in a specific memory region (_like the ESP32
  external PSRAM_).

I recommend you to use the **Static** objects whenever possible, because they are more efficient in
terms of memory usage and performance. It is a good practice to avoid dynamic memory allocation in a
microcontroller with limited resources.

## RTOS objects

For now, only the constructors are explained here. You can check the methods using the code
completion feature of your IDE or checking the source code of each object type.

### Using tasks

```cpp
#include "RTOScppTask.h"
using namespace RTOS::Tasks;

// Dynamic version example
// - The stack size is 4096 bytes
// - Empty constructor, so you need to call the `create(parameters)` method to create the task
void task1Function(void* params);
TaskDynamic</*stack size*/ 4096> task_1;

// Static version example
// - The stack size is 4096 bytes
// - All parameters are passed to the constructor
// - You can choose to create the task in the constructor or later with the `create()` method
void task2Function(void* params);
TaskStatic</*stack size*/ 4096> task_1("Task2Name", task2Function, /*priority*/ 1, /*parameters*/ nullptr, /*core*/ 1, /*create*/ false);

void setup() {
  // Setup and create the dynamic task
  task_1.create("Task1Name", task1Function, /*priority*/ 1, /*parameters*/ nullptr, /*core*/ 1);

  // Setup and create the static task
  task_2.create();
}
```

### Using timers

```cpp
#include "RTOScppTimer.h"
using namespace RTOS::Timers;

// Dynamic version example
// - Empty constructor, so you need to call the `create(parameters)` method to create the timer
void timer1Callback(TimerHandle_t timer);
TimerDynamic timer_1;

// Static version example
// - All parameters are passed to the constructor
// - You can choose to start the timer in the constructor or later with the `start()` method
void timer2Callback(TimerHandle_t timer);
TimerStatic timer_2(
  /*name*/ "Timer2Name",
  /*callback*/ timer2Callback,
  /*period*/ 1000,
  /*id*/ nullptr,
  /*autoreload*/ true,
  /*start*/ true);

void setup() {
  // Create and start the dynamic timer
  timer_1.create(
    /*name*/ "Timer1Name",
    /*callback*/ timer1Callback,
    /*period*/ 1000,
    /*id*/ nullptr,
    /*autoreload*/ true,
    /*start*/ true);
}
```

### Using locks

```cpp
#include "RTOScppLock.h"
using namespace RTOS::Locks;

// Mutexes
MutexDynamic mutex_1; // Dynamic version
MutexStatic mutex_2; // Static version

// Recursive mutexes
MutexRecursiveDynamic mutex_recursive_1; // Dynamic version
MutexRecursiveStatic mutex_recursive_2; // Static version

// Binary semaphores
SemBinaryDynamic sem_binary_1; // Dynamic version
SemBinaryStatic sem_binary_2; // Static version

// Counting semaphores
SemCountingDynamic</*max count*/ 5, /*initial count*/ 0> sem_counting_1; // Dynamic version
SemCountingStatic</*max count*/ 5, /*initial count*/ 0> sem_counting_2; // Static version
```

### Using queues

```cpp
#include "RTOScppQueue.h"
using namespace RTOS::Queues;

// Dynamic version
QueueDynamic</*type*/ uint32_t, /*length*/ 10> queue_1;

// Static version
QueueStatic</*type*/ uint32_t, /*length*/ 10> queue_2;

// External version
QueueExternal</*type*/ uint32_t, /*length*/ 10> queue_3;

void setup() {
  // Create the external queue
  static uint8_t* ext_buffer = static_cast<uint8_t*>(malloc(queue_3.REQUIRED_SIZE));
  queue_3.create(ext_buffer);
}
```

### Using FreeRTOS buffers

```cpp
#include "RTOScppBuffer.h"
using namespace RTOS::Buffers;

// Stream buffers
StreamBufferDynamic</*trigger bytes*/ 5, /*length*/ 10> stream_buffer_1; // Dynamic version
StreamBufferStatic</*trigger bytes*/ 5, /*length*/ 10> stream_buffer_2; // Static version
StreamBufferExternalStorage</*trigger bytes*/ 5, /*length*/ 10> stream_buffer_3; // External version

// Message buffers
MessageBufferDynamic</*length*/ 10> message_buffer_1; // Dynamic version
MessageBufferStatic</*length*/ 10> message_buffer_2; // Static version
MessageBufferExternalStorage</*length*/ 10> message_buffer_3; // External version

void setup() {
  // Create the external stream buffer
  static uint8_t* sb_ext_buffer = static_cast<uint8_t*>(malloc(stream_buffer_3.REQUIRED_SIZE));
  stream_buffer_3.create(sb_ext_buffer);

  // Create the external message buffer
  static uint8_t* mb_ext_buffer = static_cast<uint8_t*>(malloc(message_buffer_3.REQUIRED_SIZE));
  message_buffer_3.create(mb_ext_buffer);
}
```

### Using ESP-IDF Ring Buffers

```cpp
#include "RTOScppRingBuffer.h"
using namespace RTOS::RingBuffers;

// No-split ring buffers
RingBufferNoSplitDynamic</*type*/ char, /*length*/ 10> ring_buffer_no_split_1; // Dynamic version
RingBufferNoSplitStatic</*type*/ char, /*length*/ 10> ring_buffer_no_split_2; // Static version
RingBufferNoSplitExternalStorage</*type*/ char, /*length*/ 10> ring_buffer_no_split_3; // External version

// Split ring buffers
RingBufferSplitDynamic</*type*/ char, /*length*/ 10> ring_buffer_split_1; // Dynamic version
RingBufferSplitStatic</*type*/ char, /*length*/ 10> ring_buffer_split_2; // Static version
RingBufferSplitExternalStorage</*type*/ char, /*length*/ 10> ring_buffer_split_3; // External version

// Byte ring buffers
RingBufferByteDynamic</*length*/ 10> ring_buffer_byte_1; // Dynamic version
RingBufferByteStatic</*length*/ 10> ring_buffer_byte_2; // Static version
RingBufferByteExternalStorage</*length*/ 10> ring_buffer_byte_3; // External version

void setup() {
  // Create the external no-split ring buffer
  static uint8_t* rbs_ext_buffer = static_cast<uint8_t*>(malloc(ring_buffer_no_split_3.REQUIRED_SIZE));
  ring_buffer_no_split_3.create(rbs_ext_buffer);

  // Create the external split ring buffer
  static uint8_t* rbs_ext_buffer = static_cast<uint8_t*>(malloc(ring_buffer_split_3.REQUIRED_SIZE));
  ring_buffer_split_3.create(rbs_ext_buffer);

  // Create the external byte ring buffer
  static uint8_t* rbs_ext_buffer = static_cast<uint8_t*>(malloc(ring_buffer_byte_3.REQUIRED_SIZE));
  ring_buffer_byte_3.create(rbs_ext_buffer);
}
```

### Using queue sets

```cpp
#include "RTOScppQueueSet.h"
using namespace RTOS::QueueSets;

// Binary Semaphore
SemBinaryStatic sem;

// Queue, type uint32_t, size 10
QueueStatic<uint32_t, 10> queue;

// Queue set, holding 1 event from the semaphore + 10 events from the queue
QueueSet queue_set(1 + 10);

void setup() {
  // Add the semaphore and queue to the queue set
  queue_set.add(sem);
  queue_set.add(queue);
}

void loop() {
  // Block indefinitely until an event occurs in the queue set
  QueueSetHandle_t member = queue_set.select();

  if (member == sem) {
    // Semaphore event
  } else if (member == queue) {
    // Queue event
  }
}
```

# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
