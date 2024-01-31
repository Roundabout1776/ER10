
/* Binding Point: 0 */
layout (std140) uniform ub_common
{
    vec2 u_screenSize;
    float u_time;
};

struct TileData
{
    uint flags;
    uint specialFlags;
    uint edgeFlags;
    uint specialEdgeFlags;
};

/* Binding Point: 1 */
layout (std140) uniform ub_map
{
    uint width;
    uint height;
    float povX;
    float povY;
    TileData[MAX_LEVEL_TILE_COUNT] tiles;
} u_map;

uniform int u_mode;
uniform vec4 u_modeControlA;
uniform vec4 u_modeControlB;
uniform vec4 u_uvRect;// minX, minY, maxX, maxY
uniform vec2 u_sizeScreenSpace;
uniform sampler2D u_commonAtlas;
uniform sampler2D u_primaryAtlas;

in vec2 f_texCoord;

out vec4 color;

int calculateTileIndex(float tileX, float tileY, float levelWidth, float levelHeight)
{
    int index = int(clamp(tileY * levelWidth + tileX, 0.0, (levelWidth * levelHeight) - 1.0));
    return index;
}

TileData getTileData(float tileX, float tileY, float levelWidth, float levelHeight)
{
    return u_map.tiles[calculateTileIndex(tileX, tileY, levelWidth, levelHeight)];
}

float calculateValidTileMask(float tileX, float tileY, float levelWidth, float levelHeight)
{
    float validTileMask = max(0.0, sign(tileX + 1));
    validTileMask = min(validTileMask, sign(tileY + 1));
    validTileMask = min(validTileMask, 1.0 - step(levelWidth - tileX, 0.0));
    validTileMask = min(validTileMask, 1.0 - step(levelHeight - tileY, 0.0));
    validTileMask = saturate(validTileMask);
    return validTileMask;
}

vec4 pixelToTile(vec2 texCoord, float tileSize)
{
    vec4 tileInfo;
    tileInfo.x = floor(texCoord.x / tileSize);
    tileInfo.y = floor(texCoord.y / tileSize);
    tileInfo.z = (texCoord.x - (tileInfo.x * tileSize)) / tileSize;
    tileInfo.w = (texCoord.y - (tileInfo.y * tileSize)) / tileSize;
    return tileInfo;
}

