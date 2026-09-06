idf_component_register(SRCS "ota_policy.cc" "ota_client.cc" "ota_health.cc"
    INCLUDE_DIRS "include"
    REQUIRES app_update esp_app_format esp_partition esp_http_client mbedtls esp_timer json)
if(NOT CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)
    message(FATAL_ERROR "LucidCairn recovery requires bootloader application rollback")
endif()
if(CONFIG_BOOTLOADER_SKIP_VALIDATE_ALWAYS)
    message(FATAL_ERROR "Use sdkconfig.defaults.lucidcairn; boot image validation must remain enabled")
endif()
