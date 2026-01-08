idf_component_register(
    SRCS "BYJ48Stepper.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio  #ligne ajoutée pour pouvoir définir les gpio dans le code
)
