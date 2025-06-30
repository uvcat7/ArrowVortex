# Canvas
## `enum` BlendMode
An enum that affects how blending should work when drawing.

* Value `BM_NONE` - No blending.
* Value `BM_ALPHA` - Alpha blending.
* Value `BM_ADD` - Additive blending.

## `class` Canvas
Draws images and shapes to a pixel buffer.

#### `constructor Canvas()`
#### `constructor Canvas(int width, int height, float lum = 0.0F)`
#### `constructor Canvas(const Canvas& other)`

#### `destructor ~Canvas()`
