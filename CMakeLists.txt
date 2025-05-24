cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(COMMAND idf_component_register)

  idf_component_register(
      SRC_DIRS      "src" "src/modules" "src/services"
      INCLUDE_DIRS  "src"
      PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_timer)

  # Nothing else in this file runs when we’re on ESP32.
  return()
endif()

file(GLOB_RECURSE LM_SRCS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/src/*.c")

if(NOT ESP_PLATFORM)
  list(FILTER LM_SRCS EXCLUDE REGEX ".*/EspHal\\.cpp$")
  list(FILTER LM_SRCS EXCLUDE REGEX ".*/services/.*\\.cpp$")
endif()

add_library(loramesher STATIC ${LM_SRCS})

target_include_directories(loramesher PUBLIC
     ${CMAKE_CURRENT_LIST_DIR}/src)

if(DEFINED FREERTOS_CFG_DIRECTORY)
  target_include_directories(loramesher PUBLIC ${FREERTOS_CFG_DIRECTORY})
endif()

target_link_libraries(loramesher
     PUBLIC
       RadioLib
       FreeRTOS-Kernel
       pico_stdlib
       hardware_spi)

set_target_properties(loramesher PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_compile_definitions(loramesher INTERFACE LORAMESHER_HAS_LIB=1)
