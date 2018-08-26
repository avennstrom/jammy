SpriteSheet = {}

function SpriteSheet:new(texture, width, height)
    spriteSheet = {
        texture = texture,
        invWidth = 1 / width,
        invHeight = 1 / height
    }
    setmetatable(spriteSheet, self)
    self.__index = self
    return spriteSheet
end

function SpriteSheet:begin()
    self.vertices = {}
    self.texcoords = {}
    self.indices = {}
end

function SpriteSheet:append(x, y, w, h, u, v)
    local baseVertex = table.getn(self.vertices)

    u = u * self.invWidth
    v = v * self.invHeight

    table.insert(self.vertices, { x,     y })
    table.insert(self.vertices, { x + w, y })
    table.insert(self.vertices, { x,     y + h })
    table.insert(self.vertices, { x + w, y + h })

    table.insert(self.texcoords, { u,                 v })
    table.insert(self.texcoords, { u + self.invWidth, v })
    table.insert(self.texcoords, { u,                 v + self.invHeight })
    table.insert(self.texcoords, { u + self.invWidth, v + self.invHeight })

    table.insert(self.indices, baseVertex)
    table.insert(self.indices, baseVertex + 1)
    table.insert(self.indices, baseVertex + 2)
    table.insert(self.indices, baseVertex + 2)
    table.insert(self.indices, baseVertex + 1)
    table.insert(self.indices, baseVertex + 3)
end

function SpriteSheet:draw()
    jam.graphics.draw{
        vertices = self.vertices,
        texcoords = self.texcoords,
        indices = self.indices,
        texture = self.texture,
		topology = jam.graphics.topology.TriangleList,
    }
end