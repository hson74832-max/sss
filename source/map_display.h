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

#ifndef RME_DISPLAY_WINDOW_H_
#define RME_DISPLAY_WINDOW_H_

#include "action.h"
#include "tile.h"
#include "creature.h"

class Item;
class Creature;
class MapWindow;
class MapPopupMenu : public wxMenu {
public:
	MapPopupMenu(Editor& editor);
	~MapPopupMenu();
	void Update();
	
	Editor& editor;
};
class AnimationTimer;
class MapDrawer;

class MapCanvas : public wxGLCanvas {
	DECLARE_CLASS(MapCanvas)
	DECLARE_EVENT_TABLE()
public:
	MapCanvas(wxWindow* parent, Editor& editor, int* attriblist);
	virtual ~MapCanvas();
	void Reset();

	// All events
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event) { }

	// FPS counter
	void UpdateFPS();
	void DrawFPS();

private:
	double m_fps;
	wxLongLong m_last_time;
	uint32_t m_frame_count;
	wxLongLong m_fps_timer;

protected:
	// State variables - protected for access by derived classes and MapDrawer
	int floor;
	double zoom;
	Editor& editor;
	
	// Drawing state - protected for access by MapDrawer
	bool dragging;
	bool dragging_draw;
	bool is_pasting;
	int drag_start_x, drag_start_y, drag_start_z;
	int last_click_abs_x, last_click_abs_y;
	int cursor_x, cursor_y;
	int last_click_map_x, last_click_map_y;
	
	// Additional state variables needed by map_display.cpp
	static bool processed[];
	bool boundbox_selection;
	bool screendragging;
	bool drawing;
	bool replace_dragging;
	uint8_t* screenshot_buffer;
	int last_cursor_map_x, last_cursor_map_y, last_cursor_map_z;
	int last_click_map_z;
	int last_click_x, last_click_y;
	int last_mmb_click_x, last_mmb_click_y;
	MapPopupMenu* popup_menu;
	AnimationTimer* animation_timer;
	MapDrawer* drawer;
	int keyCode;
	wxStopWatch refresh_watch;
	int view_scroll_x, view_scroll_y;

public:
	void Refresh();
	virtual void ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y);
	virtual void GetScreenCenter(int* map_x, int* map_y);
	virtual int GetClientWidth() const {
		return wxGLCanvas::GetClientSize().GetWidth();
	}
	virtual int GetClientHeight() const {
		return wxGLCanvas::GetClientSize().GetHeight();
	}
	void StartPasting();
	void EndPasting();
	void EnterSelectionMode();
	void EnterDrawingMode();
	void UpdatePositionStatus(int x = -1, int y = -1);
	void UpdateZoomStatus();
	void ChangeFloor(int new_floor);
	void SetFloor(int new_floor) {
		floor = new_floor;
	}
	int GetFloor() const {
		return floor;
	}
	double GetZoom() const {
		return zoom;
	}
	virtual void SetZoom(double value);
	virtual void GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const;
	virtual Position GetCursorPosition() const;
	void TakeScreenshot(wxFileName path, wxString format);
	void MouseToMap(int* map_x, int* map_y) {
		wxPoint pos = wxGetMousePosition();
		ScreenToMap(pos.x, pos.y, map_x, map_y);
	}

	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeftRelease(wxMouseEvent& event);
	void OnMouseLeftClick(wxMouseEvent& event);
	void OnMouseLeftDoubleClick(wxMouseEvent& event);
	void OnMouseCenterClick(wxMouseEvent& event);
	void OnMouseCenterRelease(wxMouseEvent& event);
	void OnMouseRightClick(wxMouseEvent& event);
	void OnMouseRightRelease(wxMouseEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnWheel(wxMouseEvent& event);
	void OnGainMouse(wxMouseEvent& event);
	void OnLoseMouse(wxMouseEvent& event);

	// Mouse events handlers (called by the above)
	void OnMouseActionRelease(wxMouseEvent& event);
	void OnMouseActionClick(wxMouseEvent& event);
	void OnMouseCameraClick(wxMouseEvent& event);
	void OnMouseCameraRelease(wxMouseEvent& event);
	void OnMousePropertiesClick(wxMouseEvent& event);
	void OnMousePropertiesRelease(wxMouseEvent& event);

	//
	void OnCut(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnCopyPosition(wxCommandEvent& event);
	void OnCopyServerId(wxCommandEvent& event);
	void OnCopyClientId(wxCommandEvent& event);
	void OnCopyName(wxCommandEvent& event);
	void OnBrowseTile(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnDelete(wxCommandEvent& event);
	// ----
	void OnGotoDestination(wxCommandEvent& event);
	void OnRotateItem(wxCommandEvent& event);
	void OnSwitchDoor(wxCommandEvent& event);
	// ----
	void OnSelectRAWBrush(wxCommandEvent& event);
	void OnSelectGroundBrush(wxCommandEvent& event);
	void OnSelectDoodadBrush(wxCommandEvent& event);
	void OnSelectDoorBrush(wxCommandEvent& event);
	void OnSelectWallBrush(wxCommandEvent& event);
	void OnSelectCarpetBrush(wxCommandEvent& event);
	void OnSelectTableBrush(wxCommandEvent& event);
	void OnSelectCreatureBrush(wxCommandEvent& event);
	void OnSelectSpawnBrush(wxCommandEvent& event);
	void OnSelectHouseBrush(wxCommandEvent& event);
	void OnSelectCollectionBrush(wxCommandEvent& event);
	void OnSelectMoveTo(wxCommandEvent& event);
	// ---
	void OnProperties(wxCommandEvent& event);
	void OnScriptMenu(wxCommandEvent& event);
	
	// Additional methods needed by map_display.cpp
	bool isPasting() const;
	void getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor, PositionVector* tilestodraw, PositionVector* tilestoborder = nullptr, bool fill = false);
	
	// Flood fill support - declared here for map_display.cpp
	static constexpr int BLOCK_SIZE = 64;
	static int countMaxFills;
	static int getFillIndex(int x, int y);
	bool floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions);
};

class AnimationTimer : public wxTimer {
public:
	AnimationTimer(MapCanvas* canvas);
	~AnimationTimer();

	void Notify();
	void Start();
	void Stop();

private:
	MapCanvas* map_canvas;
	bool started;
};

#endif
