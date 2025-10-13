#include "Map_Objects.h"

MapObject::MapObject(const int command_l) {
	this->command_l = command_l;
	this->commands = new GeometryCommand*[command_l];
	
	for(int i=0;i<command_l;i+=1)
		this->commands[i] = nullptr;
}

MapObject::MapObject(const MapObject &copy) {
	this->command_l = copy.command_l;
	this->commands = new GeometryCommand*[command_l];

	for(int i=0;i<command_l;i+=1) {
		if(typeid(*copy.commands[i]) == typeid(GeometryPointCommand))
			commands[i] = new GeometryPointCommand(*(GeometryPointCommand*)copy.commands[i]);
		else
			commands[i] = new GeometryCommand(*copy.commands[i]);
	}
}

MapObject::~MapObject() {
	for(int i=0;i<command_l;i+=1)
		delete this->commands[i];
	
	delete[] this->commands;
}

MapObject MapObject::operator=(const MapObject &copy) {
	delete[] this->commands;
	this->command_l = copy.command_l;

	this->commands = new GeometryCommand*[this->command_l];

	for(int i=0;i<this->command_l;i+=1) {
		if(typeid(*copy.commands[i]) == typeid(GeometryPointCommand))
			commands[i] = new GeometryPointCommand(*(GeometryPointCommand*)copy.commands[i]);
		else
			commands[i] = new GeometryCommand(*copy.commands[i]);
	}

	return *this;
}

//Draw the object on the map.
void MapObject::drawOnMap(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Color color, const uint16_t thickness, const int32_t x_offset, const int32_t y_offset) {
	int cursor_x = x_offset, cursor_y = y_offset, cursor_x_start = cursor_x, cursor_y_start = cursor_y;

	SDL_Texture* last_texture = SDL_GetRenderTarget(renderer);
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	const bool closed_shape = getClosed(); //Determine whether the shape should be closed/filled.

	if(closed_shape) {
		vector<SDL_Vertex> points(0);

		for(int g=0;g<command_l;g+=1) {
			GeometryCommand* command = commands[g];
			if(command->command == 1 && typeid(*command) == typeid(GeometryPointCommand)) { //Move.
				GeometryPointCommand* point_command = (GeometryPointCommand*)(command);
				if(point_command->point_count > 0) {
					GeometryPoint* move_point = &point_command->points[0];
					cursor_x = x_offset + move_point->x;
					cursor_y = y_offset + move_point->y;
					cursor_x_start = cursor_x;
					cursor_y_start = cursor_y;

					SDL_Vertex point = {{float(cursor_x), float(cursor_y)}, {color.r, color.g, color.b, color.a}, {1,1}};
					points.push_back(point);
				}
			} else if(command->command == 2 && typeid(*command) == typeid(GeometryPointCommand)) { //Draw.
				GeometryPointCommand* point_command = (GeometryPointCommand*)(command);
				for(int p=0;p<point_command->point_count;p+=1) {
					GeometryPoint* move_point = &point_command->points[p];
					cursor_x += move_point->x;
					cursor_y += move_point->y;

					SDL_Vertex point = {{float(cursor_x), float(cursor_y)}, {color.r, color.g, color.b, color.a}, {1,1}};
					points.push_back(point);
				}
			} else if(command->command == 7) { //Close.
				SDL_Vertex points_array[points.size()];
				for(int i=0;i<points.size();i+=1)
					points_array[i] = points.at(i);

				SDL_RenderGeometry(renderer, texture, points_array, 3, NULL, 0);
				points.clear();
			}
		} 
	} else {
		for(int g=0;g<command_l;g+=1) {
			GeometryCommand* command = commands[g];
			if(command->command == 1 && typeid(*command) == typeid(GeometryPointCommand)) { //Move.
				GeometryPointCommand* point_command = (GeometryPointCommand*)(command);
				if(point_command->point_count > 0) {
					GeometryPoint* move_point = &point_command->points[0];
					cursor_x = x_offset + move_point->x;
					cursor_y = y_offset + move_point->y;
					cursor_x_start = cursor_x;
					cursor_y_start = cursor_y;

					thickLineRGBA(renderer, cursor_x, cursor_y, cursor_x, cursor_y, thickness*3/2, 255, 0, 0, 255);
				}
			} else if(command->command == 2 && typeid(*command) == typeid(GeometryPointCommand)) { //Draw.
				GeometryPointCommand* point_command = (GeometryPointCommand*)(command);
				for(int p=0;p<point_command->point_count;p+=1) {
					GeometryPoint* move_point = &point_command->points[p];
					const int new_x = cursor_x + move_point->x, new_y = cursor_y + move_point->y;
					
					thickLineRGBA(renderer, cursor_x, cursor_y, new_x, new_y, thickness, color.r, color.g, color.b, color.a);

					cursor_x += move_point->x;
					cursor_y += move_point->y;
				}
			}
		}
	}

	SDL_SetRenderTarget(renderer, last_texture);
}

