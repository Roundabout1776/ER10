#include "Game.hxx"

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include "Blob.hxx"
#include "CommonTypes.hxx"
#include "Constants.hxx"
#include "Level.hxx"
#include "Log.hxx"
#include "SharedConstants.hxx"
#include "AssetTools.hxx"
#include "Audio.hxx"
#include "Draw.hxx"
#include "Serialization.hxx"
#include "Tile.hxx"

namespace Asset::Common
{
    EXTERN_ASSET(FramePNG)
    EXTERN_ASSET(RefPNG)
    EXTERN_ASSET(AngelPNG)
    EXTERN_ASSET(NoisePNG)
    EXTERN_ASSET(QuadOBJ)
    EXTERN_ASSET(PillarOBJ)
    EXTERN_ASSET(DoorCreekWAV)
}

namespace Asset::Tileset::Hotel
{
    EXTERN_ASSET(FloorOBJ)
    EXTERN_ASSET(HoleOBJ)
    EXTERN_ASSET(WallOBJ)
    EXTERN_ASSET(WallJointOBJ)
    EXTERN_ASSET(DoorFrameOBJ)
    EXTERN_ASSET(DoorOBJ)
    EXTERN_ASSET(AtlasPNG)
}

namespace Asset::Map
{
    EXTERN_ASSET(TestMapERM)
}

static const URectInt MapRectMin{ SCREEN_WIDTH - 128, 10, 12 * 7 + 1, 12 * 5 + 1 };
static const URectInt MapRectMax{ SCENE_OFFSET, 54, SCENE_WIDTH + 1, SCENE_HEIGHT + 1 };

SGame::SGame() : MapRect(MapRectMin)
{
#ifdef EQUINOX_REACH_DEVELOPMENT
    DevTools.Init(Window.Window, Window.Context);
#endif

    Renderer.Init(SCREEN_WIDTH, SCREEN_HEIGHT);

    auto& CommonAtlas2D = Renderer.Atlases[ATLAS_COMMON];
    NoiseSprite = CommonAtlas2D.AddSprite(
        Asset::Common::NoisePNG);
    RefSprite = CommonAtlas2D.AddSprite(
        Asset::Common::RefPNG);
    CommonAtlas2D.Build();

    auto& PrimaryAtlas2D = Renderer.Atlases[ATLAS_PRIMARY2D];
    AngelSprite = PrimaryAtlas2D.AddSprite(
        Asset::Common::AngelPNG);
    FrameSprite = PrimaryAtlas2D.AddSprite(
        Asset::Common::FramePNG);
    PrimaryAtlas2D.Build();

    auto& PrimaryAtlas3D = Renderer.Atlases[ATLAS_PRIMARY3D];
    PrimaryAtlas3D.AddSprite(
        Asset::Tileset::Hotel::AtlasPNG);
    PrimaryAtlas3D.Build();

    // TestGeometry.InitFromRawMesh(CRawMesh(
    //     Asset::Common::PillarOBJ));

    Tileset.InitBasic(
        Asset::Tileset::Hotel::FloorOBJ,
        Asset::Tileset::Hotel::HoleOBJ,
        Asset::Tileset::Hotel::WallOBJ,
        Asset::Tileset::Hotel::WallJointOBJ,
        Asset::Tileset::Hotel::DoorFrameOBJ,
        Asset::Tileset::Hotel::DoorOBJ);
    Tileset.DoorAnimationType = EDoorAnimationType::TwoDoors;
    Tileset.DoorOffset = 0.22f;
    Renderer.SetupLevelDrawData(Tileset);

    Blob.Direction = SDirection::East();
    Blob.ApplyDirection(true);

    Camera.RegenerateProjection();

    // Level = SLevel{
    //     5,
    //     5,
    //     {
    //         STile::WallNWS(),
    //         STile::WallN(),
    //         STile::WallN(),
    //         STile::WallNE(),
    //         STile::WallSW(false),
    //
    //         STile::WallNEW(),
    //         STile::WallWDoorS(),
    //         STile::WallS(),
    //         STile::Floor(),
    //         STile::WallNE(),
    //
    //         STile::WallWE(),
    //         STile::WallEWDoorN(),
    //         STile::WallsNF(),
    //         STile::WallW(),
    //         STile::WallE(),
    //
    //         STile::WallWE(),
    //         STile::WallSWE(),
    //         STile::WallNW(),
    //         STile::Floor(),
    //         STile::WallE(),
    //
    //         STile::WallSW(),
    //         STile::WallNS(),
    //         STile::WallS(),
    //         STile::WallS(),
    //         STile::WallSE(),
    //     },
    //     true
    // };

    ChangeLevel(Asset::Map::TestMapERM);

#ifdef EQUINOX_REACH_DEVELOPMENT
    DevTools.LevelEditor.Level = Level;
#endif

    SpriteDemoState = 5;

    PlayerParty.AddCharacter({ "PC", 75.0f, 100.0f, 10, 1 });
    PlayerParty.AddCharacter({ "Juggernaut", 30.0f, 50.0f, 10, 1 });
    PlayerParty.AddCharacter({ "Vulture", 45.0f, 50.0f, 10, 2, false });
    PlayerParty.AddCharacter({ "Mortar", 30.0f, 30.0f, 10, 2, true });

    Audio.LoadSoundClip(Asset::Common::DoorCreekWAV, DoorCreek);
}

