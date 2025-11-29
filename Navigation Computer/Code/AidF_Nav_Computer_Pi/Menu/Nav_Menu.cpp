#include "Nav_Menu.h"

NavMenu::NavMenu(AttributeList* attribute_list,
							const int16_t x,
							const int16_t y,
							const uint16_t text_w,
							const uint16_t text_h,
							const uint16_t newl,
							const int8_t indent,
							const uint8_t size,
							const uint8_t rows,
							const bool loop,
							std::string title) :
							title_box(attribute_list->renderer, x, y, text_w, text_h*7/6, ALIGN_H_L, ALIGN_V_M, text_h, &attribute_list->color_profile->text) {
	this->attribute_list = attribute_list;
	this->renderer = attribute_list->renderer;
	
	this->length = newl;
	this->items = new std::string[newl];

	this->color_profile = attribute_list->color_profile;

	this->x = x;
	this->y = y;
	this->text_h = text_h;
	this->text_w = text_w;
	this->w = attribute_list->w;
	this->h = attribute_list->h;

	this->indent = indent;

	if(title.compare("") == 0)
		this->title = " ";
	else
		this->title = title;

	if(rows > 0)
		this->rows = rows;
	else
		this->rows = 1;
	this->loop = loop;

	this->title_box.setText(this->title);

	for(uint16_t i=0;i<this->length;i+=1) {
		int16_t x_pos = x, y_pos = y;
		align_h_t h_indent;
		
		getIndexPosition(i, &x_pos, &y_pos);
		if(indent < 0) {
			if((this->length-1)/rows < 1)
				h_indent = ALIGN_H_L;
			else if(i/rows == 0)
				h_indent = ALIGN_H_L;
			else if(i/rows > 0 && i/rows < (this->length-1)/rows)
				h_indent = ALIGN_H_C;
			else if(i/rows >= (this->length-1)/rows)
				h_indent = ALIGN_H_R;
			else
				h_indent = ALIGN_H_C;
		} else h_indent = (align_h_t)indent;

		this->item_text_box.push_back(TextBox(this->renderer, x_pos, y_pos, text_w-SQ_WIDTH, text_h, h_indent, ALIGN_V_M, size, &this->attribute_list->color_profile->text));
		this->item_slider.push_back(NULL);
		this->item_slider_text.push_back(NULL);
	}
}

NavMenu::~NavMenu() {
	delete[] this->items;
	
	while(this->item_slider.size() > 0) {
		delete this->item_slider.back();
		this->item_slider.erase(this->item_slider.end());
	}

	while(this->item_slider_text.size() > 0) {
		delete this->item_slider_text.back();
		this->item_slider_text.erase(this->item_slider_text.end());
	}
}

void NavMenu::setItem(std::string text, const uint16_t item) {
	if(item >= this->length)
		return;

	if(text.compare("") == 0)
		text = " ";

	this->items[item] = text;
	this->item_text_changed = true;
	text_change_time = clock();
}

//Refresh all listings on the menu.
void NavMenu::setTextItems() {
	for(uint16_t i=0;i<this->length;i+=1) {
		this->item_text_box[i].setText(this->items[i]);
	}
}

//Set a slider at index.
void NavMenu::setSlider(std::string text, const uint8_t max, const uint8_t val, const uint16_t item) {
	int16_t slider_x, slider_y;
	this->getIndexPosition(item, &slider_x, &slider_y);
	const int16_t text_y = slider_y;
	
	slider_x = this->x;
	slider_x += this->w/2;
	slider_y += this->text_h/2 - this->text_h*4/7/2;

	this->item_slider[item] = new NavSlider(this->attribute_list, slider_x, slider_y, this->w/3, this->text_h*4/7, max, val, false);
	this->item_slider_text[item] = new TextBox(this->renderer, slider_x + this->w/3 + 5, text_y, this->w/3 - 5, this->text_h, ALIGN_H_L, ALIGN_V_M, this->text_h*5/8, &this->attribute_list->color_profile->text);

	this->item_slider_text[item]->setText(text);
}

//Get slider at index or null.
NavSlider* NavMenu::getSlider(const uint16_t index) {
	if(index < this->item_slider.size())
		return this->item_slider[index];
	else
		return NULL;
}

