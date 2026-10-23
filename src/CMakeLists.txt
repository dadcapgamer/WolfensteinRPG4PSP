# Agrupar archivos por tipo
file(GLOB_RECURSE HEADER_FILES
    "*.hpp"
    "*.h"
)

file(GLOB_RECURSE SOURCE_FILES
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE RESOURCES_FILES
    "*.rc"
    "*.ico"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Resources Files" FILES ${RESOURCES_FILES})

# Crear el ejecutable
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCES_FILES})

# Enlace de bibliotecas
target_link_libraries(${PROJECT_NAME} SDL2::SDL2 ZLIB::ZLIB OpenGL::GL ${HASH_LIBRARY_TGT_NAME})

if(PSP)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WOLFENRPG_PSP=1)
    # The upstream headers rely on desktop SDK headers pulling in stdint.h.
    target_compile_options(${PROJECT_NAME} PRIVATE
        -include cstdint
        -fpermissive
        -Wno-narrowing
        -Wno-write-strings)
    target_link_libraries(${PROJECT_NAME}
        OpenAL::OpenAL
        GL pspvram pspaudiolib pspaudio pspvfpu
        pspdisplay pspgu pspge psphprm pspctrl psppower
        stdc++ m)
    target_link_options(${PROJECT_NAME} PRIVATE
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map")
    create_pbp_file(
        TARGET ${PROJECT_NAME}
        TITLE "Wolfenstein RPG4PSP"
        ICON_PATH "${PROJECT_SOURCE_DIR}/assets/psp/ICON0.PNG"
        BACKGROUND_PATH "${PROJECT_SOURCE_DIR}/assets/psp/PIC1.PNG"
        PREVIEW_PATH NULL)
endif()

# Enlace de OpenAL según el sistema operativo
if (PSP)
    # Linked above with the remaining PSP runtime libraries.
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} OpenAL::OpenAL ${OPENGL_gl_LIBRARY})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} OpenGL::GL ${OPENAL_LIBRARY})
    # Verifica si realmente necesitas una compilación estática
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
elseif (APPLE)
    target_link_libraries(${PROJECT_NAME} OpenAL::OpenAL)
endif()
