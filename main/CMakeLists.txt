idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES BYJ48Stepper driver
                    REQUIRES esp_timer) #REQUIRES driver changé pour REQUIRES BYJ48Stepper driver