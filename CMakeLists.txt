# Установить минимальную версию cmake
cmake_minimum_required(VERSION 3.21)

# Задать имя для проекта, версию, название языка
project(test_telnet_ssh-solution VERSION 1.0.0  LANGUAGES C CXX)

# Установить стандарт C++
set(CMAKE_CXX_STANDARD 20)
# Установить требование стандарта С++
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Отключить расширения компилятора
set(CMAKE_CXX_EXTENSIONS OFF)

# Поместить все целевые файлы RUNTIME при сборке
set(Runtime_Output_Directory "../../runtime")

# Установить языковые флаги при сборке для всех конфигураций. Эти флаги будут передаваться при всех вызовах компилятора
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -static -LC:\\MinGW-W64\\x86_64-13.2.0-release-posix-seh-ucrt-rt_v11-rev1\\x86_64-w64-mingw32\\lib -lwinpthread")

# Задать имя приложения
set(Application_Name "test_telnet_ssh")
# Задать переменную списка директорий с заголовками
set(Include_Directories "")
# Добавить директории с заголовками
LIST(APPEND Include_Directories ${CMAKE_SOURCE_DIR}/include)
LIST(APPEND Include_Directories ${CMAKE_SOURCE_DIR}/src)

set(Boost_INCLUDE_DIR "C:\\boost_1_87_0")
find_package(Boost REQUIRED)

#find_package(libssh2 REQUIRED)

# В зависимости от добавления библиотеки в проект, закомментировать-раскомментировать if-endif и find_package
#find_package(libssh2 REQUIRED)
if (NOT TARGET libssh2)
    add_subdirectory(./libraries/libssh2-libssh2-1.11.1)
endif()

if (NOT TARGET snmp_pp)
    add_subdirectory(./libraries/snmp++-3.6.3)
endif()

add_subdirectory(src)

# Указать библиотеки, которые нужно использовать при компоновке заданной цели и/или её зависимостей
target_link_libraries(${Application_Name} PRIVATE ws2_32)
target_link_libraries(${Application_Name} PRIVATE libssh2::libssh2)
target_link_libraries(${Application_Name} PRIVATE snmp_pp)
#target_link_libraries(${Application_Name} PRIVATE KazDragon::telnetpp)




