#pragma once
#include <stdexcept>
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NOT_FOUND = 1, ESP_ERR_NOT_SUPPORTED = 2;
inline const char* esp_err_to_name(int) { return "fake-error"; }
#define ESP_ERROR_CHECK(x) do { if ((x) != ESP_OK) throw std::runtime_error("ESP failure"); } while (0)
