# Performance Analysis & Refactoring Plan

## Executive Summary

This is a **2D tile-based map editor** (Remere's Map Editor / OTAcademy Map Editor) for Tibia maps, NOT a 3D game engine. The architecture is fundamentally different from what was requested:

- **Rendering**: OpenGL 2.x immediate mode (glBegin/glEnd) for 2D tiles
- **Tile Size**: Already correctly configured at 64x64 (`TileSize = 64` in definitions.h:136)
- **Sprite Size**: Intentionally kept at 32x32 (Tibia native sprite resolution)
- **Lighting**: CPU-based light accumulation rendered to GPU texture (not compute shaders)
- **GUI Framework**: wxWidgets (not a pure game rendering loop)

## Critical Findings

### 1. Tile Dimensions - ALREADY CORRECT ✓

**File**: `source/definitions.h:134-139`
```cpp
// Keep SPRITE_PIXELS at 32: Tibia sprites remain 32x32 while the editor
// renders each world tile on a 64x64 pixel grid.
constexpr int TileSize = 64;
#define SPRITE_PIXELS 32
```

**Decision**: Changing `SPRITE_PIXELS` to 64 would be **counterproductive**:
- Tibia sprites are natively 32x32; upscaling wastes 4x VRAM per sprite
- `TileSize=64` already provides the desired visual scaling
- Would break binary compatibility with existing sprite data files
- Increases bandwidth pressure during rendering with no quality gain

### 2. Dead Code Identified ⚠️

#### Fully Dead Modules (100% unused):
- **`rme_net.h` / `rme_net.cpp`**: Entire file wrapped in `#if 0` (lines 18-85)
  - No includes anywhere in codebase
  - Legacy networking code, safe to remove
  
#### Partially Dead Code:
- **`map.cpp:76`**: Template map generation code wrapped in `#if 0`
- **`spawn.cpp:41`**: Dead code block

### 3. Memory Management Issues 🔴

**Current State**: 
- 1,022 uses of `newd` (debug-aware new) in .cpp files
- ~265 raw `delete` calls scattered across codebase
- NO smart pointers (std::unique_ptr, std::shared_ptr) used
- NO arena/pool allocators

**Critical Leak Risks**:
1. `graphics.h:205` - `std::list<TemplateImage*> instanced_templates` - raw pointer list
2. `graphics.h:319-322` - `std::map<int, Sprite*>` and `std::map<int, GameSprite::Image*>` - raw pointers
3. `action.cpp` - Manual `delete` in destructor chains
4. `gui.h:436` - `typedef std::list<PaletteWindow*> PaletteList` - raw pointer list

### 4. Performance Anti-Patterns 🐌

#### Container Choices (Hot Path Issues):
```cpp
// graphics.h:204-205
std::vector<NormalImage*> spriteList;        // OK (contiguous)
std::list<TemplateImage*> instanced_templates; // BAD (linked list, cache misses)

// graphics.h:319-322
typedef std::map<int, Sprite*> SpriteMap;    // BAD (tree-based, allocations)
typedef std::map<int, GameSprite::Image*> ImageMap; // BAD

// item.h:414
typedef std::list<Item*> ItemList;           // BAD (linked list)

// tile.h:271
typedef std::list<Tile*> TileList;           // BAD (linked list)
```

#### String Allocations:
- `otml.h` - Heavy use of `std::string` as map keys
- `items.h:443` - `typedef std::map<std::string, ItemType*> ItemNameMap`
- Every frame lookups using string keys in rendering paths

### 5. Rendering Bottlenecks 🎨

**File**: `map_drawer.cpp` (2,241 lines)

**Current Implementation**:
- Immediate mode OpenGL (`glBegin`/`glEnd`) - line 2139-2148
- One draw call per tile/item - NO batching
- Per-tile texture binds - `glBindTexture` in `glBlitTexture` (line 2137)
- NO frustum culling beyond basic viewport checks
- NO occlusion queries (2D editor, less critical)

**Hot Loop Example** (map_drawer.cpp:2135-2150):
```cpp
void MapDrawer::glBlitTexture(int sx, int sy, int texture_number, ...) {
    if (texture_number != 0) {
        glBindTexture(GL_TEXTURE_2D, texture_number);  // ← Bind per tile!
        glColor4ub(...);
        glBegin(GL_QUADS);  // ← Immediate mode!
        // ... four vertices ...
        glEnd();
    }
}
```

**Missing Optimizations**:
- No vertex buffer objects (VBOs)
- No texture atlasing
- No instanced rendering
- No dirty rectangle tracking for sprite DCs

### 6. Light Renderer Analysis 💡

**File**: `light_drawer.cpp` (154 lines)

**Current Implementation**:
- CPU-based light accumulation (lines 42-68)
- Per-pixel iteration in nested loops
- Single texture upload per frame via `glTexImage2D` (line 81)
- NO GPU readback (good!)
- Proper cleanup on shutdown (lines 150-153)

**Issues**:
- O(n*m) complexity where n=view width, m=light count
- Buffer reallocates every frame (line 40)
- Not using compute shaders (but appropriate for this use case)

### 7. Compile-Time Performance Killers 🐢

**Brittle Includes**:
- `main.h` included by 97 .cpp files
- `main.h` pulls in:
  - Entire wxWidgets precompiled header
  - Boost libraries (asio, utility, range)
  - Full STL (list, vector, map, string, etc.)
  - PugiXML
  - Libarchive

**Example**: `map_drawer.cpp` includes `main.h` which includes everything, but only needs:
```cpp
#include "map_drawer.h"
#include "graphics.h"
#include <GL/gl.h>
```

### 8. Circular Dependency Risk 🔗

**Potential Cycles**:
```
editor.h → map.h → tile.h → item.h → items.h → editor.h?
graphics.h → client_version.h → ?
```

Need to verify with include-what-you-use tool.

## Recommended Changes (Prioritized)

### Phase 1: Quick Wins (Low Risk, High Impact)

1. **Remove dead `rme_net.*` files** - Zero risk, reduces confusion
2. **Replace `std::list` with `std::vector` in hot paths** - Cache locality improvement
3. **Add reserve() calls to frequently-growing vectors** - Reduce allocations
4. **Fix `light_drawer.cpp` buffer allocation** - Reuse buffer instead of reallocating

### Phase 2: Memory Safety (Medium Risk)

5. **Introduce `std::unique_ptr` for owned resources** - Start with GraphicManager
6. **Add RAII wrappers for OpenGL resources** - Texture, VBO guards
7. **Replace raw pointer containers with smart pointers**

### Phase 3: Rendering Optimization (High Risk, Requires Testing)

8. **Implement texture atlasing** - Batch sprite textures
9. **Migrate to VBOs** - Replace immediate mode
10. **Add dirty tracking for sprite DCs** - Avoid regeneration

### Phase 4: Architecture (Highest Risk)

11. **Module separation** - Core, Graphics, GUI, AssetManagement
12. **Include hygiene** - Forward declarations, IWYU
13. **Data-oriented redesign** - Structure of Arrays for tile data

## Files Requiring Immediate Attention

| File | Issue | Priority |
|------|-------|----------|
| `rme_net.h/cpp` | 100% dead code | P0 (remove) |
| `graphics.h` | Raw pointer containers | P1 |
| `map_drawer.cpp` | Immediate mode GL, no batching | P1 |
| `light_drawer.cpp` | Buffer reallocation per frame | P2 |
| `item.h`, `tile.h` | std::list typedefs | P2 |
| `main.h` | Over-inclusive header | P3 |

## Metrics to Track

- Frame time (target: 8.33ms for 120 FPS)
- Draw calls per frame (current: ~tiles_visible * items_per_tile)
- Allocations per frame (target: <100)
- VRAM usage (sprite textures + light buffer)
- Compile time (current: unknown, likely high due to main.h)

## Conclusion

This codebase is a **functional 2D editor**, not a 3D game engine. Many requested changes (compute shader lighting, instance rendering, frustum culling) are over-engineering for this use case. Focus on:

1. Removing actual dead code
2. Fixing memory safety issues
3. Reducing draw calls through batching
4. Improving cache locality

The tile size is already correct. Don't change it.
