# draw

Syntax:
```lua
jam.draw(params)
```

Example:
```lua
-- draw a red rectangle
local x = 50
local y = 50
local w = 150
local h = 100
jam.draw{
    vertices = {
        { x, y },
        { x + w, y },
        { x, y + h },
        { x + w, y + h },
    },
    topology = jam.topology.TriangleStrip,
    color = { 1, 0, 0 },
}
```

#### Required Parameters

`vertices` - Vertices used for rendering. 

#### Optional Parameters

`texcoords` - Texcoords used for drawing. If this parameter is provided, the length must match the length of the `vertices` parameter.

`indices` - Indices used for drawing.

`topology` - The topology used for rendering. Must be one of the following:
```lua
jam.topology.LineList
jam.topology.LineStrip
jam.topology.TriangleList
jam.topology.TriangleStrip
```

#### Remarks

Internally, indices are stored as 16-bit unsigned integers. So please do not submit draw calls with more than 65534 vertices.

# drawText

Syntax:
```lua
jam.draw(params)
```

Example:
```lua
-- draw a green "Hello, World!"
myFont = jam.loadFont(...)
...
jam.drawText{
    text = "Hello, World!", 
    font = myFont, 
    x = 10,
    y = 10,
    color = { 0, 1, 0 },
}
```

#### Required Parameters

`text` - Text to render. 

`font` - Font to use when rendering.

`x` - The x coordinate to render the text at.

`y` - The y coordinate to render the text at.

#### Optional Parameters

`color` - The color of the text.

`range` - The range of the `text` to display. Useful for highlighting text segments.