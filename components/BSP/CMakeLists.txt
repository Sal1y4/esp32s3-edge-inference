set(src_dirs
            .
            LED
            BLE
            DC01
            DHT11
            MQ1_LIGHT
            DRV8833_FUN
            TOUCH
            MODEL)

set(include_dirs
            .
            LED
            BLE
            DC01
            DHT11
            MQ1_LIGHT
            DRV8833_FUN
            TOUCH
            MODEL)

set(requires
            driver
            bt
            nvs_flash
            esp_adc
            esp-dl)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)