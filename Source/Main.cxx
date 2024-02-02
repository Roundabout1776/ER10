#include <SDL3/SDL_main.h>
#include "Memory.hxx"
#include "Game.hxx"

int main([[maybe_unused]] int ArgCount, [[maybe_unused]] char* Arguments[])
{
    Platform::Init();

    auto Game = Memory::MakeShared<SGame>();
    Game->Run();

    return 0;
}
