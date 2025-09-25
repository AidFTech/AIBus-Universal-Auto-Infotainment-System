#include <stdint.h>
#include <vector>
#include <cmath>

#ifndef map_geometry_h
#define map_geometry_h

#define GEO_COMMAND_MOVE 1
#define GEO_COMMAND_DRAW 2
#define GEO_COMMAND_CLOSE 7

using namespace std;

//Geometry command.
struct GeometryCommand {
	uint8_t command;

	GeometryCommand(const uint8_t command = 0);
	virtual ~GeometryCommand();
	GeometryCommand(const GeometryCommand& copy);
};

//Point.
struct GeometryPoint {
	int32_t x, y;

	GeometryPoint(const int32_t x = 0, const int32_t y = 0);
	GeometryPoint(const GeometryPoint &copy);

	GeometryPoint operator=(const GeometryPoint &copy);
};

//Point command.
struct GeometryPointCommand : public GeometryCommand {
	int point_count = 0;
	GeometryPoint* points = nullptr;

	GeometryPointCommand(const uint8_t command, const int l);
	~GeometryPointCommand();
	GeometryPointCommand(const GeometryPointCommand& copy);
};

vector<GeometryCommand*> getCommands(uint32_t* geometry, const int l);

#endif