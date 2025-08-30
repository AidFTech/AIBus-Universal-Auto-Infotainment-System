#include <elapsedMillis.h>
#include <MCP4251.h>

#include <Wire.h>

#include "AIBus.h"
#include "AIBus_Handler.h"
#include "Audio_Source.h"
#include "Text_Handler.h"
#include "Parameter_List.h"
#include "Si4735_AidF.h"
#include "Volume_Handler.h"
#include "Background_Tune_Handler.h"
#include "ADC_Handler.h"

#include "Text_Split.h"

#include "Radio_EEPROM.h"

#if defined(DXCORE)
#define AI_RX PIN_PA7
#define TUNER_RESET PIN_PC0
#define IDAT PIN_PA2
#define ICLK PIN_PA3

#define AUDIO_SW PIN_PC1 //Audio switch.
#define NAV_SW PIN_PC2 //Nav audio switch. Input.
#define AUDIO_ON_SW PIN_PC3 //Audio on/off. Audio on when low.
#define POWER_ON_SW PIN_PD0 //Output, vehicle power is on when high.
#define AUX_SW PIN_PD1 //Input, aux port mechanical switch.

#define DAC_FILTER_MODE PIN_PD2 //Output, digital filter mode.
#define DAC_MUTE PIN_PD3 //Output, analog mute.

#define ADC_CS PIN_PD4
#define VOL_CS PIN_PD5
#define TREBLE_CS PIN_PD6
#define BASS_CS PIN_PF2
#define FADE_CS PIN_PD7

#define NAV_MUTE PIN_PF3
#define RAM_CS PIN_PF4
#define DIGITAL_ERROR PIN_PF5

#else
#define AI_RX 3
#define TUNER_RESET 6
#define IDAT A4
#define ICLK A5

#define AUDIO_SW 7 //Audio switch.
#define NAV_SW 9 //Nav audio switch. Input.
#define AUDIO_ON_SW 10 //Audio on/off. Audio on when low.
#define POWER_ON_SW 13 //Output, vehicle power is on when high.
#define AUX_SW 14 //Input, aux port mechanical switch.

#define DAC_FILTER_MODE 15 //Output, digital filter mode.
#define DAC_MUTE 22 //Output, analog mute.

#define ADC_CS 16
#define VOL_CS 17
#define TREBLE_CS 18
#define BASS_CS 19
#define FADE_CS 20

#define NAV_MUTE 21
#define RAM_CS 22
#define DIGITAL_ERROR 23
#endif

#define SOURCE_COUNT 16
#define SOURCE_PING_DELAY 5000
#define COMPUTER_PING_DELAY 4000
#define SCREEN_PING_DELAY 5000
#define PARAMETER_DELAY 250

#define DISPLAY_INFO_TIMER 750
#define IMID_TIMER 300

#define DOOR_TIMER 30000
#define CONTROL_TIMER 7000

#define RDS_SEGMENT_COUNT 12
#define RDS_IMID_TIMER 3000

ParameterList parameters;

#define AISerial Serial
AIBusHandler aibus_handler(&AISerial, AI_RX);

TextHandler text_handler(&aibus_handler, &parameters);

MCP4251 vol_controller(VOL_CS, 10000, 0, 10000, 0);
MCP4251 treble_controller(TREBLE_CS, 50000, 0, 50000, 0);
MCP4251 bass_controller(BASS_CS, 10000, 0, 10000, 0);
MCP4251 fade_controller(FADE_CS, 10000, 0, 10000, 0);

VolumeHandler volume_handler(&vol_controller, &treble_controller, &bass_controller, &fade_controller, &parameters, &aibus_handler);

Si4735Controller tuner(TUNER_RESET, HIGH, &parameters), br_tuner(TUNER_RESET, LOW, &parameters);
BackgroundTuneHandler background_tuner(&br_tuner, &parameters);
SourceHandler source_handler(&aibus_handler, &tuner, &background_tuner, &parameters, &volume_handler, SOURCE_COUNT);

PCM9211Handler adc_handler(ADC_CS);

elapsedMillis aibus_timer, source_text_timer;
elapsedMillis src_ping_timer, computer_ping_timer, parameter_timer, screen_ping_timer;

bool source_text_timer_enabled = false;

bool info_timer_enabled = false;
elapsedMillis info_timer;

bool imid_timer_enabled = false;
elapsedMillis imid_timer;

elapsedMillis rds_imid_timer;
int rds_imid_index = 0;

elapsedMillis background_tune_timer = 0;

String rds_program_split[12];

bool* power_on = &parameters.power_on, *digital_mode = &parameters.digital_mode;

elapsedMillis door_timer;
bool door_timer_enabled = false;

elapsedMillis control_timer;

volatile bool tuner_reset = false;

