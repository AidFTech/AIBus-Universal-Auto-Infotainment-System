#include "Map_Main_Window.h"

MapMainWindow::MapMainWindow(AttributeList* attribute_list, NavParameters* parameters) : NavWindow(attribute_list),
	map_fail_msg(renderer, MAIN_TITLE_AREA_X, MAIN_TITLE_AREA_Y *3/2, attribute_list->w - 2*MAIN_TITLE_AREA_X, TITLE_HEIGHT, 32, &attribute_list->color_profile->text) {
	this->nav_parameters = parameters;

	this->map_canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, this->w, this->h);
	map_fail_msg.setText(getString(LOCALE_STRING_MAP_NOT_FOUND, attribute_list->locale));

	refreshWindow();
}

MapMainWindow::~MapMainWindow() {
	delete this->settings_menu;
	delete[] this->roads;
	
	if(this->map_canvas != NULL)
		SDL_DestroyTexture(this->map_canvas);

	if(this->sq_statement != nullptr && this->sq_statement != NULL)
		sqlite3_finalize(sq_statement);
	if(this->sq_database != nullptr && this->sq_database != NULL)
		sqlite3_close(sq_database);
}

//Draw the window.
void MapMainWindow::drawWindow() {
	if(!map_success) {
		map_fail_msg.drawText();
		return;
	}

	if(nav_parameters->update_map) {
		nav_parameters->update_map = false;
		const uint32_t br_color = attribute_list->night ? MAP_BR_NIGHT : MAP_BR_DAY;

		SDL_SetRenderTarget(renderer, map_canvas);
		SDL_SetRenderDrawColor(renderer, getRedComponent(br_color), getGreenComponent(br_color), getBlueComponent(br_color), getAlphaComponent(br_color));
		SDL_RenderClear(renderer);

		const int left_limit = -offset_h, up_limit = -offset_v, down_limit = -offset_v + int(h), right_limit = -offset_h + int(w);

		for(int i=0;i<road_l;i+=1) {
			int x1, y1, x2, y2;
			roads[i].getBounds(&x1, &y1, &x2, &y2);
			
			if(((x1 >= left_limit || x2 >= left_limit) &&
				(y1 >= up_limit || y2 >= up_limit) &&
				(x1 <= right_limit || x2 <= right_limit) &&
				(y1 <= down_limit || y2 <= down_limit)) ||
				(x1 <= left_limit && y1 <= up_limit && x2 >= right_limit && y2 >= down_limit)) {
					roads[i].drawOnMap(renderer, map_canvas, this->offset_h, this->offset_v, attribute_list->night);
			
					TextBox* road_name = roads[i].getNameTextBox();
					if(road_name != NULL && road_name != nullptr) {
						road_name->drawText();
					}
				}
		}

		SDL_SetRenderTarget(renderer, NULL);
	}

	SDL_Rect map_rect = {0, 0, this->w, this->h};
	SDL_RenderCopy(renderer, this->map_canvas, NULL, &map_rect);
}

//Refresh the window.
void MapMainWindow::refreshWindow() {
	loadMapData();
	nav_parameters->update_map = true;

	this->offset_h = -int(double(tile_position_x)/UINT32_MAX*tile_bounds) + this->w/2;
	this->offset_v = -int(double(tile_position_y)/UINT32_MAX*tile_bounds) + this->h/2;

	map_fail_msg.renderText();
}

//Load the map data.
void MapMainWindow::loadMapData() {
	map_success = false;

	//Check that the map file exists.
	if(this->nav_parameters->map_path.length() <= 0)
		return;

	//Open the map file.
	sqlite3_open(this->nav_parameters->map_path.c_str(), &this->sq_database);
	if(sq_database == NULL)
		return;

	const int zoom = nav_parameters->zoom;
	const int row = getRow(nav_parameters->latitude, zoom, &this->tile_position_y), column = getColumn(nav_parameters->longitude, zoom, &this->tile_position_x);

	if(row != set_row || column != set_column || !this->tile_set) {
		Bytes tile_data = getTileData(row, column, zoom);
		nav_parameters->update_map = true;

		if(tile_data.l > 0) {
			std::vector<MapObjectRoad> roads = getRoads(&tile_data);
			delete[] this->roads;
			this->roads = new MapObjectRoad[roads.size()];
			this->road_l = roads.size();

			for(int i=0;i<roads.size();i+=1)
				this->roads[i] = roads.at(i);
		}
		
		sqlite3_finalize(sq_statement);
	}

	set_row = row;
	set_column = column;
	tile_set = true;

	map_success = true;

	//TODO: Do we want to close this if we are going to need the map again?
	sqlite3_close(sq_database);
}

