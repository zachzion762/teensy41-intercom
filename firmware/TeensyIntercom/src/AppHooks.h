#pragma once
//
// Callbacks the configuration front-ends (serial console, web page) use to
// reach back into the application. Plain function pointers keep both
// front-ends independent of the application type.
//

#include <Arduino.h>

struct AppHooks {
  void (*onSave)(void* ctx)                  = nullptr;
  void (*onReboot)(void* ctx)                = nullptr;
  void (*onFactoryReset)(void* ctx)          = nullptr;
  void (*printStatus)(void* ctx, Print& out) = nullptr;
  void* ctx                                  = nullptr;
};