void main()
{
    vec2 pixelPos = f_texCoord * u_sizeScreenSpace;

    vec2 texCoordNDC = (f_texCoord * 2) - 1.0;

    // Border setup
    vec3 borderColor = vec3(0.5, 0.5, 0.5);
    float borderSize = 2.0;

    // Border mask
    float borderX = clamp(abs((texCoordNDC.x * u_sizeScreenSpace.x))  - (u_sizeScreenSpace.x - borderSize), 0.0, 1.0);
    float borderY = clamp(abs((texCoordNDC.y * u_sizeScreenSpace.y))  - (u_sizeScreenSpace.y - borderSize), 0.0, 1.0);

    if (u_mode == HUD_MODE_BORDER_DASHED)
    {
        // Border dash
        float borderDashSize = 8.0;
        float borderDashSpeed = u_time * 10.0;
        float borderDashDirectionX = sign(texCoordNDC.x);
        float borderDashDirectionY = sign(texCoordNDC.y);
        borderX = borderX * (round(mod(pixelPos.y + (borderDashSpeed * -borderDashDirectionX), borderDashSize) / borderDashSize));
        borderY = borderY * (round(mod(pixelPos.x + (borderDashSpeed * borderDashDirectionY), borderDashSize) / borderDashSize));

        float borderMask = clamp(borderX + borderY, 0.0, 1.0);

        // Border color
        borderColor = borderMask * mix(vec3(0.4, 0.1, 0.7), vec3(0.5, 0.2, 0.3), (f_texCoord.x + f_texCoord.y) / 2.0);
    }

    if (u_mode == HUD_MODE_BUTTON)
    {
        float borderMask = clamp(borderX + borderY, 0.0, 1.0);
        borderColor = borderMask * vec3(1.0, 1.0, 1.0);
    }

    if (u_mode == HUD_MODE_MAP)
    {
        vec3 finalColor = mix(vec3(0.0, 0.0, 0.0), vec3(0.03, 0.03, 0.08), 1.0 - f_texCoord.y);

        float levelWidth = float(u_map.width);
        float levelHeight = float(u_map.height);
        float levelTileCount = levelWidth * levelHeight;

        float povX = u_map.povX;
        float povY = u_map.povY;
        vec2 pov = vec2(povX, povY);

        vec2 texCoord = f_texCoord;
        vec2 texCoordOriginal = texCoord;
        texCoord *= u_sizeScreenSpace;

// #define ISOMETRIC 1
#ifdef ISOMETRIC
        const float tileSize = 16.0;

        texCoord = cartesianToIsometric(texCoord);
        texCoordOriginal = cartesianToIsometric(texCoordOriginal);

        vec2 sizeIso = round(cartesianToIsometric(u_sizeScreenSpace / 2.0));

        float centerOffsetX = round(sizeIso.x - (povX + 0.5) * tileSize);
        float centerOffsetY = round(sizeIso.y - (povY + 0.5) * tileSize);

        texCoord -= vec2(centerOffsetX, centerOffsetY);
#else
        const float tileSize = 12.0;

        float centerOffsetX = round(u_sizeScreenSpace.x * 0.5 - (povX + 0.5) * tileSize);
        float centerOffsetY = round(u_sizeScreenSpace.y * 0.5 - (povY + 0.5) * tileSize);

        texCoord += vec2(-centerOffsetX, -centerOffsetY);
#endif

        const float tileSizeReciprocal = 1.0 / tileSize;

        vec4 tileInfo = pixelToTile(texCoord, tileSize);
        TileData tileData = getTileData(tileInfo.x, tileInfo.y, levelWidth, levelHeight);
        float validTileMask = calculateValidTileMask(tileInfo.x, tileInfo.y, levelWidth, levelHeight);
        float levelBoundsMask = step(texCoord.x, levelWidth * tileSize + 1) * step(texCoord.y, levelHeight * tileSize + 1) * step(0.0, texCoord.x) * step(0.0, texCoord.y);

        // Floor
        float holeMask = bitMask(tileData.flags, TILE_HOLE_BIT) * validTileMask;
        float floorTileMask = (bitMask(tileData.flags, TILE_FLOOR_BIT) + holeMask) * validTileMask;

        // float checkerMask = floor(mod(tileX + mod(tileY, 2.0), 2.0));
        float visitedMask = bitMask(tileData.specialFlags, TILE_SPECIAL_VISITED_BIT);
        vec3 floorTile = vec3(0.0, 0.0, 1.0 - ((1.0 - visitedMask) * 0.6));
        finalColor = overlay(finalColor, floorTile, floorTileMask);

        holeMask *= step(abs(map1to1(tileInfo.z - tileSizeReciprocal / 2.0)), 0.55);
        holeMask *= step(abs(map1to1(tileInfo.w - tileSizeReciprocal / 2.0)), 0.55);
        vec3 holeColor = vec3(0.0, 0.0, 0.1);
        finalColor = overlay(finalColor, holeColor, holeMask);

        // Current POV
        float povTileMask = (1.0 - min(abs(tileInfo.x - povX), 1.0)) * (1.0 - min(abs(tileInfo.y - povY), 1.0));
        float povAnim = (abs(sin((u_time * 10.0))) * 0.6) + 0.4;
        float povMask = saturate((povTileMask * (1.0 - distance(vec2(map1to1(tileInfo.z - tileSizeReciprocal / 2.0), map1to1(tileInfo.w - tileSizeReciprocal / 2.0)) * 2.0, vec2(0.0, 0.0))))) * povAnim;
        vec3 povColor = vec3(1.0, 1.0, 1.0);
        finalColor = overlay(finalColor, povColor, povMask * levelBoundsMask);

        // Edges
        float edgeMaskHor = saturate(1.0 - abs(mod(texCoord.y, tileSize)));
        float edgeMaskVert = saturate(1.0 - abs(mod(texCoord.x, tileSize)));
        float edgeMask = 1.0 - step(saturate(edgeMaskHor + edgeMaskVert), 0.0);
        float bothEdgesMask = ceil(edgeMaskHor * edgeMaskVert);

        vec4 tileInfoNorth = pixelToTile(texCoord + vec2(0.0, -1.0), tileSize);
        float validTileMaskNorth = calculateValidTileMask(tileInfoNorth.x, tileInfoNorth.y, levelWidth, levelHeight);
        TileData tileDataNorth = getTileData(tileInfoNorth.x, tileInfoNorth.y, levelWidth, levelHeight);
        float exploredMaskNorth = bitMask(tileDataNorth.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMaskNorth *= exploredMaskNorth;

        vec4 tileInfoSouth = pixelToTile(texCoord + vec2(0.0, 1.0), tileSize);
        float validTileMaskSouth = calculateValidTileMask(tileInfoSouth.x, tileInfoSouth.y, levelWidth, levelHeight);
        TileData tileDataSouth = getTileData(tileInfoSouth.x, tileInfoSouth.y, levelWidth, levelHeight);
        float exploredMaskSouth = bitMask(tileDataSouth.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMaskSouth *= exploredMaskSouth;

        vec4 tileInfoEast = pixelToTile(texCoord + vec2(1.0, 0.0), tileSize);
        float validTileMaskEast = calculateValidTileMask(tileInfoEast.x, tileInfoEast.y, levelWidth, levelHeight);
        TileData tileDataEast = getTileData(tileInfoEast.x, tileInfoEast.y, levelWidth, levelHeight);
        float exploredMaskEast = bitMask(tileDataEast.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMaskEast *= exploredMaskEast;

        vec4 tileInfoWest = pixelToTile(texCoord + vec2(-1.0, 0.0), tileSize);
        float validTileMaskWest = calculateValidTileMask(tileInfoWest.x, tileInfoWest.y, levelWidth, levelHeight);
        TileData tileDataWest = getTileData(tileInfoWest.x, tileInfoWest.y, levelWidth, levelHeight);
        float exploredMaskWest = bitMask(tileDataWest.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMaskWest *= exploredMaskWest;

        // Corners hack
        vec4 tileInfoCorner = pixelToTile(texCoord + vec2(-1.0, -1.0), tileSize);
        float validTileMaskCorner = calculateValidTileMask(tileInfoWest.x, tileInfoCorner.y, levelWidth, levelHeight);
        TileData tileDataCorner = getTileData(tileInfoCorner.x, tileInfoCorner.y, levelWidth, levelHeight);
        float exploredMaskCorner = bitMask(tileDataCorner.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMaskCorner *= exploredMaskCorner;

        // Walls
        float wallMaskNorth = bitMask(tileDataNorth.edgeFlags, TILE_EDGE_WALL_SOUTH_BIT) * validTileMaskNorth;
        wallMaskNorth *= edgeMaskHor;
        wallMaskNorth = ceil(wallMaskNorth);

        float wallMaskSouth = bitMask(tileDataSouth.edgeFlags, TILE_EDGE_WALL_BIT) * validTileMaskSouth;
        wallMaskSouth *= edgeMaskHor;
        wallMaskSouth = ceil(wallMaskSouth);

        float wallMaskEast = bitMask(tileDataEast.edgeFlags, TILE_EDGE_WALL_WEST_BIT) * validTileMaskEast;
        wallMaskEast *= edgeMaskVert;
        wallMaskEast = ceil(wallMaskEast);

        float wallMaskWest = bitMask(tileDataWest.edgeFlags, TILE_EDGE_WALL_EAST_BIT) * validTileMaskWest;
        wallMaskWest *= edgeMaskVert;
        wallMaskWest = ceil(wallMaskWest);

        float wallMaskCorner = (bitMask(tileDataCorner.edgeFlags, TILE_EDGE_WALL_SOUTH_BIT) + bitMask(tileDataCorner.edgeFlags, TILE_EDGE_WALL_EAST_BIT));
        wallMaskCorner *= validTileMaskCorner;
        wallMaskCorner *= bothEdgesMask;
        wallMaskCorner = ceil(wallMaskCorner);

        float wallMasks = saturate(wallMaskNorth + wallMaskSouth + wallMaskEast + wallMaskWest + wallMaskCorner);
        wallMasks *= edgeMask;
        wallMasks *= levelBoundsMask;

        vec3 wallColor = vec3(1.0, 1.0, 1.0);// - vec3((1.0 - visitedMask) * 0.25);
        finalColor = mix(finalColor, wallColor, wallMasks);

        // Doors
        float validDoorHor = saturate(validTileMaskNorth + validTileMaskSouth);
        float validDoorVert = saturate(validTileMaskEast + validTileMaskWest);

        float doorMaskNorth = bitMask(tileDataNorth.edgeFlags, TILE_EDGE_DOOR_SOUTH_BIT) * validDoorHor;
        doorMaskNorth *= edgeMaskHor * round(abs(map1to1(tileInfo.z - tileSizeReciprocal)) + 0.05);
        doorMaskNorth = ceil(doorMaskNorth);
        float doorMaskSouth = bitMask(tileDataSouth.edgeFlags, TILE_EDGE_DOOR_BIT) * validDoorHor;
        doorMaskSouth *= edgeMaskHor * round(abs(map1to1(tileInfo.z)) + 0.05);
        doorMaskSouth = ceil(doorMaskSouth);
        float doorMaskEast = bitMask(tileDataEast.edgeFlags, TILE_EDGE_DOOR_WEST_BIT) * validDoorVert;
        doorMaskEast *= edgeMaskVert * round(abs(map1to1(tileInfo.w - tileSizeReciprocal)) + 0.05);
        doorMaskEast = ceil(doorMaskEast);
        float doorMaskWest = bitMask(tileDataWest.edgeFlags, TILE_EDGE_DOOR_EAST_BIT) * validDoorVert;
        doorMaskWest *= edgeMaskVert * round(abs(map1to1(tileInfo.w)) + 0.05);
        doorMaskWest = ceil(doorMaskWest);
        float doorMaskCorner = (bitMask(tileDataCorner.edgeFlags, TILE_EDGE_DOOR_SOUTH_BIT) + bitMask(tileDataCorner.edgeFlags, TILE_EDGE_DOOR_EAST_BIT));
        doorMaskCorner *= validTileMaskCorner;
        doorMaskCorner *= bothEdgesMask;
        doorMaskCorner = ceil(doorMaskCorner);

        float doorMasks = saturate(doorMaskNorth + doorMaskSouth + doorMaskEast + doorMaskWest + doorMaskCorner);
        doorMasks *= edgeMask;
        doorMasks *= levelBoundsMask;

        finalColor = overlay(finalColor, wallColor, doorMasks);

        // Grid
        float gridMasks = edgeMask;
        float gridPulseX = saturate(abs((fract(texCoordOriginal.x + (u_time * 0.25)) * 2.0) - 1.0));
        gridPulseX = pow(gridPulseX, 4);
        float gridPulseY = saturate(abs((fract(texCoordOriginal.y + (u_time * 0.15)) * 2.0) - 1.0));
        gridPulseY = pow(gridPulseY, 4);
        float gridPulse = (max(gridPulseX, gridPulseY) * 0.5) + 0.5;
        vec3 grid = mix(vec3(0.05, 0.15, 0.6) * gridPulse, vec3(0.15, 0.25, 0.5) * 1.2, floorTileMask * 0.7f);
        // texCoordOriginal *= u_sizeScreenSpace;
        // texCoordOriginal = floor(texCoordOriginal);
        // float gridForceMask = (1.0 - step(0.001, mod(texCoordOriginal.x, u_sizeScreenSpace.x - 1))) + (1.0 - step(0.001, mod(texCoordOriginal.y, u_sizeScreenSpace.y - 1)));
        // gridForceMask = saturate(gridForceMask);
        // gridForceMask = 0.0;
        finalColor = mix(finalColor, grid, saturate(gridMasks * (1.0 - wallMasks) * (1.0 - doorMasks)));

        color = vec4(finalColor, 1.0);
    }
    else
    {
        color = vec4(borderColor, 1.0);
    }
}
