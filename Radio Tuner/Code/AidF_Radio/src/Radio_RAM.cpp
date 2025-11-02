#include "Radio_RAM.h"

SRAMHandler::SRAMHandler(const uint8_t ram_cs) :
	sram(&SPI, ram_cs, SRAM_23K640) {
}

//Start the RAM handler.
void SRAMHandler::begin() {
	sram.begin();
}

//Write the model number to RAM.
void SRAMHandler::writeHeader() {
	sram.writeBlock(ADDR_AIDF_HEADER, sizeof(AIDF_RAM_HEADER), (void*)AIDF_RAM_HEADER);
}

//Return whether data saved in RAM is valid, i.e. the addresses start with the model number.
bool SRAMHandler::getValid() {
	for(int i=ADDR_AIDF_HEADER;i<ADDR_AIDF_HEADER + sizeof(AIDF_RAM_HEADER);i+=1) {
		if(char(sram.readByte(i)) != AIDF_RAM_HEADER[i])
			return false;
	}

	return true;
}

//Clear the RAM.
void SRAMHandler::clearRAM() {
	for(int i=0;i<0x2000;i+=1) {
		sram.writeByte(i, 0);
	}
}

//Save startup parameters to RAM.
void SRAMHandler::setStartParams(StartParams* start_params) {
	sram.writeByte(SELECTED_SOURCE, start_params->selected_source);
	sram.writeByte(SELECTED_SUBSOURCE, start_params->selected_subsource);
	sram.writeByte(AUDIO_ON, start_params->audio_on ? 1 : 0);
	writeUint16(FM1_FREQ, start_params->fm1_freq);
	writeUint16(FM2_FREQ, start_params->fm2_freq);
	writeUint16(AM_FREQ, start_params->am_freq);
	writeInt16(CLOCK_FREQ, start_params->clock_freq);

	writeUint16(VOL, start_params->vol);
	writeUint16(MAX_VOL, start_params->max_vol);
	writeUint16(TREBLE, start_params->treble);
	writeUint16(BASS, start_params->bass);
	writeInt16(BALANCE, start_params->balance);
	writeInt16(FADER, start_params->fader);
}

//Load startup parameters from RAM.
void SRAMHandler::getStartParams(StartParams* start_params) {
	start_params->selected_source = sram.readByte(SELECTED_SOURCE);
	start_params->selected_subsource = sram.readByte(SELECTED_SUBSOURCE);
	start_params->fm1_freq = readUint16(FM1_FREQ);
	start_params->fm2_freq = readUint16(FM2_FREQ);
	start_params->am_freq = readUint16(AM_FREQ);
	start_params->clock_freq = readInt16(CLOCK_FREQ);

	start_params->audio_on = sram.readByte(AUDIO_ON) != 0;
	
	start_params->vol = readUint16(VOL);
	start_params->max_vol = readUint16(MAX_VOL);
	start_params->treble = readUint16(TREBLE);
	start_params->bass = readUint16(BASS);
	start_params->balance = readInt16(BALANCE);
	start_params->fader = readInt16(FADER);
}

//Get the number of sources stored in RAM.
uint8_t SRAMHandler::getSourceCount() {
	if(!getValid())
		return 0;

	return sram.readByte(SOURCE_COUNT);
}

//Save the sources to RAM.
void SRAMHandler::setSources(const uint16_t l, AudioSource* source_list) {
	sram.writeByte(SOURCE_COUNT, l);

	int head = SOURCE_START;
	for(int i=0;i<l;i+=1) {
		sram.writeByte(head, source_list[i].source_id);
		sram.writeByte(head + SOURCE_SUB_ID, source_list[i].sub_id);

		head += SOURCE_SUB_ID + 1;

		if(source_list[i].source_name.length() > 0) {
			const int name_len = source_list[i].source_name.length() + 1;
			const char* source_name = source_list[i].source_name.c_str();
			sram.writeBlock(head, name_len, (void*)source_name);
			head += name_len;
		} else {
			sram.writeByte(head, 0);
			head += 1;
		}

		if(source_list[i].source_short.length() > 0) {
			const int short_len = source_list[i].source_short.length() + 1;
			const char* short_name = source_list[i].source_short.c_str();
			sram.writeBlock(head, short_len, (void*)short_name);
			head += short_len;
		} else {
			sram.writeByte(head, 0);
			head += 1;
		}
	}
}

