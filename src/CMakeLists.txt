# Установить минимальную версию cmake
cmake_minimum_required(VERSION 3.21)

# Предоставляет удобные команды, предназначенные для отладки, для вывода значений свойств и переменных
include(CMakePrintHelpers)

# Сформировать список поддиректорий текущей директории, рекурсивно
function(Collect_Subdirectories result curdir)
    FILE(GLOB_RECURSE children LIST_DIRECTORIES true CONFIGURE_DEPENDS ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist} PARENT_SCOPE)
endfunction()

# Поместить все целевые файлы RUNTIME при сборке
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${Runtime_Output_Directory})

add_executable(${Application_Name} main.cpp libtelnet.c termiWin.c ConfigManager.cpp DNSResolver.cpp Logger.cpp)

# Задать переменную списка поддиректорий с заголовками
set(Include_Subdirectories "")
foreach(subdirectory ${Include_Directories})
    set(subdirectory_List "")
    Collect_Subdirectories(subdirectory_List ${subdirectory})
    LIST(APPEND Include_Subdirectories ${subdirectory_List} ${subdirectory})
endforeach()

# Для отладки. Пока оставлю
cmake_print_variables(Include_Subdirectories)

# Добавить к цели директории с заголовками
target_include_directories(${Application_Name} PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(${Application_Name} PUBLIC ${Include_Subdirectories})