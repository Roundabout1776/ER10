#pragma once

#define GAME_NAME "Equinox Reach"

#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720
#define WINDOW_ASPECT   static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT)
#define WINDOW_CLEAR_COLOR {0.0f, 0.0f, 0.0f}

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define SCREEN_ASPECT   (static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT))

#define SCENE_WIDTH  288
#define SCENE_HEIGHT 120
#define SCENE_OFFSET ((SCREEN_WIDTH - SCENE_WIDTH) / 2)
#define SCENE_ASPECT (static_cast<float>(SCENE_WIDTH) / static_cast<float>(SCENE_HEIGHT))