//Arduino setup function.
void setup() {
	pinMode(AI_RX, INPUT_PULLUP);
	
	pinMode(TUNER_RESET, OUTPUT);

	pinMode(AUDIO_SW, OUTPUT);
	pinMode(NAV_SW, INPUT);
	pinMode(AUDIO_ON_SW, OUTPUT);
	pinMode(POWER_ON_SW, OUTPUT);
	pinMode(AUX_SW, INPUT_PULLUP);
	pinMode(NAV_MUTE, INPUT_PULLUP);

	pinMode(DAC_FILTER_MODE, OUTPUT);
	pinMode(DAC_MUTE, OUTPUT);

	pinMode(VOL_CS, OUTPUT);
	pinMode(TREBLE_CS, OUTPUT);
	pinMode(BASS_CS, OUTPUT);
	pinMode(FADE_CS, OUTPUT);
	pinMode(ADC_CS, OUTPUT);

	pinMode(RAM_CS, OUTPUT);

	pinMode(DIGITAL_ERROR, INPUT);
	
	digitalWrite(TUNER_RESET, HIGH);

	digitalWrite(AUDIO_SW, LOW);

	digitalWrite(AUDIO_ON_SW, LOW);
	digitalWrite(POWER_ON_SW, HIGH);

	digitalWrite(DAC_FILTER_MODE, LOW);
	digitalWrite(DAC_MUTE, HIGH);
	digitalWrite(ADC_CS, HIGH);

	digitalWrite(RAM_CS, HIGH);

	#if defined(DXCORE)
	SPI.swap(SPI0_SWAP_DEFAULT);
	Wire.swap(0);
	Wire.usePullups();
	#endif

	AISerial.begin(AI_BAUD);
	Wire.begin();
	SPI.begin();

	tuner.init1();
	br_tuner.init1();
	tuner.init2();
	br_tuner.init2();

	br_tuner.setPower(true, SUB_FM1);

	parameters.fm1_tune = tuner.getFrequency();
	parameters.fm2_tune = tuner.getFrequency();
	parameters.am_tune = parameters.am_start;

	getEEPROMPresets(&parameters);
	for(int i=0;i<sizeof(parameters.fm1_presets)/sizeof(uint16_t);i+=1) { 
		if(parameters.fm1_presets[i] < parameters.fm_lower_limit || parameters.fm1_presets[i] > parameters.fm_upper_limit)
			parameters.fm1_presets[i] = parameters.fm1_tune;
	}

	for(int i=0;i<sizeof(parameters.fm2_presets)/sizeof(uint16_t);i+=1) {
		if(parameters.fm2_presets[i] < parameters.fm_lower_limit || parameters.fm2_presets[i] > parameters.fm_upper_limit)
			parameters.fm2_presets[i] = parameters.fm2_tune;
	}

	for(int i=0;i<sizeof(parameters.am_presets)/sizeof(uint16_t);i+=1) {
		if(parameters.am_presets[i] < parameters.am_lower_limit || parameters.am_presets[i] > parameters.am_upper_limit)
			parameters.am_presets[i] = parameters.am_tune;
	}
	setEEPROMPresets(&parameters);

	parameters.handshake_sources.setStorage(parameters.handshake_source_list, 0);

	AudioSource src_fm1, src_fm2, src_am;
	src_fm1.source_name = "FM1";
	src_fm1.source_id = ID_RADIO;
	src_fm1.sub_id = 0;
	src_fm2.source_name = "FM2";
	src_fm2.source_id = ID_RADIO;
	src_fm2.sub_id = 1;
	src_am.source_name = "AM";
	src_am.source_id = ID_RADIO;
	src_am.sub_id = 2;

	AudioSource src_aux;
	src_aux.source_name = "Aux";
	src_aux.source_id = ID_RADIO;
	src_aux.sub_id = 3;

	source_handler.source_list[0] = src_fm1;
	source_handler.source_list[1] = src_fm2;
	source_handler.source_list[2] = src_am;

	source_handler.source_list[SOURCE_COUNT - 3] = src_aux;

	volume_handler.init();
	adc_handler.init();

	//source_handler.sendRadioHandshake();

	uint8_t init_data[] = {0x4A, 0x1F};
	AIData init_msg(sizeof(init_data), ID_RADIO, ID_CANSLATOR);
	init_msg.refreshAIData(init_data);
	aibus_handler.writeAIData(&init_msg, false);

	powerOff();
}

