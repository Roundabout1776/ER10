#pragma once

#include <array>
#include "CommonTypes.hxx"
#include "Tilemap.hxx"
#include "Math.hxx"

inline constexpr int WorldMaxLevels = 8;

struct SWorldLevel : STilemap
{
    SVec3Int Position{};

    /* Draw State */
    SDrawDoorInfo DoorInfo{};
    uint32_t DirtyFlags = ELevelDirtyFlags::POVChanged | ELevelDirtyFlags::DrawSet;
    TVec2<std::size_t> DirtyRange{};
};

struct SWorldStartInfo
{
    SCoordsAndDirection POV{};
    size_t LevelIndex{};
};

struct SWorld
{
    SWorldStartInfo StartInfo{};
    size_t CurrentLevelIndex{};
    std::array<SWorldLevel, WorldMaxLevels> Levels;

    void Init();

    void Update(float DeltaTime);

    [[nodiscard]] SWorldLevel* GetLevel() { return &Levels[CurrentLevelIndex]; }
};