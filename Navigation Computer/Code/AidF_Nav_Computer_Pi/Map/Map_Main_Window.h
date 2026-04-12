#include <SDL2/SDL.h>
#include <stdint.h>
#include <sqlite3.h>
#include <zlib.h>
#include <unistd.h>

#include <string>

#include "Nav_Colors.h"
#include "Nav_Parameters.h"
#include "Map_Objects.h"
#include "Bytes.h"
#include "vector_tile.pb.h"

#include "../Window/Nav_Window.h"
#include "../Menu/Nav_Menu.h"
#include "../Text_Box.h"
#include "../Serial_AIBus_Handler.h"
#include "../AidF_Color_Profile.h"
#include "../Locale/Locale.h"

#ifndef map_main_window_h
#define map_main_window_h

#define DEFAULT_OFFSET 20

using namespace std;

class MapMainWindow : public NavWindow {
public:
	MapMainWindow(AttributeList* attribute_list, NavParameters* parameters);
	~MapMainWindow();

	void drawWindow();
	void refreshWindow();

	bool handleAIBus(AIData* msg);
private:
	NavMenu* settings_menu = nullptr;

	NavParameters* nav_parameters;
	SDL_Texture* map_canvas = NULL;

	WrapTextBox map_fail_msg;

	bool map_success = false; //True once a map has been loaded successfully.

	int tile_bounds = 4096;
	int set_row = 0, set_column = 0;
	bool tile_set = false; //True if the initial tile has been set.

	MapObjectRoad* roads = nullptr;
	int road_l = 0;

	int offset_h = 0, offset_v = 0;

	uint32_t tile_position_x = 0, tile_position_y = 0;

	sqlite3* sq_database = nullptr;
	sqlite3_stmt* sq_statement = nullptr;

	void loadMapData();
	Bytes getTileData(const int row, const int column, const int zoom);
};

int getRow(const double latitude, const int zoom, uint32_t* y_pos);
int getColumn(const double longitude, const int zoom, uint32_t* x_pos);

Bytes decompressGZ(Bytes* compressed);
std::vector<MapObjectRoad> getRoads(Bytes* tile_data);

#endif
