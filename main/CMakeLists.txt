file(GLOB_RECURSE FILES "*.c")
file(GLOB_RECURSE LIBFILES "../../TSGL/TSGL/*.c")

idf_component_register(
    SRCS
        ${FILES}
        ${LIBFILES}
        
    INCLUDE_DIRS
        "."
        "../../TSGL/TSGL"
)

fatfs_create_spiflash_image(storage ../filesystem FLASH_IN_PROJECT)