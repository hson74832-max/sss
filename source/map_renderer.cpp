#include "map_renderer.h"
#include "graphics.h"
#include "sprites.h"
#include "tile.h"
#include "creature.h"
#include "spawn.h"
#include "house.h"
#include "waypoints.h"
#include "items.h"
#include "item.h"
#include "map.h"
#include <GL/glew.h>
#include <cmath>

#ifdef __APPLE__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

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
    if (!map_data) {
        return;
    }

    int start_x = view_info.world_left;
    int end_x = view_info.world_right;
    int start_y = view_info.world_top;
    int end_y = view_info.world_bottom;

    glEnable(GL_TEXTURE_2D);
    
    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            Tile* tile = map_data->getTile(x, y);
            if (!tile) {
                continue;
            }

            // Calculate screen position
            int screen_x, screen_y;
            worldToScreen(x, y, view_info, screen_x, screen_y);
            
            // Apply floor offset
            int map_z = tile->getZ();
            int offset;
            if (map_z <= GROUND_LAYER) {
                offset = (GROUND_LAYER - map_z) * 32;
            } else {
                offset = 32 * (view_info.floor - map_z);
            }
            
            int draw_x = screen_x - offset;
            int draw_y = screen_y - offset;
            
            // Render ground tile
            if (tile->ground) {
                int r = 255, g = 255, b = 255;
                BlitItem(draw_x, draw_y, tile, tile->ground, false, r, g, b, 255, map_data);
            }
            
            // Render items on tile
            for (Item* item : tile->items) {
                if (item->isBorder()) {
                    BlitItem(draw_x, draw_y, tile, item, false, 255, 255, 255, 255, map_data);
                } else {
                    BlitItem(draw_x, draw_y, tile, item, false, 255, 255, 255, 255, map_data);
                }
            }
            
            // Render creature on tile
            if (tile->creature) {
                BlitCreature(draw_x, draw_y, tile->creature, 255, 255, 255, 255, map_data);
            }
        }
    }
}

// Helper method to render an item sprite
void MapRenderer::BlitItem(int& draw_x, int& draw_y, const Tile* tile, Item* item, 
                           bool ephemeral, int red, int green, int blue, int alpha,
                           const IMapRenderDataSource* map_data)
{
    if (!item || !map_data) {
        return;
    }
    
    const Position& pos = tile->getPosition();
    ItemType& it = g_items[item->getID()];
    
    GameSprite* spr = it.sprite;
    if (!spr) {
        return;
    }
    
    int screenx = draw_x - spr->getDrawOffset().first;
    int screeny = draw_y - spr->getDrawOffset().second;
    
    // Update drawing height
    draw_x -= spr->getDrawHeight();
    draw_y -= spr->getDrawHeight();
    
    int subtype = -1;
    int pattern_x = pos.x % spr->pattern_x;
    int pattern_y = pos.y % spr->pattern_y;
    int pattern_z = pos.z % spr->pattern_z;
    
    // Handle special item types
    if (it.isSplash() || it.isFluidContainer()) {
        subtype = item->getSubtype();
    } else if (it.isHangable) {
        if (tile && tile->hasProperty(HOOK_SOUTH)) {
            pattern_x = 1;
        } else if (tile && tile->hasProperty(HOOK_EAST)) {
            pattern_x = 2;
        } else {
            pattern_x = 0;
        }
    } else if (it.stackable) {
        if (item->getSubtype() <= 1) {
            subtype = 0;
        } else if (item->getSubtype() <= 2) {
            subtype = 1;
        } else if (item->getSubtype() <= 3) {
            subtype = 2;
        } else if (item->getSubtype() <= 4) {
            subtype = 3;
        } else if (item->getSubtype() < 10) {
            subtype = 4;
        } else if (item->getSubtype() < 25) {
            subtype = 5;
        } else if (item->getSubtype() < 50) {
            subtype = 6;
        } else {
            subtype = 7;
        }
    }
    
    int frame = item->getFrame();
    
    for (int cx = 0; cx != spr->width; cx++) {
        for (int cy = 0; cy != spr->height; cy++) {
            for (int cf = 0; cf != spr->layers; cf++) {
                int texnum = spr->getHardwareID(cx, cy, cf, subtype, pattern_x, pattern_y, pattern_z, frame);
                
                if (texnum >= 0) {
                    glBlitTexture(screenx + cx * 32, screeny + cy * 32, texnum, red, green, blue, alpha);
                }
            }
        }
    }
}

// Helper method to render a creature
void MapRenderer::BlitCreature(int screenx, int screeny, const Creature* creature, 
                               int red, int green, int blue, int alpha,
                               const IMapRenderDataSource* map_data)
{
    if (!creature || !map_data) {
        return;
    }
    
    Outfit outfit = creature->getOutfit();
    Direction dir = creature->getDirection();
    
    // Get creature sprite and render
    GameSprite* spr = g_items.getCreatureSprite(creature->getID());
    if (spr) {
        int frame = creature->getFrame();
        int pattern_x = 0;
        int pattern_y = 0;
        int pattern_z = 0;
        
        for (int cx = 0; cx != spr->width; cx++) {
            for (int cy = 0; cy != spr->height; cy++) {
                for (int cf = 0; cf != spr->layers; cf++) {
                    int texnum = spr->getHardwareID(cx, cy, cf, -1, pattern_x, pattern_y, pattern_z, frame);
                    if (texnum >= 0) {
                        glBlitTexture(screenx + cx * 32, screeny + cy * 32, texnum, red, green, blue, alpha);
                    }
                }
            }
        }
    }
}

// Helper method to blit a texture
void MapRenderer::glBlitTexture(int sx, int sy, int texture_number, int red, int green, int blue, int alpha)
{
    glBindTexture(GL_TEXTURE_2D, texture_number);
    
    glColor4ub(red, green, blue, alpha);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2i(sx, sy);
    glTexCoord2f(1.0f, 0.0f); glVertex2i(sx + 32, sy);
    glTexCoord2f(1.0f, 1.0f); glVertex2i(sx + 32, sy + 32);
    glTexCoord2f(0.0f, 1.0f); glVertex2i(sx, sy + 32);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
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
