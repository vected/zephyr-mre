cmake_minimum_required(VERSION 3.13.1)

# Set default board
set(BOARD "mimxrt1170_evk@B/mimxrt1176/cm7" CACHE STRING "Selected Board")

find_package(Zephyr-sdk 0.17.2 EXACT REQUIRED)
find_package(Zephyr 4.2.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app LANGUAGES C CXX)

target_compile_options(app PRIVATE
    -Wall -Wextra
    # Checks that are incompatible with Zephyr
    -Wno-variadic-macros
    -Wno-type-limits
)

add_subdirectory(src)