//Get data from a specific tile.
Bytes MapMainWindow::getTileData(const int row, const int column, const int zoom) {
	Bytes data(0);

	std::string command = "select tile_data from tiles where zoom_level=" + to_string(zoom) + " and tile_column=" + to_string(column) + " and tile_row=" + to_string(pow(2, zoom) - 1 - row);
	sqlite3_prepare(this->sq_database, command.c_str(), -1, &this->sq_statement, NULL);
	while(sqlite3_step(sq_statement) != SQLITE_DONE) {
		const int column_count = sqlite3_column_count(sq_statement);
		for(int c=0;c<column_count;c+=1) {
			if(sqlite3_column_type(sq_statement, c) == SQLITE_BLOB) {
				const int l = sqlite3_column_bytes(sq_statement, c);
				if(l > 0) {
					uint8_t* n_data = (uint8_t*)sqlite3_column_blob(sq_statement, c);
					data = Bytes(l, n_data);
				}
			}
		}
	}

	data = decompressGZ(&data);
	return data;
}

//Handle an AIBus message.
bool MapMainWindow::handleAIBus(AIData* msg) {
	if(msg->receiver != ID_NAV_COMPUTER)
		return false;

	const int last_offset_v = this->offset_v, last_offset_h = this->offset_h;
	
	SerialAIBusHandler* aibus_handler = this->attribute_list->aibus_handler;
	if(msg->sender == ID_NAV_SCREEN) {
		if(msg->l >= 3 && msg->data[0] == 0x30) { //Button press.
			const uint8_t button = msg->data[1], state = msg->data[2]>>6;

			if(button == 0x28 && state == 0x0) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				this->offset_v += DEFAULT_OFFSET;
				nav_parameters->update_map = true;
				return true;
			} else if(button == 0x29 && state == 0x0) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				this->offset_v -= DEFAULT_OFFSET;
				nav_parameters->update_map = true;
				return true;
			} else if(button == 0x2A && state == 0x0) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				this->offset_h += DEFAULT_OFFSET;
				nav_parameters->update_map = true;
				return true;
			} else if(button == 0x2B && state == 0x0) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				this->offset_h -= DEFAULT_OFFSET;
				nav_parameters->update_map = true;
				return true;
			}
		} else if(msg->l >= 3 && msg->data[0] == 0x32) { //Knob turn.
			const uint8_t knob = msg->data[1], steps = msg->data[2]&0xF;
			const bool cw = (msg->data[2]&0x10) != 0;
			
			if(knob == 0x7) {
				aibus_handler->sendAcknowledgement(ID_NAV_COMPUTER, msg->sender);
				if(cw) {
					if(nav_parameters->zoom + steps <= 14)
						nav_parameters->zoom += steps;
					else
					 	nav_parameters->zoom = 14;
				} else {
					if(nav_parameters->zoom - steps >= 0)
						nav_parameters->zoom -= steps;
					else
					 	nav_parameters->zoom = 0;
				}
				refreshWindow();
				return true;
			}
		}
	}

	if(last_offset_h != this->offset_h || last_offset_v != this->offset_v)
		nav_parameters->update_map = true;

	return false;
}

//Get the row.
int getRow(const double latitude, const int zoom, uint32_t* y_pos) {
	const int n = 1<<zoom;

	const double lat_rad = latitude*2*M_PI/360;

	const double row_d = n*(1-(asinh(tan(lat_rad))/M_PI))/2;
	const int row = int(row_d);
	*y_pos = (row_d - row)*UINT32_MAX;

	return row;
}

//Get the column.
int getColumn(const double longitude, const int zoom, uint32_t* x_pos) {
	const int n = 1<<zoom;

	const double column_d = n*(longitude + 180)/360;
	const int column = int(column_d);
	*x_pos = (column_d - column)*UINT32_MAX;

	return column;
}

