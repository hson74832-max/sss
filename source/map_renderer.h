#pragma once

#include "renderer_types.h"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <memory>
#include <sstream>

// Forward declarations to avoid heavy includes
class Map;
class Tile;
class Item;
class Creature;
class Spawn;
class House;
class Waypoint;
class MinimapColor;
class Brush;
class Outfit;
enum class Direction;
struct Position;
struct ItemType;
enum class TileLocation;
class GameSprite;
class GraphicManager;
class QTreeNode;

// Pure data structures for rendering state
// These contain ONLY data needed for rendering, no GUI logic

struct RenderViewInfo {
    int camera_x;
    int camera_y;
    float zoom;
    int screen_width;
    int screen_height;
    
    // Pre-calculated view bounds in world coordinates
    int world_left;
    int world_top;
    int world_right;
    int world_bottom;
    
    // Floor information
    int floor;
    int start_z;
    int end_z;
    int superend_z;
};

struct RenderSettings {
    bool show_grid;
    bool show_spawn;
    bool show_houses;
    bool show_waypoints;
    bool show_creatures;
    bool show_items;
    bool show_debug_text;
    bool highlight_selected_tiles;
    bool highlight_drag_area;
    bool transparent_floors;
    bool transparent_items;
    bool show_shade;
    bool show_all_floors;
    bool show_blocking;
    bool show_hooks;
    bool hide_items_when_zoomed;
    bool show_only_colors;
    bool show_preview;
    
    ColorRGBA grid_color;
    ColorRGBA selection_color;
    ColorRGBA drag_color;
};

struct RenderSelection {
    std::vector<std::pair<int, int>> tiles; // List of selected tile coordinates
    int drag_start_x;
    int drag_start_y;
    int drag_end_x;
    int drag_end_y;
    bool is_dragging;
};

// Abstract interface for map access
// This allows the renderer to query map data without knowing about the Map class details
class IMapRenderDataSource {
public:
    virtual ~IMapRenderDataSource() = default;
    
    virtual Tile* getTile(int x, int y) = 0;
    virtual const Tile* getTile(int x, int y) const = 0;
    
    virtual std::vector<Creature*>& getCreatures() = 0;
    virtual std::vector<Spawn*>& getSpawns() = 0;
    virtual std::vector<House*>& getHouses() = 0;
    virtual std::vector<Waypoint*>& getWaypoints() = 0;
    
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    
    // Additional methods for detailed tile rendering
    virtual QTreeNode* getLeaf(int x, int y) = 0;
    virtual GraphicManager& getGraphics() = 0;
};

// Main OpenGL Renderer Class
// NO wxWidgets dependencies allowed here
class MapRenderer {
public:
    MapRenderer();
    ~MapRenderer();
    
    // Initialize OpenGL resources (called once after context creation)
    bool initialize();
    
    // Cleanup OpenGL resources
    void shutdown();
    
    // Main render function
    void render(const IMapRenderDataSource* map_data,
                const RenderViewInfo& view_info,
                const RenderSettings& settings,
                const RenderSelection& selection);
    
    // Render overlay elements (tooltips, etc.)
    void renderOverlay(const std::vector<RendererOverlayCommand>& commands);
    
    // Handle window resize
    void onResize(int width, int height);
    
    // Set text rendering callback (since text rendering might need GUI integration)
    using TextRenderCallback = void(*)(int x, int y, const std::string& text, const ColorRGBA& color);
    void setTextRenderCallback(TextRenderCallback callback);

private:
    // Internal rendering methods
    void renderMapTiles(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info);
    void renderGrid(const RenderViewInfo& view_info, const RenderSettings& settings);
    void renderSelection(const RenderViewInfo& view_info, const RenderSettings& settings, const RenderSelection& selection);
    void renderCreatures(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info);
    void renderSpawns(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info);
    void renderHouses(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info);
    void renderWaypoints(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info);
    
    // Helper methods
    void worldToScreen(int world_x, int world_y, const RenderViewInfo& view_info, int& screen_x, int& screen_y);
    bool isVisible(int world_x, int world_y, const RenderViewInfo& view_info);
    
    // OpenGL state
    bool m_initialized;
    GLuint m_tileVBO = 0;
    GLuint m_tileVAO = 0;
    
    // Text rendering callback
    TextRenderCallback m_textCallback;
};
