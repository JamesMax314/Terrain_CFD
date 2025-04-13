cmake_minimum_required(VERSION 3.6...3.14 FATAL_ERROR)
cmake_policy(VERSION 3.6...3.14)
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
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

find_package(Vulkan REQUIRED)

# Add Vulkan library path to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${Vulkan_LIBRARY_DIRS}")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
		add_subdirectory(tests)
endif()