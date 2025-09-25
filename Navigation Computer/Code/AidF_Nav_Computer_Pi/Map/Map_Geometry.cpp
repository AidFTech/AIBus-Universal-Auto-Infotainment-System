#include "Map_Geometry.h"

GeometryCommand::GeometryCommand(const uint8_t command) {
	this->command = command;
}

GeometryCommand::~GeometryCommand() {

}

GeometryCommand::GeometryCommand(const GeometryCommand& copy) {
	this->command = copy.command;
}

GeometryPoint::GeometryPoint(const int32_t x, const int32_t y) {
	this->x = x;
	this->y = y;
}

GeometryPoint::GeometryPoint(const GeometryPoint &copy) {
	this->x = copy.x;
	this->y = copy.y;
}

GeometryPoint GeometryPoint::operator=(const GeometryPoint &copy) {
	this->x = copy.x;
	this->y = copy.y;
	return *this;
}

GeometryPointCommand::GeometryPointCommand(const uint8_t command, const int l) : GeometryCommand(command) {
	this->point_count = l;
	this->points = new GeometryPoint[l];
}

GeometryPointCommand::~GeometryPointCommand() {
	delete[] this->points;
}

GeometryPointCommand::GeometryPointCommand(const GeometryPointCommand& copy) : GeometryCommand(copy) {
	this->point_count = copy.point_count;
	this->points = new GeometryPoint[copy.point_count];

	for(int i=0;i<copy.point_count;i+=1)
		this->points[i] = copy.points[i];
}

//Get point commands from a geometry list.
vector<GeometryCommand*> getCommands(uint32_t* geometry, const int l) {
	vector<GeometryCommand*> command_list(0);

	for(int i=0;i<l;i+=1) {
		const uint8_t command_id = geometry[i]&0x7;
		const uint32_t count = geometry[i]>>3;
		
		if(command_id == 1 || command_id == 2) { //Move or line. Point required.
			GeometryPointCommand new_command(command_id, count);
			const int start = i+1;
			for(int j=0;j<count*2;j+=1) {
				const uint32_t parameter = geometry[start+j];
				const int32_t value = (parameter>>1)*pow(-1, parameter&0x1);

				if(j%2 == 0)
					new_command.points[j/2].x = value;
				else
				 	new_command.points[j/2].y = value;
			}

			command_list.emplace_back(new GeometryPointCommand(new_command));
			i += count*2;
		} else { //No point.
			GeometryCommand new_command(command_id);
			command_list.emplace_back(new GeometryCommand(new_command));
		}
	}

	return command_list;
}