//Arduino loop function.
void loop() {
	bool power_switched = false; //Keep track if power was just turned on.

	if(parameters.minute_timer > MINUTE_TIMER) {
		parameters.minute_timer = 0;
		if(parameters.min >= 0 && parameters.hour >= 0) {
			parameters.min += 1;
			if(parameters.min >= 60) {
				parameters.min = 0;
				parameters.hour += 1;
			}
			if(parameters.hour >= 24)
				parameters.hour = 0;

			if(*power_on)
				text_handler.sendTime();
		}
	}
	
	/*if(!*power_on) {
		AIData msg;
		if(aibus_handler.readAIData(&msg)) {
			if(msg.l == 1 && msg.data[0] == 0x1 && msg.receiver == ID_RADIO) //Ping.
				aibus_handler.sendAcknowledgement(ID_RADIO, msg.sender);
			else if(msg.l >= 1 && msg.data[0] == 0x80) { //Acknowledgement. Ignore.
			} else if(msg.receiver == 0xFF && msg.data[0] == 0xA1) {
				if(msg.data[1] == 0x2) { //Key position.
					const uint8_t pos = msg.data[2]&0xF;
					if(pos != 0) {
						*power_on = true;
						power_switched = true;
					}
				}
			}
		}
		
		delay(500);

		if(!power_switched)
			return;
	}*/

	if(door_timer_enabled && door_timer > DOOR_TIMER && parameters.key_position == 0)
		powerOff();

	if(power_switched) {
		digitalWrite(POWER_ON_SW, HIGH);
		adc_handler.init();
		source_handler.sendRadioHandshake();
	}
	
	const uint8_t last_active_source_id = source_handler.getCurrentSourceID();
	const uint16_t last_source_count = source_handler.getFilledSourceCount(), last_active_source = source_handler.getCurrentSource();

	const uint16_t last_fm1 = parameters.fm1_tune, last_fm2 = parameters.fm2_tune, last_am = parameters.am_tune;
	const bool last_info = parameters.info_mode;

	const uint8_t last_preset = parameters.current_preset;

	const int8_t last_hour = parameters.hour, last_min = parameters.min;

	const bool last_send_time = parameters.send_time, last_12h = parameters.send_12h, last_auto_clock = parameters.auto_clock;

	const bool last_phone = parameters.phone_active;

	AIData msg;
	do {
		if(aibus_handler.dataAvailable() > 0) {
			if(aibus_handler.readAIData(&msg)) {
				aibus_timer = 0;
				computer_ping_timer = COMPUTER_PING_DELAY - 1000;

				if(msg.sender == ID_RADIO)
					continue;
				
				if(msg.receiver == ID_RADIO) {
					if(msg.sender == source_handler.getCurrentSourceID())
						source_text_timer = 0;
				}
				
				handleAIBus(&msg);
			}
		}
	} while(aibus_timer < 50);

	if(!*power_on && !power_switched)
		return;

	AudioSource source_list[SOURCE_COUNT];
	const uint16_t source_count = source_handler.getFilledSources(source_list), current_source = source_handler.getCurrentSource();
	
	//Source changed.
	if(power_switched || source_handler.getCurrentSourceID() != last_active_source_id || source_handler.getCurrentSource() != last_active_source || parameters.phone_active != last_phone) {
		src_ping_timer = 0;
		parameters.info_mode = false;
		parameters.current_preset = 0;
		parameters.preferred_preset = 0;
		
		const uint8_t current_source_id = source_handler.getCurrentSourceID();
		const uint16_t current_source = source_handler.getCurrentSource();
		
		if(!parameters.phone_active) {
			uint8_t sub_id = source_handler.source_list[current_source].sub_id;
			if(current_source_id == 0)
				sub_id = 0;

			parameters.last_sub = sub_id;
			
			uint8_t function_data[] = {0x40, 0x10, current_source_id, sub_id};
			AIData function_msg(sizeof(function_data), ID_RADIO, last_active_source_id);
			function_msg.refreshAIData(function_data);
			
			if(function_msg.receiver != 0 && function_msg.receiver != ID_RADIO)
				aibus_handler.writeAIData(&function_msg);
			else if(function_msg.receiver == ID_RADIO && current_source_id != ID_RADIO)
				tuner.setPower(false);
			
			setSourceName();

			parameters.audio_on = current_source_id != 0;
			
			if(current_source_id != 0 && current_source_id != ID_RADIO) {
				function_msg.receiver = current_source_id;
				aibus_handler.writeAIData(&function_msg, function_msg.receiver != 0 && function_msg.receiver != ID_RADIO);

				source_text_timer_enabled = true;
				source_text_timer = 0;
			} else if(current_source_id == ID_RADIO) {
				const uint8_t sub_id = source_handler.source_list[current_source].sub_id;
				setTunerFrequency(sub_id);
				sendTunedFrequencyMessage(sub_id);
				clearFMData();
				text_handler.createRadioMenu(sub_id);
			}
		
			//Set audio switch.
			if(current_source_id == 0) { //Audio off.
				digitalWrite(AUDIO_ON_SW, HIGH);
				digitalWrite(AUDIO_SW, LOW);
				digitalWrite(DAC_MUTE, LOW);
				adc_handler.powerOff();
			} else if(current_source_id == ID_RADIO) {
				if(volume_handler.getVolume() > 0)
					digitalWrite(DAC_MUTE, HIGH);
				adc_handler.setADCOn();
				digitalWrite(AUDIO_ON_SW, LOW);
				const uint8_t sub_id = source_handler.source_list[current_source].sub_id;
				if(sub_id < 3) { //Tuner.
					digitalWrite(AUDIO_SW, LOW);
				} else { //Aux.
					digitalWrite(AUDIO_SW, HIGH);
				}
			} else if(current_source_id == ID_PHONE || current_source_id == ID_ANDROID_AUTO) { //Pi. TODO: Use an AIBus flag.
				if(volume_handler.getVolume() > 0)
					digitalWrite(DAC_MUTE, HIGH);
				digitalWrite(AUDIO_ON_SW, HIGH);
				adc_handler.setPiOut();
			} else { //External audio.
				if(volume_handler.getVolume() > 0)
					digitalWrite(DAC_MUTE, HIGH);
				digitalWrite(AUDIO_ON_SW, HIGH);
				adc_handler.setExtOut();
			}
		} else {
			parameters.audio_on = true;

			uint8_t function_data[] = {0x40, 0x10, 0x0};
			AIData function_msg(sizeof(function_data), ID_RADIO, last_active_source_id);
			function_msg.refreshAIData(function_data);
			
			if(function_msg.receiver != 0 && function_msg.receiver != ID_RADIO)
				aibus_handler.writeAIData(&function_msg);
			else if(function_msg.receiver == ID_RADIO)
				tuner.setPower(false);
			
			if(!last_phone)
				text_handler.createPhoneWindow();

			digitalWrite(DAC_MUTE, HIGH);
			
			digitalWrite(AUDIO_ON_SW, HIGH);
			digitalWrite(AUDIO_SW, HIGH);

			adc_handler.setPiOut();
		}

		sendAudioLightMessage(current_source_id != 0);
	}
	
	if(source_text_timer_enabled && source_text_timer > 50 && !parameters.phone_active) {
		source_text_timer_enabled = false;
		const uint8_t current_source_id = source_handler.getCurrentSourceID();
		if(current_source_id != ID_RADIO && current_source_id != 0)
			text_handler.sendSourceTextControl(current_source_id, current_source_id);
	}

	if(imid_timer_enabled && imid_timer > IMID_TIMER) {
		imid_timer_enabled = false;

		sendIMIDRequest();

		const uint8_t current_source = source_handler.getCurrentSourceID();
		
		if(current_source == ID_RADIO) {
			const uint16_t current_source_num = source_handler.getCurrentSource();
			const uint8_t sub_id = source_handler.source_list[current_source_num].sub_id;
			
			text_handler.sendIMIDSourceMessage(current_source, sub_id);
			sendTunedFrequencyMessage(sub_id);
		} else if(current_source != 0) {
			text_handler.sendSourceTextControl(current_source, current_source);
		}
	}

	parameters.timer_active = info_timer_enabled || source_text_timer_enabled || imid_timer_enabled || MINUTE_TIMER - parameters.minute_timer <= 100 || SCREEN_PING_DELAY - screen_ping_timer <= 100;

	if(parameters.handshake_timer_active && parameters.handshake_timer > 200) {
		parameters.handshake_timer_active = false;

		source_handler.checkSources();
		/*while(parameters.handshake_sources.size() > 0) {
			uint8_t handshake_data[] = {0x5, parameters.handshake_sources.at(0), 0x2};
			AIData handshake_msg(sizeof(handshake_data), ID_RADIO, parameters.handshake_sources.at(0));
			handshake_msg.refreshAIData(handshake_data);
			aibus_handler.writeAIData(&handshake_msg);

			parameters.handshake_sources.remove(0);
		}*/

	}

	do {
		if(source_handler.getCurrentSourceID() == ID_RADIO) {
			tuner.loop();
			
			const bool last_stereo = parameters.fm_stereo;
			const uint16_t current_source = source_handler.getCurrentSource();

			const uint8_t sub_id = source_handler.source_list[current_source].sub_id;

			uint16_t last_compare, *current_frequency;
			if(sub_id == SUB_FM1) {
				last_compare = last_fm1;
				current_frequency = &parameters.fm1_tune;
			} else if(sub_id == SUB_FM2) {
				last_compare = last_fm2;
				current_frequency = &parameters.fm2_tune;
			} else if(sub_id == SUB_AM) {
				last_compare = last_am;
				current_frequency = &parameters.am_tune;
			} else break;

			uint8_t current_preset = 0;
			bool preset_found = false;

			if(parameters.preferred_preset > 0) {
				if(sub_id == SUB_FM1) {
					if(parameters.fm1_presets[parameters.preferred_preset-1] == *current_frequency) {
						current_preset = parameters.preferred_preset;
						preset_found = true;
					}
				} else if(sub_id == SUB_FM2) {
					if(parameters.fm2_presets[parameters.preferred_preset-1] == *current_frequency) {
						current_preset = parameters.preferred_preset;
						preset_found = true;
					}
				} else if(sub_id == SUB_AM) {
					if(parameters.am_presets[parameters.preferred_preset-1] == *current_frequency) {
						current_preset = parameters.preferred_preset;
						preset_found = true;
					}
				}
			} 

			if(!preset_found) {
				if(sub_id == SUB_FM1) {
					for(int i=0;i<PRESET_COUNT;i+=1) {
						if(parameters.fm1_presets[i] == *current_frequency) {
							current_preset = i + 1;
							break;
						}
					}
				} else if(sub_id == SUB_FM2) {
					for(int i=0;i<PRESET_COUNT;i+=1) {
						if(parameters.fm2_presets[i] == *current_frequency) {
							current_preset = i + 1;
							break;
						}
					}
				} else if(sub_id == SUB_AM) {
					for(int i=0;i<PRESET_COUNT;i+=1) {
						if(parameters.am_presets[i] == *current_frequency) {
							current_preset = i + 1;
							break;
						}
					}
				}
			}

			parameters.current_preset = current_preset;
			parameters.preferred_preset = current_preset;

			if(parameters.info_mode && !last_info) {
				info_timer_enabled = true;
				info_timer = 0;
				text_handler.sendIMIDInfoMessage("RDS");
			} else if(!parameters.info_mode && last_info) {
				String current_rds = parameters.rds_program_name;

				if(parameters.imid_radio)
					text_handler.sendIMIDSourceMessage(ID_RADIO, sub_id);

				text_handler.sendTunedFrequencyMessage(parameters.current_preset, *current_frequency, sub_id != SUB_AM, true);

				text_handler.sendLongRDSMessage(current_rds);
				text_handler.sendIMIDRDSMessage(rds_program_split[rds_imid_index]);
			}

			if(info_timer_enabled && info_timer > DISPLAY_INFO_TIMER && parameters.info_mode) {
				info_timer_enabled = false;
				rds_imid_index = 0;
				rds_imid_timer = 0;
				text_handler.sendIMIDInfoMessage(rds_program_split[rds_imid_index]);
			}
			
			if(sub_id == SUB_FM1 || sub_id == SUB_FM2) {
				const bool seeking = tuner.getSeeking(current_frequency);
				if(!seeking && (parameter_timer >= PARAMETER_DELAY)) {
					parameter_timer = 0;

					const String last_rds = parameters.rds_program_name, last_station_name = parameters.rds_station_name;
					tuner.getParameters(&parameters, sub_id);
					if(*current_frequency != tuner.getFrequency()) {
						*current_frequency = tuner.getFrequency();
						parameters.tune_changed = true;
					}

					if(parameters.fm_stereo != last_stereo) {
						text_handler.sendStereoMessage(parameters.fm_stereo);
						parameter_timer = 0;
					}

					String current_rds = parameters.rds_program_name;
					if(parameters.has_rds && current_rds.compareTo(last_rds) != 0) {
						if(current_rds.substring(0,8).indexOf(last_rds.substring(0,8)) < 0)
							tuner.clearRds();

						text_handler.sendLongRDSMessage(current_rds);
						
						uint8_t rds_char = parameters.imid_char;
						if(parameters.imid_radio)
							rds_char = 8;

						for(int i=0;i<12;i+=1)
							rds_program_split[i] = "";

						splitText(rds_char, current_rds, rds_program_split, 12);
						
						text_handler.sendIMIDRDSMessage(rds_program_split[rds_imid_index]);
						rds_imid_timer = 0;

						if(parameters.info_mode && !info_timer_enabled)
							text_handler.sendIMIDInfoMessage(rds_program_split[rds_imid_index]);

						parameter_timer = 0;
					}
				
					if(parameters.has_rds && parameters.rds_station_name.compareTo(last_station_name) != 0) {
						AIData station_name = getTextMessage(parameters.rds_station_name, 1, 1);
						station_name.data[1] |= 0x10;
						aibus_handler.writeAIData(&station_name, parameters.computer_connected);

						text_handler.sendIMIDCallsignMessage(parameters.rds_station_name);
						text_handler.sendMirrorMessage(parameters.rds_station_name, 3, false);
					}
				}

				if(rds_imid_timer > RDS_IMID_TIMER) {
					rds_imid_timer = 0;
					const int old_len = rds_program_split[rds_imid_index].length();

					if(rds_imid_index < 12)
						rds_imid_index += 1;
					else
						rds_imid_index = 0;

					if(rds_program_split[rds_imid_index].length() <= 0)
						rds_imid_index = 0;

					if(rds_program_split[rds_imid_index].length() > 0 && old_len != 0) {
						if(parameters.info_mode && !info_timer_enabled)
							text_handler.sendIMIDInfoMessage(rds_program_split[rds_imid_index]);
						else
							text_handler.sendIMIDRDSMessage(rds_program_split[rds_imid_index]);
					}
				}
			}
			
			if(last_compare != *current_frequency || parameters.tune_changed) {
				if(parameters.info_mode) {
					parameters.info_mode = false;
					if(parameters.imid_radio)
						text_handler.sendIMIDSourceMessage(ID_RADIO, sub_id);
				}
				text_handler.sendTunedFrequencyMessage(*current_frequency, sub_id != SUB_AM, true);
				clearFMData();
				parameter_timer = 0;
			}

			if(last_preset != current_preset || parameters.tune_changed) {
				parameters.tune_changed = false;
				
				String header_text = "";
				switch(sub_id) {
				case SUB_FM1:
					header_text += "FM1";
					break;
				case SUB_FM2:
					header_text += "FM2";
					break;
				case SUB_AM:
					header_text += "AM";
					break;
				}

				if(current_preset > 0)
					header_text += "-" + String(current_preset);

				AIData text_msg = getTextMessage(header_text, 0, 0);
				aibus_handler.writeAIData(&text_msg, parameters.computer_connected);

				text_handler.sendMirrorMessage(header_text, 0, true);
				text_handler.sendTunedFrequencyMessage(current_preset, *current_frequency, sub_id != SUB_AM, true);
			}
			
			break;
		} else if(source_handler.getCurrentSourceID() != 0) {
			//TODO: Anything here?
		}
	} while(false);

	if(tuner_reset) {
		tuner_reset = false;
		setTunerFrequency(SUB_FM1);
		setTunerFrequency(SUB_FM2);
		setTunerFrequency(SUB_AM);
	}

	if(src_ping_timer >= SOURCE_PING_DELAY)
		pingActiveSource();

	if(!parameters.computer_connected && computer_ping_timer >= COMPUTER_PING_DELAY)
		pingComputer();

	if(screen_ping_timer >= SCREEN_PING_DELAY)
		getScreenControlRequest(!parameters.computer_connected || parameters.manual_tune_mode
																|| parameters.bass_adjust
																|| parameters.treble_adjust
																|| parameters.balance_adjust
																|| parameters.fader_adjust);

	if(background_tune_timer >= 100) {
		background_tuner.loop();
		background_tune_timer = 0;
	}

	//Check the control timer.
	if(parameters.last_control != ID_NAV_COMPUTER && control_timer > CONTROL_TIMER)
		parameters.last_control = ID_NAV_COMPUTER;

	if(parameters.send_time && parameters.hour >= 0 && parameters.min >= 0 &&
			(parameters.hour != last_hour ||
			parameters.min != last_min ||
			last_send_time != parameters.send_time ||
			parameters.send_12h != last_12h ||
			parameters.auto_clock != last_auto_clock))
		text_handler.sendTime();

	//Send the volume IMID message.
	if(volume_handler.getVolumeChanged()) {
		const uint16_t volume = volume_handler.getVolume(), range = volume_handler.getVolRange();

		if(volume != 0)
			digitalWrite(DAC_MUTE, HIGH);
		else
			digitalWrite(DAC_MUTE, LOW);
	}
}

