#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp


std::vector<glm::vec2> generate2DPositions(PointsGenerationParameters const& params)
{
    float const cell = params.radius / std::sqrt(2.0f);
    int const gW = static_cast<int>(std::ceil(1.0f / cell));
    int const gH = static_cast<int>(std::ceil(1.0f / cell));

    std::vector<int> grid(gW * gH, -1);
    std::vector<glm::vec2> points;
    std::vector<int> active;

    auto rand01 = []() { return static_cast<float>(GetRandomValue(0, INT_MAX)) / INT_MAX; };
    auto toCell = [&](glm::vec2 p) { return glm::ivec2{ p / cell }; };
    auto addPoint = [&](glm::vec2 p) {
        auto c = toCell(p);
        grid[c.y * gW + c.x] = points.size();
        active.push_back(points.size());
        points.push_back(p);
    };

    auto isValid = [&](glm::vec2 p) {
        if (p.x < 0 || p.x >= 1 || p.y < 0 || p.y >= 1) return false;
        auto c = toCell(p);
        for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx) {
            auto n = c + glm::ivec2{dx, dy};
            if (n.x < 0 || n.x >= gW || n.y < 0 || n.y >= gH) continue;
            if (grid[n.y * gW + n.x] != -1 && glm::distance(p, points[grid[n.y * gW + n.x]]) < params.radius) return false;
        }
        return true;
    };

    addPoint({ rand01(), rand01() });

    while (!active.empty()) {
        int i = GetRandomValue(0, active.size() - 1);
        glm::vec2 ori = points[active[i]];
        bool found = false;

        for (int a = 0; a < params.k && !found; ++a) {
            float angle = rand01() * 2.0f * PI;
            float dist  = params.radius * (1.0f + rand01());
            glm::vec2 c = ori + glm::vec2{ std::cos(angle), std::sin(angle) } * dist;
            if (isValid(c)) { addPoint(c); found = true; }
        }

        if (!found) { active[i] = active.back(); active.pop_back(); }
    }

    return points;
}

    // std::vector<glm::vec2> positions {};

    // positions.reserve(1000);
    // // Naive random generation
    // for (int i {0}; i < 1000; ++i)
    // {
    //     positions.emplace_back(
    //         static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX),
    //         static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)
    //     );
    // }

    // TODO(student): implement Poisson disk sampling to replace the above naive random generation
    // points output should be in [0..1] range, where (0,0) is one corner of the terrain and (1,1) is the opposite corner, so they can be easily scaled to terrain size and sampled from heightmap.

void generateObjectsPositions(AppContext& context) {
    std::vector<glm::vec2> const positions {generate2DPositions(context.pointsGenerationParameters)};

    context.objectPositions.clear();
    context.objectPositions.reserve(positions.size());
    for (glm::vec2 const& p : positions)
    {
        context.objectPositions.emplace_back(
            p.x, // x
            p.y, // y
            // sample height from heightmap for each point (asuming positions are normalized in [0..1] range)
            sampleHeightmap(context, p.x, p.y)
        );
    }
    // TODO(student): extension - filter positions by sampled height range.
}

float sampleHeightmap(AppContext const& context, float u, float v)
{
    if (!context.heightmapImage.data || context.heightmapImage.width <= 0 || context.heightmapImage.height <= 0) return 0.0f;

    int const px = std::clamp(static_cast<int>(u * static_cast<float>(context.heightmapImage.width - 1)), 0, context.heightmapImage.width - 1);
    int const py = std::clamp(static_cast<int>(v * static_cast<float>(context.heightmapImage.height - 1)), 0, context.heightmapImage.height - 1);

    // If the heightmap is in R32 format, we can directly read the height value as a float. 
    if (context.heightmapImage.format == PIXELFORMAT_UNCOMPRESSED_R32)
    {
        float const* heightData = static_cast<float const*>(context.heightmapImage.data);
        int const idx = py * context.heightmapImage.width + px;
        return std::clamp(heightData[idx], 0.0f, 1.0f);
    }

    // Otherwise, we assume it's in a color format and we read the red channel as height (with normalization from [0..255] to [0..1]).
    Color const c = GetImageColor(context.heightmapImage, px, py);
    return static_cast<float>(c.r)/255.0f;
}

void generateHeightmap(AppContext& context) {

    if (context.texture.id > 0) {
        UnloadTexture(context.texture);
        context.texture = {};
    }

    if(context.image.data) {
        UnloadImage(context.image);
        context.image = {};
    }

    if (context.heightmapImage.data) {
        UnloadImage(context.heightmapImage);
        context.heightmapImage = {};
    }

    int const resolution = std::max(1, context.imageGenerationParameters.resolution);

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float {
        auto noiseFunc = [&](glm::vec2 const& pos) -> float {
            return perlinNoiseSeeded(pos, context.imageGenerationParameters.noiseSeed);
        };
        glm::vec2 const pCentered = p - glm::vec2{0.5};
        float factor = 1- glm::smoothstep(0.25f, 1.0f, glm::length(pCentered)/glm::length(glm::vec2{0.5}));

        return (octaveNoise(p, noiseFunc, context.imageGenerationParameters.fbmParams) * 0.5f + 0.5f)*factor;
        });

    // exemple conversion from heightmap to color image
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {
        Color sand = color_from({ 238, 214, 175 });
        Color grass = color_from({ 34, 130, 34 });
        Color snow = color_from({ 200, 200, 200 });
        if (v < 0.3f)
        {
            return ColorLerp(DARKBLUE,BLUE,v/0.3f); // water
        }
        else if (v < 0.4f)
        {

            return ColorLerp(BLUE,sand,(v-0.3f)/0.1f);// sand
        }
        else if (v < 0.5f)
        {

            return ColorLerp(sand,grass,(v-0.4f)/0.1f);// sand
        }
        else if (v < 0.9f)
        {
            return ColorLerp(grass,DARKGREEN,(v-0.8f)/0.4f); // grass
        } else 
        {
            return ColorLerp(grass,snow,(v-0.9f)/0.1f);
        }
        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}