require('sprite_sheet')
require('levels')

jam.name = "Moon Destroyer"

jam.graphics.width = 64
jam.graphics.height = 64
jam.graphics.pixelScale = 8
jam.graphics.vsync = true

tileSize = 8

tileCount = 8

playerSpeed = 32

function start()
    spriteSheet = SpriteSheet:new(jam.graphics.loadTexture("data/moon_destroyer.png"), 16, 4)

    currentLevelIndex = 1
    currentLevel = levels[1]

    respawnPlayer()
end

function killPlayer()
    player.isDead = true
    player.deathTimer = 0
end

function respawnPlayer()
    player = {
        isDead = false,
        isCharged = false,
        t = 1,
        x = currentLevel.spawnX,
        y = currentLevel.spawnY,
        x1 = currentLevel.spawnX,
        y1 = currentLevel.spawnY,
    }
end

local xInput = 0
local yInput = 0

function resetInput()
    xInput = 0
    yInput = 0
end

function keyDown(key)
    if player.isDead or isMoonDestroyed then 
        return
    end

    if key == jam.input.key.LeftArrow then
        xInput = -1
        yInput = 0
    elseif key == jam.input.key.RightArrow then
        xInput = 1
        yInput = 0
    elseif key == jam.input.key.UpArrow then
        xInput = 0
        yInput = -1
    elseif key == jam.input.key.DownArrow then
        xInput = 0
        yInput = 1
    end
end

isMoonDestroyed = false

function nextLevel()
    isMoonDestroyed = false
    currentLevelIndex = currentLevelIndex + 1
    currentLevel = levels[currentLevelIndex]
    if currentLevel then
        respawnPlayer()
    end
end

function destroyMoon()
    isMoonDestroyed = true
    moonDestroyTimer = 0
end

function tick()
    if not currentLevel then
        return
    end

    -- moon explosion
    if isMoonDestroyed then
        moonDestroyTimer = moonDestroyTimer + jam.deltaTime * 10
        if moonDestroyTimer >= 19 then
            nextLevel()
        end
        return
    end

    -- player explosion
    if player.isDead then
        player.deathTimer = player.deathTimer + jam.deltaTime * 10
        if player.deathTimer >= 6 then
            respawnPlayer()
        end
        return
    end

    if xInput + yInput == 0 then
        return
    end

    player.t = player.t + playerSpeed * jam.deltaTime
    if player.t >= 1 then
        player.t = player.t - 1
        player.x = player.x1
        player.y = player.y1

        local tile = getTile(currentLevel, player.x, player.y)

        if tile == tile_moon then
            destroyMoon()
            resetInput()
            return
        elseif tile == tile_pickup then
            player.isCharged = true
        end
        
        player.x1 = player.x1 + xInput
        player.y1 = player.y1 + yInput

        local nextTile = getTile(currentLevel, player.x1, player.y1)

        -- collision
        if (nextTile == 1 or 
            player.x1 < 0 or player.x1 >= tileCount or
            player.y1 < 0 or player.y1 >= tileCount) then
            killPlayer()
            resetInput()
            return
        end
    end
end

function math.lerp(a, b, t)
	return a + (b - a) * t
end

function draw()
    jam.graphics.setCamera(jam.graphics.width, jam.graphics.height)

    if not currentLevel then
        return
    end

    math.randomseed(0)

    spriteSheet:begin()

    -- draw player
    if not isMoonDestroyed then
        local row = 2
        if player.isCharged then
            row = 3
        end
        if player.isDead then
            -- explosion
            local playerX = player.x * tileSize
            local playerY = player.y * tileSize
            spriteSheet:append(playerX, playerY, tileSize, tileSize, math.floor(player.deathTimer) + 1, row)
        else
            local playerX = math.lerp(player.x, player.x1, player.t) * tileSize
            local playerY = math.lerp(player.y, player.y1, player.t) * tileSize
            spriteSheet:append(playerX, playerY, tileSize, tileSize, 0, row)
        end
    end

    -- draw level
    for tileIndex, tile in ipairs(currentLevel.tiles) do
        local x = ((tileIndex - 1) % tileCount) * tileSize
        local y = math.floor((tileIndex - 1) / tileCount) * tileSize
        if tile == 1 then
            -- draw obstacle tiles
            spriteSheet:append(x, y, tileSize, tileSize, math.random(0, 6), 0)
        elseif tile == tile_moon then
            -- draw the moon
            if isMoonDestroyed then
                -- explosion
                if moonDestroyTimer < 15 then
                    spriteSheet:append(x, y, tileSize, tileSize, math.floor(moonDestroyTimer) + 1, 1)
                end
            else
                spriteSheet:append(x, y, tileSize, tileSize, 0, 1)
            end
        elseif tile == tile_pickup and not player.isCharged then
            -- draw the pickup
            spriteSheet:append(x, y, tileSize, tileSize, 7 + math.floor(jam.elapsedTime * 10 % 3), 2)
        end
    end

    spriteSheet:draw()

    -- draw background
    jam.graphics.draw{
        vertices = {
            { 0,                  0 },
            { jam.graphics.width, 0 },
            { 0,                  jam.graphics.height },
            { jam.graphics.width, jam.graphics.height },
        },
        texcoords = {
            { 0, 0 }, 
            { 1, 0 }, 
            { 0, 1 }, 
            { 1, 1 }, 
        },
        texture = jam.graphics.loadTexture("data/background.png"),
        topology = jam.graphics.topology.TriangleStrip,
    }
end