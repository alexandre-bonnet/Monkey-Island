#include "draw.hpp"

#include "app.hpp"

#include "generation.hpp"
#include <iostream>

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"


int frameCount{0};
Vector3 boatPos = {0,0,0};
int boatSpeed{1};

void draw3DScene(AppContext& context) {
    ClearBackground(context.isNight ? Color{ 5,  10,  40, 255} : DARKBLUE);
    
    BeginMode3D(context.camera);

    Matrix const terrainCentering { getTerrainCenteringMatrix(context) };
    Vector3 const terrainCenterOffset { terrainCentering.m12, terrainCentering.m13, terrainCentering.m14 };

    DrawModel(context.model, terrainCenterOffset, 1.0f, WHITE);
    drawCubes(context, terrainCentering);
    //DrawGrid(20, 1.0f);
    drawBoat(context,terrainCentering);
    frameCount++;
    EndMode3D();
}

void drawBoat(AppContext const& context, Matrix const& terrainCentering){
    Vector3 boatscale = {context.cubeScale, context.cubeScale, context.cubeScale};
    context.boat.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.boat_texture;
    
    float angle = boatSpeed*frameCount/50.f;
    boatPos = {6*cos(angle), 2.0, -6*sin(angle)};
    DrawModelEx(context.boat,boatPos, { 0.0f, 1.0f, 0.0f }, angle*RAD2DEG-90, boatscale ,WHITE);

}

void drawCubes(AppContext const& context, Matrix const& terrainCentering)
{
    context.palm_tree.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.palm_tree_texture;
    context.rocks.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.rocks_texture;
    context.house.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.house_texture;

    if (context.objectPositions.empty()) {
        return;
    }

    float const cubeHalfHeight { 0.5f * context.cubeScale };

    for (glm::vec3 const& pos : context.objectPositions) {
        Matrix const objectTranslation { MatrixTranslate(
            pos.x * context.terrainSize.x,
                pos.z * context.terrainSize.y + cubeHalfHeight,
            pos.y * context.terrainSize.z
        )};
            
            Matrix const centeredTranslation { MatrixMultiply(objectTranslation, terrainCentering) };
            Matrix const scale { MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale) };
            Matrix const transform { MatrixMultiply(scale, centeredTranslation) };
            
            
            if( pos.z*context.terrainSize.y<2.3f){
                Vector3 treePos = {
                    transform.m12,
                    transform.m13-0.8f*context.cubeScale,
                    transform.m14
                };
              DrawModel(context.palm_tree,treePos, 0.08f*context.cubeScale ,WHITE);
            }
            if( pos.z*context.terrainSize.y>2.3f && pos.z*context.terrainSize.y<3){
                Vector3 rocksPos = {
                    transform.m12,
                    transform.m13-context.cubeScale,
                    transform.m14
                };
                Vector3 rockScale = {0.2f*context.cubeScale, 0.2f*context.cubeScale, 0.2f*context.cubeScale};
              DrawModelEx(context.rocks,rocksPos,{ 0.0f, 1.0f, 0.0f }, rocksPos.x*RAD2DEG, rockScale,WHITE);
            }
            if( pos.z*context.terrainSize.y>3){
                Vector3 housePos = {
                    transform.m12,
                    transform.m13-0.2f*context.cubeScale,
                    transform.m14
                };
                Vector3 houseScale = {0.4f*context.cubeScale, 0.4f*context.cubeScale, 0.4f*context.cubeScale};
              DrawModelEx(context.house,housePos,{ 0.0f, 1.0f, 0.0f }, housePos.x*10000, houseScale,WHITE);
            }
        }
}