//Get the slider text box at index.
TextBox* NavMenu::getSliderTextBox(const uint16_t index) {
	if(index < this->item_slider_text.size())
		return this->item_slider_text[index];
	else
		return NULL;
}

//Set the selected index.
void NavMenu::setSelected(const uint16_t selected) {
	this->selected = selected;
}

//Get the selected index.
uint16_t NavMenu::getSelected() {
	return this->selected;
}

//Get the selected item string.
std::string NavMenu::getSelectedString() {
	if(selected > 0 && selected <= this->length)
		return this->items[selected - 1];
	else
		return "";
}

uint16_t NavMenu::getLength() {
	return this->length;
}

uint16_t NavMenu::getFilledTextItems() {
	uint16_t filled_len = 0;

	for(uint16_t i=1;i<=this->length;i+=1) {
		if(items[i - 1].compare("") != 0 && items[i - 1].compare(" ") != 0)
			filled_len += 1;
	}

	return filled_len;
}

void NavMenu::setTitle(std::string title) {
	if(title.compare("") == 0)
		this->title = " ";
	else
		this->title = title;

	this->title_box.setText(this->title);
}

int16_t NavMenu::getX() {
	return this->x;
}

int16_t NavMenu::getY() {
	return this->y;
}

//Increment the selected option.
void NavMenu::incrementSelected() {
	this->incrementSelected(true);
}

//Decrement the selected option.
void NavMenu::decrementSelected() {
	this->decrementSelected(true);
}

//Increment the selected option.
void NavMenu::incrementSelected(const bool two_row_support) {
	if(this->rows == this->length || this->rows != 2 || this->loop || !two_row_support) {
		uint16_t new_selected = this->selected + 1;

		if(new_selected > this->length)
			new_selected = 1;
		
		bool loop = false;
		while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected <= this->length) {
			new_selected += 1;
			if(new_selected > this->length && !loop) {
				new_selected = 1;
				loop = true;
			} else if(new_selected > this->length)
				return;
		}

		if(new_selected > this->length)
			new_selected = 1;

		this->setSelected(new_selected);
	} else {
		int new_selected = this->selected;
		if(this->selected%2 == 1)
			new_selected = this->selected - 2;
		else
			new_selected = this->selected + 2;

		if(new_selected <= 0)
			new_selected = 2;
		else if(new_selected > this->length)
			new_selected = this->length - 1;

		bool loop = false;

		while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected <= this->length) {
			if(new_selected%2 == 1)
				new_selected -= 2;
			else
				new_selected += 2;
			
			if(new_selected <= 0) {
				new_selected = 2;
			} else if(new_selected > this->length) {
				new_selected = this->length - 1;
				loop = true;
			}

			if(loop && new_selected == selected)
				return;
		}

		this->setSelected(uint16_t(new_selected));
	}
}

//Decrement the selected option.
void NavMenu::decrementSelected(const bool two_row_support) {
	if(this->rows == this->length || this->rows != 2 || this->loop || !two_row_support) {
		uint16_t new_selected = this->selected - 1;

		if(new_selected < 1 || this->selected < 1)
			new_selected = this->length;
		
		bool loop = false;
		while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected >= 1) {
			new_selected -= 1;
			if(new_selected < 1 && !loop) {
				new_selected = this->length;
				loop = true;
			} else if(new_selected < 1)
				return;
		}

		this->setSelected(new_selected);
	} else {
		int new_selected = this->selected;
		if(this->selected%2 == 1)
			new_selected = this->selected + 2;
		else
			new_selected = this->selected - 2;

		if(new_selected <= 0)
			new_selected = 1;
		else if(new_selected > this->length)
			new_selected = this->length;

		bool loop = false;

		while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected <= this->length) {
			if(new_selected%2 == 1)
				new_selected += 2;
			else
				new_selected -= 2;
			
			if(new_selected <= 0) {
				new_selected = 1;
				loop = true;
			} else if(new_selected > this->length) {
				new_selected = this->length;
			}

			if(loop && new_selected == selected)
				return;
		}

		this->setSelected(uint16_t(new_selected));
	}
}

