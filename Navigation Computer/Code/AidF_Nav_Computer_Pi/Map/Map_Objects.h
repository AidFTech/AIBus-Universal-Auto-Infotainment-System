#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <string>
#include <typeinfo>
#include <vector>
#include <cmath>

#include "Nav_Colors.h"
#include "Map_Geometry.h"

#include "../AidF_Color_Profile.h"
#include "../Text_Box.h"

using namespace std;

#ifndef map_objects_h
#define map_objects_h

#define ROAD_CLASS_MINOR 0
#define ROAD_CLASS_SECONDARY 1
#define ROAD_CLASS_PRIMARY 2
#define ROAD_CLASS_MOTORWAY 3

#define TEXT_SIZE 32

//Abstract map object.
class MapObject {
protected:
	MapObject(const int command_l);
	MapObject(const MapObject &copy);
	MapObject operator=(const MapObject &copy);

	virtual void drawOnMap(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Color color, const uint16_t thickness, const int32_t x_offset, const int32_t y_offset);
	virtual vector<GeometryPoint> getPoints();
	
	
	GeometryCommand** commands = nullptr;
	int command_l;
public:
	~MapObject();
	
	virtual void setCommand(GeometryPointCommand command, const int index);
	virtual void setCommand(GeometryCommand command, const int index);
	virtual void setCommand(GeometryCommand* command, const int index);

	virtual bool getClosed();
	
	virtual void getBounds(int* x1, int* y1, int* x2, int* y2);
	virtual void getMidpoint(int* x, int* y, double* angle);
};

//Road object.
class MapObjectRoad : public MapObject {
public:
	MapObjectRoad(std::string road_name = "", const uint8_t road_class = 0, const int command_l = 0);
	~MapObjectRoad();
	MapObjectRoad(const MapObjectRoad &copy);
	MapObjectRoad operator=(const MapObjectRoad& copy);
	
	void drawOnMap(SDL_Renderer* renderer, SDL_Texture* texture, const int32_t x_offset, const int32_t y_offset, const bool night);

	std::string getName();
	
private:
	std::string road_name = "";
	uint8_t road_class;

	uint32_t color;

	AngledTextBox* road_name_render = nullptr;
};

#endif