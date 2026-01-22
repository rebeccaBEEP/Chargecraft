cmake_minimum_required(VERSION 3.20)

project(ev_project_starter_v2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optionnel mais recommandé (warnings pédagogiques)
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(ev_demo
        main.c
        events.c
        slist.c
        queue.c
        stack.c
        nary.c
        station_index.c
        rules.c
        csv_loader.c
        json_loader.c
)

# Pour que les headers soient trouvés (ici tout est à la racine)
target_include_directories(ev_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
