use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use image::Rgba;

use crate::aap::aa_handler::AapHandler;
use crate::aibus_handler::AIBusHandler;
use crate::mirror::messages::*;
use crate::mirror::mpv::{MpvVideo, RdAudio};
use crate::aibus::*;
use crate::context::Context;
use crate::mirror::handler::*;

use crate::text_split::split_text;

const APP_NAME: u8 = 0x4;
const SONG_NAME: u8 = 0x1;
const ARTIST_NAME: u8 = 0x2;
const ALBUM_NAME: u8 = 0x3;

const MENU_NONE: u8 = 0;
const MENU_SETTINGS: u8 = 1;
const MENU_DISPLAY: u8 = 2;

pub struct AMirror<'a> {
	context: &'a Arc<Mutex<Context>>,
	mpv_video: &'a Arc<Mutex<MpvVideo>>,

	dongle_handler: MirrorHandler<'a>,
	aa_handler: AapHandler<'a>,

	aibus_handler: Arc<Mutex<AIBusHandler>>,

	display_title: bool,
	display_artist: bool,
	display_album: bool,
	display_app: bool,
	display_phone: bool,
	
	imid_scroll: i8,
	imid_split: bool,
	imid_scroll_pos: usize,
	imid_refresh: bool,

	auto_music_start: bool,
	auto_mirror_start: bool,

	local_radio_connected: bool,
	local_imid_native: bool,
	local_imid_text_len: u8,
	local_imid_rows: u8,

	menu_open: u8,

	w: u16,

	source_request_timer: Instant,
	scroll_timer: Instant,

	overlay_timer: Instant,
	overlay_on: bool,

	audio_held: bool,

	pub run: bool,
}

impl <'a> AMirror<'a> {
	pub fn new(context: &'a Arc<Mutex<Context>>, aibus_handler: Arc<Mutex<AIBusHandler>>, mpv_video: &'a Arc<Mutex<MpvVideo>>, rd_audio: &'a Arc<Mutex<RdAudio>>, nav_audio: &'a Arc<Mutex<RdAudio>>, w: u16, h: u16) -> Self {
		let mutex_mirror_handler = MirrorHandler::new(context, mpv_video, rd_audio, nav_audio, w, h);
		let mutex_aa_handler = AapHandler::new(context, mpv_video, rd_audio, nav_audio, w, h);

		return Self{
			context,
			mpv_video,

			dongle_handler: mutex_mirror_handler,
			aa_handler: mutex_aa_handler,

			aibus_handler,

			display_title: false,
			display_artist: false,
			display_album: false,
			display_app: false,
			display_phone: false,

			auto_music_start: false, //TODO: Load this in from an external file.
			auto_mirror_start: true, //TODO: Ditto.

			imid_scroll: -1,
			imid_split: true,
			imid_scroll_pos: 0,
			imid_refresh: false,

			local_radio_connected: false,
			local_imid_native: false,
			local_imid_text_len: 0,
			local_imid_rows: 0,

			menu_open: MENU_NONE,

			w,

			source_request_timer: Instant::now(),
			scroll_timer: Instant::now(),

			overlay_timer: Instant::now(),
			overlay_on: false,

			audio_held: false,

			run: true,
		};
	}

