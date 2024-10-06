use std::fs::File;
use std::io::Read;
use std::path::Path;
use std::sync::{Arc, Mutex};
use std::time::SystemTime;

use crate::{Context, AIBusMessage};
use crate::mirror::usb::USBConnection;
use crate::mirror::messages::*;

use crate::aibus::*;

use super::messages::{get_carplay_command_message, get_sendfile_message};
use super::messages::get_manufacturer_info;
use super::messages::get_open_message;
use super::messages::get_sendint_message;
use super::messages::get_sendstring_message;
use super::messages::get_heartbeat_message;
use super::messages::MirrorMessage;
use super::messages::MetaDataMessage;
use super::mpv::{get_decode_type, MpvVideo};
use super::mpv::RdAudio;

pub struct MirrorHandler<'a> {
	context: &'a Arc<Mutex<Context>>,
	usb_conn: USBConnection,
	run: bool,
	startup: bool,
	mpv_video: MpvVideo,
	rd_audio: RdAudio,
	nav_audio: RdAudio,
	heartbeat_time: SystemTime,

	enter_hold: bool,
}

impl<'a> MirrorHandler<'a> {
	pub fn new(context: &'a Arc<Mutex<Context>>, w: u16, h: u16) -> MirrorHandler <'a> {
		let mut mpv_found = 0;
		let mut mpv_video: Option<MpvVideo> = None;
		let mut rd_audio: Option<RdAudio> = None;
		let mut nav_audio: Option<RdAudio> = None;

		while mpv_found < 3 {
			match MpvVideo::new(w, h) {
				Err(e) => println!("Failed to Start Mpv: {}", e.to_string()),
				Ok(mpv) => {
					mpv_video = Some(mpv);
					mpv_found += 1;
				}
			};

			match RdAudio::new() {
				Err(e) => println!("Failed to Start Rodio: {}", e.to_string()),
				Ok(rodio) => {
					rd_audio = Some(rodio);
					mpv_found += 1;
				}
			}
			
			match RdAudio::new() {
				Err(e) => println!("Failed to Start Rodio: {}", e.to_string()),
				Ok(rodio) => {
					nav_audio = Some(rodio);
					mpv_found += 1;
				}
			}
		}

		return MirrorHandler {
			context,
			usb_conn: USBConnection::new(),
			run: true,
			startup: false,
			mpv_video: mpv_video.unwrap(),
			rd_audio: rd_audio.unwrap(),
			nav_audio: nav_audio.unwrap(),
			heartbeat_time: SystemTime::now(),

			enter_hold: false,
		};
	}

	pub fn process(&mut self) {
		if !self.run {
			return;
		}
		if !self.usb_conn.connected {
			self.startup = false;
			let run = self.usb_conn.connect();

			if !run {
				return; //TODO: Should we still run full_loop even if no dongle is connected?
			} else {
				self.run = true;
			}
		} else if !self.startup {
			self.send_dongle_startup();
		}
		let mirror_message = self.usb_conn.read();

		match mirror_message {
			Some(mirror_message) => self.interpret_message(&mirror_message),
			None => ()
		}
		self.heartbeat();
	}

	fn heartbeat(&mut self) {
		if self.heartbeat_time.elapsed().unwrap().as_millis() > 2000 {
			self.heartbeat_time = SystemTime::now();
			self.usb_conn.write_message(get_heartbeat_message());
		}
	}

	pub fn send_carplay_command(&mut self, command: u32) {
		let msg = get_carplay_command_message(command);
		self.usb_conn.write_message(msg);
	}

	pub fn handle_aibus_message(&mut self, ai_msg: AIBusMessage) {
		if ai_msg.receiver != AIBUS_DEVICE_AMIRROR {
			return;
		}

		if ai_msg.sender == AIBUS_DEVICE_NAV_SCREEN {
			if ai_msg.l() >= 3 && ai_msg.data[0] == 0x30 { //Button press.
				let button = ai_msg.data[1];
				let state = ai_msg.data[2]>>6;

				if button == 0x2A && state == 0x0 { //Toggle left.
					self.send_carplay_command(PHONE_COMMAND_LEFT);
				} else if button == 0x2B && state == 0x0 { //Toggle right.
					self.send_carplay_command(PHONE_COMMAND_RIGHT);
				} else if button == 0x28 && state == 0x0 { //Toggle up.
					self.send_carplay_command(PHONE_COMMAND_ANDROID_UP);
				} else if button == 0x29 && state == 0x0 { //Toggle down.
					self.send_carplay_command(PHONE_COMMAND_ANDROID_DOWN);
				} else if button == 0x7 && state == 0x2 { //Enter.
					if !self.enter_hold {
						self.send_carplay_command(PHONE_COMMAND_ENTER_DOWN);
						self.send_carplay_command(PHONE_COMMAND_ENTER_UP);
					}
					self.enter_hold = false;
				} else if button == 0x7 && state == 0x1 { //Enter hold.
					self.send_carplay_command(PHONE_COMMAND_VOICE);
					self.enter_hold = true;
				} else if button == 0x25 && state == 0x0 { //Next track.
					self.send_carplay_command(PHONE_COMMAND_NEXT_TRACK);
				} else if button == 0x24 && state == 0x0 { //Previous track.
					self.send_carplay_command(PHONE_COMMAND_PREV_TRACK);
				} else if button == 0x46 && state == 0x2 { //Direction/pause.
					self.send_carplay_command(PHONE_COMMAND_PLAYPAUSE);
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x32 { //Knob turn.
				if ai_msg.data[1] == 0x7 { //Navigation knob.
					let steps = ai_msg.data[2]&0xF;
					let clockwise = (ai_msg.data[2]&0x10) != 0;
					
					for _i in 0..steps {
						if clockwise {
							self.send_carplay_command(PHONE_COMMAND_RIGHT);
						} else {
							self.send_carplay_command(PHONE_COMMAND_LEFT);
						}
					}
				}
			}
		}
	}

	fn send_dongle_startup(&mut self) {
		let mut dongle_message_dpi = get_sendint_message(String::from("/tmp/screen_dpi"), 160);
		let mut dongle_message_android = get_sendint_message(String::from("/etc/android_work_mode"), 1);
		let dongle_message_open = get_open_message(800, 480, 30, 5, 49152, 2, 2);

		self.usb_conn.write_message(dongle_message_dpi.get_mirror_message());
		self.usb_conn.write_message(dongle_message_android.get_mirror_message());
		self.usb_conn.write_message(dongle_message_open);
		
		//Send airplay.conf...
		let mut config_file = match File::open(Path::new("airplay.conf")) {
			Ok(file) => file,
			Err(err) => {
				println!("Error opening file: {}", err);
				return;
			}
		};
		
		let mut config_data = Vec::new();
		match config_file.read_to_end(&mut config_data) {
			Ok(_) => {

			}
			Err(err) => {
				println!("Error reading file: {}", err);
				return;
			}
		};
		
		let mut config_msg = get_sendfile_message("/etc/airplay.conf".to_string(), config_data);
		self.usb_conn.write_message(config_msg.get_mirror_message());
		
		//Send BMW.png...
		let mut android_icon_file = match File::open(Path::new("AidF Android.png")) {
			Ok(file) => file,
			Err(err) => {
				println!("Error opening file: {}", err);
				return;
			}
		};

		let mut android_icon_data: Vec<u8> = Vec::new();
		match android_icon_file.read_to_end(&mut android_icon_data) {
			Ok(_) => {

			}
			Err(err) => {
				println!("Error reading file: {}", err);
				return;
			}
		};

		let mut android_icon_msg = get_sendfile_message("/etc/oem_icon.png".to_string(), android_icon_data);
		self.usb_conn.write_message(android_icon_msg.get_mirror_message());
		
		//Send BMW_icon.png...
		let mut carplay_icon_file = match File::open(Path::new("AidF Apple.png")) {
			Ok(file) => file,
			Err(err) => {
				println!("Error opening file: {}", err);
				return;
			}
		};
		
		let mut carplay_icon_data: Vec<u8> = Vec::new();
		match carplay_icon_file.read_to_end(&mut carplay_icon_data) {
			Ok(_) => {

			}
			Err(err) => {
				println!("Error reading file: {}", err);
				return;
			}
		};
		
		let mut carplay_icon_msg = get_sendfile_message("/etc/icon_120x120.png".to_string(), carplay_icon_data);
		self.usb_conn.write_message(carplay_icon_msg.get_mirror_message());
	}

	fn interpret_message(&mut self, message: &MirrorMessage) {
		if message.message_type == 0x1 {
			// Open message.
			self.startup = true;
			println!("Starting Carlinkit...");

			let startup_msg_manufacturer = get_manufacturer_info(0, 0);
			let mut startup_msg_night = get_sendint_message(String::from("/tmp/night_mode"), 0);
			let mut startup_msg_hand_drive = get_sendint_message(String::from("/tmp/hand_drive_mode"), 0); //0=left, 1=right
			let mut startup_msg_charge = get_sendint_message(String::from("/tmp/charge_mode"), 0);
			let mut startup_msg_name = get_sendstring_message(String::from("/etc/box_name"), String::from("AMirror"));
			let startup_msg_carplay = get_carplay_command_message(101);

			self.usb_conn.write_message(startup_msg_manufacturer);
			self.usb_conn.write_message(startup_msg_night.get_mirror_message());
			self.usb_conn.write_message(startup_msg_hand_drive.get_mirror_message());
			self.usb_conn.write_message(startup_msg_charge.get_mirror_message());
			self.usb_conn.write_message(startup_msg_name.get_mirror_message());
			self.usb_conn.write_message(startup_msg_carplay);

			let mut startup_msg_meta = MetaDataMessage::new(25);
			startup_msg_meta.add_int(String::from("mediaDelay"), 300);
			startup_msg_meta.add_int(String::from("androidAutoSizeW"), 800);
			startup_msg_meta.add_int(String::from("androidAutoSizeH"), 480);
			self.usb_conn.write_message(startup_msg_meta.get_mirror_message());

			let mut msg_91 = MirrorMessage::new(9);
			msg_91.push_int(1);
			self.usb_conn.write_message(msg_91);

			let mut msg_88 = MirrorMessage::new(0x88);
			msg_88.push_int(1);
			self.usb_conn.write_message(msg_88);
			self.heartbeat_time = SystemTime::now();
		} else if message.message_type == 2 {
			println!("Phone trying to connect...");
			//Phone connected.
			let data = message.clone().decode();
			if data.len() <= 0 {
				return;
			}
			println!("Phone Connected!");
			let phone_type = data[0];
			let mut selected = false;
			match self.context.try_lock() {
				Ok(mut context) => {
					context.phone_type = phone_type as u8;
					context.phone_name = "".to_string();
					context.song_title = "".to_string();
					context.artist = "".to_string();
					context.album = "".to_string();
					context.app = "".to_string();
					selected = context.audio_selected;
				}
				Err(_) => {
				}
			}
			
			self.mpv_video.start();

			if selected {
				self.send_carplay_command(PHONE_COMMAND_PLAY);
			}

		} else if message.message_type == 4 {
			// Phone disconnected.
			self.mpv_video.stop();

			match self.context.try_lock() {
				Ok(mut context) => {
					context.phone_type = 0;
					context.phone_name = "".to_string();
					context.song_title = "".to_string();
					context.artist = "".to_string();
					context.album = "".to_string();
					context.app = "".to_string();
				}
				Err(_) => {
				}
			}

		} else if message.message_type == 6 { //Video.
			let mut data = vec![0;0];
			for i in 20..message.data.len() {
				data.push(message.data[i]);
			}
			self.mpv_video.send_video(&data);
		} else if message.message_type == 7 { //Audio.
			if message.data.len() > 16 {
				let (current_sample, current_bits, current_channel) = self.rd_audio.get_audio_profile();

				let decode_num = u32::from_le_bytes([message.data[0], message.data[1], message.data[2], message.data[3]]);
				let (new_sample, new_bits, new_channel) = get_decode_type(decode_num);

				let audio_type = u32::from_le_bytes([message.data[8], message.data[9], message.data[10], message.data[11]]);

				if new_sample != current_sample || new_bits != current_bits || new_channel != current_channel {
					if audio_type == 1 {
						self.rd_audio.set_audio_profile(new_sample, new_bits, new_channel);
					} else if audio_type == 2 {
						self.nav_audio.set_audio_profile(new_sample, new_bits, new_channel);
					}
				}

				let mut data = Vec::new();
				for i in 12..message.data.len() {
					data.push(message.data[i]);
				}

				if audio_type == 1 {
					match self.context.try_lock() {
						Ok(mut context) => {
							context.playing = true;
						}
						Err(_) => {
						}
					}
					self.rd_audio.send_audio(&data);
				} else if audio_type == 2 {
					self.nav_audio.send_audio(&data);
				}
			}
		} else if message.message_type == 25 || message.message_type == 42 {
			// Handle metadata.
			let meta_message = MetaDataMessage::from(message.clone());
			self.handle_metadata(meta_message);
		}
	}

	fn handle_metadata(&mut self, meta_message: MetaDataMessage) {
		let mut context = match self.context.try_lock() {
			 Ok(context) =>{
				context
			}
			Err(_) => {
				println!("Metadata: Context locked.");
				return;
			}
		};

		let mut song_title_changed = false;
		let mut artist_changed = false;
		let mut album_changed = false;

		for string_var in meta_message.string_vars {
			if string_var.variable == "MDModel" {
				context.phone_name = string_var.value;
			} else if string_var.variable == "MediaSongName" {
				if context.song_title != string_var.value {
					context.song_title = string_var.value;
					song_title_changed = true;
				}
			} else if string_var.variable == "MediaArtistName" {
				if context.artist != string_var.value {
					context.artist = string_var.value;
					artist_changed = true;
				}
			} else if string_var.variable == "MediaAlbumName" {
				if context.album != string_var.value {
					context.album = string_var.value;
					album_changed = true;
				}
			} else if string_var.variable == "MediaAPPName" {
				if context.app != string_var.value {
					context.app = string_var.value;

					if !song_title_changed {
						context.song_title = "".to_string();
						song_title_changed = true;
					}
					if !artist_changed {
						context.artist = "".to_string();
						artist_changed = true;
					}
					if !album_changed {
						context.album = "".to_string();
						album_changed = true;
					}
				}
			}
		}
	}

	pub fn set_minimize(&mut self, minimize: bool) {
		self.set_minimize(minimize);
	}
}
