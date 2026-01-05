#include "Nav_EDID_Check.h"

int main() {
	bool run = true;
	ElapsedMillis elapsed_millis;
	elapsed_millis.run = &run;

	pthread_t timer_thread;
	pthread_create(&timer_thread, NULL, millisThread, (void*)&elapsed_millis);

	string edid_path = "", res_path = "";
	{
		vector<IniList> path_file = loadIniFile(FILE_PATH);
		for(int i=0;i<path_file.size();i+=1) {
			IniList list = path_file[i];
			if(list.title.compare("AidF_Display_File_Paths") != 0)
				continue;

			for(int s=0;s<list.l_s;s+=1) {
				if(list.str_vars[s].compare("EDIDPath") == 0)
					edid_path = list.str_values[s];
				else if(list.str_vars[s].compare("ResPath") == 0)
					res_path = list.str_values[s];
			}
		}
	}

	if(edid_path.length() <= 0 || res_path.length() <= 0)
		return 1;

	#ifdef RPI_UART
	SerialAIBusHandler ai_handler("/dev/ttyS0", ID_COMPUTER_PROXY, &elapsed_millis.time);
	#else
	SerialAIBusHandler ai_handler(ID_COMPUTER_PROXY, &elapsed_millis.time);
	#endif

	uint8_t poweroff_data[] = {0xA0};
	AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_COMPUTER, 0xFF, poweroff_data);
	ai_handler.writeAIData(&poweroff_msg, false);

	bool received_edid = false;
	unsigned long start_time = elapsed_millis.time, last_send = elapsed_millis.time + 100;

	bool edid_match = true; //True if the EDID data matches.

	vector<uint8_t> edid_bin(0);

	while(!received_edid && elapsed_millis.time - start_time < 750) {
		if(elapsed_millis.time - last_send > 100) {
			last_send = elapsed_millis.time;

			uint8_t edid_request_data[] = {0x3E, 0xF0};
			AIData edid_request_msg(sizeof(edid_request_data), ID_COMPUTER_PROXY, ID_NAV_SCREEN, edid_request_data);
			ai_handler.writeAIData(&edid_request_msg, false);
		}

		AIData rec;
		if(ai_handler.readAIData(&rec)) {
			if(rec.sender == ID_COMPUTER_PROXY)
				continue;

			if(rec.receiver == ID_COMPUTER_PROXY && rec.l > 0 && rec[0] != 0x80)
				ai_handler.sendAcknowledgement(ID_COMPUTER_PROXY, rec.sender);
			else
				continue;

			if(rec.sender == ID_NAV_SCREEN && rec.l >= 1 && rec.data[0] == 0x3E) { //EDID message.
				for(int i=1;i<rec.l;i+=1)
					edid_bin.push_back(rec[i]);

				uint8_t edid_size_data[] = {0xA1, 0x3E, uint8_t(edid_bin.size()&0xFF)};
				AIData edid_size_msg(sizeof(edid_size_data), ID_COMPUTER_PROXY, 0xFF, edid_size_data);
				//ai_handler.writeAIData(&edid_size_msg, false);

				if(edid_bin.size() < 128) {
					edid_bin.clear();
					continue;
				}

				long chx = 0;
				for(int i=0;i<edid_bin.size()-1;i+=1)
					chx += edid_bin[i];
				chx = (~(chx&0xFF) + 1)&0xFF;

				if(chx != edid_bin[edid_bin.size()-1]) {
					edid_bin.clear();
					continue;
				}

				received_edid = true;
			}
		}

		if(!received_edid)
			continue;

		//Compare the EDID with the saved file.
		ifstream edid_file(edid_path, ios::in | ios::binary);

		if(edid_file.fail()) {
			//edid_match = false;
			goto final_edid;
		} else {
			vector<char> edid_bin_from_file(istreambuf_iterator<char>(edid_file), {});

			if(edid_bin.size() != edid_bin_from_file.size()) {
				edid_match = false;
				goto final_edid;
			}

			for(int i=0;i<edid_bin_from_file.size() && i<edid_bin.size(); i+=1) {
				if(edid_bin[i] != uint8_t(edid_bin_from_file[i])) {
					edid_match = false;
					break;
				}
			}
		}

		final_edid: {
			if(!edid_match) { //Save the file and reboot.
				ofstream edid_write(edid_path, ios::out | ios::binary);
				edid_write.write((const char*)edid_bin.data(), edid_bin.size());
				edid_write.close();
			}
		}
	}

	bool received_res = false, res_match = true;
	uint16_t w = 800, h = 480;

	{
		vector<IniList> res_file = loadIniFile(res_path.c_str());
		for(int i=0;i<res_file.size();i+=1) {
			IniList list = res_file[i];
			if(list.title.compare("AidF_Navigation_Screen_Dimensions") != 0)
				continue;

			for(int n=0;n<list.l_n;n+=1) {
				if(list.num_vars[n].compare("w") == 0)
					w = list.num_values[n];
				else if(list.num_vars[n].compare("h") == 0)
					h = list.num_values[n];
			}
		}
	}

	start_time = elapsed_millis.time;
	last_send = elapsed_millis.time + 100;

	while(!received_res && elapsed_millis.time - start_time < 750) {
		if(elapsed_millis.time - last_send > 100) {
			last_send = elapsed_millis.time;

			uint8_t res_request_data[] = {0x2C, 0xF0};
			AIData res_request_msg(sizeof(res_request_data), ID_COMPUTER_PROXY, ID_NAV_SCREEN, res_request_data);
			ai_handler.writeAIData(&res_request_msg, false);
		}

		uint16_t new_w = w, new_h = h;

		AIData rec;
		if(ai_handler.readAIData(&rec)) {
			if(rec.sender == ID_COMPUTER_PROXY)
				continue;

			if(rec.receiver == ID_COMPUTER_PROXY && rec.l > 0 && rec[0] != 0x80)
				ai_handler.sendAcknowledgement(ID_COMPUTER_PROXY, rec.sender);
			else
				continue;

			if(rec.sender == ID_NAV_SCREEN && rec.l >= 5 && rec.data[0] == 0x2C) { //Resolution message.
				received_res = true;

				new_w = (rec[1]<<8) | rec[2];
				new_h = (rec[3]<<8) | rec[4];
			}
		}

		if(!received_res)
			continue;

		if(new_w != w || new_h != h) {
			res_match = false;
			IniList res_file(2, 0);
			res_file.title = "AidF_Navigation_Screen_Dimensions";
			res_file.num_values[0] = new_w;
			res_file.num_values[1] = new_h;
			res_file.num_vars[0] = "w";
			res_file.num_vars[1] = "h";

			saveIniFile(res_path.c_str(), vector<IniList>({res_file}));
		}
	}

	if(!edid_match) {
		uint8_t poweroff_data[] = {0xA0};
		AIData poweroff_msg(sizeof(poweroff_data), ID_NAV_COMPUTER, 0xFF, poweroff_data);
		ai_handler.writeAIData(&poweroff_msg, false);
	}

	run = false;
	pthread_cancel(timer_thread);

	#ifdef RPI_UART
	if(!edid_match)
		system("reboot");
	#endif
}

//Timer thread function.
void *millisThread(void* millis_v) {
	ElapsedMillis* elapsed_millis = (ElapsedMillis*)millis_v;

	while(*elapsed_millis->run) {
		usleep(1000);

		elapsed_millis->time += 1;

		if(!*elapsed_millis->run)
			break;
	}

	void* result;
	return result;
}