void NavMenu::navigateSelected(const int8_t direction) {
	if(this->selected <= 0) {
		if(direction == NAV_DOWN || direction == NAV_RIGHT)
			this->setSelected(1);
		else if(direction == NAV_UP)
			this->setSelected(this->rows);
		else if(direction == NAV_LEFT)
			this->setSelected(this->length/this->rows);
		
		return;
	}
	
	const uint16_t last_selected = this->selected;

	if(this->rows == this->length && (direction == NAV_LEFT || direction == NAV_RIGHT))
		return;
	
	if(direction == NAV_DOWN || direction == NAV_UP) {
		if(((!this->loop || ((last_selected-1)/this->rows)%2 != 1) && direction == NAV_DOWN) 
			|| ((this->loop && ((last_selected-1)/this->rows)%2 == 1) && direction == NAV_UP)){ //Increment.
			if((last_selected - 1)%this->rows < this->rows - 1) {
				incrementSelected(false);
				return;
			} else {
				uint16_t new_selected = last_selected - rows + 1;
				
				while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected < last_selected) 
					new_selected += 1;
				
				setSelected(new_selected);
			}
		} else { //Decrement.
			if((last_selected - 1)%this->rows > 0) {
				decrementSelected(false);
				return;
			} else {
				uint16_t new_selected = last_selected + rows - 1;
				while((items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) && new_selected > last_selected) 
					new_selected -= 1;
					
				setSelected(new_selected);
			}
		}
	} else if(direction == NAV_LEFT) {
		int new_selected = last_selected - rows;
		if(last_selected <= rows) {
			new_selected = rows*((this->length - 1)/rows) + (last_selected-1)%rows + 1;
			if(loop && ((last_selected-1)/this->rows)%2 != ((new_selected-1)/this->rows)%2) {
				new_selected = rows*((this->length-1)/rows+1) - ((new_selected-1)%rows);
			}
		} else {
			if(loop) {
				new_selected = rows*((last_selected-1)/rows) - ((new_selected-1)%rows);
			}
		}

		if(new_selected <= 0 || new_selected > this->length)
			new_selected = 1;

		if(items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) {
			uint16_t up_selected = new_selected, down_selected = new_selected;
			while((up_selected) > 1 && (items[up_selected - 1].compare("") == 0 || items[up_selected - 1].compare(" ") == 0)) {
				up_selected -= 1;
				if(up_selected == last_selected && up_selected > 1)
					up_selected -= 1;
			}

			while((down_selected) < this->length/*rows*((new_selected-1)/rows + 1)*/ && (items[down_selected - 1].compare("") == 0 || items[down_selected - 1].compare(" ") == 0)) {
				down_selected += 1;
				if(down_selected == last_selected && down_selected < this->length)
					down_selected += 1;
			}

			if(items[down_selected - 1].compare("") != 0 && items[down_selected - 1].compare(" ") != 0 && items[up_selected - 1].compare("") != 0 && items[up_selected-1].compare(" ") != 0) {
				if(abs(new_selected - up_selected) <= abs(new_selected - down_selected))
					new_selected = up_selected;
				else
					new_selected = down_selected;
			} else if(items[down_selected - 1].compare("") != 0 && items[down_selected - 1].compare(" ") != 0) { //down_selected is populated.
				uint16_t test_selected = this->length - (rows - (last_selected-1)%rows);
				while(test_selected > down_selected && (items[test_selected - 1].compare("") == 0 || items[test_selected - 1].compare(" ") == 0))
					test_selected -= 1;
				
				new_selected = test_selected;
			} else if(items[up_selected - 1].compare("") != 0 && items[up_selected-1].compare(" ") != 0) { //up_selected is populated.
				new_selected = up_selected;
			} else { //Last selected is populated.
				new_selected = last_selected;
			}
		}
		
		setSelected(new_selected);
	} else if(direction == NAV_RIGHT) { 
		int new_selected = last_selected + rows;
		if(last_selected > rows*((this->length-1)/rows)) {
			new_selected = ((last_selected-1)%rows + 1);
			if(loop && ((last_selected-1)/this->rows)%2 != ((new_selected-1)/this->rows)%2) {
				new_selected = rows - ((new_selected-1)%rows);
			}
		} else {
			if(loop) {
				new_selected = rows*((last_selected-1)/rows + 2) - ((new_selected-1)%rows);
			}
		}

		if(items[new_selected - 1].compare("") == 0 || items[new_selected - 1].compare(" ") == 0) {
			uint16_t up_selected = new_selected, down_selected = new_selected;
			while((up_selected) > 1 /*rows*((new_selected-1)/rows)*/ && (items[up_selected - 1].compare("") == 0 || items[up_selected - 1].compare(" ") == 0)) {
				up_selected -= 1;
				if(up_selected == last_selected && up_selected > 1)
					up_selected -= 1;
			}

			while((down_selected) < this->length && (items[down_selected - 1].compare("") == 0 || items[down_selected - 1].compare(" ") == 0)) {
				down_selected += 1;
				if(down_selected == last_selected && down_selected < this->length)
					down_selected += 1;
			}

			if(items[down_selected - 1].compare("") != 0 && items[down_selected - 1].compare(" ") != 0 && items[up_selected - 1].compare("") != 0 && items[up_selected-1].compare(" ") != 0) {
				if(abs(new_selected - up_selected) < abs(new_selected - down_selected))
					new_selected = up_selected;
				else
					new_selected = down_selected;
			} else if(items[down_selected - 1].compare("") != 0 && items[down_selected - 1].compare(" ") != 0) { //down_selected is populated.
				new_selected = down_selected;
			} else if(items[up_selected - 1].compare("") != 0 && items[up_selected-1].compare(" ") != 0) { //up_selected is populated.
				uint16_t test_selected = 1 + ((last_selected-1)%rows);
				while(test_selected < up_selected && (items[test_selected - 1].compare("") == 0 || items[test_selected - 1].compare(" ") == 0))
					test_selected += 1;
				
				new_selected = test_selected;
			} else { //Last selected is populated.
				new_selected = last_selected;
			}
		}
		
		setSelected(new_selected);
	}
}