//Interpret a received AIBus message.
void handleAIBus(AIData* msg) {
	if(msg->receiver == ID_NAV_SCREEN && msg->l >= 3 && msg->data[0] == 0x77) {
		if((msg->data[2]&0x10) != 0) {
			parameters.last_control = msg->data[1];
			control_timer = 0;
		}
	}

	if(!parameters.mirror_connected && msg->sender == ID_ANDROID_AUTO)
		parameters.mirror_connected = true;

	if(msg->receiver != ID_RADIO && msg->receiver != 0xFF)
		return;

	if(msg->l == 1 && msg->data[0] == 0x1 && msg->receiver == ID_RADIO) { //Ping.
		aibus_handler.sendAcknowledgement(ID_RADIO, msg->sender);
	} else if(msg->l >= 1 && msg->data[0] == 0x80) { //Acknowledgement. Ignore.
	} else if(msg->receiver == ID_RADIO && msg->l >= 2 && msg->data[0] == 0x1D) { //Clock message.
		aibus_handler.sendAcknowledgement(ID_RADIO, msg->sender);
		parameters.send_time = (msg->data[1]) != 0;
		if((msg->data[1]&0x01) != 0)
			parameters.auto_clock = true;
		else if((msg->data[1]&0x02) != 0)
			parameters.auto_clock = false;

		parameters.send_12h = (msg->data[1]&0x80) != 0;
	} else if(msg->receiver == ID_RADIO && msg->sender == ID_PHONE && msg->l >= 3) { //Phone message.
		if(msg->data[1] == 0x6) {
			parameters.phone_active = msg->data[2] != 0x0;
		}
	} else if(msg->receiver == ID_RADIO) { //Radio message.
		bool answered = false;
		answered = volume_handler.handleAIBus(msg);
		if(!answered)
			answered = source_handler.handleAIBus(msg);

		if(parameters.tune_changed && source_handler.getCurrentSourceID() == ID_RADIO) {
			uint16_t current_frequency = parameters.fm1_tune;
			const uint8_t sub_id = source_handler.source_list[source_handler.getCurrentSource()].sub_id;

			if(sub_id <= SUB_AM) {
				switch(sub_id) {
				case SUB_FM2:
					current_frequency = parameters.fm2_tune;
					break;
				case SUB_AM:
					current_frequency = parameters.am_tune;
				}

				text_handler.sendTunedFrequencyMessage(current_frequency, sub_id != SUB_AM, true);
			}
		}
	} else if(msg->receiver == 0xFF && msg->l >= 1 && msg->data[0] == 0xA1 && msg->sender != ID_RADIO) {
		if(msg->l >= 3 && msg->data[1] == 0x2) { //Key position.
			const uint8_t pos = msg->data[2]&0xF;

			if(pos != parameters.key_position) {
				if(pos == 0) {
					if(parameters.power_on) {
						if((parameters.door_position&0xC) != 0)
							powerOff();
						else {
							door_timer = 0;
							door_timer_enabled = true;
						}
					}
				} else {
					digitalWrite(POWER_ON_SW, HIGH);
					parameters.power_on = true;
					source_handler.sendRadioHandshake();
					sendIMIDPing();
				}
			}
			
			parameters.key_position = pos;
			
		} else if(msg->data[1] == 0x43 && msg->l >= 3) { //Door position.
			const uint8_t pos = msg->data[2]&0xF;
			
			if(parameters.key_position == 0 && pos != parameters.door_position) {
				if((pos&0xC) != 0) {
					if(parameters.power_on)
						powerOff();
					/*else {
						door_timer = 0;
						door_timer_enabled = true;
						digitalWrite(POWER_ON_SW, HIGH);
					}*/
				}
			}
			
			parameters.door_position = pos;
		} else if(msg->data[1] == 0x1F && msg->l >= 3) {
			if(msg->data[2] == 0x1 && msg->l >= 6) { //Time.
				int16_t new_minute = 60*msg->data[3] + msg->data[4] - parameters.offset*30;
				
				while(new_minute >= 1440)
					new_minute -= 1440;
				while(new_minute < 0)
					new_minute += 1440;
					
				parameters.hour = new_minute/60;
				parameters.min = new_minute%60;
				parameters.minute_timer = msg->data[5]*1000;

				parameters.send_12h = (msg->data[3]&0x80) != 0;
					
			} else if(msg->data[2] == 0x4) { //Vehicle speed.
				double speed = getSpeed(msg);
				if(msg->data[3]&0x80 != 0) //Speed in mph.
					parameters.vehicle_speed = uint16_t(speed*1.6);
				else
					parameters.vehicle_speed = uint16_t(speed);
			}
		}
	} else if(msg->receiver == 0xFF && msg->sender == ID_IMID_SCR && msg->data[0] == 0x3B) {
		if(msg->data[1] == 0x23 && msg-> l >= 4) {
			parameters.imid_char = msg->data[2];
			parameters.imid_lines = msg->data[3];
		} else if(msg->data[1] == 0x57 && msg->l >= 3) {
			parameters.imid_radio = false;

			for(int i=2;i<msg->l;i+=1) {
				if(msg->data[i] == ID_RADIO)
					parameters.imid_radio = true;
				if(parameters.imid_radio)
					break;
			}
		}
		
		imid_timer = 0;
		imid_timer_enabled = true;
	}

	if(!parameters.computer_connected && msg->sender == ID_NAV_COMPUTER) {
		parameters.computer_connected = true;
		screenInit();
		
		{
			uint8_t data[] = {0x77, parameters.last_control, 0x10};

			AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN);
			screen_msg.refreshAIData(data);
			aibus_handler.writeAIData(&screen_msg, parameters.screen_connected);
		}
	}

	if(!parameters.imid_connected && msg->sender == ID_IMID_SCR) {
		parameters.imid_connected = true;
		
		if(parameters.min >= 0 && parameters.hour >= 0)
			text_handler.sendTime();
	}

	if(!parameters.screen_connected && msg->sender == ID_NAV_SCREEN) {
		parameters.screen_connected = true;
		sendAudioLightMessage(parameters.audio_on);
	}
}

