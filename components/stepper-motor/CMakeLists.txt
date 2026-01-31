<<<<<<< HEAD:components/BYJ48Stepper/CMakeLists.txt
<<<<<<< HEAD
idf_component_register(
    SRCS "BYJ48Stepper.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio  #ligne ajoutée pour pouvoir définir les gpio dans le code
)
=======
idf_component_register(SRCS "BYJ48Stepper.cpp"
=======
idf_component_register(SRCS "stepper-motor.cpp" "stepper-motor-4p.cpp"
>>>>>>> 6529d3f9b116523d76b1d92b84388d0b6768ebbc:components/stepper-motor/CMakeLists.txt
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_gpio)
>>>>>>> 8e704f166c44933ac91895bf2b7e1fd2c0d4830f
