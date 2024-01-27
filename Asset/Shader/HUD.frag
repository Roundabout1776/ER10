
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
    uint povX;
    uint povY;
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
        vec3 finalColor = vec3(0.03, 0.03, 0.02);

        float levelWidth = float(u_map.width);
        float levelHeight = float(u_map.height);
        float levelTileCount = levelWidth * levelHeight;

        float povX = float(u_map.povX);
        float povY = float(u_map.povY);
        vec2 pov = vec2(povX, povY);

        vec2 texCoord = f_texCoord;
        vec2 texCoordOriginal = texCoord;
        texCoord *= u_sizeScreenSpace;

        const float tileSize = 12.0;

        float centerOffsetX = round(u_sizeScreenSpace.x * 0.5 - (povX + 0.5) * tileSize);
        float centerOffsetY = round(u_sizeScreenSpace.y * 0.5 - (povY + 0.5) * tileSize);

        texCoord += vec2(-centerOffsetX, -centerOffsetY); // Center offset

        // texCoord = cartesianToIsometric(texCoord);

        float tileX = floor(texCoord.x / tileSize);
        float tileY = floor(texCoord.y / tileSize);
        float tileU = (texCoord.x - (tileX * tileSize)) / tileSize;
        float tileV = (texCoord.y - (tileY * tileSize)) / tileSize;

        int index = int(clamp(tileY * levelWidth + tileX, 0.0, levelTileCount - 1.0));
        TileData tile = u_map.tiles[index];

        float validTileMask = max(0.0, sign(tileX + 1));
        validTileMask = min(validTileMask, sign(tileY + 1));
        validTileMask = min(validTileMask, 1.0 - saturate(trunc(tileX / levelWidth)));
        validTileMask = min(validTileMask, 1.0 - saturate(trunc(tileY / levelHeight)));
        validTileMask = saturate(validTileMask);

        float exploredMask = bitMask(tile.specialFlags, TILE_SPECIAL_EXPLORED_BIT);
        validTileMask *= exploredMask;

        // Floor
        float floorTileMask = bitMask(tile.flags, TILE_FLOOR_BIT) * validTileMask;
        float checkerMask = floor(mod(tileX + mod(tileY, 2.0), 2.0));
        float visitedMask = bitMask(tile.specialFlags, TILE_SPECIAL_VISITED_BIT);
        vec3 floorTile = vec3(0.0, 0.0, 1.0 - ((visitedMask) * checkerMask * 0.2) - ((1.0 - visitedMask) * 0.8));
        finalColor = overlay(finalColor, floorTile, floorTileMask);

        // Current POV
        float povTileMask = (1.0 - min(abs(tileX - povX), 1.0)) * (1.0 - min(abs(tileY - povY), 1.0));
        float povAnim = (abs(sin((u_time * 10.0))) * 0.6) + 0.4;
        float povMask = saturate((povTileMask * (1.0 - distance(vec2(map1to1(tileU), map1to1(tileV)) * 2.0, vec2(0.0, 0.0))))) * povAnim;
        vec3 povColor = vec3(1.0, 1.0, 1.0);
        finalColor = overlay(finalColor, povColor, povMask * validTileMask);

        // Edges
        const float edgeMaskSize = 1.0 / tileSize;
        float northMask = floor((1.0 - tileV) + edgeMaskSize);
        float northWallMask = northMask * bitMask(tile.edgeFlags, TILE_EDGE_WALL_BIT);
        float southMask = floor(tileV + edgeMaskSize);
        float southWallMask = southMask * bitMask(tile.edgeFlags, TILE_EDGE_WALL_SOUTH_BIT);
        float eastMask = floor(tileU + edgeMaskSize);
        float eastWallMask = eastMask * bitMask(tile.edgeFlags, TILE_EDGE_WALL_EAST_BIT);
        float westMask = floor((1.0 - tileU) + edgeMaskSize);
        float westWallMask = westMask * bitMask(tile.edgeFlags, TILE_EDGE_WALL_WEST_BIT);
        float wallMasks = saturate(northWallMask + southWallMask + eastWallMask + westWallMask) * validTileMask * floorTileMask;
        vec3 wallColor = vec3(0.95, 0.95, 0.95) - vec3((1.0 - visitedMask) * 0.25);
        finalColor = overlay(finalColor, wallColor, wallMasks);

        float northDoorMask = northMask * bitMask(tile.edgeFlags, TILE_EDGE_DOOR_BIT) * round(abs(map1to1(tileU)) + 0.2);
        float southDoorMask = southMask * bitMask(tile.edgeFlags, TILE_EDGE_DOOR_SOUTH_BIT) * round(abs(map1to1(tileU)) + 0.2);
        float eastDoorMask = eastMask * bitMask(tile.edgeFlags, TILE_EDGE_DOOR_EAST_BIT) * round(abs(map1to1(tileV)) + 0.2);
        float westDoorMask = westMask * bitMask(tile.edgeFlags, TILE_EDGE_DOOR_WEST_BIT) * round(abs(map1to1(tileV)) + 0.2);
        float doorMasks = saturate(northDoorMask + southDoorMask + eastDoorMask + westDoorMask) * validTileMask * floorTileMask;
        finalColor = overlay(finalColor, wallColor, doorMasks);

        // Grid
        const float gridMaskSize = 1.0 / tileSize;
        northMask = floor((1.0 - tileV) + gridMaskSize);
        westMask = floor((1.0 - tileU) + gridMaskSize);
        float gridMasks = northMask + westMask;
        float gridPulseX = saturate(abs((fract(texCoordOriginal.x + (u_time * 0.25)) * 2.0) - 1.0));
        gridPulseX = pow(gridPulseX, 4);
        float gridPulseY = saturate(abs((fract(texCoordOriginal.y + (u_time * 0.15)) * 2.0) - 1.0));
        gridPulseY = pow(gridPulseY, 4);
        float gridPulse = (max(gridPulseX, gridPulseY) * 0.5) + 0.5;
        gridPulse *= (1.0 - floorTileMask);
        vec3 grid = vec3(0.05, 0.15, 0.6) * gridPulse;
        finalColor = overlay(finalColor, grid, gridMasks * (1.0 - wallMasks) * (1.0 - doorMasks) * (1.0 - floorTileMask));

        color = vec4(finalColor, 1.0);
    }
    else
    {
        color = vec4(borderColor, 1.0);
    }
}