//Send the frequency message to the screen.
void sendTunedFrequencyMessage(const uint8_t sub_id) {
	if(sub_id == 0)
		text_handler.sendTunedFrequencyMessage(parameters.fm1_tune, true, true);
	else if(sub_id == 1)
		text_handler.sendTunedFrequencyMessage(parameters.fm2_tune, true, true);
	else if(sub_id == 2)
		text_handler.sendTunedFrequencyMessage(parameters.am_tune, false, true);
}

//Set the tuner frequency to a pre-set value based on the active sub-id.
void setTunerFrequency(const uint8_t sub_id) {
	if(sub_id == 0) {
		tuner.setPower(true, SUB_FM1);
		parameters.fm1_tune = tuner.setFrequency(parameters.fm1_tune);
	} else if(sub_id == 1) {
		tuner.setPower(true, SUB_FM2);
		parameters.fm2_tune = tuner.setFrequency(parameters.fm2_tune);
	} else if(sub_id == 2) {
		tuner.setPower(true, SUB_AM);
		parameters.am_tune = tuner.setFrequency(parameters.am_tune);
	} else
		tuner.setPower(false);
	
	parameter_timer = PARAMETER_DELAY;
}

//Send the heartbeat/redundant message to the active source.
void pingActiveSource() {
	src_ping_timer = 0;
	const uint8_t current_source = source_handler.getCurrentSourceID();
	if(current_source == ID_RADIO || current_source == 0)
		return;

	AIData ping_msg(3, ID_RADIO, current_source);
	uint8_t ping_data[] = {0x70, 0x10, current_source};
	ping_msg.refreshAIData(ping_data);

	aibus_handler.writeAIData(&ping_msg);
}

