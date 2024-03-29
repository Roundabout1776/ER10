#pragma once

#include <optional>
#include <filesystem>
#include <imgui/imgui.h>
#include "Draw.hxx"
#include "World.hxx"

struct SGame;

struct SEditorFramebuffer
{
    int Width{};
    int Height{};
    SVec4 ClearColor{};
    uint32_t FBO{};
    uint32_t ColorID{};

    void Init(int InWidth, int InHeight, SVec4 InClearColor);
    void Resize(int InWidth, int InHeight);
    void Cleanup();
};

struct SEditorBase
{
    SGame* Game{};
    SEditorFramebuffer Framebuffer{};
    SVec4 CursorPosition{};
    float Scale{};

    virtual void EditorDraw(const SVec2& ScaledSize) = 0;
    virtual void EditorUpdate() = 0;
    virtual void EditorTools() = 0;
};

struct SWorldEditor : SEditorBase
{
    SWorld World;

    void Init(SGame* InGame);
    void Cleanup();

    void RenderLayers(SGame& Game);

    void EditorDraw(const SVec2& ScaledSize) final;
    void EditorUpdate() final;
    void EditorTools() final;
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

struct SLevelEditor : SEditorBase
{
    ELevelEditorMode LevelEditorMode{};
    uint32_t ToggleEdgeType{};
    SVec2Int NewLevelSize{};
    bool bDrawWallJoints{};
    bool bDrawEdges{};
    bool bDrawGridLines{};
    bool bResetView = true;
    SVec2Int SelectedTileCoords{};
    SVec2Int BlockModeTileCoords{};
    SWorldLevel Level{};

    bool bLevelChanged{};
    bool bEditorStateChanged{};

    void Init(SGame* InGame);
    void Cleanup();

    void EditorDraw(const SVec2& ScaledSize) final;
    void EditorUpdate() final;
    void EditorTools() final;

    void SaveTilemapToFile(const std::filesystem::path& Path);
    void LoadTilemapFromFile(const std::filesystem::path& Path);

    void ScanForLevels();

    SVec2Int CalculateMapSize();

    SValidationResult Validate(bool bFix);
};

enum class EDevToolsMode
{
    Game,
    LevelEditor,
    WorldEditor
};

struct SDevTools
{
    SGame* Game{};
    EDevToolsMode Mode{};
    SLevelEditor LevelEditor;
    SWorldEditor WorldEditor;

    void Init(SGame* InGame);

    void Cleanup();

    static void ProcessEvent(const union SDL_Event* Event);

    void Update();

    void ShowDebugTools() const;

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
