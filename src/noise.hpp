#pragma once

#include <glm/glm.hpp>
#include <functional>

float perlinNoise(glm::vec2 const& position);
float perlinNoiseSeeded(glm::vec2 const& position, int seed);

struct FBM {
    int octaves = 8;
    float lacunarity = 2.0f;
    float gain = 0.5f;
    float scale = 3.0f; 
};

float octaveNoise(glm::vec2 const& position, std::function<float(glm::vec2 const&)> noiseFunction, FBM const& params = {});
