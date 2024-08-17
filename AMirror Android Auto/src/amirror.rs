use std::os::unix::net::UnixStream;
use std::sync::{Arc, Mutex};

use crate::mirror::messages::*;
use crate::{aibus::*, write_aibus_message};
use crate::context::Context;
use crate::mirror::handler::*;

const APP_NAME: u8 = 0x0;
const SONG_NAME: u8 = 0x1;
const ARTIST_NAME: u8 = 0x2;
const ALBUM_NAME: u8 = 0x3;
pub struct AMirror<'a> {
	context: &'a Arc<Mutex<Context>>,
	stream: &'a Arc<Mutex<UnixStream>>,
	handler: MirrorHandler<'a>,
}

impl <'a> AMirror<'a> {
	pub fn new(context: &'a Arc<Mutex<Context>>, stream: &'a Arc<Mutex<UnixStream>>, w: u16, h: u16) -> Self {
		let mutex_mirror_handler = MirrorHandler::new(context, w, h);

		return Self{
			context,
			stream,
			handler: mutex_mirror_handler
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
					self.write_nav_text("Phone Mirror".to_string(), 0, 0, true);
				} else if context.phone_type == 3 {
					self.write_nav_text("Carplay".to_string(), 0, 0, true);
				} else if context.phone_type == 5 {
					self.write_nav_text("Android".to_string(), 0, 0, true);
				}
			}
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
		}
	
		if context.song_title != song_title {
			if context.audio_selected {
				self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
				if context.audio_text {
					self.write_nav_text(context.song_title.clone(), 1, 0, context.artist != artist || context.album != album || context.app != app);
				}
			}
		}

		if context.artist != artist {
			if context.audio_selected {
				self.write_radio_metadata(context.artist.clone(), ARTIST_NAME);
				if context.audio_text {
					self.write_nav_text(context.artist.clone(), 2, 0, context.album != album || context.app != app);
				}
			}
		}

		if context.album != album {
			if context.audio_selected {
				self.write_radio_metadata(context.album.clone(), ALBUM_NAME);
				if context.audio_text {
					self.write_nav_text(context.album.clone(), 3, 0, context.app != app);
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
	}

	pub fn handle_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Handle AIBus Message: Context Locked");
				return;
			}
		};

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
					
				} else { //Deselected!
					self.handler.send_carplay_command(PHONE_COMMAND_PAUSE);
					context.audio_selected = false;
					context.audio_text = false;
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

		write_aibus_message(&mut stream, ai_msg);
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
			self.write_nav_text("Phone Mirror".to_string(), 0, 0, true);
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
	}
}