//Ping the nav computer to check that it is connected.
void pingComputer() {
	computer_ping_timer = 0;
	
	uint8_t ping_data[] = {1};
	AIData ping_msg(sizeof(ping_data), ID_RADIO, ID_NAV_COMPUTER);
	ping_msg.refreshAIData(ping_data);

	if(aibus_handler.writeAIData(&ping_msg, false));
}

//Send the initial message to the nav computer.
void screenInit() {
	if(parameters.min >= 0 && parameters.hour >= 0)
		text_handler.sendTime();
	
	setSourceName();
	const uint8_t current_source = source_handler.getCurrentSourceID();
	if(current_source != 0 && current_source != ID_RADIO)
		text_handler.sendSourceTextControl(current_source, current_source);
}

//Send the active source name to the nav computer.
void setSourceName() {
	String audio_off_msg = F("Audio Off");
	text_handler.clearAllText();
	const uint8_t source = source_handler.getCurrentSourceID();

	if(source == 0) {
		text_handler.setBlankHeader(audio_off_msg);
		text_handler.setOverlayHeader(audio_off_msg);
		text_handler.sendIMIDSourceMessage(0,0);
		text_handler.sendMirrorMessage(audio_off_msg, 0, true);
		return;
	}

	if(source_handler.source_list[source_handler.getCurrentSource()].source_name.compareTo("") != 0) {
		text_handler.setBlankHeader(source_handler.source_list[source_handler.getCurrentSource()].source_name);
		text_handler.sendMirrorMessage(source_handler.source_list[source_handler.getCurrentSource()].source_name, 0, true);
		text_handler.setOverlayHeader(source_handler.source_list[source_handler.getCurrentSource()].source_short);
	} else {
		String source_name = "";
		switch(source) {
		case 0:
			source_name = audio_off_msg;
			break;
		case ID_RADIO:
			source_name = "Radio";
			break;
		case ID_TAPE:
			source_name = "Tape";
			break;
		case ID_CD:
		case ID_CDC:
			source_name = "CD";
			break;
		case ID_XM:
			source_name = "XM";
			break;
		case ID_ANDROID_AUTO:
			source_name = "Mirror";
			break;
		default:
			source_name = "EXT.";
			break;
		}

		text_handler.setBlankHeader(source_name);
		text_handler.sendMirrorMessage(source_name, 0, true);
		text_handler.setOverlayHeader(source_name);
	}



	text_handler.sendIMIDSourceMessage(source_handler.getCurrentSourceID(), source_handler.source_list[source_handler.getCurrentSource()].sub_id);
}