//Get the sources stored in RAM.
void SRAMHandler::getSources(const uint16_t l, AudioSource* source_list) {
	if(getSourceCount() != l || !getValid())
		return;

	int head = SOURCE_START, index = 0;
	while(index < l) {
		source_list[index].source_id = sram.readByte(head);
		source_list[index].sub_id = sram.readByte(head + SOURCE_SUB_ID);

		head += SOURCE_SUB_ID + 1;
		String source_name = "";
		uint8_t ptr = sram.readByte(head);
		while(ptr != 0) {
			source_name += char(ptr);
			head += 1;
			ptr = sram.readByte(head);
		}
		head += 1;
		
		ptr = sram.readByte(head);
		String short_name = "";
		while(ptr != 0) {
			short_name += char(ptr);
			head += 1;
			ptr = sram.readByte(head);
		}
		head += 1;

		source_list[index].source_name = source_name;
		source_list[index].source_short = short_name;

		index += 1;
	}
}

//Save tuner frequencies to RAM.
void SRAMHandler::setFrequencies(BackgroundTuneHandler* tuner) {
	String names[MAXIMUM_FREQUENCY_COUNT];
	const int l = tuner->getRawStationNames(names);

	int head = FM_STATION_START;
	
	for(int i=0;i<l && i < MAXIMUM_FREQUENCY_COUNT;i+=1) {
		const uint16_t freq = tuner->getStationFrequency(i);
		writeUint16(head, freq);
		head += sizeof(uint16_t);

		if(names[i].length() > 0) {
			const int name_l = names[i].length() + 1;
			const char* name_c = names[i].c_str();

			sram.writeBlock(head, name_l, (void*)name_c);
			head += name_l;
		} else {
			sram.writeByte(head, 0);
			head += 1;
		}
	}

	sram.writeByte(FM_STATION_COUNT, l);
}

//Load tuner frequencies.
void SRAMHandler::getFrequencies(BackgroundTuneHandler* tuner) {
	if(!getValid())
		return;

	const uint8_t l = sram.readByte(FM_STATION_COUNT);
	String station_names[l];
	uint16_t frequencies[l];

	int head = FM_STATION_START;

	for(int i=0;i<l;i+=1) {
		frequencies[i] = readUint16(head);
		head += sizeof(uint16_t);
		
		String station_name = "";
		uint8_t ptr = sram.readByte(head);
		while(ptr != 0) {
			station_name += char(ptr);
			head += 1;
			ptr = sram.readByte(head);
		}
		head += 1;

		station_names[i] = station_name;
	}

	tuner->setStations(l, station_names, frequencies);
}

//Read a 2B number.
uint16_t SRAMHandler::readUint16(const uint32_t addr) {
	uint16_t num;
	sram.readBlock(addr, sizeof(uint16_t), (void*)&num);

	return num;
}

//Read a 2B number.
int16_t SRAMHandler::readInt16(const uint32_t addr) {
	int16_t num;
	sram.readBlock(addr, sizeof(int16_t), (void*)&num);

	return num;
}

//Write a 2B number.
void SRAMHandler::writeUint16(const uint32_t addr, const uint16_t data) {
	sram.writeBlock(addr, sizeof(uint16_t), (void*)&data);
}

//Write a 2B number.
void SRAMHandler::writeInt16(const uint32_t addr, const int16_t data) {
	sram.writeBlock(addr, sizeof(int16_t), (void*)&data);
}