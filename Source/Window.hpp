#pragma once

#include "SDL_video.h"
#include "CommonTypes.hpp"

struct SWindow : SWindowData {
    SDL_Window *Window{};
    SDL_GLContext Context{};

    void Init();

    void Cleanup() const;

    [[nodiscard]] bool IsExclusiveFullscreen() const;

    void SwapBuffers() const;

    void OnWindowResized(int InWidth, int InHeight);

    void ToggleBorderlessFullscreen() const;
};
