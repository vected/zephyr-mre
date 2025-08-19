zephyr_library()
zephyr_library_compile_options(
    -Wall -Wextra
    # Checks that are incompatible with Zephyr
    -Wno-variadic-macros
    -Wno-type-limits
)

zephyr_library_sources(
    tiff_display.c
)
