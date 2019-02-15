tile_moon = 2

M = tile_moon

levels = {
    {
        tiles = {
            0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 0,
            0, 0, 0, 0, 0, 0, 1, 0,
            0, 1, 1, 1, 1, 0, 1, 0,
            0, 1, M, 1, 1, 0, 1, 0,
            0, 1, 0, 0, 0, 0, 1, 0,
            0, 1, 1, 1, 1, 1, 1, 0,
            0, 0, 0, 0, 0, 0, 0, 0
        },
        spawnX = 0,
        spawnY = 0,
    },
    {
        tiles = {
            1, 1, 1, 1, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 0, M, 1,
            0, 0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 0, 1, 1,
            0, 0, 0, 0, 0, 0, 1, 1,
            0, 0, 0, 0, 0, 1, 1, 1
        },
        spawnX = 1,
        spawnY = 3,
    },
    {
        tiles = {
            0, 0, 0, 0, 0, 1, 1, 0,
            0, 0, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 1, 1, 0, 0, 0,
            0, 0, 0, 1, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, M, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 1, 1,
            0, 0, 0, 0, 0, 1, 1, 1
        },
        spawnX = 1,
        spawnY = 0,
    },
    {
        tiles = {
            0, 0, 0, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0,
            1, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 0, M, 0,
            1, 1, 1, 1, 1, 0, 0, 0
        },
        spawnX = 1,
        spawnY = 0,
    },
    {
        tiles = {
            1, 1, 1, 1, 1, 1, 0, 0,
            1, 1, 1, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 1, 1, 0, 0, 0,
            0, M, 0, 1, 1, 0, 0, 0,
            0, 0, 0, 1, 1, 1, 0, 0,
            0, 0, 1, 1, 1, 1, 0, 0
        },
        spawnX = 6,
        spawnY = 7,
    },
}

function getTileIndex(x, y)
    --assert(x >= 0 and x < tileCount)
    --assert(y >= 0 and y < tileCount)
    return (y * tileCount) + x + 1
end

function getTile(level, x, y)
    return level.tiles[getTileIndex(x, y)]
end