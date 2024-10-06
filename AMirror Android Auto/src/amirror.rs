use std::os::unix::net::UnixStream;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use crate::mirror::messages::*;
use crate::{aibus::*, write_aibus_message};
use crate::context::Context;
use crate::mirror::handler::*;
use crate::ipc::*;

const APP_NAME: u8 = 0x0;
const SONG_NAME: u8 = 0x1;
const ARTIST_NAME: u8 = 0x2;
const ALBUM_NAME: u8 = 0x3;
pub struct AMirror<'a> {
	context: &'a Arc<Mutex<Context>>,
	stream: &'a Arc<Mutex<UnixStream>>,
	handler: MirrorHandler<'a>,

	source_request_timer: Instant,
	pub run: bool,
}

impl <'a> AMirror<'a> {
	pub fn new(context: &'a Arc<Mutex<Context>>, stream: &'a Arc<Mutex<UnixStream>>, w: u16, h: u16) -> Self {
		let mutex_mirror_handler = MirrorHandler::new(context, w, h);

		return Self{
			context,
			stream,
			handler: mutex_mirror_handler,

			source_request_timer: Instant::now(),
			run: true,
		};
	}

	pub fn process(&mut self) {
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Process: Context Locked.");
				return;
			}
		};

		let phone_type = context.phone_type;
		let phone_name = context.phone_name.clone();

		let song_title = context.song_title.clone();
		let artist = context.artist.clone();
		let album = context.album.clone();
		let app = context.app.clone();

		let night = context.night;
		let radio_connected = context.radio_connected;

		let playing = context.playing;

		std::mem::drop(context);

		self.handler.process();

		context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Process: Context Locked.");
				return;
			}
		};

		if context.phone_type != phone_type {
			let phone_type_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: [0x30, context.phone_type].to_vec(),
			};

			self.write_aibus_message(phone_type_msg);
			
			if context.audio_selected && context.audio_text {
				if context.phone_type == 0 {
					self.write_nav_text("Mirror".to_string(), 0, 0, true);
				} else if context.phone_type == 3 {
					self.write_nav_text("Carplay".to_string(), 0, 0, true);
				} else if context.phone_type == 5 {
					self.write_nav_text("Android".to_string(), 0, 0, true);
				}
			}

			if context.radio_connected {
				std::mem::drop(context);

				self.write_radio_name();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process: Context Locked.");
						return;
					}
				};
			}
		}

		if context.radio_connected && !radio_connected {
			std::mem::drop(context);

			self.write_radio_handshake();

			context = match self.context.try_lock() {
				Ok(context) => context,
				Err(_) => {
					println!("AMirror Process: Context Locked.");
					return;
				}
			};
		}

		if context.phone_name != phone_name {
			let mut phone_name_data = [0x23, 0x30].to_vec();
			let phone_name_bytes = context.phone_name.as_bytes();

			for i in 0..phone_name_bytes.len() {
				phone_name_data.push(phone_name_bytes[i]);
			}

			let phone_name_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: phone_name_data,
			};

			self.write_aibus_message(phone_name_msg);

			if context.audio_selected && context.audio_text {
				self.write_nav_text(context.phone_name.clone(), 2, 1, true);
			}

			if context.radio_connected {
				std::mem::drop(context);

				self.write_radio_name();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process: Context Locked.");
						return;
					}
				};
			}
		}

		if context.playing != playing {
			if context.audio_selected && context.audio_text {
				if context.playing {
					self.write_nav_text("#FWD".to_string(), 1, 1, true);
				} else {
					self.write_nav_text("||".to_string(), 1, 1, true);
				}
			}
		}
	
		if context.song_title != song_title {
			if context.audio_selected {
				self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
				if context.audio_text {
					self.write_nav_text(context.song_title.clone(), 1, 0, true);
				}
			}
		}

		if context.artist != artist {
			if context.audio_selected {
				self.write_radio_metadata(context.artist.clone(), ARTIST_NAME);
				if context.audio_text {
					self.write_nav_text(context.artist.clone(), 2, 0, true);
				}
			}
		}

		if context.album != album {
			if context.audio_selected {
				self.write_radio_metadata(context.album.clone(), ALBUM_NAME);
				if context.audio_text {
					self.write_nav_text(context.album.clone(), 3, 0, true);
				}
			}
		}

		if context.app != app {
			if context.audio_selected {
				self.write_radio_metadata(context.app.clone(), APP_NAME);
				if context.audio_text {
					self.write_nav_text(context.app.clone(), 4, 0, true);
				}
			}
		}

		if context.night != night {
			if context.night {
				self.handler.send_carplay_command(PHONE_COMMAND_NIGHT);
			} else {
				self.handler.send_carplay_command(PHONE_COMMAND_DAY);
			}
		}

		if Instant::now() - self.source_request_timer > Duration::from_millis(5000) {
			self.source_request_timer = Instant::now();

			let mut control_req = 0x0;
			if context.phone_active {
				control_req |= 0x10;
			}
			if context.audio_selected {
				control_req |= 0x80;
			}

			if control_req != 0 {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_NAV_SCREEN,
					data: [0x77, AIBUS_DEVICE_AMIRROR, control_req].to_vec(),
				});
			}
		}
	}

	pub fn handle_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Handle AIBus Message: Context Locked");
				return;
			}
		};

		if ai_msg.sender == AIBUS_DEVICE_RADIO && !context.radio_connected {
			context.radio_connected = true;
		}

		if ai_msg.receiver != AIBUS_DEVICE_AMIRROR && ai_msg.receiver != 0xFF {
			return;
		}

		if ai_msg.receiver == AIBUS_DEVICE_AMIRROR && ai_msg.l() >= 1 && ai_msg.data[0] != 0x80 {
			let mut ack_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: ai_msg.sender,
				data: Vec::new(),
			};
			ack_msg.data.push(0x80);

			self.write_aibus_message(ack_msg);
		}

		if ai_msg.sender == AIBUS_DEVICE_RADIO {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x40 && ai_msg.data[1] == 0x10 { //Source change.
				let new_device = ai_msg.data[2];
				if new_device == AIBUS_DEVICE_AMIRROR { //Selected!
					context.audio_selected = true;
					self.handler.send_carplay_command(PHONE_COMMAND_PLAY);
					
					std::mem::drop(context);
					self.write_all_metadata();
					
					context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							println!("AMirror Handle AIBus Message: Context Locked");
							return;
						}
					};

					let mut control_req = 0x80;
					if context.phone_active {
						control_req |= 0x10;
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_AMIRROR, control_req].to_vec(),
					});
					
				} else { //Deselected!
					self.handler.send_carplay_command(PHONE_COMMAND_PAUSE);
					context.audio_selected = false;
					context.audio_text = false;

					if new_device != 0 {
						self.write_aibus_message(AIBusMessage {
							sender: AIBUS_DEVICE_AMIRROR,
							receiver: AIBUS_DEVICE_NAV_SCREEN,
							data: [0x77, new_device, 0x80].to_vec(),
						});
					}
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x40 && ai_msg.data[1] == 0x1 { //Text control change.
				let new_device = ai_msg.data[2];
				if new_device == AIBUS_DEVICE_AMIRROR { //Text control!
					context.audio_text = true;
					
					std::mem::drop(context);
					self.write_all_text();
					
					context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							println!("AMirror Handle AIBus Message: Context Locked");
							return;
						}
					};
				} else {
					context.audio_text = false;
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x30 { //Control.
				if ai_msg.data[1] == 0x0 { //Status query.

				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_NAV_COMPUTER {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x48 && ai_msg.data[1] == 0x8E { //Turn on/off the interface.
				//self.handler.set_minimize(ai_msg.data[2] == 0);
				context.phone_active = ai_msg.data[2] != 0;

				if context.phone_active {
					let mut control_req = 0x10;
					if context.audio_selected {
						control_req |= 0x80;
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_AMIRROR, control_req].to_vec(),
					});
				} else {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_NAV_COMPUTER, 0x10].to_vec(), //TODO: If another device has requested control, send that instead.
					});
				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_IMID {
			if ai_msg.l() >= 2 && ai_msg.data[0] == 0x3B {
				if ai_msg.data[1] == 0x23 && ai_msg.l() >= 4 { //Character count.
					context.imid_text_len = ai_msg.data[2];
					context.imid_row_count = ai_msg.data[3];
				} else if ai_msg.data[1] == 0x57 && ai_msg.l() >= 3 { //Supported device list.
					context.imid_native_mirror = false;
					for i in 2..ai_msg.l() {
						if ai_msg.data[i] == 0x8E {
							context.imid_native_mirror = true;
							break;
						}
					}
				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_CANSLATOR {
			if ai_msg.receiver == 0xFF && ai_msg.l() >= 1 && ai_msg.data[0] == 0xA1 {
				if ai_msg.l() >= 4 && ai_msg.data[1] == 0x10 { //Light status message.
					context.night = (ai_msg.data[3]&0x80) != 0;
				}
			}
		}

		std::mem::drop(context);
		self.handler.handle_aibus_message(ai_msg);
	}

	fn write_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut stream = match self.stream.try_lock() {
			Ok(stream) => stream,
			Err(_) => {
				println!("AMirror Write AIBus Message: Stream Locked");
				return;
			}
		};

		let msg_copy = ai_msg.clone();
		write_aibus_message(&mut stream, ai_msg);

		if msg_copy.l() >=1 && msg_copy.data[0] != 0x80 && msg_copy.receiver != 0xFF && msg_copy.receiver != 0x10 { //TODO: Cancel the 0x10 if the radio is connected.
			let mut ack = false;
			let mut num_tries = 0;
			let mut last_try = Instant::now();
			while !ack && num_tries < 15 {
				let mut msg = SocketMessage {
					opcode: 0,
					data: Vec::new(),
				};

				if read_socket_message(&mut stream, &mut msg) > 0 {
					if msg.opcode != OPCODE_AIBUS_RECV {
						continue;
					}
		
					let rx_msg = get_aibus_message(msg.data);
					if rx_msg.receiver == msg_copy.sender && rx_msg.sender == msg_copy.receiver && rx_msg.l() >= 1 && rx_msg.data[0] == 0x80 {
						ack = true;
					}
				}

				if !ack && Instant::now() - last_try > Duration::from_millis(100) {
					last_try = Instant::now();
					let resend = msg_copy.clone();
					write_aibus_message(&mut stream, resend);
					num_tries += 1;
				}
			}
		}
	}

	//Write metadata.
	fn write_metadata(&mut self, recipient: u8, text: String, position: u8) {
		if position > 3 {
			return;
		}

		let mut meta_data = [0x23, 0x60|(position&0xF)].to_vec();
		let text_bytes = text.as_bytes();

		for i in 0..text_bytes.len() {
			meta_data.push(text_bytes[i]);
		}

		let meta_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: recipient,
			data: meta_data,
		};

		self.write_aibus_message(meta_msg);
	}

	//Write metadata to the radio.
	fn write_radio_metadata(&mut self, text: String, position: u8) {
		self.write_metadata(AIBUS_DEVICE_RADIO, text, position);
	}

	//Write the radio handshake.
	fn write_radio_handshake(&mut self) {
		let handshake_data = [0x1, 0x1, AIBUS_DEVICE_AMIRROR].to_vec();
		let handshake_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_RADIO,
			data: handshake_data,
		};

		self.write_aibus_message(handshake_msg);
		self.write_radio_name();
	}

	//Write the device name.
	fn write_radio_name(&mut self) {
		let mut name_data = [0x1, 0x23, 0x0].to_vec();

		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Write Radio Name: Context Locked.");
				return;
			}
		};

		let mut name_str = "Phone Mirror".to_string();

		if context.phone_type == 3 {
			name_str = "Carplay: ".to_string() + &context.phone_name;
		} else if context.phone_type == 5 {
			name_str = "Android Auto: ".to_string() + &context.phone_name;
		}

		let name_bytes = name_str.as_bytes();
		for i in 0..name_bytes.len() {
			name_data.push(name_bytes[i]);
		}

		let name_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_RADIO,
			data: name_data,
		};

		self.write_aibus_message(name_msg);
	}

	//Write metadata to the nav computer.
	fn write_nav_text(&mut self, text: String, position: u8, subtitle: u8, refresh: bool) {
		let mut meta_data = [0x23, 0x60|(subtitle&0xF), position].to_vec();
		
		if refresh {
			meta_data[1] |= 0x10;
		}

		let text_bytes = text.as_bytes();

		for i in 0..text_bytes.len() {
			meta_data.push(text_bytes[i]);
		}

		let meta_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: meta_data,
		};

		self.write_aibus_message(meta_msg);
	}

	//Write text to the IMID.
	fn write_imid_text(&mut self, text: String, xpos: u8, ypos: u8) {
		let mut meta_data = [0x23, 0x60, xpos, ypos].to_vec();

		let text_bytes = text.as_bytes();

		for i in 0..text_bytes.len() {
			meta_data.push(text_bytes[i]);
		}

		let meta_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_IMID,
			data: meta_data,
		};

		self.write_aibus_message(meta_msg);
	}
	
	//Write all metadata to the radio after being selected.
	fn write_all_metadata(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Write All Metadata: Context Locked.");
				return;
			}
		};
		
		self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
		self.write_radio_metadata(context.artist.clone(), ARTIST_NAME);
		self.write_radio_metadata(context.album.clone(), ALBUM_NAME);
		self.write_radio_metadata(context.app.clone(), APP_NAME);
	}

	//Write all relevant text to the nav screen.
	fn write_all_text(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Write All Text: Context Locked.");
				return;
			}
		};
		
		if context.phone_type == 0 {
			self.write_nav_text("Mirror".to_string(), 0, 0, true);
		} else if context.phone_type == 3 {
			self.write_nav_text("Carplay".to_string(), 0, 0, true);
		} else if context.phone_type == 5 {
			self.write_nav_text("Android".to_string(), 0, 0, true);
		}
		
		self.write_nav_text(context.phone_name.clone(), 2, 1, false);
		self.write_nav_text(context.song_title.clone(), 1, 0, false);
		self.write_nav_text(context.artist.clone(), 2, 0, false);
		self.write_nav_text(context.album.clone(), 3, 0, false);
		self.write_nav_text(context.app.clone(), 4, 0, true);

		if context.playing {
			self.write_nav_text("#FWD".to_string(), 1, 1, true);
		} else {
			self.write_nav_text("||".to_string(), 1, 1, true);
		}
	}
}
