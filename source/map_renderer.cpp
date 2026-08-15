#include "map_renderer.h"
#include "map_drawer.h" // For tile rendering helpers - will be refactored later
#include <GL/glew.h>
#include <cmath>

MapRenderer::MapRenderer()
    : m_initialized(false)
    , m_textCallback(nullptr)
{
}

MapRenderer::~MapRenderer()
{
    shutdown();
}

bool MapRenderer::initialize()
{
    if (m_initialized) {
        return true;
    }

    // Initialize GLEW (should already be done by the canvas, but safe to check)
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        return false;
    }

    // Create VAO and VBO for tile rendering
    glGenVertexArrays(1, &m_tileVAO);
    glGenBuffers(1, &m_tileVBO);

    glBindVertexArray(m_tileVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_tileVBO);

    // Setup vertex attribute pointers for textured quads
    // This is a simplified setup - actual implementation depends on your tile rendering approach
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_initialized = true;
    return true;
}

void MapRenderer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_tileVBO != 0) {
        glDeleteBuffers(1, &m_tileVBO);
        m_tileVBO = 0;
    }

    if (m_tileVAO != 0) {
        glDeleteVertexArrays(1, &m_tileVAO);
        m_tileVAO = 0;
    }

    m_initialized = false;
}

void MapRenderer::render(const IMapRenderDataSource* map_data,
                         const RenderViewInfo& view_info,
                         const RenderSettings& settings,
                         const RenderSelection& selection)
{
    if (!m_initialized || !map_data) {
        return;
    }

    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup orthographic projection based on view info
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, view_info.screen_width, view_info.screen_height, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Render in layers
    renderMapTiles(map_data, view_info);

    if (settings.show_grid) {
        renderGrid(view_info, settings);
    }

    if (settings.highlight_selected_tiles || selection.is_dragging) {
        renderSelection(view_info, settings, selection);
    }

    if (settings.show_creatures) {
        renderCreatures(map_data, view_info);
    }

    if (settings.show_spawn) {
        renderSpawns(map_data, view_info);
    }

    if (settings.show_houses) {
        renderHouses(map_data, view_info);
    }

    if (settings.show_waypoints) {
        renderWaypoints(map_data, view_info);
    }
}

void MapRenderer::renderOverlay(const std::vector<RendererOverlayCommand>& commands)
{
    if (!m_initialized) {
        return;
    }

    // Process overlay commands (tooltips, etc.)
    for (const auto& cmd : commands) {
        if (cmd.type == RendererOverlayCommand::Type::Text && m_textCallback) {
            m_textCallback(cmd.x, cmd.y, cmd.text, cmd.color);
        }
        // Handle other overlay command types as needed
    }
}

void MapRenderer::onResize(int width, int height)
{
    glViewport(0, 0, width, height);
}

void MapRenderer::setTextRenderCallback(TextRenderCallback callback)
{
    m_textCallback = callback;
}

void MapRenderer::renderMapTiles(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info)
{
    // Delegate to existing tile rendering logic from MapDrawer
    // This will be refactored to use pure OpenGL without wxWidgets dependencies
    // For now, we'll use the existing MapDrawer helpers
    
    int start_x = view_info.world_left;
    int end_x = view_info.world_right;
    int start_y = view_info.world_top;
    int end_y = view_info.world_bottom;

    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            const Tile* tile = map_data->getTile(x, y);
            if (tile) {
                // Render tile using existing logic
                // This needs to be refactored to pure OpenGL
                int screen_x, screen_y;
                worldToScreen(x, y, view_info, screen_x, screen_y);
                
                // Placeholder: actual tile rendering logic goes here
                // Will extract from MapDrawer::DrawTile or similar
            }
        }
    }
}

void MapRenderer::renderGrid(const RenderViewInfo& view_info, const RenderSettings& settings)
{
    glBegin(GL_LINES);
    glColor4ub(settings.grid_color.r, settings.grid_color.g, settings.grid_color.b, settings.grid_color.a);

    int step = static_cast<int>(32 * view_info.zoom); // Assuming 32px tiles
    
    // Vertical lines
    for (int x = view_info.world_left; x <= view_info.world_right; x += step) {
        int screen_x, screen_y_top, screen_y_bottom;
        worldToScreen(x, view_info.world_top, view_info, screen_x, screen_y_top);
        worldToScreen(x, view_info.world_bottom, view_info, screen_x, screen_y_bottom);
        
        glVertex2i(screen_x, screen_y_top);
        glVertex2i(screen_x, screen_y_bottom);
    }

    // Horizontal lines
    for (int y = view_info.world_top; y <= view_info.world_bottom; y += step) {
        int screen_x_left, screen_x_right, screen_y;
        worldToScreen(view_info.world_left, y, view_info, screen_x_left, screen_y);
        worldToScreen(view_info.world_right, y, view_info, screen_x_right, screen_y);
        
        glVertex2i(screen_x_left, screen_y);
        glVertex2i(screen_x_right, screen_y);
    }

    glEnd();
}