EKeyState SGame::UpdateKeyState(EKeyState OldKeyState, const uint8_t* KeyboardState, const uint8_t Scancode)
{
    bool bCurrentlyPressed = KeyboardState[Scancode] == 1;
    bool bWasPressed = OldKeyState == EKeyState::Held || OldKeyState == EKeyState::Pressed;
    if (bCurrentlyPressed)
    {
        if (bWasPressed)
        {
            return EKeyState::Held;
        }
        else
        {
            return EKeyState::Pressed;
        }
    }
    else
    {
        if (bWasPressed)
        {
            return EKeyState::Released;
        }
        else
        {
            return EKeyState::None;
        }
    }
}

void SGame::Run()
{
    Window.Now = SDL_GetTicks();
    while (!Window.bQuit)
    {
        Window.Last = Window.Now;
        Window.Now = SDL_GetTicks();
        Window.DeltaTime = (float)(Window.Now - Window.Last) / 1000.0f;
        Window.Seconds += Window.DeltaTime;

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
#ifdef EQUINOX_REACH_DEVELOPMENT
            DevTools.ProcessEvent(&Event);
#endif
            switch (Event.type)
            {
                case SDL_EVENT_WINDOW_RESIZED:
                    Window.OnWindowResized(Event.window.data1, Event.window.data2);
                    break;
                case SDL_EVENT_QUIT:
                    Window.bQuit = true;
                    break;
                default:
                    break;
            }
        }

        UpdateInputState();

#ifdef EQUINOX_REACH_DEVELOPMENT
        DevTools.Update(*this);
#endif
        // if (InputState.Cancel == EKeyState::Pressed)
        // {
        //     Window.bQuit = true;
        // }

        if (InputState.ToggleFullscreen == EKeyState::Pressed)
        {
            Window.ToggleBorderlessFullscreen();
        }

        if (IsGameRunning())
        {
            HandleBlobMovement();

            // if (InputState.ZL == EKeyState::Pressed)
            // {
            //     SpriteDemoState = std::max(0, SpriteDemoState - 1);
            // }
            //
            // if (InputState.ZR == EKeyState::Pressed)
            // {
            //     SpriteDemoState = std::min(5, SpriteDemoState + 1);
            // }

            if (InputState.ZR == EKeyState::Pressed)
            {
                bMapMaximized = !bMapMaximized;
            }

            if (InputState.Accept == EKeyState::Pressed)
            {
                Audio.TestAudio();
            }

            Camera.Position = Blob.EyePositionCurrent;
            Camera.Target = Camera.Position + Blob.EyeForwardCurrent;
            Camera.Update();

            Renderer.UploadProjectionAndViewFromCamera(Camera);
            // Renderer.Draw3D({ -7.0f, 0.0f, -4.0f }, &Floor);

            Level.Update(Window.DeltaTime);
            Renderer.Draw3DLevel(Level, Blob.Coords, Blob.Direction);

            MapRect = Math::Mix(MapRect, URect(bMapMaximized ? MapRectMax : MapRectMin), Window.DeltaTime * 10.0f);
            Renderer.DrawHUDMap(Level, UVec3(MapRect.Min), UVec2Int(MapRect.Max), Blob.UnreliableCoords());

            switch (SpriteDemoState)
            {
                case 0:
                    Renderer.Draw2DEx({ 220, 80.0f, 0.0f }, AngelSprite, UBER2D_MODE_DISINTEGRATE_PLASMA,
                        { Window.Seconds / 2.0f, 0.9f, 0.2f, 0.1f },
                        NoiseSprite.Sprite->UVRect);
                    break;
                case 1:
                    Renderer.Draw2DHaze({ 220, 80.0f, 0.0f }, AngelSprite, 0.07f, 4.0f, 4.0f);
                    break;
                case 2:
                    Renderer.Draw2DBackBlur({ 220, 80.0f, 0.0f }, AngelSprite, 4.0f, 2.9f, 0.08f);
                    break;
                case 3:
                    Renderer.Draw2DGlow({ 220, 80.0f, 0.0f }, AngelSprite, { 1.0f, 1.0f, 1.0f }, 2.0f);
                    break;
                case 4:
                    Renderer.Draw2DDisintegrate({ 220, 80.0f, 0.0f }, AngelSprite, NoiseSprite, Window.Seconds / 4.0f);
                    break;
                default:
                    break;
            }

            // Renderer.DrawHUD({32.0f, 250.0f, 0.0f}, {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4}, HUD_MODE_BORDER_DASHED);
            // Renderer.DrawHUD({128.0f, 250.0f, 0.0f}, {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4}, HUD_MODE_BUTTON);

            Renderer.Flush(Window);
        }

#ifdef EQUINOX_REACH_DEVELOPMENT
        DevTools.Draw();
#endif

        Window.SwapBuffers();
    }

