#pragma once

#include <glm/glm.hpp>
#include <functional>

float perlinNoise(glm::vec2 const& position);
float perlinNoiseSeeded(glm::vec2 const& position, int seed);

float octaveNoise(glm::vec2 const& position, std::function<float(glm::vec2 const&)> noiseFunction);

struct FBM {
    int octaves = 2;
    float lacunarity = 0.5f;
    float gain = 0.5f;
    float scale = 4.0f; 
};