#pragma once

#include <GLFW/glfw3.h>

#include "vkHelper.hpp"
#include "shaderHelper.hpp"

int create_command_buffers(Init& init, RenderData& data, std::vector<texture>& textures);
int create_graphics_pipeline(Init& init, RenderData& data, std::vector<texture>& textures);
