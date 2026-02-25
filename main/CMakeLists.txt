idf_component_register(SRCS "main.cpp" "clockwheel.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES led-panel button-panel stepper-motor esp_timer) 