//Set a geometry command at the specified index.
void MapObject::setCommand(GeometryCommand* command, const int index) {
	if(index >= this->command_l)
		return;
	
	this->commands[index] = command;
}

//Set a geometry command at the specified index.
void MapObject::setCommand(GeometryCommand command, const int index) {
	setCommand(new GeometryCommand(command), index);
}

//Set a geometry command at the specified index.
void MapObject::setCommand(GeometryPointCommand command, const int index) {
	setCommand(new GeometryPointCommand(command), index);
}

MapObjectRoad::MapObjectRoad(std::string road_name, const uint8_t road_class, const int command_l) : MapObject(command_l) {
	this->road_class = road_class;
	this->road_name = road_name;
}

MapObjectRoad::~MapObjectRoad() {
	delete this->road_name_render;
}

MapObjectRoad::MapObjectRoad(const MapObjectRoad &copy) : MapObject(copy) {
	this->road_class = copy.road_class;
	this->road_name = copy.road_name;

	if(copy.road_name_render != nullptr && copy.road_name_render != NULL)
		this->road_name_render = new AngledTextBox(*copy.road_name_render);
}

MapObjectRoad MapObjectRoad::operator=(const MapObjectRoad& copy) {
	delete[] this->commands;
	this->command_l = copy.command_l;

	this->commands = new GeometryCommand*[this->command_l];

	for(int i=0;i<this->command_l;i+=1) {
		if(typeid(*copy.commands[i]) == typeid(GeometryPointCommand))
			commands[i] = new GeometryPointCommand(*(GeometryPointCommand*)copy.commands[i]);
		else
			commands[i] = new GeometryCommand(*copy.commands[i]);
	}

	this->road_class = copy.road_class;
	this->road_name = copy.road_name;

	if(copy.road_name_render != nullptr && copy.road_name_render != NULL)
		this->road_name_render = new AngledTextBox(*copy.road_name_render);
	else
	 	this->road_name_render = nullptr;
	
	return *this;
}

//Draw the road on the map.
void MapObjectRoad::drawOnMap(SDL_Renderer* renderer, SDL_Texture* texture, const int32_t x_offset, const int32_t y_offset, const bool night) {
	uint32_t color = 0;
	uint16_t thickness = 3;
	if(!night) {
		switch(this->road_class) {
		case ROAD_CLASS_MINOR:
			color = ROAD_LEVEL0_DAY;
			break;
		case ROAD_CLASS_SECONDARY:
			color = ROAD_LEVEL1_DAY;
			thickness = 4;
			break;
		case ROAD_CLASS_PRIMARY:
			color = ROAD_LEVEL2_DAY;
			thickness = 5;
			break;
		case ROAD_CLASS_MOTORWAY:
			color = ROAD_LEVEL3_DAY;
			thickness = 6;
			break;
		}
	} else {
		switch(this->road_class) {
		case ROAD_CLASS_MINOR:
			color = ROAD_LEVEL0_NIGHT;
			break;
		case ROAD_CLASS_SECONDARY:
			color = ROAD_LEVEL1_NIGHT;
			break;
		case ROAD_CLASS_PRIMARY:
			color = ROAD_LEVEL2_NIGHT;
			break;
		case ROAD_CLASS_MOTORWAY:
			color = ROAD_LEVEL3_NIGHT;
			break;
		}
	}

	this->color = color;

	if(this->road_name.length() > 0) {
		//Calculate the midpoint.
		int x1, y1, x2, y2;
		this->getBounds(&x1, &y1, &x2, &y2);

		//TODO: Calculate this more accurately.
		const int text_thresh = TEXT_SIZE*this->road_name.length()/2;

		if(x2-x1 >= text_thresh || y2-y1 >= text_thresh) {
			int mid_x = 0, mid_y = 0;
			double angle = atan(1.0*(y2-y1)/(x2-x1));
			this->getMidpoint(&mid_x, &mid_y, &angle);

			if(this->road_name_render != NULL)
				delete this->road_name_render;

			this->road_name_render = new AngledTextBox(renderer, mid_x + x_offset, mid_y + y_offset, TEXT_SIZE*this->road_name.length(), TEXT_SIZE, ALIGN_H_L, ALIGN_V_M, TEXT_SIZE*6/7, angle, &this->color);
			this->road_name_render->setText(this->road_name);

			/*cout<<this->road_name<<": ";
			vector<GeometryPoint> points = getPoints();
			for(int i=0;i<points.size();i+=1)
				cout<<points.at(i).x<<','<<points.at(i).y<<' ';
			cout<<endl<<"MP: "<<mid_x<<','<<mid_y<<endl;*/
		}
	}

	MapObject::drawOnMap(renderer, texture, getSDLColor(color), thickness, x_offset, y_offset);
}

