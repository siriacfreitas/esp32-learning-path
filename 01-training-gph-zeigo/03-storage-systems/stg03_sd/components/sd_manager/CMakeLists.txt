idf_component_register(SRCS "sd_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_sdspi
                    REQUIRES fatfs
                    REQUIRES spi_dev
                    REQUIRES driver)