//Decompress a file. Thanks to https://windrealm.org/tutorials/decompress-gzip-stream.php
Bytes decompressGZ(Bytes* compressed) {
	const int full_length = compressed->l, half_length = compressed->l/2;
	int uncomp_length = full_length;

	z_stream zstrm;
	zstrm.next_in = (Bytef*) compressed->data;
	zstrm.avail_in = compressed->l;
	zstrm.total_out = 0;
	zstrm.zalloc = Z_NULL;
	zstrm.zfree = Z_NULL;

	if(inflateInit2(&zstrm, (16+MAX_WBITS)) != Z_OK)
		return Bytes(0);

	uint8_t* uncomp = new uint8_t[uncomp_length];

	bool done = false;
	while(!done) {
		if(zstrm.total_out >= uncomp_length) {
			uint8_t* uncomp2 = new uint8_t[uncomp_length + half_length];
			for(int i=0;i<uncomp_length;i+=1)
				uncomp2[i] = uncomp[i];
			
			uncomp_length += half_length;
			delete[] uncomp;
			uncomp = uncomp2;
		}

		zstrm.next_out = (Bytef*) (&uncomp[zstrm.total_out]);
		zstrm.avail_out = uncomp_length - zstrm.total_out;

		const int err = inflate(&zstrm, Z_SYNC_FLUSH);
		if(err == Z_STREAM_END)
			done = true;
		else if(err != Z_OK)
			break;
	}

	if(inflateEnd(&zstrm) != Z_OK) {
		delete[] uncomp;
		return Bytes(0);
	}

	Bytes the_return(zstrm.total_out, uncomp);
	delete[] uncomp;

	return the_return;
}

//Get roads from a tile.
std::vector<MapObjectRoad> getRoads(Bytes* tile_data) {
	if(tile_data->l <= 0)
		return std::vector<MapObjectRoad>(0);

	vector_tile::Tile tile_buffer;
	tile_buffer.ParseFromArray(tile_data->data, tile_data->l);

	std::vector<MapObjectRoad> roads(0);

	const int layer_count = tile_buffer.layers_size();

	for(int l=0;l<layer_count;l+=1) {
		const vector_tile::Tile_Layer layer = tile_buffer.layers(l);
		if(layer.name().compare("transportation") != 0 && layer.name().compare("transportation_name") != 0)
			continue;

		const int feature_count = layer.features_size();
		for(int f=0;f<feature_count;f+=1) {
			const vector_tile::Tile_Feature feature = layer.features(f);
			uint8_t road_class = 0;
			std::string road_name = "";

			bool is_road = false; //True if this road should be plotted.

			const int tag_limit = feature.tags_size()%2 == 0 ? feature.tags_size() : feature.tags_size() - 1;

			for(int p=0;p<tag_limit;p+=2) {
				const int k = feature.tags(p), v = feature.tags(p+1);

				if(k >= layer.keys_size() || v >= layer.values_size())
					continue;

				const string key = layer.keys(k);
				const vector_tile::Tile_Value value = layer.values(v);

				if(road_name.empty() && key.compare("ref") == 0 && value.has_string_value()) //Route name.
					road_name = value.string_value();
				else if(key.find("name") != std::string::npos && value.has_string_value()) //Road name.
					road_name = value.string_value();
				else if(key.compare("class") == 0 && value.has_string_value()) { //Road class.
					const std::string road_class_s = value.string_value();
					if(road_class_s.compare("motorway") == 0) {
						road_class = ROAD_CLASS_MOTORWAY;
						is_road = true;
					} else if(road_class_s.compare("primary") == 0) {
						road_class = ROAD_CLASS_PRIMARY;
						is_road = true;
					} else if(road_class_s.compare("secondary") == 0) {
						road_class = ROAD_CLASS_SECONDARY;
						is_road = true;
					} else
						road_class = ROAD_CLASS_MINOR;

					if(road_class_s.compare("minor") == 0)
						is_road = true;

				} else if(key.compare("network") == 0 && value.has_string_value()) {
					if(value.string_value().compare("road") == 0)
						is_road = true;
				}
			}

			if(is_road) {
				const int geometry_size = feature.geometry_size();
				uint32_t geometry[geometry_size];
				for(int g=0;g<geometry_size;g+=1)
					geometry[g] = feature.geometry(g);

				std::vector<GeometryCommand*> commands = getCommands(geometry, geometry_size);
				MapObjectRoad new_road(road_name, road_class, commands.size());

				for(int i=0;i<commands.size();i+=1)
					new_road.setCommand(commands.at(i), i);

				roads.emplace_back(new_road);
			}
		}
	}

	return roads;
}