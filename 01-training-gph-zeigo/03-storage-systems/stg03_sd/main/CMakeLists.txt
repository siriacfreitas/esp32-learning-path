idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES sd_manager
                    REQUIRES spiffs_manager
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_timer )
