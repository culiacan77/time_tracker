<<<<<<< HEAD
<<<<<<< HEAD
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES BYJ48Stepper driver  #signifie: « Mon composant main dépend du composant BYJ48Stepper (composant que j'ai crée) 
                                  #et du composant système driver. » 
                                  #il sait ou trouver BYJ48Stepper.h car lorsque l'on fait un build, tous les sous dossier dans le dossier components sont parcourus. 
                                  #Chaque sous dossier contenant un fichier CMakeLists.txt est considéré comme un composant. 
                                  #le fichier CMakeLists.txt fait en sorte que l'on puisse utiliser #include dans nos fichier C++. 
                                  #C'est une automatisation qui nous simplifie la vie dans le sens ou comme les fichiers sont parcourus automatiquement
                                  #on a pas besoin de fournir les path de ce qu'on inclus.
)
=======
idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES BYJ48Stepper driver
                    REQUIRES esp_timer) #REQUIRES driver changé pour REQUIRES BYJ48Stepper driver
>>>>>>> 8e704f166c44933ac91895bf2b7e1fd2c0d4830f
=======
idf_component_register(SRCS "main.cpp" "clockwheel.cpp"
                    INCLUDE_DIRS "."
                    ) 
>>>>>>> 6529d3f9b116523d76b1d92b84388d0b6768ebbc
