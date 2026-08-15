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

#ifndef RME_MAP_OVERLAY_H
#define RME_MAP_OVERLAY_H

#include <string>
#include <vector>
#include <wx/colour.h>
#include "renderer_types.h"

struct MapViewInfo {
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

struct MapOverlayCommand {
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
	wxColor color = wxColor(255, 255, 255, 255);
	
	// Conversion to renderer-compatible format
	RendererOverlayCommand toRendererCommand() const {
		RendererOverlayCommand cmd;
		cmd.type = static_cast<RendererOverlayCommand::Type>(static_cast<int>(type));
		cmd.screen_space = screen_space;
		cmd.filled = filled;
		cmd.dashed = dashed;
		cmd.width = width;
		cmd.x = x;
		cmd.y = y;
		cmd.z = z;
		cmd.w = w;
		cmd.h = h;
		cmd.x2 = x2;
		cmd.y2 = y2;
		cmd.z2 = z2;
		cmd.sprite_id = sprite_id;
		cmd.text = text;
		cmd.color = ColorRGBA(color.Red(), color.Green(), color.Blue(), color.Alpha());
		return cmd;
	}
};

struct MapOverlayTooltip {
	int x = 0;
	int y = 0;
	int z = 0;
	std::string text;
	wxColor color = wxColor(255, 255, 255, 255);
	
	// Conversion to renderer-compatible format
	RendererOverlayTooltip toRendererTooltip() const {
		RendererOverlayTooltip tooltip;
		tooltip.x = x;
		tooltip.y = y;
		tooltip.z = z;
		tooltip.text = text;
		tooltip.color = ColorRGBA(color.Red(), color.Green(), color.Blue(), color.Alpha());
		return tooltip;
	}
};

struct MapOverlayHoverState {
	bool valid = false;
	int x = 0;
	int y = 0;
	int z = 0;
	std::vector<MapOverlayCommand> commands;
	std::vector<MapOverlayTooltip> tooltips;
};

#endif // RME_MAP_OVERLAY_H