//Handle menu navigation messages.
bool NavMenu::handleAIBus(AIData *ai_d) {
	if(ai_d->sender != ID_NAV_SCREEN)
		return false;

	if(ai_d->l >= 3 && ai_d->data[0] == 0x32) { //Knob turn.
		const bool clockwise = (ai_d->data[2]&0x10) != 0;
		const uint8_t steps = ai_d->data[2]&0xF;

		if(!clockwise) {
			for(uint8_t i=0;i<steps;i+=1)
				this->incrementSelected();
		} else {
			for(uint8_t i=0;i<steps;i+=1)
				this->decrementSelected();
		}

		return true;
	} else if(ai_d->l >= 3 && ai_d->data[0] == 0x30) { //Button press.
		if(ai_d->data[1] >= 0x28 && ai_d->data[1] <= 0x2B && (ai_d->data[2] >> 6) == 0x2) {
			int8_t control = -1;
			switch(ai_d->data[1]) {
				case 0x28:
					control = NAV_UP;
					break;
				case 0x29:
					control = NAV_DOWN;
					break;
				case 0x2A:
					control = NAV_LEFT;
					break;
				case 0x2B:
					control = NAV_RIGHT;
					break;
				default:
					return false;
			}

			this->navigateSelected(control);

			return true;
		}
	}
	return false;
}