	pub fn process(&mut self) {
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Process Initial: Context Locked.");
				return;
			}
		};

		let phone_type = context.phone_type;
		let phone_name = context.phone_name.clone();

		let song_title = context.song_title.clone();
		let artist = context.artist.clone();
		let album = context.album.clone();
		let app = context.app.clone();

		let track_time = context.track_time;

		let playing = context.playing;
		let phone_active = context.phone_active;

		let mut change_imid = false;

		std::mem::drop(context);

		self.dongle_handler.process();
		self.aa_handler.process();

		context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Process Post Handler: Context Locked.");
				return;
			}
		};

		if context.phone_active != phone_active || context.home_held {
			context.home_held = false;
			self.dongle_handler.set_minimize(!context.phone_active);

			let mut phone_connect_byte = 0x1;
			if !context.phone_active {
				phone_connect_byte = 0x0;
			}

			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: [0x48, 0x8e, phone_connect_byte].to_vec(),
			});

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

			self.source_request_timer = Instant::now();
		}
		
		if context.phone_type != phone_type {
			let phone_type_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: [0x30, context.phone_type].to_vec(),
			};

			if context.phone_type == 0 {
				context.phone_name = "".to_string();
				context.song_title = "".to_string();
				context.artist = "".to_string();
				context.album = "".to_string();
				context.app = "".to_string();

				context.playing = false;

				if context.phone_active {
					match self.mpv_video.try_lock() {
						Ok(mut mpv_video) => {
							mpv_video.set_minimize(true);
						}
						Err(_) => {
							println!("AMirror Process: MPV handler locked.")
						}
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_COMPUTER,
						data: [0x48, AIBUS_DEVICE_AMIRROR, 0x0].to_vec(),
					});
				}
			} else {
				if context.night {
					self.dongle_handler.send_carplay_command(PHONE_COMMAND_NIGHT);
				} else {
					self.dongle_handler.send_carplay_command(PHONE_COMMAND_DAY);
				}

				if self.auto_mirror_start && context.phone_type != 0 {
					context.phone_active = true;

					match self.mpv_video.try_lock() {
						Ok(mut mpv_video) => {
							mpv_video.set_minimize(false);
						}
						Err(_) => {
							println!("AMirror Process: MPV handler locked.")
						}
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_COMPUTER,
						data: [0x48, AIBUS_DEVICE_AMIRROR, 0x1].to_vec(),
					});
				}
			}

			self.write_aibus_message(phone_type_msg);
			
			if context.audio_text && context.imid_native_mirror {
				let phone_type_msg = AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_IMID,
					data: [0x30, context.phone_type].to_vec(),
				};

				self.write_aibus_message(phone_type_msg);
			}

			if context.audio_selected && context.audio_text {
				if context.phone_type == 0 {
					self.write_nav_text("Mirror".to_string(), 0, 0, true);
				} else if context.phone_type == 3 {
					self.write_nav_text("Carplay".to_string(), 0, 0, true);
				} else if context.phone_type == 5 {
					self.write_nav_text("Android".to_string(), 0, 0, true);
				}

				std::mem::drop(context);

				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process Phone Type: Context Locked.");
						return;
					}
				};
			}

			match self.mpv_video.try_lock() {
				Ok(mut mpv_video) => {
					self.overlay_on = false;
					mpv_video.clear_overlay();
				}
				Err(_) => {
					println!("AMirror Process Phone Type: MPV Locked.");
				}
			}

			context.track_time = -1;

			if context.radio_connected {
				std::mem::drop(context);

				self.write_radio_name();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process Phone Type: Context Locked.");
						return;
					}
				};
			}

			if self.auto_music_start && context.phone_type != 0 {
				if !context.audio_selected {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_RADIO,
						data: [0x10, 0x10, AIBUS_DEVICE_AMIRROR].to_vec(),					
					});
				}

				if context.phone_type == 3 {
					self.dongle_handler.send_carplay_command(PHONE_COMMAND_PLAY);
				} else if context.phone_type == 5 {
					self.aa_handler.start_stop_audio(true);
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
				data: phone_name_data.clone(),
			};

			self.write_aibus_message(phone_name_msg);

			if context.audio_selected && context.audio_text {
				self.write_nav_text(context.phone_name.clone(), 2, 1, true);

				std::mem::drop(context);

				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process Phone Name: Context Locked.");
						return;
					}
				};
			}

			if context.radio_connected {
				std::mem::drop(context);

				self.write_radio_name();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process Phone Name: Context Locked.");
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
					context.track_time = -1;
				}
			}
		}
	
		if context.song_title != song_title {
			if context.audio_selected {
				self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
				if context.audio_text {
					self.write_nav_text(context.song_title.clone(), 1, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 {
						self.write_metadata(AIBUS_DEVICE_IMID, context.song_title.clone(), SONG_NAME);
					} else if context.imid_row_count > 0 && context.imid_text_len > 0 && context.phone_type != 0 {
						if self.display_title && self.imid_scroll < 0 {
							let mut imid_x = (context.imid_text_len/2) as isize - (context.song_title.len()/2) as isize;
							if imid_x < 0 || imid_x > context.imid_text_len as isize {
								imid_x = 0;
							}

							let mut imid_y = 1;
							if self.display_phone {
								imid_y += 1;
							}
	
							if imid_y <= context.imid_row_count {
								self.write_imid_text(context.song_title.clone(), imid_x as u8, imid_y);
							}
						} else if self.imid_scroll >= 0 {
							change_imid = true;
						}
					}
				}
			}
		}

		if context.artist != artist {
			if context.audio_selected {
				self.write_radio_metadata(context.artist.clone(), ARTIST_NAME);
				if context.audio_text {
					self.write_nav_text(context.artist.clone(), 2, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 {
						self.write_metadata(AIBUS_DEVICE_IMID, context.artist.clone(), ARTIST_NAME);
					} else if context.imid_row_count > 0 && context.imid_text_len > 0 && context.phone_type != 0 {
						if self.display_artist && self.imid_scroll < 0 {
							let mut imid_x = (context.imid_text_len/2) as isize - (context.artist.len()/2) as isize;
							if imid_x < 0 || imid_x > context.imid_text_len as isize {
								imid_x = 0;
							}

							let mut imid_y = 1;
							if self.display_phone {
								imid_y += 1;
							}
							if self.display_title {
								imid_y += 1;
							}
	
							if imid_y <= context.imid_row_count {
								self.write_imid_text(context.artist.clone(), imid_x as u8, imid_y);
							}
						} else if self.imid_scroll >= 0 {
							change_imid = true;
						}
					}
				}
			}
		}

		if context.album != album {
			if context.audio_selected {
				self.write_radio_metadata(context.album.clone(), ALBUM_NAME);
				if context.audio_text {
					self.write_nav_text(context.album.clone(), 3, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 {
						self.write_metadata(AIBUS_DEVICE_IMID, context.album.clone(), ALBUM_NAME);
					} else if context.imid_row_count > 0 && context.imid_text_len > 0 && context.phone_type != 0 {
						if self.display_album && self.imid_scroll < 0 {
							let mut imid_x = (context.imid_text_len/2) as isize - (context.album.len()/2) as isize;
							if imid_x < 0 || imid_x > context.imid_text_len as isize {
								imid_x = 0;
							}

							let mut imid_y = 1;
							if self.display_phone {
								imid_y += 1;
							}
							if self.display_title {
								imid_y += 1;
							}
							if self.display_album {
								imid_y += 1;
							}
	
							if imid_y <= context.imid_row_count {
								self.write_imid_text(context.album.clone(), imid_x as u8, imid_y);
							}
						} else if self.imid_scroll >= 0 {
							change_imid = true;
						}
					}
				}
			}
		}

		if context.app != app {
			if context.audio_selected {
				self.write_radio_metadata(context.app.clone(), APP_NAME);
				if context.audio_text {
					self.write_nav_text(context.app.clone(), 4, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 {
						self.write_metadata(AIBUS_DEVICE_IMID, context.app.clone(), APP_NAME);
					} else if context.imid_row_count > 0 && context.imid_text_len > 0 && context.phone_type != 0 {
						if self.display_app && self.imid_scroll < 0 {
							let mut imid_x = (context.imid_text_len/2) as isize - (context.app.len()/2) as isize;
							if imid_x < 0 || imid_x > context.imid_text_len as isize {
								imid_x = 0;
							}

							let mut imid_y = 1;
							if self.display_phone {
								imid_y += 1;
							}
							if self.display_title {
								imid_y += 1;
							}
							if self.display_album {
								imid_y += 1;
							}
							if self.display_app {
								imid_y += 1;
							}
	
							if imid_y <= context.imid_row_count {
								self.write_imid_text(context.app.clone(), imid_x as u8, imid_y);
							}
						} else if self.imid_scroll >= 0 {
							change_imid = true;
						}
					}
				}
			}
		}

		if context.track_time != track_time {
			if context.audio_selected {
				if context.audio_text {
					if context.track_time >= 0 {
						let min = context.track_time/60;
						let sec = context.track_time%60;

						let mut time_text = min.to_string() + ":";
						if sec >= 10 {
							time_text += &sec.to_string();
						} else {
							time_text += "0";
							time_text += &sec.to_string();
						}

						self.write_nav_text(time_text, 0, 1, true);
					} else {
						let clear_data = [0x20, 0x71, 0x0].to_vec();
						self.write_aibus_message(AIBusMessage {
							sender: AIBUS_DEVICE_AMIRROR,
							receiver: AIBUS_DEVICE_NAV_COMPUTER,
							data: clear_data,
						});
					}
				}
			}
		}

		self.local_radio_connected = context.radio_connected;
		self.local_imid_native = context.imid_native_mirror;
		self.local_imid_rows = context.imid_row_count;
		self.local_imid_text_len = context.imid_text_len;

		if Instant::now() - self.source_request_timer > Duration::from_millis(5000) {
			self.source_request_timer = Instant::now();

			let mut control_req = 0x0;
			if context.phone_active && context.phone_type != 0 {
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
			
			if !context.radio_connected {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_RADIO,
					data: [0x1].to_vec(),
				});
			}
			
			if !context.imid_native_mirror && context.imid_row_count <= 0 && context.imid_text_len <= 0 {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_IMID,
					data: [0x4, 0xE6, 0x3B].to_vec(),
				});
			}
		}

		let mut scroll_limit = 300;
		if self.imid_split {
			scroll_limit = 3000;
		} else if self.imid_scroll_pos == 0 { 
			scroll_limit = 750;
		}

		if self.imid_scroll >= 0 && context.phone_type != 0 && (Instant::now() - self.scroll_timer > Duration::from_millis(scroll_limit) || change_imid || self.imid_refresh) {
			if context.audio_text && context.imid_row_count > 0 && context.imid_text_len > 0 {
				let mut scroll_param = "".to_string();

				if self.imid_scroll == 0 {
					scroll_param = context.phone_name.clone();
				} else if self.imid_scroll == 1 {
					scroll_param = context.song_title.clone();
				} else if self.imid_scroll == 2 {
					scroll_param = context.artist.clone();
				} else if self.imid_scroll == 3 {
					scroll_param = context.album.clone();
				} else if self.imid_scroll == 4 {
					scroll_param = context.app.clone();
				}

				let ascii = scroll_param.is_ascii();
				if !ascii {
					let scroll_param_copy = scroll_param.clone();
					let scroll_bytes = scroll_param_copy.as_bytes();
					scroll_param = "".to_string();
					for i in 0..scroll_bytes.len() {
						if scroll_bytes[i] >= b' ' {
							let char_byte = match String::from_utf8([scroll_bytes[i]].to_vec()) {
								Ok(b) => b,
								Err(_) => {
									continue;
								}
							};
							scroll_param += char_byte.as_str();
						}
					}
				}

				let last_scroll_pos = self.imid_scroll_pos;
				let update = change_imid || self.imid_refresh;

				if Instant::now() - self.scroll_timer > Duration::from_millis(scroll_limit) {
					self.imid_scroll_pos += 1;
					self.scroll_timer = Instant::now();
				} else if change_imid || self.imid_refresh {
					self.imid_scroll_pos = 0;
				}

				if self.imid_refresh {
					self.imid_refresh = false;
				}

				let mut imid_y = 1;
				if context.imid_row_count >= 2 {
					imid_y = context.imid_row_count/2;
				}

				if self.imid_split {
					let scroll_split = split_text(context.imid_text_len as usize, &scroll_param);
					if self.imid_scroll_pos >= scroll_split.len() {
						self.imid_scroll_pos = 0;
					}

					if scroll_split[self.imid_scroll_pos].len() == 0 {
						self.imid_scroll_pos = 0;
					}

					let mut imid_x = 0;
					if context.imid_text_len >= 10 {
						imid_x = (context.imid_text_len/2) as isize - (context.phone_name.len()/2) as isize;
						if imid_x < 0 || imid_x > context.imid_text_len as isize {
							imid_x = 0;
						}
					}

					if self.imid_scroll_pos != last_scroll_pos || update {
						self.write_imid_text(scroll_split[self.imid_scroll_pos].clone(), imid_x as u8, imid_y);
					}
				} else {
					if (scroll_param.len() as isize - self.imid_scroll_pos as isize) < (context.imid_text_len as isize) {
						self.imid_scroll_pos = 0;
					}

					if self.imid_scroll_pos != last_scroll_pos || update {
						if scroll_param.len() > context.imid_text_len as usize {
							let scroll_string = scroll_param[self.imid_scroll_pos..self.imid_scroll_pos + context.imid_text_len as usize].to_string();
							self.write_imid_text(scroll_string, 0, imid_y);
						} else {
							self.write_imid_text(scroll_param, 0, imid_y);
						}
					}
				}
			}
		}

		if self.imid_refresh && self.imid_scroll < 0 {
			self.imid_refresh = false;
			if context.audio_text {
				std::mem::drop(context);
				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process: Context Locked");
						return;
					}
				};
			}
		}

		if Instant::now() - self.overlay_timer > Duration::from_millis(3500) && self.overlay_on {
			self.overlay_on = false;
			match self.mpv_video.try_lock() {
				Ok(mut mpv_video) => {
					mpv_video.clear_overlay();
				}
				Err(_) => {
					println!("AMirror Process: MPV Locked.")
				}
			}
		}

		std::mem::drop(context);
		let mut ai_rx_list = Vec::new();

		match self.aibus_handler.try_lock() {
			Ok(mut aibus_handler) => {
				let ai_rx = aibus_handler.get_ai_rx();
				for ai_data in &mut *ai_rx {
					ai_rx_list.push(ai_data.clone());
				}

				ai_rx.clear();
			}
			Err(_) => {
				
			}
		}

		for ai_data in ai_rx_list {
			self.handle_aibus_message(ai_data);
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

			std::mem::drop(context);

			self.write_radio_handshake();

			context = match self.context.try_lock() {
				Ok(context) => context,
				Err(_) => {
					println!("AMirror Process Radio Connection: Context Locked.");
					return;
				}
			};
		}

		if ai_msg.receiver != AIBUS_DEVICE_AMIRROR && ai_msg.receiver != 0xFF {
			return;
		}

		/*if ai_msg.receiver == AIBUS_DEVICE_AMIRROR && ai_msg.l() >= 1 && ai_msg.data[0] != 0x80 {
			let mut ack_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: ai_msg.sender,
				data: Vec::new(),
			};
			ack_msg.data.push(0x80);

			self.write_aibus_message(ack_msg);
		}*/

		if ai_msg.l() >= 2 && ai_msg.data[0] == 0x23 { //Overlay.
			let set_overlay = (ai_msg.data[1]&0x10) != 0;

			let text;
			if ai_msg.l() >= 3 {
				text = match std::str::from_utf8(&ai_msg.data[2..]) {
					Ok(text) => text,
					Err(e) => {
						println!("Error: {}", e);
						return;
					}
				};
			} else {
				text = "";
			}

			let index = (ai_msg.data[1]&0xF) as usize;

			match self.mpv_video.try_lock() {
				Ok(mut mpv) => {		
					mpv.set_overlay_text(text.to_string(), index);

					if set_overlay {
						self.overlay_on = true;
						self.overlay_timer = Instant::now();
						mpv.show_overlay();
					} else if self.overlay_on {
						mpv.show_overlay();
					}
				}
				Err(_) => {
					println!("Overlay: MPV handler locked.");
				}
			}
		} else if ai_msg.l() >= 5 && ai_msg.data[0] == 0x60 { //Color change.
			let color = Rgba([ai_msg.data[2], ai_msg.data[3], ai_msg.data[4], 0xFF]);
			let mut mpv = match self.mpv_video.try_lock() {
				Ok(mpv) => mpv,
				Err(_) => {
					println!("Color changed: MPV handler locked.");
					return;
				}
			};

			if ai_msg.data[1] == 0x21 { //Text color.
				mpv.set_overlay_text_color(color);
			} else if ai_msg.data[1] == 0x22 { //Background color.
				mpv.set_header_text_color(color);
			}

			if self.overlay_on {
				mpv.show_overlay();
			}
		} else if ai_msg.sender == AIBUS_DEVICE_RADIO {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x4 && ai_msg.data[1] == 0xE6 && ai_msg.data[2] == 0x10 { //Name request.
				std::mem::drop(context);

				self.write_radio_handshake();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Process Radio Connection: Context Locked.");
						return;
					}
				};
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x40 && ai_msg.data[1] == 0x10 { //Source change.
				let new_device = ai_msg.data[2];
				if new_device == AIBUS_DEVICE_AMIRROR { //Selected!
					if context.imid_row_count != 1 || context.imid_native_mirror {
						self.imid_scroll = -1;
					} else {
						self.imid_scroll = 1;
					}
					self.imid_refresh = true;
					self.imid_scroll_pos = 0;
					self.scroll_timer = Instant::now();
					context.audio_selected = true;
					
					if context.phone_type != 0 {
						self.dongle_handler.send_carplay_command(PHONE_COMMAND_PLAY);
						self.aa_handler.start_stop_audio(true);
					}
					
					std::mem::drop(context);
					self.write_all_metadata();
					
					context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							println!("AMirror Handle AIBus Message Source Change: Context Locked");
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
					self.imid_scroll = -1;
					self.imid_refresh = true;

					if context.phone_type != 0 {
						self.dongle_handler.send_carplay_command(PHONE_COMMAND_PAUSE);
						self.aa_handler.start_stop_audio(false);
					}
					
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
							println!("AMirror Handle AIBus Message Text Control: Context Locked");
							return;
						}
					};
				} else {
					context.audio_text = false;
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x30 { //Control.
				if ai_msg.data[1] == 0x0 { //Status query.
					//TODO: This.
				}
			} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x4A { //Create a settings menu.
				std::mem::drop(context);
				
				self.write_settings_menu();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						println!("AMirror Handle AIBus Message Create Menu: Context Locked");
						return;
					}
				};
			}
		} else if ai_msg.sender == AIBUS_DEVICE_NAV_COMPUTER {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x48 && ai_msg.data[1] == 0x8E { //Turn on/off the interface.
				self.dongle_handler.set_minimize(ai_msg.data[2] == 0 || context.phone_type == 0);
				context.phone_active = ai_msg.data[2] != 0;

				if context.phone_active && context.phone_type != 0 {
					let mut control_req = 0x10;
					if context.audio_selected {
						control_req |= 0x80;
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_AMIRROR, control_req].to_vec(),
					});

					if context.phone_req_off {
						context.phone_req_off = false;
						self.dongle_handler.send_carplay_command(PHONE_COMMAND_HOME);
					}
				} else {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_NAV_COMPUTER, 0x10].to_vec(), //TODO: If another device has requested control, send that instead.
					});
				}

				self.source_request_timer = Instant::now();
			} else if ai_msg.l() >= 1 && ai_msg.data[0] == 0x2B { //Menu operation.
				if ai_msg.l() >= 2 && ai_msg.data[1] == 0x40 { //Menu cleared.
					self.menu_open = MENU_NONE;
				} else if ai_msg.l() >= 3 && ai_msg.data[1] == 0x60 { //Entry selected.
					if self.menu_open == MENU_SETTINGS {
						let native_mirror = context.imid_native_mirror;
						let rows = context.imid_row_count;
						let char_count = context.imid_text_len;
						
						std::mem::drop(context);

						let option = ai_msg.data[2];
						if option == 1 && rows > 0 && char_count > 0 { //Open display menu.
							if !native_mirror {
								self.write_display_menu();
							}
						} else if option == 2 {
							self.auto_music_start = !self.auto_music_start;
							self.write_settings_menu_option(1);
						}

						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								println!("AMirror Handle AIBus Message Handle Menu Option: Context Locked");
								return;
							}
						};
					} else if self.menu_open == MENU_DISPLAY {
						std::mem::drop(context);

						let option = ai_msg.data[2];
						if option == 1 { //Display phone name.
							self.display_phone = !self.display_phone;
							self.write_display_menu_option(option - 1);	
						} else if option == 2 { //Display song title.
							self.display_title = !self.display_title;
							self.write_display_menu_option(option - 1);
						} else if option == 3 { //Display artist name.
							self.display_artist = !self.display_artist;
							self.write_display_menu_option(option - 1);
						} else if option == 4 { //Display album name.
							self.display_album = !self.display_album;
							self.write_display_menu_option(option - 1);
						} else if option == 5 { //Display app name.
							self.display_app = !self.display_app;
							self.write_display_menu_option(option - 1);
						} else if option == 6 { //Scroll.
							self.imid_split = !self.imid_split;
							self.write_display_menu_option(option - 1);
							self.imid_refresh = true;
							self.imid_scroll_pos = 0;
						}

						if option <= 5 {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									println!("AMirror Handle AIBus Message Handle Menu Option: Context Locked");
									return;
								}
							};

							let row_count = context.imid_row_count;
							let native_mirror = context.imid_native_mirror;
							std::mem::drop(context);

							if row_count == 1 && !native_mirror {
								self.imid_scroll = (option - 1) as i8;
								self.imid_refresh = true;
								self.imid_scroll_pos = 0;
							}

							let mut display_selected = 0;
							
							if self.display_phone {
								display_selected += 1;
							}
							if self.display_title {
								display_selected += 1;
							}
							if self.display_artist {
								display_selected += 1;
							}
							if self.display_album {
								display_selected += 1;
							}
							if self.display_app {
								display_selected += 1;
							}

							if display_selected > row_count {
								if option != 1 && display_selected > row_count && self.display_phone {
									self.display_phone = false;
									self.write_display_menu_option(0);
									display_selected -= 1;
								}
								if option != 2 && display_selected > row_count && self.display_title {
									self.display_title = false;
									self.write_display_menu_option(1);
									display_selected -= 1;
								}
								if option != 3 && display_selected > row_count && self.display_artist {
									self.display_artist = false;
									self.write_display_menu_option(2);
									display_selected -= 1;
								}
								if option != 4 && display_selected > row_count && self.display_album {
									self.display_album = false;
									self.write_display_menu_option(3);
									display_selected -= 1;
								}
								if option != 5 && display_selected > row_count && self.display_app {
									self.display_app = false;
									self.write_display_menu_option(4);
								}
							}

							self.write_all_imid_text();
						}

						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								println!("AMirror Handle AIBus Message Handle Menu Option: Context Locked");
								return;
							}
						};
					}
				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_IMID {
			if ai_msg.l() >= 2 && ai_msg.data[0] == 0x3B {
				if ai_msg.data[1] == 0x23 && ai_msg.l() >= 4 { //Character count.
					if context.imid_text_len != ai_msg.data[2] || context.imid_row_count != ai_msg.data[3] {
						context.imid_text_len = ai_msg.data[2];
						context.imid_row_count = ai_msg.data[3];

						if context.imid_row_count >= 5 {
							self.display_app = true;
							self.display_album = true;
							self.display_artist = true;
							self.display_title = true;
							self.display_phone = true;
						} else if context.imid_row_count >= 4 {
							self.display_album = true;
							self.display_artist = true;
							self.display_title = true;
							self.display_phone = true;
						} else if context.imid_row_count >= 3 {
							self.display_album = true;
							self.display_artist = true;
							self.display_title = true;
						} else if context.imid_row_count >= 2 {
							self.display_artist = true;
							self.display_title = true;
						} else if context.imid_row_count >= 1 {
							self.display_title = true;
						}
					}
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
					let night = context.night;
					context.night = (ai_msg.data[3]&0x80) != 0;

					if context.night != night {
						self.aa_handler.set_night_mode(context.night);
						if context.night {
							self.dongle_handler.send_carplay_command(PHONE_COMMAND_NIGHT);
						} else {
							self.dongle_handler.send_carplay_command(PHONE_COMMAND_DAY);
						}
					}
				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_NAV_SCREEN {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x30 {
				let button = ai_msg.data[1];
				let state = ai_msg.data[2]>>6;

				if button == 0x53 && state == 0x0 && context.audio_selected && context.audio_text { //Info button.
					if self.imid_scroll < 4 {
						self.imid_scroll += 1;
					} else {
						if context.imid_row_count != 1 || context.imid_native_mirror {
							self.imid_scroll = -1;
						} else {
							self.imid_scroll = 0;
						}
					}
					
					if context.imid_row_count == 1 {
						self.reset_imid_info_display();
					}

					self.imid_refresh = true;
					self.imid_scroll_pos = 0;
					self.scroll_timer = Instant::now();
				} else if button == 0x26 { //Audio button.
					if state == 0x2 {
						if !self.audio_held {
							if !context.audio_selected {
								match self.mpv_video.try_lock() {
									Ok(mut mpv) => {
										self.overlay_on = true;
										self.overlay_timer = Instant::now();
										mpv.show_overlay();
									}
									Err(_) => {
										println!("Audio button: MPV handler locked.");
									}
								}
							} else {
								self.aa_handler.show_audio_window();
							}
						} else {
							self.write_aibus_message(AIBusMessage {
								sender: AIBUS_DEVICE_AMIRROR,
								receiver: AIBUS_DEVICE_NAV_COMPUTER,
								data: [0x27, 0x30, 0x26].to_vec(),
							});
						}
						self.audio_held = false;
					} else if state == 0x1 {
						self.audio_held = true;
					}
				}
			}
		}

		std::mem::drop(context);
		self.dongle_handler.handle_aibus_message(ai_msg.clone());
		self.aa_handler.handle_aibus_message(ai_msg);
	}

	pub fn get_context(&mut self) -> &'a Arc<Mutex<Context>> {
		return self.context;
	}

	fn write_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				let ai_handler;
				loop {
					 ai_handler = match self.aibus_handler.try_lock() {
						Ok(aibus_handler) => aibus_handler,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				ai_handler
			}
		};

		let ai_tx = aibus_handler.get_ai_tx();
		ai_tx.push(ai_msg);

		/*let mut stream = match self.stream.try_lock() {
			Ok(stream) => stream,
			Err(_) => {
				println!("AMirror Write AIBus Message: Stream Locked");
				return;
			}
		};
		
		let radio_connected_local = self.local_radio_connected;
		let native_imid_local = self.local_imid_native;
		let rows_local = self.local_imid_rows;
		let text_len_local = self.local_imid_text_len;

		let msg_copy = ai_msg.clone();
		write_aibus_message(&mut stream, ai_msg);

		let mut send_ack = msg_copy.l() >=1 && msg_copy.data[0] != 0x80;
		if send_ack && msg_copy.receiver == 0xFF {
			send_ack = false;
		}
		if send_ack && msg_copy.receiver == AIBUS_DEVICE_RADIO && !radio_connected_local {
			send_ack = false;
		}
		if send_ack && msg_copy.receiver == AIBUS_DEVICE_IMID && !native_imid_local && rows_local <= 0 && text_len_local <= 0 {
			send_ack = false;
		}

		if send_ack { 
			let mut ack = false;
			let mut num_tries = 0;
			let mut last_try = Instant::now();
			while !ack && num_tries < 15 {
				let mut msg_list = Vec::new();

				if read_socket_message(&mut stream, &mut msg_list) > 0 {
					for i in 0..msg_list.len() {
						let msg = &msg_list[i];
						if msg.opcode != OPCODE_AIBUS_RECV {
							continue;
						}
			
						let rx_msg = get_aibus_message(msg.data.clone());
						if rx_msg.receiver == msg_copy.sender && rx_msg.sender == msg_copy.receiver && rx_msg.l() >= 1 && rx_msg.data[0] == 0x80 {
							ack = true;
						} else if rx_msg.receiver == AIBUS_DEVICE_AMIRROR {
							std::mem::drop(stream);
							self.write_aibus_message(AIBusMessage {
								sender: AIBUS_DEVICE_AMIRROR,
								receiver: rx_msg.sender,
								data: [0x80].to_vec(),
							});

							self.aibus_list.push(rx_msg);

							stream = match self.stream.try_lock() {
								Ok(stream) => stream,
								Err(_) => {
									println!("AMirror Write AIBus Message: Stream Locked");
									return;
								}
							};
						}
					}
				}

				if !ack && Instant::now() - last_try > Duration::from_millis(100) {
					last_try = Instant::now();
					let resend = msg_copy.clone();
					write_aibus_message(&mut stream, resend);
					num_tries += 1;
				}
			}
		}*/
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

		std::mem::drop(context);
		self.write_all_imid_text();
	}

	//Write all relevant text to the IMID.
	fn write_all_imid_text(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Write All IMID Text: Context Locked.");
				return;
			}
		};

		if context.imid_native_mirror {
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_IMID,
				data: [0x30, context.phone_type].to_vec(),
			});

			let mut phone_name_data = [0x23, 0x30].to_vec();
			let phone_name_bytes = context.phone_name.as_bytes();

			for i in 0..phone_name_bytes.len() {
				phone_name_data.push(phone_name_bytes[i]);
			}

			let phone_name_msg = AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_IMID,
				data: phone_name_data.clone(),
			};

			self.write_aibus_message(phone_name_msg);

			self.write_metadata(AIBUS_DEVICE_IMID, context.song_title.clone(), SONG_NAME);
			self.write_metadata(AIBUS_DEVICE_IMID, context.artist.clone(), ARTIST_NAME);
			self.write_metadata(AIBUS_DEVICE_IMID, context.album.clone(), ALBUM_NAME);
			self.write_metadata(AIBUS_DEVICE_IMID, context.app.clone(), APP_NAME);
		} else if context.imid_row_count > 0 && context.imid_text_len > 0 {
			if self.imid_scroll < 0 && context.phone_name.len() > 0 && context.phone_type != 0 {

				let mut row = 1;
				if self.display_phone {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.phone_name.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.phone_name.clone(), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_title {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.song_title.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.song_title.clone(), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_artist {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.artist.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.artist.clone(), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_album {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.album.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.album.clone(), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_app {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.app.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.app.clone(), imid_x as u8, row);
					}
				} else {
					row -= 1;
				}

				if row < context.imid_row_count {
					let mut row_msg = [0x20, 0x60].to_vec();

					for i in row+1..context.imid_row_count+1 {
						row_msg.push(i);
					}
	
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_IMID,
						data: row_msg,
					});
				}

			} else if context.phone_name.len() <= 0 {
				let mut clear_msg = [0x20, 0x60].to_vec();
				for i in 0..context.imid_row_count {
					clear_msg.push(i+1);
				}

				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_IMID,
					data: clear_msg,
				});

				if context.phone_type == 0 {
					let mut msg = "Phone Not Connected"; //Phone not connected.
					if context.imid_text_len < msg.len() as u8 {
						msg = "Phone N/C";
					}

					if context.imid_text_len < msg.len() as u8 {
						msg = "N/C";
					}

					let mut imid_x = (context.imid_text_len/2) as usize - msg.len()/2;
					if imid_x > context.imid_row_count as usize {
						imid_x = 0;
					}

					if context.imid_row_count == 1 {
						self.write_imid_text(msg.to_string(), imid_x as u8, 1);
					} else {
						self.write_imid_text(msg.to_string(), imid_x as u8, context.imid_row_count/2);
					}
				} else if context.phone_type == 3 { //Carplay.
					let msg = "CarPlay";

					let mut imid_x = (context.imid_text_len/2) as usize - msg.len()/2;

					if imid_x > context.imid_row_count as usize {
						imid_x = 0;
					}

					if context.imid_row_count == 1 {
						self.write_imid_text(msg.to_string(), imid_x as u8, 1);
					} else {
						let mut imid_y = 1;
						if self.display_phone && (context.song_title.len() <= 0 && context.artist.len() <= 0 && context.album.len() <= 0) {
							imid_y = context.imid_row_count/2;
						}
						self.write_imid_text(msg.to_string(), imid_x as u8, imid_y);
					}
				}
			}
		}
	}

	//Reset the info display if the info state changed.
	fn reset_imid_info_display(&mut self) {
		self.display_album = false;
		self.display_app = false;
		self.display_artist = false;
		self.display_title = false;
		self.display_phone = false;

		if self.imid_scroll == 0 {
			self.display_phone = true;
		} else if self.imid_scroll == 1 {
			self.display_title = true;
		} else if self.imid_scroll == 2 {
			self.display_artist = true;
		} else if self.imid_scroll == 3 {
			self.display_album = true;
		} else if self.imid_scroll == 4 {
			self.display_app = true;
		}
	}

	//Send the menu creation message and wait for denial. Return true if successful (i.e. not denied).
	fn create_menu(&mut self, title: String, size: u8) -> bool {
		let y_pos: u16 = 140;
		let menu_h: u16 = 35;
		
		let mut menu_req_data = [0x2B, 0x5A, size, size, 0, 0, (y_pos>>8) as u8, (y_pos&0xFF) as u8, (self.w>>8) as u8, (self.w&0xFF) as u8, (menu_h>>8) as u8, (menu_h&0xFF) as u8].to_vec();

		let menu_header_bytes = title.as_bytes();

		for i in 0..menu_header_bytes.len() {
			menu_req_data.push(menu_header_bytes[i]);
		}

		let ai_request = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: menu_req_data,
		};

		self.write_aibus_message(ai_request);

		let stream_timer = Instant::now();

		while Instant::now() - stream_timer < Duration::from_millis(50) {
		}

		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				let ai_handler;
				loop {
					match self.aibus_handler.try_lock() {
						Ok(handler) => {
							ai_handler = handler;
							break;
						}
						Err(_) => {
							continue;
						}
					}
				}
				ai_handler
			}
		};

		let rx_list = aibus_handler.get_ai_rx();
		for i in 0..rx_list.len() {
			let test_ai_msg = &rx_list[i];
			if test_ai_msg.receiver != AIBUS_DEVICE_AMIRROR {
				continue;
			}

			if test_ai_msg.sender == AIBUS_DEVICE_NAV_COMPUTER && test_ai_msg.l() >= 2 && test_ai_msg.data[0] == 0x2B && test_ai_msg.data[1] == 0x40 {
				rx_list.remove(i);
				return false;
			}
		}

		std::mem::drop(aibus_handler);

		return true;
	}

	//Clear the active menu.
	fn clear_menu(&mut self) -> bool {
		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x4A].to_vec(),
		});

		let stream_timer = Instant::now();

		while Instant::now() - stream_timer < Duration::from_millis(100) {
		}

		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				let ai_handler;
				loop {
					match self.aibus_handler.try_lock() {
						Ok(handler) => {
							ai_handler = handler;
							break;
						}
						Err(_) => {
							continue;
						}
					}
				}
				ai_handler
			}
		};

		let rx_list = aibus_handler.get_ai_rx();
		for i in 0..rx_list.len() {
			let test_ai_msg = &rx_list[i];
			if test_ai_msg.receiver != AIBUS_DEVICE_AMIRROR {
				continue;
			}

			if test_ai_msg.sender == AIBUS_DEVICE_NAV_COMPUTER && test_ai_msg.l() >= 2 && test_ai_msg.data[0] == 0x2B && test_ai_msg.data[1] == 0x40 {
				rx_list.remove(i);
				return true;
			}
		}

		std::mem::drop(aibus_handler);

		return false;
	}

	//Write the settings menu.
	fn write_settings_menu(&mut self) {
		let settings_count = 2;
		
		if !self.create_menu("Mirror Settings".to_string(), settings_count) {
			return;
		}

		for i in 0..settings_count {
			self.write_settings_menu_option(i);
		}

		let mut sel = 0x2;
		match self.context.try_lock() {
			Ok(context) => {
				if context.imid_row_count > 0 {
					sel = 0x1;
				}
			}
			Err(_) => {
				println!("AMirror Settings Menu: Context Locked.");
				return;
			}
		}

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, sel].to_vec(),
		});

		self.menu_open = MENU_SETTINGS;
	}

	//Write a settings menu option.
	fn write_settings_menu_option(&mut self, option: u8) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Menu Option: Context Locked.");
				return;
			}
		};

		let mut menu_req_data = [0x2B, 0x51, option].to_vec();

		let mut req_text: String = "".to_string();

		if option == 0 && context.imid_row_count > 0 && context.imid_text_len > 0 {
			if !context.imid_native_mirror {
				req_text = "Cluster Display Text".to_string();
			} else {
				if self.imid_split {
					req_text = "#ROF".to_string();
				} else {
					req_text = "#RON".to_string();
				}

				req_text += " Scroll Info Text";
			}
		} else if option == 1 {
			if self.auto_music_start {
				req_text = "#RON".to_string();
			} else {
				req_text = "#ROF".to_string();
			}
			req_text += " Auto Music Start";
		}

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		}
	}

	//Write the display menu.
	fn write_display_menu(&mut self) {
		println!("Display menu function called!");
		if !self.clear_menu() {
			println!("Clear menu failed...");
			return;
		}

		let settings_count = 6;
		
		if !self.create_menu("Cluster Display Text".to_string(), settings_count) {
			println!("Create menu failed...");
			return;
		}

		for i in 0..settings_count {
			self.write_display_menu_option(i);
		}

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, 0x1].to_vec(),
		});

		self.menu_open = MENU_DISPLAY;
	}

	//Write a display menu option.
	fn write_display_menu_option(&mut self, option: u8) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				println!("AMirror Display Menu Option: Context Locked.");
				return;
			}
		};

		let mut menu_req_data = [0x2B, 0x51, option].to_vec();

		let mut req_text = "".to_string();
		let mut unchecked_text = "#ROF";
		let mut checked_text = "#RON";

		if context.imid_row_count == 1 {
			unchecked_text = "#COF";
			checked_text = "#CON";
		}

		if option == 0 {
			if self.display_phone {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
			req_text += " Display Phone Name"
		} else if option == 1 {
			if self.display_title {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
			req_text += " Display Song Title";
		} else if option == 2 {
			if self.display_artist {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
			req_text += " Display Artist";
		} else if option == 3 {
			if self.display_album {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
			req_text += " Display Album";
		} else if option == 4 {
			if self.display_app {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
			req_text += " Display App Name";
		} else if option == 5 {
			if self.imid_split {
				req_text = "#ROF".to_string();
			} else {
				req_text = "#RON".to_string();
			}
			if context.imid_row_count != 1 {
				req_text += " Scroll Cluster Display";
			} else {
				req_text += " Scroll Info Text";
			}
		}

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		}
	}
}
