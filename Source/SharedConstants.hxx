#ifdef SHARED_CONSTANTS_LITERAL
    #define SHARED_CONST(NAME, INDEX) R"(const int )" #NAME " = " #INDEX ";\n"
#else
    #define SHARED_CONST(NAME, INDEX) \
        constexpr int NAME = INDEX;
#endif

#if defined(SHARED_CONSTANTS_LITERAL)
#define SHARED_CONSTANTS_FORCE
#endif

#if defined(SHARED_CONSTANTS_FORCE) || !defined(SHARED_CONSTANTS_GUARD)

/* Level Constants */
SHARED_CONST(MAX_LEVEL_WIDTH, 32)
SHARED_CONST(MAX_LEVEL_HEIGHT, 32)
SHARED_CONST(MAX_LEVEL_TILE_COUNT, (MAX_LEVEL_WIDTH * MAX_LEVEL_HEIGHT))

/* Uber2D Modes */
SHARED_CONST(UBER2D_MODE_TEXTURE, 0)
SHARED_CONST(UBER2D_MODE_HAZE, 1)
SHARED_CONST(UBER2D_MODE_BACK_BLUR, 2)
SHARED_CONST(UBER2D_MODE_GLOW, 3)
SHARED_CONST(UBER2D_MODE_DISINTEGRATE, 4)
SHARED_CONST(UBER2D_MODE_DISINTEGRATE_PLASMA, 5)

/* Uber3D Modes */
SHARED_CONST(UBER3D_MODE_BASIC, 0)
SHARED_CONST(UBER3D_MODE_LEVEL, 1)

/* Uber3D Limits */
SHARED_CONST(UBER3D_MODEL_COUNT, 64)

/* HUD Modes */
SHARED_CONST(HUD_MODE_BORDER_DASHED, 0)
SHARED_CONST(HUD_MODE_BUTTON, 1)
SHARED_CONST(HUD_MODE_MAP, 2)

#if !defined(SHARED_CONSTANTS_FORCE) && !defined(SHARED_CONSTANTS_GUARD)
#define SHARED_CONSTANTS_GUARD 1
#endif

#endif

#undef SHARED_CONST