//Get the current vehicle speed.
double getSpeed(AIData* msg) {
	const uint8_t byte_count = msg->data[3]&0xF, dec = (msg->data[3]&0x70)>>4;
	unsigned long speed_int = 0;
	for(int i=0;i<byte_count;i+=1) {
		speed_int <<= 8;
		speed_int |= msg->data[4+i];
	}

	double speed = double(speed_int);
	for(int i=0;i<dec;i+=1) {
		speed = speed/10.0;
	}
	return speed;
}

//Clear RDS and stereo indicator.
void clearFMData() {
	const bool last_stereo = parameters.fm_stereo;
	const String last_rds = parameters.rds_program_name, last_station_name = parameters.rds_station_name;
	
	parameters.fm_stereo = false;
	
	if(parameters.fm_stereo != last_stereo)
		text_handler.sendStereoMessage(false);
	
	parameters.rds_program_name = "";
	for(int i=0;i<12;i+=1)
		rds_program_split[i] = "";
	
	if(parameters.rds_program_name.compareTo(last_rds) != 0) {
		text_handler.sendLongRDSMessage("");
		text_handler.sendIMIDRDSMessage("");
	}

	parameters.rds_station_name = "";
	if(parameters.rds_station_name.compareTo(last_station_name) != 0) {
		uint8_t clear_data[] = {0x20, 0x71, 0x1};
		AIData clear_msg(sizeof(clear_data), ID_RADIO, ID_NAV_COMPUTER);
		clear_msg.refreshAIData(clear_data);

		aibus_handler.writeAIData(&clear_msg, parameters.computer_connected);

		text_handler.sendIMIDCallsignMessage("");
		text_handler.sendMirrorMessage("", 3, false);
	}
}

//Send the screen control request message.
void getScreenControlRequest(const bool all) {
	screen_ping_timer = 0;
	uint8_t data[] = {0x77, ID_RADIO, 0x20};
	if(all)
		data[2] |= 0x10;

	AIData screen_msg(sizeof(data), ID_RADIO, ID_NAV_SCREEN);
	screen_msg.refreshAIData(data);
	aibus_handler.writeAIData(&screen_msg, parameters.screen_connected);
}

