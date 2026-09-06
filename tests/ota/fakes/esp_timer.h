#pragma once
#include "esp_err.h"
#include <cstdint>
using esp_timer_handle_t = void*;
struct esp_timer_create_args_t { void (*callback)(void*); const char* name; };
esp_err_t esp_timer_create(const esp_timer_create_args_t*, esp_timer_handle_t*);
esp_err_t esp_timer_start_once(esp_timer_handle_t, int64_t);
esp_err_t esp_timer_stop(esp_timer_handle_t);
esp_err_t esp_timer_delete(esp_timer_handle_t);