void MapRenderer::renderSelection(const RenderViewInfo& view_info, const RenderSettings& settings, const RenderSelection& selection)
{
    if (selection.is_dragging) {
        // Render drag rectangle
        int start_screen_x, start_screen_y;
        int end_screen_x, end_screen_y;
        
        worldToScreen(selection.drag_start_x, selection.drag_start_y, view_info, start_screen_x, start_screen_y);
        worldToScreen(selection.drag_end_x, selection.drag_end_y, view_info, end_screen_x, end_screen_y);
        
        int width = std::abs(end_screen_x - start_screen_x);
        int height = std::abs(end_screen_y - start_screen_y);
        int x = std::min(start_screen_x, end_screen_x);
        int y = std::min(start_screen_y, end_screen_y);
        
        glColor4ub(settings.drag_color.r, settings.drag_color.g, settings.drag_color.b, 64); // Semi-transparent
        glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x + width, y);
        glVertex2i(x + width, y + height);
        glVertex2i(x, y + height);
        glEnd();
        
        // Border
        glColor4ub(settings.drag_color.r, settings.drag_color.g, settings.drag_color.b, 255);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x + width, y);
        glVertex2i(x + width, y + height);
        glVertex2i(x, y + height);
        glEnd();
        glLineWidth(1.0f);
    }

    // Render selected tiles highlight
    if (settings.highlight_selected_tiles) {
        glColor4ub(settings.selection_color.r, settings.selection_color.g, settings.selection_color.b, 128);
        for (const auto& tile_pos : selection.tiles) {
            int screen_x, screen_y;
            worldToScreen(tile_pos.first, tile_pos.second, view_info, screen_x, screen_y);
            int tile_size = static_cast<int>(32 * view_info.zoom);
            
            glBegin(GL_QUADS);
            glVertex2i(screen_x, screen_y);
            glVertex2i(screen_x + tile_size, screen_y);
            glVertex2i(screen_x + tile_size, screen_y + tile_size);
            glVertex2i(screen_x, screen_y + tile_size);
            glEnd();
        }
    }
}

void MapRenderer::renderCreatures(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info)
{
    auto& creatures = map_data->getCreatures();
    for (Creature* creature : creatures) {
        if (creature && isVisible(creature->getX(), creature->getY(), view_info)) {
            // Render creature sprite
            // Placeholder: actual creature rendering logic
        }
    }
}

void MapRenderer::renderSpawns(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info)
{
    auto& spawns = map_data->getSpawns();
    for (Spawn* spawn : spawns) {
        if (spawn && isVisible(spawn->getX(), spawn->getY(), view_info)) {
            // Render spawn indicator
            // Placeholder: actual spawn rendering logic
        }
    }
}

void MapRenderer::renderHouses(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info)
{
    auto& houses = map_data->getHouses();
    for (House* house : houses) {
        if (house) {
            // Render house boundaries or labels
            // Placeholder: actual house rendering logic
        }
    }
}

void MapRenderer::renderWaypoints(const IMapRenderDataSource* map_data, const RenderViewInfo& view_info)
{
    auto& waypoints = map_data->getWaypoints();
    for (Waypoint* waypoint : waypoints) {
        if (waypoint && isVisible(waypoint->getX(), waypoint->getY(), view_info)) {
            // Render waypoint marker
            // Placeholder: actual waypoint rendering logic
        }
    }
}

void MapRenderer::worldToScreen(int world_x, int world_y, const RenderViewInfo& view_info, int& screen_x, int& screen_y)
{
    float zoom = view_info.zoom;
    screen_x = static_cast<int>((world_x - view_info.camera_x) * zoom);
    screen_y = static_cast<int>((world_y - view_info.camera_y) * zoom);
}

bool MapRenderer::isVisible(int world_x, int world_y, const RenderViewInfo& view_info)
{
    return world_x >= view_info.world_left && world_x <= view_info.world_right &&
           world_y >= view_info.world_top && world_y <= view_info.world_bottom;
}