//Send a ping to the IMID.
void sendIMIDPing() {
	uint8_t imid_request_data[] = {0x4, 0xE6, 0x3B};
	AIData imid_request_msg(sizeof(imid_request_data), ID_RADIO, ID_IMID_SCR);
	imid_request_msg.refreshAIData(imid_request_data);

	aibus_handler.writeAIData(&imid_request_msg, false);
}

//Send a request to the IMID for its full specs.
void sendIMIDRequest() {
	uint8_t imid_request_data[] = {0x4, 0xE6, 0x3B};
	AIData imid_request_msg(sizeof(imid_request_data), ID_RADIO, ID_IMID_SCR);
	imid_request_msg.refreshAIData(imid_request_data);

	aibus_handler.writeAIData(&imid_request_msg);

	elapsedMillis response_timer;
	while(response_timer < 100) {
		AIData reply;
		if(aibus_handler.dataAvailable(false) > 0) {
			if(aibus_handler.readAIData(&reply, false)) {
				if(reply.sender == ID_RADIO || (reply.l >=1 && reply.data[0] == 0x80))
					continue;

				if(reply.receiver != ID_RADIO && reply.receiver != 0xFF)
					continue;

				//response_timer = 0;
				if(reply.sender == ID_IMID_SCR && reply.receiver == ID_RADIO) {
					aibus_handler.sendAcknowledgement(ID_RADIO, reply.sender);
					if(reply.data[1] == 0x23 && reply.l >= 4) {
						parameters.imid_char = reply.data[2];
						parameters.imid_lines = reply.data[3];
					} else if(reply.data[1] == 0x57 && reply.l >= 3) {
						parameters.imid_radio = false;
						for(int i=2;i<reply.l;i+=1) {
							if(reply.data[i] == ID_RADIO)
								parameters.imid_radio = true;
							if(parameters.imid_radio)
								break;
						}
					} else
						aibus_handler.cacheMessage(&reply);

					parameters.imid_connected = true;
				} else if(reply.receiver == ID_RADIO || reply.receiver == 0xFF) {
					if(reply.receiver == ID_RADIO)
						aibus_handler.sendAcknowledgement(ID_RADIO, reply.sender);
						
					aibus_handler.cacheMessage(&reply);
				}
			}
		}
	}

	if(parameters.min >= 0 && parameters.hour >= 0)
		text_handler.sendTime();
}

//Send the message to turn the screen light on.
void sendAudioLightMessage(const bool audio_on) {
	uint8_t light_data[] = {0x34, audio_on ? 0x1 : 0x0};
	AIData light_msg(sizeof(light_data), ID_RADIO, ID_NAV_SCREEN);
	light_msg.refreshAIData(light_data);
	aibus_handler.writeAIData(&light_msg, parameters.screen_connected);
}

//Check the I2C levels.
/*inline void checkI2C() {
	#if defined(DXCORE)
	const uint8_t levels = Wire.checkPinLevels();

	if(levels == 0x3) //Ready.
		return;

	uint8_t error_data[] = {0xA1, 0xFF}; //TODO: Adjust.
	AIData error_msg(sizeof(error_data), ID_RADIO, 0xFF);
	error_msg.refreshAIData(error_data);

	aibus_handler.writeAIData(&error_msg, false);

	/*Wire.end();
	pinMode(ICLK, OUTPUT);
	pinMode(IDAT, INPUT_PULLUP);

	int iterations = 0;
	do {
		bool high_read = false;
		for(int i=0;i<8;i+=1) {
			digitalWrite(ICLK, LOW);
			delayMicroseconds(67);
			digitalWrite(ICLK, HIGH);

			if(iterations > 0) {
				if(digitalRead(IDAT) == HIGH)
					high_read = true;
			}
			//if(high_read)
			//	break;

			delayMicroseconds(67);
		}
		if(high_read)
			break;

		digitalWrite(ICLK, LOW);
		delay(1);
		digitalWrite(ICLK, HIGH);

		iterations += 1;
	} while(digitalRead(IDAT) != HIGH && iterations < 7);

	aibus_handler.cachePending(ID_RADIO);

	iterations = 0;
	while(digitalRead(IDAT) != HIGH && iterations < 7) {
		pinMode(IDAT, OUTPUT);
		digitalWrite(IDAT, HIGH);
		delay(2);
		digitalWrite(IDAT, LOW);
		delay(500);
		digitalWrite(IDAT, HIGH);
		pinMode(IDAT, INPUT_PULLUP);

		iterations += 1;
	}

	aibus_handler.cachePending(ID_RADIO);

	while(digitalRead(IDAT) != HIGH) {
		pinMode(IDAT, OUTPUT);
		tuner_reset = true;
		digitalWrite(TUNER_RESET, LOW);
		delay(100);
		digitalWrite(TUNER_RESET, HIGH);
		delay(50);
		digitalWrite(IDAT, LOW);
		delay(100);
		pinMode(IDAT, INPUT_PULLUP);
	}
	aibus_handler.cachePending(ID_RADIO);

	Wire.begin();
	Wire.setClock(7500);
	#endif
}*/

//Power off procedure.
void powerOff() {
	*power_on = false;

	parameters.computer_connected = false;
	parameters.amp_connected = false;
	parameters.mirror_connected = false;
	parameters.imid_connected = false;

	parameters.imid_char = 0;
	parameters.imid_lines = 0;
	parameters.imid_radio = false;

	digitalWrite(POWER_ON_SW, LOW);
	digitalWrite(AUDIO_ON_SW, HIGH);
	adc_handler.powerOff();
}