void drawImGui(AppContext& context) {
    bool regen = false;

    if (ImGui::CollapsingHeader("Fractal Noise (FBM)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& p = context.imageGenerationParameters;
        regen |= ImGui::SliderInt ("Seed", &p.noiseSeed, 0, 6767);
        regen |= ImGui::SliderInt ("Resolution", &p.resolution, 32, 1080);
        regen |= ImGui::SliderInt ("Octaves", &p.fbmParams.octaves, 1, 10);
        regen |= ImGui::SliderFloat("Scale", &p.fbmParams.scale, 0.5f, 16.0f);
        regen |= ImGui::SliderFloat("Lacunarity", &p.fbmParams.lacunarity, 1.0f, 4.0f);
        regen |= ImGui::SliderFloat("Gain", &p.fbmParams.gain, 0.1f, 1.0f);
    }
    if (regen) {
        generateHeightmap(context);
        regenerateMeshFromImage(context);
        generateObjectsPositions(context);
    }

    //objects
    if (ImGui::Button("Generate random positions")) {
        generateObjectsPositions(context);
    }
    if (ImGui::CollapsingHeader("objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Cube Scale", &context.cubeScale, 0.01f, 1.0f);
        if (ImGui::SliderFloat("Min distance (r)", &context.pointsGenerationParameters.radius, 0.01f, 0.2f))
            generateObjectsPositions(context);
        ImGui::SliderInt("Boat Speed", &boatSpeed, 1, 20);
    }
    

    //la zicmu
    if (ImGui::CollapsingHeader("Musique", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::RadioButton("MONKEY ISLAND", context.currentMusic == 1))
        {
            context.currentMusic = 1;
            StopMusicStream(context.music2);
            PlayMusicStream(context.music1);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("WII SPORT", context.currentMusic == 2))
        {
            context.currentMusic = 2;
            StopMusicStream(context.music1);
            PlayMusicStream(context.music2);
        }
    //mode
    if (ImGui::CollapsingHeader("Mode", ImGuiTreeNodeFlags_DefaultOpen))
{
    if (ImGui::Button(context.isNight ? "Jour" : "Nuit"))
    {
    context.isNight = !context.isNight;
    generateHeightmap(context);
    regenerateMeshFromImage(context);

    if (context.isNight) {
        context.currentMusic = 1;
        StopMusicStream(context.music2);
        PlayMusicStream(context.music1);
    } else {
        context.currentMusic = 2;
        StopMusicStream(context.music1);
        PlayMusicStream(context.music2);
    }
    }

        static float volume = 1.0f;
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f))
        {
            SetMusicVolume(context.music1, volume);
            SetMusicVolume(context.music2, volume);
        }
    }

}
    if (ImGui::CollapsingHeader("Logo", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Afficher logo", &context.showLogo);
    }
}

void drawRaylibUI(AppContext& context) {
    int screenWidth { GetScreenWidth() };
    
    float wanted_size { 400.f };
    float scale_factor { wanted_size / std::max(context.texture.width, context.texture.height) };
    float const preview_x { screenWidth - wanted_size - 20.f };
    float const preview_y { 20.f };
    float const preview_w { context.texture.width * scale_factor };
    float const preview_h { context.texture.height * scale_factor };
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, { preview_x, preview_y }, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    //draw positions on top of the heightmap
    for (auto const& pos : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px { preview_x + Clamp(pos.x, 0.0f, 1.0f) * preview_w };
        float const py { preview_y + Clamp(pos.y, 0.0f, 1.0f) * preview_h };

        DrawCircleV({ px, py }, 2.0f, RED);
    }
    float const px { preview_x + ((float(boatPos.x))/18.f +0.5f) * preview_w };
    float const py { preview_y + ((float(boatPos.z))/18.f +0.5f) * preview_h };
    DrawCircleV({ px,py }, 4.0f, BROWN);

    DrawFPS(10, 10);
    Texture2D const& logo = context.isNight ? context.logoNight : context.logoDay;
    if (logo.id > 0 && context.showLogo)
    {
        float const screenW = static_cast<float>(GetScreenWidth());
        float const screenH = static_cast<float>(GetScreenHeight());

        float logoW, logoH, logoX, logoY;

        if (context.isNight)
        {
            float const maxW = screenW * 0.25f;
            float const maxH = screenH * 0.25f;
            float const scale = std::min(maxW / logo.width, maxH / logo.height);
            logoW = logo.width * scale;
            logoH = logo.height * scale;
        }
        else
        {
            float const scale = std::min(screenW / logo.width, screenH / logo.height);
            logoW = logo.width * scale;
            logoH = logo.height * scale;
        }
        
        logoX = (screenW - logoW) * 0.5f;
        logoY = (screenH - logoH) * 0.5f;
        DrawTextureEx(logo, { logoX, logoY }, 0.0f, logoW / logo.width, WHITE);
    }
}

