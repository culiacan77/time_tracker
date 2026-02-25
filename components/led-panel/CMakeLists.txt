idf_component_register(SRCS "led_panel.cpp" 
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio led_strip button-panel) 
