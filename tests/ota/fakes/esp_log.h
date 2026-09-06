#pragma once
template<typename... Args> void fake_log(const char*, const char*, Args...) {}
#define ESP_LOGE(...) fake_log(__VA_ARGS__)
#define ESP_LOGI(...) fake_log(__VA_ARGS__)