void NavMenu::drawMenu() {
	//First figure out if item text was changed.
	if(item_text_changed && (clock()-text_change_time)/(CLOCKS_PER_SEC/1000000) > 1000) {
		item_text_changed = false;
		setTextItems();
	}

	this->title_box.drawText();

	uint8_t title_height = 0;
	if(this->title.compare("") != 0 && this->title.compare(" ") != 0)
		title_height = text_h*7/5;

	const uint16_t row_fit_count = ((this->h-this->y-title_height)/this->text_h)*(this->length/this->rows);

	for(uint16_t i=0;i<this->item_text_box.size();i+=1) {
		if((this->selected - 1)/row_fit_count != i/row_fit_count)
			continue;

		this->item_text_box[i].drawText();

		if(i < this->item_slider.size() && this->item_slider[i] != NULL)
			this->item_slider[i]->drawSlider();

		if(i < this->item_slider_text.size() && this->item_slider_text[i] != NULL)
			this->item_slider_text[i]->drawText();
	}

	uint8_t last_r, last_g, last_b, last_a;
	const uint32_t button_color = this->color_profile->button, outline_color = this->color_profile->outline;
	SDL_GetRenderDrawColor(renderer, &last_r, &last_g, &last_b, &last_a);

	for(uint16_t i=0;i<this->item_text_box.size();i+=1) {
		if(this->items[i].compare("") == 0 || this->items[i].compare(" ") == 0)
			continue;

		if((this->selected - 1)/row_fit_count != i/row_fit_count)
			continue;
		
		const bool right_square = ((this->indent < 0 && ((i/rows)%2 == 1)) || this->indent == ALIGN_H_R);
		int16_t x_pos, y_pos;
		getIndexPosition(i, &x_pos, &y_pos);

		if(!right_square)
			x_pos -= SQ_WIDTH;
		else
			x_pos += (text_w - SQ_WIDTH);

		SDL_Rect rect = {x_pos, y_pos, SQ_WIDTH, text_h};
		SDL_SetRenderDrawColor(renderer, getRedComponent(button_color), getGreenComponent(button_color), getBlueComponent(button_color), getAlphaComponent(button_color));
		SDL_RenderFillRect(renderer, &rect);
		SDL_SetRenderDrawColor(renderer, getRedComponent(outline_color), getGreenComponent(outline_color), getBlueComponent(outline_color), getAlphaComponent(outline_color));
		SDL_RenderDrawRect(renderer, &rect);

		if(selected-1 == i) {
			const uint32_t selection_color = this->color_profile->selection;
			SDL_SetRenderDrawColor(renderer, getRedComponent(selection_color), getGreenComponent(selection_color), getBlueComponent(selection_color), getAlphaComponent(selection_color));

			SDL_RenderFillRect(renderer, &rect);
			getIndexPosition(i, &x_pos, &y_pos);
			if(!right_square)
				x_pos -= SQ_WIDTH;

			uint16_t outline_w = text_w;
			if(item_slider[i] != NULL)
				outline_w = outline_w/2 - 10;
			
			for(uint8_t s=0;s<OUTLINE_THICKNESS;s+=1) {
				SDL_Rect outline_rect = {x_pos+s, y_pos+s, outline_w-2*s, text_h-2*s};
				SDL_RenderDrawRect(renderer, &outline_rect);
			}
		}
	}

	SDL_SetRenderDrawColor(renderer, last_r, last_g, last_b, last_a);
}

void NavMenu::refreshItems() {
	title_box.renderText();

	for(uint8_t i=0;i<this->item_text_box.size();i+=1)
		this->item_text_box[i].renderText();
}

//Get the position of the selected index.
void NavMenu::getSelectedIndexPosition(int16_t* x_pos, int16_t* y_pos) {
	if(selected <= 0)
		return;

	getIndexPosition(selected - 1, x_pos, y_pos);
}

//Get the position of the index.
void NavMenu::getIndexPosition(uint16_t index, int16_t* x_pos, int16_t* y_pos) {
	uint8_t title_height = 0;
	if(this->title.compare("") != 0 && this->title.compare(" ") != 0)
		title_height = text_h*7/5;

	const uint16_t row_fit_count = ((this->h-this->y - title_height)/this->text_h)*(this->length/this->rows);
	index = index%row_fit_count;
	
	*x_pos = x + (index/rows)*text_w;
	if(loop && (index/rows)%2 == 1) {
		if(index/rows >= this->length/rows-1)
			*y_pos = y + ((this->length-1-index)%rows)*text_h;
		else
			*y_pos = y + (rows-1-(index%rows))*text_h;
	}
	else
		*y_pos = y + (index%rows)*text_h;

	if(!((this->indent < 0 && ((index/rows)%2 == 1)) || this->indent == ALIGN_H_R))
		*x_pos += SQ_WIDTH;

	if(this->title.compare("") != 0 && this->title.compare(" ") != 0)
		*y_pos += text_h*7/5;
}
