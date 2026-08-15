//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERER_TYPES_H
#define RME_RENDERER_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

// Forward declaration for wxColour conversion (only when wxWidgets is available)
#ifdef __WX__
#include <wx/colour.h>
#endif

// Pure color structure without wxWidgets dependency
struct ColorRGBA {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    ColorRGBA() = default;
    ColorRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

#ifdef __WX__
    // Conversion from wxColour
    ColorRGBA(const wxColour& colour)
        : r(colour.Red()), g(colour.Green()), b(colour.Blue()), a(colour.Alpha()) {}
    
    // Conversion to wxColour
    operator wxColour() const {
        return wxColour(r, g, b, a);
    }
    
    // Assignment from wxColour
    ColorRGBA& operator=(const wxColour& colour) {
        r = colour.Red();
        g = colour.Green();
        b = colour.Blue();
        a = colour.Alpha();
        return *this;
    }
#endif

    bool operator==(const ColorRGBA& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const ColorRGBA& other) const {
        return !(*this == other);
    }
};

// View information for rendering - pure data structure
struct RendererViewInfo {
    int start_x = 0;
    int start_y = 0;
    int end_x = 0;
    int end_y = 0;
    int floor = 0;
    float zoom = 1.0f;
    int view_scroll_x = 0;
    int view_scroll_y = 0;
    int tile_size = 64;
    int screen_width = 0;
    int screen_height = 0;
};

// Overlay command for rendering - uses pure ColorRGBA instead of wxColor
struct RendererOverlayCommand {
    enum class Type {
        Rect,
        Line,
        Text,
        Sprite,
    };

    Type type = Type::Rect;
    bool screen_space = false;
    bool filled = true;
    bool dashed = false;
    int width = 1;

    int x = 0;
    int y = 0;
    int z = 0;
    int w = 0;
    int h = 0;
    int x2 = 0;
    int y2 = 0;
    int z2 = 0;

    uint32_t sprite_id = 0;

    std::string text;
    ColorRGBA color = ColorRGBA(255, 255, 255, 255);
};

// Overlay tooltip for rendering - uses pure ColorRGBA instead of wxColor
struct RendererOverlayTooltip {
    int x = 0;
    int y = 0;
    int z = 0;
    std::string text;
    ColorRGBA color = ColorRGBA(255, 255, 255, 255);
};

// Hover state for overlay rendering
struct RendererHoverState {
    bool valid = false;
    int x = 0;
    int y = 0;
    int z = 0;
    std::vector<RendererOverlayCommand> commands;
    std::vector<RendererOverlayTooltip> tooltips;
};

#endif // RME_RENDERER_TYPES_H
