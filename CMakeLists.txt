cmake_minimum_required(VERSION 3.6...3.14 FATAL_ERROR)
cmake_policy(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)

project(thermal_cfd VERSION 0.1
			  DESCRIPTION "Computes thermal evolution using Navia Stokes equations"
			  LANGUAGES CXX)

set(CMAKE_CUDA_FLAGS_DEBUG "-G -g")

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glm)

find_package(Vulkan REQUIRED)

# Add Vulkan library path to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${Vulkan_LIBRARY_DIRS}")


# Add tests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
		add_subdirectory(tests)
endif()


# Add main executable
set(CMAKE_CURRENT_SOURCE_DIR src)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(thermal_cfd ${SRC_FILES})
target_link_libraries(thermal_cfd PRIVATE glfw glm Vulkan::Vulkan)
target_compile_definitions(thermal_cfd PRIVATE SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Set the directory for shaders
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SPIRV_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# Create the output directory for SPIR-V files
file(MAKE_DIRECTORY ${SPIRV_OUTPUT_DIR})

# List of shaders to compile
file(GLOB SHADERS "${SHADER_DIR}/*")

# Loop through each shader and add a custom command to compile it
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE) # Get the shader name without extension
    set(SPIRV_FILE ${SPIRV_OUTPUT_DIR}/${SHADER_NAME}.spv) # Output SPIR-V file path

    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND glslc ${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER} to ${SPIRV_FILE}"
        VERBATIM
    )

    # Add the SPIR-V file as a dependency for the executable
    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach()

# Ensure shaders are compiled before building the executable
add_custom_target(main_shader_comp DEPENDS ${SPIRV_FILES})
add_dependencies(thermal_cfd main_shader_comp)