#ifdef EQUINOX_REACH_DEVELOPMENT
    DevTools.Cleanup();
#endif
    Renderer.Cleanup();
    /* @TODO: Fix this. */
    DoorCreek.Free();
    Tileset.Cleanup();
}

void SGame::UpdateInputState()
{
    OldInputState = InputState;
    const Uint8* KeyboardState = SDL_GetKeyboardState(nullptr);
    InputState.Up = UpdateKeyState(OldInputState.Up, KeyboardState, SDL_SCANCODE_W);
    InputState.Right = UpdateKeyState(OldInputState.Right, KeyboardState, SDL_SCANCODE_D);
    InputState.Down = UpdateKeyState(OldInputState.Down, KeyboardState, SDL_SCANCODE_S);
    InputState.Left = UpdateKeyState(OldInputState.Left, KeyboardState, SDL_SCANCODE_A);
    InputState.L = UpdateKeyState(OldInputState.L, KeyboardState, SDL_SCANCODE_Q);
    InputState.R = UpdateKeyState(OldInputState.R, KeyboardState, SDL_SCANCODE_E);
    InputState.ZL = UpdateKeyState(OldInputState.ZL, KeyboardState, SDL_SCANCODE_Z);
    InputState.ZR = UpdateKeyState(OldInputState.ZR, KeyboardState, SDL_SCANCODE_C);
    InputState.Accept = UpdateKeyState(OldInputState.Accept, KeyboardState, SDL_SCANCODE_SPACE);
    InputState.Cancel = UpdateKeyState(OldInputState.Cancel, KeyboardState, SDL_SCANCODE_ESCAPE);
    InputState.ToggleFullscreen = UpdateKeyState(OldInputState.ToggleFullscreen, KeyboardState, SDL_SCANCODE_F11);
#ifdef EQUINOX_REACH_DEVELOPMENT
    InputState.ToggleLevelEditor = UpdateKeyState(OldInputState.ToggleLevelEditor, KeyboardState, SDL_SCANCODE_F9);
#endif
}

void SGame::HandleBlobMovement()
{
    Blob.Update(Window.DeltaTime);

    const auto bBlobWasIdle = !Blob.IsMoving();
    if (Blob.MoveSeq.IsFinished())
    {
        if (Blob.IsReadyForBuffering())
        {
            BufferedInputState.Buffer(InputState);
        }
        if (bBlobWasIdle)
        {
            BufferedInputState.Buffer(InputState);

            /* Moving */
            if (BufferedInputState.L == EKeyState::Held)
            {
                auto PlayerDirection = Blob.Direction;
                PlayerDirection.CycleCCW();
                if (!AttemptBlobStep(PlayerDirection))
                {
                    Blob.HijackLF();
                }
            }
            if (BufferedInputState.R == EKeyState::Held)
            {
                auto PlayerDirection = Blob.Direction;
                PlayerDirection.CycleCW();
                if (!AttemptBlobStep(PlayerDirection))
                {
                    Blob.HijackRF();
                }
            }
            if (BufferedInputState.Up == EKeyState::Held)
            {
                auto PlayerDirection = Blob.Direction;
                AttemptBlobStep(PlayerDirection);
            }
            if (BufferedInputState.Down == EKeyState::Held)
            {
                auto PlayerDirection = Blob.Direction;
                if (!AttemptBlobStep(PlayerDirection.Inverted()))
                {
                    Blob.HijackRRF();
                }
            }

            /* Turning */
            if (BufferedInputState.Left == EKeyState::Held)
            {
                Blob.Turn(false);
            }
            else if (BufferedInputState.Right == EKeyState::Held)
            {
                Blob.Turn(true);
            }

            BufferedInputState.Value = 0;
        }
        else
        {
            Level.DrawState.DirtyFlags |= ELevelDirtyFlags::POVChanged;
        }
    }
    else
    {
        if (bBlobWasIdle)
        {
            switch (Blob.MoveSeq.GetCurrentDirection().Index)
            {
                case SDirection::North().Index:
                {
                    AttemptBlobStep(Blob.Direction);
                }
                break;
                case SDirection::East().Index:
                {
                    Blob.Turn(true);
                }
                break;
                case SDirection::West().Index:
                {
                    Blob.Turn(false);
                }
                break;
                case SDirection::South().Index:
                default:
                {
                    // AttemptPlayerStep(Blob.Direction);
                }
                break;
            }
            Blob.MoveSeq.Current++;
        }
    }
    if (bBlobWasIdle && Blob.IsMoving())
    {
        OnBlobMoved();
    }
    else if (Blob.ShouldHandleAnimationEnd())
    {
        auto LastAnimation = Blob.HandleAnimationEnd();

        switch (LastAnimation)
        {
            case EBlobAnimationType::Fall:
            {
                Blob.Coords = {};
                Blob.Direction = {};
                Blob.ResetEye();
                Blob.ApplyDirection(true);
                OnBlobMoved();
            }
            break;
            default:
                break;
        }
    }
}

