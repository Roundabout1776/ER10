#pragma once

#include "Math.hxx"

struct SWindowData
{
    int Width{};
    int Height{};
    int BlitWidth{};
    int BlitHeight{};
    int BlitX{};
    int BlitY{};
    unsigned long long Last{};
    unsigned long long Now{};
    float Seconds{};
    float DeltaTime{};
    bool bQuit{};
};

struct SDirection
{
    using Type = unsigned;
    static constexpr Type Count = 4;
    Type Index : 2;

    void CycleCW()
    {
        Index++;
    }

    void CycleCCW()
    {
        Index--;
    }

    constexpr static const char* Names[] = { "North", "East", "South", "West" };

    constexpr static SDirection North()
    {
        return SDirection{ 0 };
    }

    constexpr static SDirection East()
    {
        return SDirection{ 1 };
    }

    constexpr static SDirection South()
    {
        return SDirection{ 2 };
    }

    constexpr static SDirection West()
    {
        return SDirection{ 3 };
    }

    constexpr static SDirection Forward()
    {
        return SDirection{ 0 };
    }

    constexpr static SDirection Right()
    {
        return SDirection{ 1 };
    }

    constexpr static SDirection Backward()
    {
        return SDirection{ 2 };
    }

    constexpr static SDirection Left()
    {
        return SDirection{ 3 };
    }

    inline SDirection Side() const
    {
        return SDirection{ Index + 1u };
    }

    inline SDirection Inverted() const
    {
        return SDirection{ Index + 2u };
    }

    template <typename T>
    constexpr SVec2<T> GetVector() const
    {
        switch (Index)
        {
            case North().Index:
                return { 0, -1 };
            case East().Index:
                return { 1, 0 };
            case South().Index:
                return { 0, 1 };
            case West().Index:
                return { -1, 0 };
            default:
                return {};
        }
    }

    [[nodiscard]] constexpr float RotationFromDirection() const
    {
        switch (Index)
        {
            case East().Index:
                return Math::PI * -0.5f;
            case South().Index:
                return Math::PI;
            case West().Index:
                return Math::PI / 2.0f;
            default:
                return 0.0f;
        }
    }

    bool operator==(const SDirection& Other) const
    {
        return Index == Other.Index;
    }
};

enum class EKeyState : unsigned
{
    None,
    Pressed,
    Released,
    Held,
};

struct SInputState
{
    union
    {
        unsigned long Value;
        struct
        {

            EKeyState Up : 2;
            EKeyState Right : 2;
            EKeyState Down : 2;
            EKeyState Left : 2;
            EKeyState Accept : 2;
            EKeyState Cancel : 2;
            EKeyState L : 2;
            EKeyState ZL : 2;
            EKeyState R : 2;
            EKeyState ZR : 2;

            EKeyState ToggleFullscreen : 2;

#ifdef EQUINOX_REACH_DEVELOPMENT
            EKeyState ToggleLevelEditor : 2;
#endif
        };
    };

    void Buffer(const SInputState& Other)
    {
        this->Value |= Other.Value;
    }
};

struct STimeline
{
    float Value = 1.0f;
    float Speed = 1.0f;

    void Advance(float DeltaTime)
    {
        Value += DeltaTime * Speed;
        if (Value > 1.0f)
        {
            Value = 1.0f;
        }
    }

    void Reset()
    {
        Value = 0.0f;
    }

    void Finish()
    {
        Value = 1.0f;
    }

    [[nodiscard]] inline bool IsFinishedPlaying() const { return Value >= 1.0f; }
    [[nodiscard]] inline bool IsPlaying() const { return Value >= 0.0f && Value < 1.0f; }
};
