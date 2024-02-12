#pragma once

#include <optional>
#include <filesystem>
#include <imgui/imgui.h>
#include "Draw.hxx"
#include "World.hxx"

struct SGame;

enum class EDevToolsMode
{
    Game,
    LevelEditor,
    WorldEditor
};

struct SWorldEditor
{
    SWorld World;
    SFramebuffer MapFramebuffer;

    void Init();
    void Cleanup();

    void SetActive(SGame& Game, bool bActive);
    void Show(SGame& Game);
    void ShowWorld(SGame& Game);
};

enum class ELevelEditorMode
{
    Normal,
    Block,
    ToggleEdge,
};

struct SValidationResult
{
    int Wall{};
    int Door{};
};

struct SLevelEditor
{
    ELevelEditorMode LevelEditorMode{};
    uint32_t ToggleEdgeType{};
    SVec2Int NewLevelSize{};
    float MapScale{};
    bool bDrawWallJoints{};
    bool bDrawEdges{};
    bool bDrawGridLines{};
    bool bResetGridPosition = true;
    std::optional<SVec2Int> SelectedTileCoords{};
    std::optional<SVec2Int> BlockModeTileCoords{};
    SWorldLevel Level{};
    SFramebuffer MapFramebuffer;

    void Init();
    void Cleanup();

    void SetActive(SGame& Game, bool bActive);
    void Show(SGame& Game);
    void ShowLevel(SGame& Game);

    void SaveTilemapToFile(const std::filesystem::path& Path);
    void LoadTilemapFromFile(const std::filesystem::path& Path);

    void ScanForLevels();

    SVec2Int CalculateMapSize();
    void FitTilemapToWindow();

    SValidationResult Validate(bool bFix);
};

struct SDevTools
{
    EDevToolsMode Mode{};
    SLevelEditor LevelEditor;
    SWorldEditor WorldEditor;

    void Init(struct SDL_Window* Window, void* Context);

    void Cleanup();

    static void ProcessEvent(const union SDL_Event* Event);

    void Update(SGame& Game);

    void ShowDebugTools(SGame& Game) const;

    static void DrawParty(struct SParty& Party, float Scale, bool bReversed);

    void Draw() const;

private:
    template <typename E>
    void EnumCombo(const char* Label, const char** Types, const int TypeCount, E* SelectedType)
    {
        if (ImGui::BeginCombo(Label, Types[(int)*SelectedType], ImGuiComboFlags_NoArrowButton))
        {
            for (int I = 0; I < TypeCount; I++)
            {
                bool bIsSelected = I == (int)*SelectedType;
                if (ImGui::Selectable(Types[I], bIsSelected))
                {
                    *SelectedType = static_cast<E>(I);
                }
                if (bIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
};