bool SGame::AttemptBlobStep(SDirection Direction)
{
    auto CurrentTile = Level.GetTileAt(Blob.Coords);
    if (CurrentTile == nullptr)
    {
        return false;
    }

    if (!CurrentTile->IsEdgeTraversable(Direction))
    {
        if (Direction == Blob.Direction)
        {
            Blob.BumpIntoWall();
        }

        return false;
    }

    auto DirectionVector = Direction.GetVector<int>();
    auto NextTile = Level.GetTileAt(Blob.Coords + DirectionVector);
    if (NextTile == nullptr || !NextTile->IsWalkable())
    {
        return false;
    }

    if (CurrentTile->IsDoorEdge(Direction))
    {
        if (Direction.Index != Blob.Direction.Index)
        {
            return false;
        }

        Level.DrawState.DoorInfo.Set(Blob.Coords, Direction);

        Blob.Step(DirectionVector, EBlobAnimationType::Enter);

        Audio.Play(DoorCreek);
    }
    else if (NextTile->CheckFlag(TILE_HOLE_BIT))
    {
        Blob.Step(DirectionVector, EBlobAnimationType::Fall);
    }
    else
    {
        Level.DrawState.DoorInfo.Invalidate();

        Blob.Step(DirectionVector);
    }

    Audio.TestAudio();

    return true;
}

void SGame::OnBlobMoved()
{
    auto CurrentTile = Level.GetTileAtMutable(Blob.Coords);
    if (CurrentTile != nullptr)
    {
        CurrentTile->SetSpecialFlag(TILE_SPECIAL_VISITED_BIT);
    }
    else
    {
        return;
    }

    UVec2Size DirtyRange{};
    DirtyRange.X = SIZE_MAX;
    DirtyRange.Y = 0;

    for (auto X = Blob.Coords.X - SBlob::ExploreRadius(); X <= Blob.Coords.X + SBlob::ExploreRadius(); ++X)
    {
        for (auto Y = Blob.Coords.Y - SBlob::ExploreRadius(); Y <= Blob.Coords.Y + SBlob::ExploreRadius(); ++Y)
        {
            auto Tile = Level.GetTileAtMutable({ X, Y });
            if (Tile != nullptr)
            {
                Tile->SetSpecialFlag(TILE_SPECIAL_EXPLORED_BIT);
                std::size_t Index = Level.CoordsToIndex(X, Y);
                DirtyRange.X = std::min(DirtyRange.X, Index);
                DirtyRange.Y = std::max(DirtyRange.Y, Index);
            }
        }
    }

    if (DirtyRange.X > DirtyRange.Y)
    {
        DirtyRange = {};
    }

    Level.DrawState.DirtyFlags = ELevelDirtyFlags::DrawSet | ELevelDirtyFlags::POVChanged;
    Level.DrawState.DirtyRange = DirtyRange;
}

void SGame::ChangeLevel()
{
    Level.PostProcess();
    Renderer.UploadLevelMapData(Level);
    OnBlobMoved();
}

void SGame::ChangeLevel(const SLevel& NewLevel)
{
    Level = NewLevel;
    ChangeLevel();
}

void SGame::ChangeLevel(const SAsset& LevelAsset)
{
    Serialization::MemoryStream LevelStream(LevelAsset.AsSignedCharPtr(), LevelAsset.Length);
    Level.Deserialize(LevelStream);
    ChangeLevel();
}

bool SGame::IsGameRunning() const
{
#ifdef EQUINOX_REACH_DEVELOPMENT
    return !DevTools.LevelEditor.bLevelEditorActive;
#else
    return true;
#endif
}