//Get the road name.
std::string MapObjectRoad::getName() {
	return this->road_name;
}

//Get the name text box.
AngledTextBox* MapObjectRoad::getNameTextBox() {
	return this->road_name_render;
}

//Return whether the object is a closed shape.
bool MapObject::getClosed() {
	for(int g=0;g<command_l;g+=1) {
		if(commands[g]->command == 0x7) {
			return true;
		}
	}
	return false;
}

//Get the object bounds.
void MapObject::getBounds(int* x1, int* y1, int* x2, int* y2) {
	if(command_l <= 0)
		return;

	bool init = false;
	vector<GeometryPoint> points = getPoints();
	for(int i=0;i<points.size();i+=1) {
		GeometryPoint pt = points.at(i);
		if(!init) {
			*x1 = pt.x;
			*x2 = pt.x;
			*y1 = pt.y;
			*y2 = pt.y;
			init = true;
		}

		if(pt.x < *x1)
			*x1 = pt.x;
		else if(pt.x > *x2)
			*x2 = pt.x;

		if(pt.y < *y1)
			*y1 = pt.y;
		else if(pt.y > *y2)
			*y2 = pt.y;
	}
}

//Get all points contained in the object. This function returns points with absolute dimensions, not relative.
vector<GeometryPoint> MapObject::getPoints() {
	int cursor_x = 0, cursor_y = 0;
	vector<GeometryPoint> points(0);
	for(int c=0;c<command_l;c+=1) {
		if(typeid(*commands[c]) == typeid(GeometryPointCommand)) {
			GeometryPointCommand* pt_cmd = (GeometryPointCommand*)commands[c];
			const bool rel = pt_cmd->command == 0x2;
			for(int p=0;p<pt_cmd->point_count;p+=1) {
				if(rel) {
					cursor_x += pt_cmd->points[p].x;
					cursor_y += pt_cmd->points[p].y;
				} else {
					cursor_x = pt_cmd->points[p].x;
					cursor_y = pt_cmd->points[p].y;
				}
				points.emplace_back(GeometryPoint(cursor_x, cursor_y));
			}
		}
	}

	return points;
}

//Get the midpoint of the object.
void MapObject::getMidpoint(int* x, int* y, double* angle) {
	if(getClosed()) {
		int x1, x2, y1, y2;
		getBounds(&x1, &y1, &x2, &y2);

		*x = (x1+x2)/2;
		*y = (y1+y2)/2;
		*angle = 0;
		return;
	} else {
		vector<GeometryPoint> points = getPoints();

		if(points.size() <= 0)
			return;

		if(points.size() == 1) {
			*x = points.at(0).x;
			*y = points.at(0).y;
			*angle = 0;
			return;
		}

		const double full_length = getPointLength(), half_length = full_length/2;
		double measured_length = 0;

		for(int p=0;p<points.size()-1;p+=1) {
			GeometryPoint* p1 = &points.at(p), *p2 = &points.at(p+1);
			const int dx = p2->x - p1->x, dy = p2->y - p1->y;
			const double newl = sqrt(dx*dx + dy*dy);

			measured_length += newl;

			if(measured_length >= half_length) { //Midpoint is between p1 and p2.
				const int start_x = p1->x, start_y = p1->y;
				const double start_l = measured_length - newl;

				//TODO: This is a placeholder.
				*x = (p2->x + p1->x)/2;
				*y = (p2->y + p1->y)/2;
				if(dx != 0)
					*angle = atan(1.0*dy/dx);
				else
					*angle = M_PI/2;
					
				break;
			}
		}
	}
}

//Get the full length of the object.
double MapObject::getPointLength() {
	double length = 0;
	vector<GeometryPoint> points = getPoints();

	if(points.size() <= 1)
		return 0;

	for(int i=0;i<points.size()-1;i+=1) {
		GeometryPoint* p1 = &points.at(i), *p2 = &points.at(i+1);
		const int dx = p2->x - p1->x, dy = p2->y - p1->y;

		length += sqrt(dx*dx + dy*dy);
	}

	return length;
}
