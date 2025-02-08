#include "Nav_Window.h"

//Generic window constructor.
NavWindow::NavWindow(AttributeList *attribute_list) {
	this->attribute_list = attribute_list;

	this->color_profile = attribute_list->color_profile;
	this->renderer = attribute_list->renderer;

	this->w = attribute_list->w;
	this->h = attribute_list->h;
}

//Generic window destructor.
NavWindow::~NavWindow() {
}

//Generic draw function. Always inherited.
void NavWindow::drawWindow() {	
}

//Generic refresh function. Always inherited.
void NavWindow::refreshWindow() {
}

//Clear the current window.
void NavWindow::clearWindow() {
	if(this->renderer != NULL)
		this->attribute_list->br->drawBackground(this->renderer, 0,0,this->w, this->h);
}

//Function to be called when the window is closed.
void NavWindow::exitWindow() {
}

//Generic AIBus-handling message.
bool NavWindow::handleAIBus(AIData* msg) {
	return false;
}

//Set whether the window is active.
void NavWindow::setActive(bool active) {
	this->active = active;
}

//Get whether the window is active.
bool NavWindow::getActive() {
	return this->active;
}
