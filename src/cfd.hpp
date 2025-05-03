#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

#include "vkHelper.hpp"
#include "shaderHelper.hpp"

struct Cfd {
    int gridSize;

    buffer boundaries;

    buffer vx;
    buffer vy;
    buffer vz;

    buffer vx2;
    buffer vy2;
    buffer vz2;

    buffer density;
    buffer pressure;

    buffer density2;
    buffer pressure2;

    texture densityTex;

    kernel kernGaussSiedel;
    kernel kern;
    kernel kern2;
    kernel kernWriteTex;
    kernel kernWriteTex2;
};

struct PushConstants {
    int gridSize;
    int shouldRed;
};

int create_command_buffers(Init& init, RenderData& data, std::vector<texture>& textures);

void init_cfd(Init& init, ComputeHandler& computeHandler, Cfd& cfd, int gridSize);

void load_terrain(Init& init, Cfd& cfd, const std::string& filename);

void evolve_cfd(Init& init, ComputeHandler& computeHandler, Cfd& cfd);

void cleanup(Init& init, Cfd& cfd);
