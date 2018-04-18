project(lyn)
cmake_minimum_required(VERSION 3.1)

option(USE_STATIC_LIBRARIES "When on, passes -static to the compiler" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(SOURCE_LIST
    main.cpp

	util/hex_write.h

    core/binary_file.h
    core/binary_file.cpp

    elf/raw_elf.h

    elf/elf_file.h
    elf/elf_file.cpp

    ea/event_code.h
    ea/event_code.cpp

    ea/event_section.h
    ea/event_section.cpp

    core/section_data.h
    core/section_data.cpp

    core/event_object.h
    core/event_object.cpp

    core/arm_relocator.h
    core/arm_relocator.cpp
)

# this is to ensure inclusion relative to base directory is allowed
include_directories(${CMAKE_SOURCE_DIR})

if(USE_STATIC_LIBRARIES)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif()

add_executable(${PROJECT_NAME} ${SOURCE_LIST})
