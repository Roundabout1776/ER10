#pragma once

#include "AssetTools.hxx"

struct SAudioSpec
{
    uint16_t Format;
    int Channels;
    int Freq;
};

struct SSoundClip
{
    int Length{};
    uint8_t* Ptr{};

    void Free() const;
};

struct SMusic : SSoundClip
{
};

struct SAudioEntry
{
    SSoundClip* SoundClip;
    int Current;
    bool bLoop;

    [[nodiscard]] inline bool IsPlaying() const { return IsValid() && Current < SoundClip->Length; }
    [[nodiscard]] inline bool IsValid() const { return SoundClip && SoundClip->Ptr; }
    int Advance(int Samples, int* Mixed)
    {
        if (IsValid())
        {
            *Mixed = std::min(Samples, SoundClip->Length - Current);
            Current += Samples;
            if (Current >= SoundClip->Length)
            {
                if (bLoop)
                {
                    auto Remaining = Samples - (SoundClip->Length - Current);
                    Current = 0;
                    return Remaining;
                }
                else
                {
                    Current = SoundClip->Length;
                    return 0;
                }
            }
        }
        return 0;
    }
};

class CAudio
{
protected:
    std::array<SAudioEntry, 8> Queue{};
    std::array<uint8_t, 1024 * 1024> Buffer{};
    SAudioSpec AudioSpec;
    int Silence;
    struct SDL_AudioStream* Stream{};
    SSoundClip TestSoundClip{};
    SSoundClip TestMusic{};

    void Clear() const;
    void Put(const SSoundClip* SoundClip) const;

public:
    float Volume = 0.25f;

    CAudio();
    ~CAudio();

    static void Callback(void* Userdata, struct SDL_AudioStream* Stream, int AdditionalAmount, int TotalAmount);
    void LoadSoundClip(const SAsset& Asset, SSoundClip& SoundClip) const;
    void TestAudio();
};