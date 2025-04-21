add_executable(bootstrap bootstrap.cpp)
target_link_libraries(bootstrap PRIVATE glfw glm Vulkan::Vulkan vk-bootstrap::vk-bootstrap)
target_compile_definitions(bootstrap PRIVATE SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")

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
add_custom_target(compile_shaders_boot DEPENDS ${SPIRV_FILES})
add_dependencies(bootstrap compile_shaders_boot)