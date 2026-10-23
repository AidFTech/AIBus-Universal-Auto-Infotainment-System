use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use image::Rgba;

use ini::Ini;

use crate::aap::aa_handler::AapHandler;
use crate::aibus_handler::AIBusHandler;
use crate::ipc::ClientAIBusHandler;
use crate::locale::{Locale, MenuIndex, MenuList};
use crate::mirror::messages::*;
use crate::mirror::mpv::{MpvVideo, RdAudio};
use crate::aibus::*;
use crate::context::*;
use crate::mirror::handler::*;

use crate::text_split::split_text;

const APP_NAME: u8 = 0x4;
const SONG_NAME: u8 = 0x1;
const ARTIST_NAME: u8 = 0x2;
const ALBUM_NAME: u8 = 0x3;

#[derive(PartialEq,Eq)]
enum AMirrorMenu {
	MenuNone,
	MenuSettings,
	MenuAudioSettings,
	MenuFlash,
	MenuDisplay,
}

const RADIO_PING_WAIT: u64 = 7250;

const SETTING_PATH: &str = "./AidF_Mirror_Settings.ini";
const SETTING_SECTION: &str = "Mirror_Options";

const SETTING_DISPLAY_TITLE: &str = "DisplayTitle";
const SETTING_DISPLAY_ARTIST: &str = "DisplayArtist";
const SETTING_DISPLAY_ALBUM: &str = "DisplayAlbum";
const SETTING_DISPLAY_APP: &str = "DisplayApp";
const SETTING_DISPLAY_PHONE: &str = "DisplayPhone";

const SETTING_SCROLL_PARAM: &str = "ScrollParam";
const SETTING_SPLIT: &str = "SplitParam";
const SETTING_AUTOMIRROR: &str = "AutoMirrorStart";
const SETTING_FLASHSONG: &str = "FlashSongTitle";
const SETTING_FLASHSONG_MIRROR: &str = "FlashSongTitleMirror";
const SETTING_AUTOMUSIC: &str = "AutoMusicStart";

pub struct AMirror<'a> {
	context: &'a Arc<Mutex<Context>>,
	mpv_video: &'a Arc<Mutex<MpvVideo>>,

	dongle_handler: MirrorHandler<'a>,
	aa_handler: AapHandler<'a>,

	client_aibus_handler: Arc<Mutex<ClientAIBusHandler>>,
	aibus_handler: Arc<Mutex<AIBusHandler>>,

	aibus_cache: Vec<AIBusMessage>,

	display_title: bool,
	display_artist: bool,
	display_album: bool,
	display_app: bool,
	display_phone: bool,
	
	imid_scroll: i8,
	imid_split: bool,
	imid_scroll_pos: usize,
	imid_scroll_wrap: bool, //True if the scrolling is to be wrapped.
	imid_scroll_header: bool, //True if we scroll/show the header.
	imid_refresh: bool,

	auto_music_start: bool,
	flash_song_title: bool,
	flash_song_title_mirror: bool,
	auto_mirror_start: bool,

	local_radio_connected: bool,
	local_imid_native: bool,
	local_imid_text_len: u8,
	local_imid_rows: u8,

	overlay_device: u8,

	menu_open: AMirrorMenu,
	next_menu: AMirrorMenu,

	w: u16,

	source_request_timer: Instant,
	screen_request_timer: Instant,
	radio_ping_timer: Instant,
	scroll_timer: Instant,

	last_flash: Instant,

	overlay_timer: Instant,
	overlay_on: bool,
	vol_overlay: bool,
	header_overlay: bool,

	audio_held: bool,

	screen_acknowledged: bool, //True if the screen buttons have been acknowledged.
	color_acknowledged: bool, //True if the nav computer has sent overlay color information.

	pub run: bool,
	power_state: u8,
	door_state: u8,

	powered_on: bool, //True if a power-on has been confirmed.

	open_audio_menu: bool,
}

impl <'a> AMirror<'a> {
	pub fn new(context: &'a Arc<Mutex<Context>>, aibus_handler: Arc<Mutex<AIBusHandler>>, client_aibus_handler: Arc<Mutex<ClientAIBusHandler>>, mpv_video: &'a Arc<Mutex<MpvVideo>>, rd_audio: &'a Arc<Mutex<RdAudio>>, nav_audio: &'a Arc<Mutex<RdAudio>>, w: u16, h: u16) -> Self {
		let mutex_mirror_handler = MirrorHandler::new(context, mpv_video, rd_audio, nav_audio, w, h);
		let mutex_aa_handler = AapHandler::new(context, mpv_video, rd_audio, nav_audio, w, h);

		let mut display_title = false;
		let mut display_artist = false;
		let mut display_album = false;
		let mut display_app = false;
		let mut display_phone = false;

		let mut auto_music_start = false;
		let mut flash_song_title = true;
		let mut flash_song_title_mirror = true;
		let mut auto_mirror_start = true;
		let mut imid_split = true;
		let mut imid_scroll = -1;

		match Ini::load_from_file(SETTING_PATH) {
			Ok(load_file) => {
				for (section, property) in load_file.iter() {
					match section {
						Some(section) => {
							if section == SETTING_SECTION { //Mirror setting list.
								for(key, mut value) in property.iter() {
									value = value.trim();
									if key == SETTING_DISPLAY_TITLE {
										if value.to_uppercase() == "TRUE" || value == "1" {
											display_title = true;
										}
									} else if key == SETTING_DISPLAY_ARTIST {
										if value.to_uppercase() == "TRUE" || value == "1" {
											display_artist = true;
										}
									} else if key == SETTING_DISPLAY_ALBUM {
										if value.to_uppercase() == "TRUE" || value == "1" {
											display_album = true;
										}
									} else if key == SETTING_DISPLAY_APP {
										if value.to_uppercase() == "TRUE" || value == "1" {
											display_app = true;
										}
									} else if key == SETTING_DISPLAY_PHONE {
										if value.to_uppercase() == "TRUE" || value == "1" {
											display_phone = true;
										}
									} else if key == SETTING_AUTOMIRROR {
										if value.to_uppercase() == "FALSE" || value == "0" {
											auto_mirror_start = false;
										}
									} else if key == SETTING_AUTOMUSIC {
										if value.to_uppercase() == "TRUE" || value == "1" {
											auto_music_start = true;
										}
									} else if key == SETTING_FLASHSONG {
										if value.to_uppercase() == "FALSE" || value == "0" {
											flash_song_title = false;
										}
									} else if key == SETTING_FLASHSONG_MIRROR {
										if value.to_uppercase() == "FALSE" || value == "0" {
											flash_song_title_mirror = false;
										}
									} else if key == SETTING_SPLIT {
										if value.to_uppercase() == "FALSE" || value == "0" {
											imid_split = false;
										}
									} else if key == SETTING_SCROLL_PARAM {
										match value.parse::<i8>() {
											Ok(int) => {
												imid_scroll = int;
											}
											Err(_) => {

											}
										}
									}
								}
							}
						}
						None => {

						}
					}
				}
			}
			Err(_) => {
				//The file wasn't found.
			}
		}

		let this = Self{
			context,
			mpv_video,
			
			dongle_handler: mutex_mirror_handler,
			aa_handler: mutex_aa_handler,

			aibus_handler,
			client_aibus_handler,
			aibus_cache: Vec::new(),

			display_title,
			display_artist,
			display_album,
			display_app,
			display_phone,

			auto_music_start,
			flash_song_title,
			flash_song_title_mirror,
			auto_mirror_start,

			imid_scroll,
			imid_split,
			imid_scroll_pos: 0,
			imid_scroll_wrap: false,
			imid_scroll_header: false,
			
			imid_refresh: false,

			overlay_device: AIBUS_DEVICE_RADIO,

			local_radio_connected: false,
			local_imid_native: false,
			local_imid_text_len: 0,
			local_imid_rows: 0,

			menu_open: AMirrorMenu::MenuNone,
			next_menu: AMirrorMenu::MenuNone,

			w,

			source_request_timer: Instant::now(),
			screen_request_timer: Instant::now(),
			radio_ping_timer: Instant::now(),
			scroll_timer: Instant::now(),
			last_flash: Instant::now(),

			overlay_timer: Instant::now(),
			overlay_on: false,
			vol_overlay: false,
			header_overlay: false,

			audio_held: false,

			screen_acknowledged: false,
			color_acknowledged: false,

			run: true,
			power_state: 0x0,
			door_state: 0x0,

			powered_on: false,
			open_audio_menu: false,
		};

		return this;
	}

	pub fn process(&mut self) {
		if !self.powered_on {
			let mut ai_rx_list = Vec::new();

			{
				let mut aibus_handler = match self.aibus_handler.try_lock() {
					Ok(aibus_handler) => aibus_handler,
					Err(_) => {
						let aibus_handler;
						loop {
							aibus_handler = match self.aibus_handler.try_lock() {
								Ok(aibus_handler) => aibus_handler,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						aibus_handler
					}
				};

				let ai_rx = aibus_handler.get_ai_rx();
				for ai_data in &mut *ai_rx {
					ai_rx_list.push(ai_data.clone());
				}

				ai_rx.clear();

				let ai_tx = aibus_handler.get_ai_tx();
				for ai_data in &self.aibus_cache {
					ai_tx.push(ai_data.clone());
				}

				self.aibus_cache.clear();
			}

			for ai_data in ai_rx_list {
				self.handle_aibus_message(ai_data);
			}
			
			//thread::sleep(Duration::from_millis(50));
			if Instant::now() - self.screen_request_timer > Duration::from_millis(3000) && !self.screen_acknowledged {
				self.screen_request_timer = Instant::now();
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_NAV_SCREEN,
					data: [0x31, 0x30].to_vec(),
				});
			}

			return;
		}
		
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
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

		let notification = context.notification;
		let phone_mute = context.phone_mute;

		let headlight_state = context.headlight_state;
		let speed = context.speed_e3;

		std::mem::drop(context);

		if self.screen_acknowledged {
			self.dongle_handler.process();
			self.aa_handler.process();
		}

		context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
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

			context.last_connection_change = Instant::now();

			if context.phone_type == 0 {
				context.phone_name = "".to_string();
				context.song_title = "".to_string();
				context.artist = "".to_string();
				context.album = "".to_string();
				context.app = "".to_string();

				context.playing = false;
				context.notification = false;
				context.phone_mute = false;

				if context.phone_active {
					match self.mpv_video.try_lock() {
						Ok(mut mpv_video) => {
							mpv_video.set_minimize(true);
						}
						Err(_) => {
							let mut mpv_video;
							loop {
								mpv_video = match self.mpv_video.try_lock() {
									Ok(mpv_video) => mpv_video,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							mpv_video.set_minimize(true);
						}
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_COMPUTER,
						data: [0x48, AIBUS_DEVICE_AMIRROR, 0x0].to_vec(),
					});

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_SCREEN,
						data: [0x77, AIBUS_DEVICE_NAV_COMPUTER, 0x10].to_vec(), //TODO: If another device has requested control, send that instead.
					});
				}

				match self.mpv_video.try_lock() {
					Ok(mut mpv_video) => {
						mpv_video.video_disconnect();
					}
					Err(_) => {
						let mut mpv_video;
						loop {
							mpv_video = match self.mpv_video.try_lock() {
								Ok(mpv_video) => mpv_video,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						mpv_video.video_disconnect();
					}
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
							let mut mpv_video;
							loop {
								mpv_video = match self.mpv_video.try_lock() {
									Ok(mpv_video) => mpv_video,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							mpv_video.set_minimize(false);
						}
					}

					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_COMPUTER,
						data: [0x48, AIBUS_DEVICE_AMIRROR, 0x1].to_vec(),
					});
				} else {
					context.phone_active = false;
				}

				if self.auto_music_start {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_RADIO,
						data: [0x10, 0x10, AIBUS_DEVICE_AMIRROR, 0x0].to_vec(),
					});
				}
			}

			self.write_aibus_message(phone_type_msg);
			
			if context.audio_selected && context.audio_text && context.imid_native_mirror {
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

				if context.phone_type == 3 {
					self.write_nav_overlay("Carplay".to_string());
				} else if context.phone_type == 5 {
					self.write_nav_overlay("Android Auto".to_string());
				}

				std::mem::drop(context);

				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
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
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
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

			match self.client_aibus_handler.try_lock() {
				Ok(mut client_handler) => {
					client_handler.set_ack_cache(context.phone_type != 0, false);
				}
				Err(_) => {
					let mut client_handler;
					loop {
						client_handler = match self.client_aibus_handler.try_lock() {
							Ok(client_handler) => client_handler,
							Err(_) => {
								continue;
							}
						};
						break;
					}
					client_handler.set_ack_cache(context.phone_type != 0, false);
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
				self.write_nav_text(context.phone_name.replace("#", "##  "), 2, 1, true);

				std::mem::drop(context);

				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
					}
				};
			}

			if context.radio_connected {
				std::mem::drop(context);

				self.write_radio_name();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
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

				if Instant::now() - self.last_flash > Duration::from_millis(3000) {
					let mirror_name = (match context.phone_type {
						3 => "Carplay",
						5 => "Android Auto",
						_ => "",
					}).to_owned() + match context.playing {
						true => " #FWD",
						false => " ||",
					};

					self.write_nav_overlay(mirror_name);
				}
			}
		}
	
		if context.song_title != song_title {
			if context.audio_selected {
				self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
				if context.audio_text {
					self.write_nav_text(context.song_title.replace("#", "##  "), 1, 0, true);

					if self.flash_song_title {
						self.write_nav_overlay(context.song_title.replace("#", "##  "));
						self.last_flash = Instant::now();
					}

					if self.flash_song_title_mirror {
						let mut mpv_video = match self.mpv_video.try_lock() {
							Ok(mpv_video) => mpv_video,
							Err(_) => {
								let mpv_video;
								loop {
									mpv_video = match self.mpv_video.try_lock() {
										Ok(mpv_video) => mpv_video,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								mpv_video
							}
						};

						mpv_video.set_header_overlay(context.song_title.clone());
						self.header_overlay = true;
						self.overlay_timer = Instant::now();
					}

					if context.imid_native_mirror && self.imid_scroll < 0 && context.phone_type != 0 {
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
								self.write_imid_text(context.song_title.replace("#", "##  "), imid_x as u8, imid_y);
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
					self.write_nav_text(context.artist.replace("#", "##  "), 2, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 && context.phone_type != 0 {
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
								self.write_imid_text(context.artist.replace("#", "##  "), imid_x as u8, imid_y);
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
					self.write_nav_text(context.album.replace("#", "##  "), 3, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 && context.phone_type != 0 {
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
								self.write_imid_text(context.album.replace("#", "##  "), imid_x as u8, imid_y);
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
					self.write_nav_text(context.app.replace("#", "##  "), 4, 0, true);

					if context.imid_native_mirror && self.imid_scroll < 0 && context.phone_type != 0 {
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
								self.write_imid_text(context.app.replace("#", "##  "), imid_x as u8, imid_y);
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
						self.write_time(AIBUS_DEVICE_RADIO, context.track_time as u64);
						if context.phone_type != 0 {
							self.write_time(AIBUS_DEVICE_IMID, context.track_time as u64);
						}
					} else {
						let clear_data = [0x20, 0x71, 0x0].to_vec();
						self.write_aibus_message(AIBusMessage {
							sender: AIBUS_DEVICE_AMIRROR,
							receiver: AIBUS_DEVICE_NAV_COMPUTER,
							data: clear_data,
						});

						self.write_time(AIBUS_DEVICE_RADIO, 0);
						if context.phone_type != 0 {
							self.write_time(AIBUS_DEVICE_IMID, 0);
						}
					}
				}
			}
		}

		self.local_radio_connected = context.radio_connected;
		self.local_imid_native = context.imid_native_mirror;
		self.local_imid_rows = context.imid_row_count;
		self.local_imid_text_len = context.imid_text_len;

		//Request control.
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

			if !context.screen_connected {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_NAV_SCREEN,
					data: [0x1].to_vec(),
				});
			}

			if context.screen_connected && !self.screen_acknowledged {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_NAV_SCREEN,
					data: [0x31, 0x30].to_vec(),
				});
			}
		}

		let mut scroll_limit = 300; //The scroll timer limit.
		if self.imid_scroll_wrap || self.imid_scroll_header {
			scroll_limit = 1000;
		} else if self.imid_split {
			scroll_limit = 3000;
		} else if self.imid_scroll_pos == 0 { 
			scroll_limit = 750;
		}

		//Scroll the IMID.
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
				let mut update = change_imid || self.imid_refresh;

				let scroll_param_name;
				if self.imid_scroll == 0 {
					scroll_param_name = "PHONE";
				} else if self.imid_scroll == 1 {
					scroll_param_name = "TRACK";
				} else if self.imid_scroll == 2 {
					scroll_param_name = "ARTIST";
				} else if self.imid_scroll == 3 {
					scroll_param_name = "ALBUM";
				} else if self.imid_scroll == 4 {
					scroll_param_name = "APP";
				} else {
					scroll_param_name = "";
				}

				if self.imid_scroll_header && self.imid_refresh {
					scroll_param = scroll_param_name.to_string();
				}

				let last_header = self.imid_scroll_header;
				if self.imid_scroll_header && !self.imid_refresh {
					self.imid_scroll_header = false;
					update = true;
				}

				if Instant::now() - self.scroll_timer > Duration::from_millis(scroll_limit) {
					if !last_header {
						self.imid_scroll_pos += 1;
					}
					self.scroll_timer = Instant::now();
				} else if update {
					self.imid_scroll_pos = 0;
				}

				if self.imid_refresh {
					self.imid_refresh = false;
				}

				let mut imid_y = 1;
				if context.imid_row_count >= 2 {
					imid_y = context.imid_row_count/2 + 1;
				}

				if update && context.imid_row_count >= 2 {
					let mut imid_x = 0;
					if context.imid_text_len >= 10 {
						imid_x = (context.imid_text_len/2) as isize - (scroll_param_name.len()/2) as isize;
						if imid_x < 0 || imid_x > context.imid_text_len as isize {
							imid_x = 0;
						}
					}

					self.write_imid_text(scroll_param_name.to_string(), imid_x as u8, context.imid_row_count/2);

					for i in 1..=context.imid_row_count {
						if i != context.imid_row_count/2 && i != context.imid_row_count/2 + 1 {
							self.write_imid_text("".to_string(), 0, i);
						}
					}
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
						imid_x = (context.imid_text_len/2) as isize - (scroll_split[self.imid_scroll_pos].len()/2) as isize;
						if imid_x < 0 || imid_x > context.imid_text_len as isize {
							imid_x = 0;
						}
					}

					if self.imid_scroll_pos != last_scroll_pos || update {
						self.write_imid_text(scroll_split[self.imid_scroll_pos].clone(), imid_x as u8, imid_y);
					}
				} else {
					if self.imid_scroll_wrap {
						self.imid_scroll_pos = 0;
						self.imid_scroll_wrap = false;
					} else if (scroll_param.len() as isize - self.imid_scroll_pos as isize) < (context.imid_text_len as isize) {
						self.imid_scroll_wrap = true;
						if scroll_param.len() >= context.imid_text_len as usize {
							self.imid_scroll_pos = scroll_param.len() - context.imid_text_len as usize;
						}
					}
					
					let mut imid_x = 0;
					if self.imid_scroll_header {
						imid_x = (context.imid_text_len/2) as isize - (scroll_param.len()/2) as isize;
						if imid_x < 0 || imid_x > context.imid_text_len as isize {
							imid_x = 0;
						}
					}

					if self.imid_scroll_pos != last_scroll_pos || update {
						if scroll_param.len() > context.imid_text_len as usize {
							let scroll_string = scroll_param[self.imid_scroll_pos..self.imid_scroll_pos + context.imid_text_len as usize].to_string();
							self.write_imid_text(scroll_string, imid_x as u8, imid_y);
						} else {
							self.write_imid_text(scroll_param, imid_x as u8, imid_y);
						}
					}
				}
			}
		}

		//Write the IMID text.
		if self.imid_refresh && self.imid_scroll < 0 {
			self.imid_refresh = false;
			if context.audio_text {
				std::mem::drop(context);
				self.write_all_imid_text();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
					}
				};
			}
		}

		let overlay_limit;
		if self.vol_overlay {
			overlay_limit = 700;
		} else {
			overlay_limit = 3500;
		}

		//Overlay timeout.
		if Instant::now() - self.overlay_timer > Duration::from_millis(overlay_limit) && (self.overlay_on || self.header_overlay) {
			self.overlay_on = false;
			self.vol_overlay = false;
			self.header_overlay = false;
			match self.mpv_video.try_lock() {
				Ok(mut mpv_video) => {
					mpv_video.clear_overlay();
				}
				Err(_) => {
					let mut mpv_video;
					loop {
						mpv_video = match self.mpv_video.try_lock() {
							Ok(mpv_video) => mpv_video,
							Err(_) => {
								continue;
							}
						};
						break;
					}
					mpv_video.clear_overlay();
				}
			}
		}

		//Radio ping timeout.
		if Instant::now() - self.radio_ping_timer > Duration::from_millis(RADIO_PING_WAIT) && context.audio_selected {
			self.radio_ping_timer = Instant::now();

			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_RADIO,
				data: [0x60, 0x10].to_vec(),
			});
		}

		//Position.
		let latitude = context.latitude;
		let longitude = context.longitude;
		let altitude = context.altitude;
		let phone_active = context.phone_active;

		std::mem::drop(context);
		let mut ai_rx_list = Vec::new();

		match self.aibus_handler.try_lock() {
			Ok(mut aibus_handler) => {
				let ai_rx = aibus_handler.get_ai_rx();
				for ai_data in &mut *ai_rx {
					ai_rx_list.push(ai_data.clone());
				}

				ai_rx.clear();

				let ai_tx = aibus_handler.get_ai_tx();
				for ai_data in &self.aibus_cache {
					ai_tx.push(ai_data.clone());
				}

				self.aibus_cache.clear();
			}
			Err(_) => {}
		}

		for ai_data in ai_rx_list {
			self.handle_aibus_message(ai_data);
			if phone_active && self.screen_acknowledged {
				self.aa_handler.process();
				self.dongle_handler.process();
			}
		}

		context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		let new_latitude = context.latitude;
		let new_longitude = context.longitude;
		let new_altitude = context.altitude;
		let phone_type = context.phone_type;

		//Notification.
		if context.notification != notification {
			self.write_aibus_message(AIBusMessage { 
									sender: AIBUS_DEVICE_AMIRROR,
									receiver: AIBUS_DEVICE_NAV_COMPUTER,
									data: [0x7F, 0x6, if context.notification {
										0x1
									} else {
										0x0
									}].to_vec() });
		}

		//Phone mute.
		if context.phone_mute != phone_mute {
			self.write_aibus_message(AIBusMessage { 
									sender: AIBUS_DEVICE_AMIRROR,
									receiver: AIBUS_DEVICE_RADIO,
									data: [0x21, 0x6, if context.phone_mute {
										0x1
									} else {
										0x0
									}].to_vec() });
		}

		if phone_type == 0x5 {
			//Headlight state.
			if context.headlight_state != headlight_state {
				self.aa_handler.set_headlight_state(context.headlight_state);
			}

			//Speed.
			if context.speed_e3 != speed {
				self.aa_handler.set_speed(context.speed_e3);
			}
		}

		let debug = context.debug.clone();
		context.debug = "".to_string();

		std::mem::drop(context);

		if phone_type == 0x5 {
			if new_latitude != latitude || new_longitude != longitude || new_altitude != altitude {
				self.aa_handler.send_coordinates();
			}
		}

		match self.mpv_video.try_lock() {
			Ok(mut mpv_video) => {
				mpv_video.process();
			}
			Err(_) => {
				println!("AMirror: MPV Locked.");
			}
		}

		if debug.len() > 0 {
			self.print_debug(&debug);
		}
	}

	///Read an AIBus message.
	pub fn handle_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		if ai_msg.sender == AIBUS_DEVICE_RADIO && !context.radio_connected && !get_init_message(&ai_msg) && !get_poweroff_message(&ai_msg) && self.powered_on {
			context.radio_connected = true;

			std::mem::drop(context);

			self.write_radio_handshake();

			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_RADIO,
				data: [0x60, 0x11].to_vec(),
			});

			context = match self.context.try_lock() {
				Ok(context) => context,
				Err(_) => {
					let context;
					loop {
						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								continue;
							}
						};
						break;
					}
					context
				}
			};
		}

		if ai_msg.sender == AIBUS_DEVICE_NAV_SCREEN && !context.screen_connected && !get_init_message(&ai_msg) && !get_poweroff_message(&ai_msg) && self.powered_on {
			context.screen_connected = true;

			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_SCREEN,
				data: [0x31, 0x30].to_vec(),
			});
		}

		if ai_msg.sender == AIBUS_DEVICE_PHONE && !context.bluetooth_connected && !get_init_message(&ai_msg) && !get_poweroff_message(&ai_msg) && self.powered_on {
			context.bluetooth_connected = true;

			//Get the MAC address.
			let addr_request = AIBusMessage{
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_PHONE,
				data: [0x12].to_vec(),
			};

			self.write_aibus_message(addr_request);
		}

		if ai_msg.sender == AIBUS_DEVICE_IMID && !context.imid_connected && !get_init_message(&ai_msg) && !get_poweroff_message(&ai_msg) && self.powered_on {
			context.imid_connected = true;
		}

		if get_init_message(&ai_msg) || get_poweroff_message(&ai_msg) {
			if ai_msg.sender == AIBUS_DEVICE_NAV_SCREEN {
				context.screen_connected = false;
			} else if ai_msg.sender == AIBUS_DEVICE_PHONE {
				context.bluetooth_connected = false;
			} else if ai_msg.sender == AIBUS_DEVICE_RADIO {
				context.radio_connected = false;
			} else if ai_msg.sender == AIBUS_DEVICE_IMID {
				context.imid_connected = false;
			}
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

		if get_init_message(&ai_msg) {
			if ai_msg.sender == AIBUS_DEVICE_RADIO {
				context.radio_connected = false;
			} else if ai_msg.sender == AIBUS_DEVICE_NAV_SCREEN {
				context.screen_connected = false;
			} else if ai_msg.sender == AIBUS_DEVICE_PHONE {
				context.bluetooth_connected = false;
			}
		}

		if ai_msg.l() >= 1 && ai_msg.data[0] == 0x1 && ai_msg.sender != AIBUS_DEVICE_AMIRROR && ai_msg.receiver == 0xFF { //Wide ping.
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: ai_msg.sender,
				data: [0x1].to_vec(),
			});
		} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x20 { //Clear overlay.
			if ai_msg.sender == self.overlay_device || ai_msg.sender == AIBUS_DEVICE_RADIO {
				let set_overlay = (ai_msg.data[1]&0x10) != 0;

				let index = (ai_msg.data[1]&0xF) as usize;

				if index < 5 {
					let mut mpv = match self.mpv_video.try_lock() {
						Ok(mpv) => mpv,
						Err(_) => {
							let mpv;
							loop {
								mpv = match self.mpv_video.try_lock() {
									Ok(mpv) => mpv,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							mpv
						}
					};

					mpv.set_overlay_text("".to_string(), index);

					if set_overlay && !self.vol_overlay {
						self.overlay_on = true;
						self.overlay_timer = Instant::now();
						mpv.show_overlay();
					} else if self.overlay_on && !self.vol_overlay {
						mpv.show_overlay();
					}
				} else if index == 0xE || index == 0xF {
					let mut mpv = match self.mpv_video.try_lock() {
						Ok(mpv) => mpv,
						Err(_) => {
							let mpv;
							loop {
								mpv = match self.mpv_video.try_lock() {
									Ok(mpv) => mpv,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							mpv
						}
					};

					for i in if index == 0xF {
						0
					} else {
						1
					}..5 {
						mpv.set_overlay_text("".to_string(), i);

						if set_overlay && !self.vol_overlay {
							self.overlay_on = true;
							self.overlay_timer = Instant::now();
							mpv.show_overlay();
						} else if self.overlay_on && !self.vol_overlay {
							mpv.show_overlay();
						}
					}
				}
			} else {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_RADIO,
					data: [0x60, 0x11].to_vec(),
				});
			}
		} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x22 && ai_msg.data[1] == 0x61 { //Header overlay.
			let text;
			if ai_msg.l() == 3 && ai_msg.data[2] == 0x0 {
				text = "".to_string();
			} else if ai_msg.l() >= 3 {
				let text_bytes = ai_msg.data[2..].to_vec();
				let utf8_bytes = get_utf8_from_ascii(text_bytes);
				text = match std::str::from_utf8(&utf8_bytes) {
					Ok(text) => text.to_string(),
					Err(e) => {
						println!("Error: {}", e);
						return;
					}
				};
			} else {
				text = "".to_string();
			}

			if !self.vol_overlay && !self.overlay_on {
				let mut mpv = match self.mpv_video.try_lock() {
					Ok(mpv) => mpv,
					Err(_) => {
						let mpv;
						loop {
							mpv = match self.mpv_video.try_lock() {
								Ok(mpv) => mpv,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						mpv
					}
				};

				mpv.set_header_overlay(text);
				self.header_overlay = true;
				self.overlay_timer = Instant::now();
			}
		} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x23 { //Overlay.
			if ai_msg.sender == self.overlay_device || ai_msg.sender == AIBUS_DEVICE_RADIO {
				let set_overlay = (ai_msg.data[1]&0x10) != 0;

				let text;
				if ai_msg.l() == 3 && ai_msg.data[2] == 0x0 {
					text = "".to_string();
				} else if ai_msg.l() >= 3 {
					let text_bytes = ai_msg.data[2..].to_vec();
					let utf8_bytes = get_utf8_from_ascii(text_bytes);
					text = match std::str::from_utf8(&utf8_bytes) {
						Ok(text) => text.to_string(),
						Err(e) => {
							println!("Error: {}", e);
							return;
						}
					};

					
				} else {
					text = "".to_string();
				}

				let index = (ai_msg.data[1]&0xF) as usize;

				let mut mpv = match self.mpv_video.try_lock() {
					Ok(mpv) => mpv,
					Err(_) => {
						let mpv;
						loop {
							mpv = match self.mpv_video.try_lock() {
								Ok(mpv) => mpv,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						mpv
					}
				};

				mpv.set_overlay_text(text, index);

				if set_overlay && !self.vol_overlay {
					self.overlay_on = true;
					self.header_overlay = false;
					self.overlay_timer = Instant::now();
					mpv.show_overlay();
				} else if self.overlay_on && !self.header_overlay && !self.vol_overlay {
					mpv.show_overlay();
				}
			} else {
				self.write_aibus_message(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_RADIO,
					data: [0x60, 0x11].to_vec(),
				});
			}
		} else if ai_msg.l() >= 5 && ai_msg.data[0] == 0x60 { //Color change.
			self.color_acknowledged = true;
			let color = Rgba([ai_msg.data[2], ai_msg.data[3], ai_msg.data[4], 0xFF]);
			let mut mpv = match self.mpv_video.try_lock() {
				Ok(mpv) => mpv,
				Err(_) => {
					let mpv;
					loop {
						mpv = match self.mpv_video.try_lock() {
							Ok(mpv) => mpv,
							Err(_) => {
								continue;
							}
						};
						break;
					}
					mpv
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
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
					}
				};
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x40 && ai_msg.data[1] == 0x10 { //Source change.
				self.radio_ping_timer = Instant::now();
				
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

					context.notification = false;
					context.phone_mute = false;
					
					if context.phone_type != 0 {
						self.dongle_handler.send_carplay_command(PHONE_COMMAND_PLAY);
						self.aa_handler.start_stop_audio(true);
					}
					
					std::mem::drop(context);
					self.write_all_metadata();
					
					context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							let context;
							loop {
								context = match self.context.try_lock() {
									Ok(context) => context,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							context
						}
					};

					let mut control_req = 0x80;
					if context.phone_active && context.phone_type != 0 {
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
				self.overlay_device = new_device;
				if new_device == AIBUS_DEVICE_AMIRROR { //Text control!
					context.audio_text = true;
					
					std::mem::drop(context);
					self.write_all_text();
					
					context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							let context;
							loop {
								context = match self.context.try_lock() {
									Ok(context) => context,
									Err(_) => {
										continue;
									}
								};
								break;
							}
							context
						}
					};
				} else {
					context.audio_text = false;
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x70 && ai_msg.data[1] == 0x10 { //Source ping.
				self.radio_ping_timer = Instant::now();
				
				let selected = ai_msg.data[2];
				if (selected != AIBUS_DEVICE_AMIRROR && context.audio_selected) || (selected == AIBUS_DEVICE_AMIRROR && !context.audio_selected) {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_RADIO,
						data: [0x60, 0x10].to_vec()
					});

					if selected == AIBUS_DEVICE_AMIRROR && (!context.audio_selected || !context.audio_text) {
						self.write_aibus_message(AIBusMessage {
							sender: AIBUS_DEVICE_AMIRROR,
							receiver: AIBUS_DEVICE_RADIO,
							data: [0x60, 0x11].to_vec()
						});
					}
				}
				
				if selected == AIBUS_DEVICE_AMIRROR && context.audio_selected && !context.audio_text {
					self.write_aibus_message(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_RADIO,
						data: [0x60, 0x11].to_vec()
					});
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x30 { //Control.
				if ai_msg.data[1] == 0x0 { //Status query.
					//TODO: This.
				}
			} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x2B && ai_msg.data[1] == 0x4A { //Create a settings menu.
				std::mem::drop(context);
				
				self.write_audio_settings_menu();

				context = match self.context.try_lock() {
					Ok(context) => context,
					Err(_) => {
						let context;
						loop {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						context
					}
				};
			} else if ai_msg.l() >= 2 && ai_msg.data[0] == 0x26 { //Volume control.
				let new_vol = ai_msg.data[1];
				let new_max = ai_msg.data[2];

				let mut mpv = match self.mpv_video.try_lock() {
					Ok(mpv) => mpv,
					Err(_) => {
						let mpv;
						loop {
							mpv = match self.mpv_video.try_lock() {
								Ok(mpv) => mpv,
								Err(_) => {
									continue;
								}
							};
							break;
						}
						mpv
					}
				};

				mpv.set_volume_overlay(new_vol, new_max);

				self.overlay_on = true;
				self.vol_overlay = true;
				self.overlay_timer = Instant::now();
				mpv.show_volume_overlay();
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
						self.aa_handler.send_start_request();
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
					if self.menu_open == AMirrorMenu::MenuSettings {
						self.write_aibus_message(AIBusMessage {
							sender: AIBUS_DEVICE_AMIRROR,
							receiver: ai_msg.sender,
							data: [0x2B, 0x40].to_vec(),
						});
					}

					self.menu_open = AMirrorMenu::MenuNone;

					if self.open_audio_menu {
						self.request_audio_settings_menu();
					} else if self.next_menu == AMirrorMenu::MenuDisplay {
						std::mem::drop(context);
						self.write_display_menu();
						
						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								let context;
								loop {
									context = match self.context.try_lock() {
										Ok(context) => context,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								context
							}
						};
					} else if self.next_menu == AMirrorMenu::MenuFlash {
						std::mem::drop(context);
						self.write_flash_menu();

						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								let context;
								loop {
									context = match self.context.try_lock() {
										Ok(context) => context,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								context
							}
						};
					}
				} else if ai_msg.l() >= 3 && ai_msg.data[1] == 0x60 { //Entry selected.
					if self.menu_open == AMirrorMenu::MenuAudioSettings {
						let native_mirror = context.imid_native_mirror;
						let rows = context.imid_row_count;
						let char_count = context.imid_text_len;
						
						std::mem::drop(context);

						if ai_msg.data[2] <= 0 {
							return;
						}

						let option = ai_msg.data[2] - 1;

						let audio_menu = match MenuList::new(MenuIndex::MirrorAudioSettings, Locale::LocaleEnglishUS) {
							Some(audio_menu) => audio_menu,
							None => {
								println!("Error loading menu option.");
								return;
							}
						};
						let menu_option = match audio_menu.get_global_index(option  as usize) {
							Some(menu_option) => menu_option,
							None => {
								return;
							}
						};

						if menu_option == MenuIndex::MirrorAudioSettingsScrollInfoText && rows > 0 && char_count > 0 && native_mirror { //Toggle split.
							self.imid_split = !self.imid_split;

							match self.get_audio_settings_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}

							self.imid_refresh = true;
							self.imid_scroll_wrap = false;
							self.imid_scroll_pos = 0;

							self.save_settings();
							
						} else if menu_option == MenuIndex::MirrorAudioSettingsClusterDisplayText && rows > 0 && char_count > 0 && !native_mirror { //Open display menu.
							self.write_display_menu();
						} else if menu_option == MenuIndex::MirrorAudioSettingsAutoMusicStart {
							self.auto_music_start = !self.auto_music_start;
							match self.get_audio_settings_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}

							self.save_settings();
						} else if menu_option == MenuIndex::MirrorAudioSettingsFlashTitle {
							/*self.flash_song_title = !self.flash_song_title;
							match self.get_audio_settings_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}

							self.save_settings();*/

							self.write_flash_menu();
						} else if menu_option == MenuIndex::MirrorAudioSettingsAudioSettings {
							self.clear_menu(false);
							self.open_audio_menu = true;
						}

						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								let context;
								loop {
									context = match self.context.try_lock() {
										Ok(context) => context,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								context
							}
						};
					} else if self.menu_open == AMirrorMenu::MenuDisplay {
						std::mem::drop(context);

						if ai_msg.data[2] <= 0 {
							return;
						}

						let option = ai_msg.data[2] - 1;

						let cluster_menu = match MenuList::new(MenuIndex::MirrorClusterSettings, Locale::LocaleEnglishUS) {
							Some(settings_menu) => settings_menu,
							None => {
								println!("Error loading menu option.");
								return;
							}
						};
						let menu_option = match cluster_menu.get_global_index(option  as usize) {
							Some(menu_option) => menu_option,
							None => {
								return;
							}
						};

						if menu_option == MenuIndex::MirrorClusterSettingsPhone { //Display phone name.
							self.display_phone = !self.display_phone;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
						} else if menu_option == MenuIndex::MirrorClusterSettingsSong { //Display song title.
							self.display_title = !self.display_title;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
						} else if menu_option == MenuIndex::MirrorClusterSettingsArtist { //Display artist name.
							self.display_artist = !self.display_artist;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
						} else if menu_option == MenuIndex::MirrorClusterSettingsAlbum { //Display album name.
							self.display_album = !self.display_album;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
						} else if menu_option == MenuIndex::MirrorClusterSettingsApp { //Display app name.
							self.display_app = !self.display_app;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
						} else if menu_option == MenuIndex::MirrorClusterSettingsScrollCluster || menu_option == MenuIndex::MirrorClusterSettingsScrollInfo { //Scroll.
							self.imid_split = !self.imid_split;
							match self.get_display_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}	
							self.imid_refresh = true;
							self.imid_scroll_wrap = false;
							self.imid_scroll_pos = 0;
						}

						if option <= 5 {
							context = match self.context.try_lock() {
								Ok(context) => context,
								Err(_) => {
									let context;
									loop {
										context = match self.context.try_lock() {
											Ok(context) => context,
											Err(_) => {
												continue;
											}
										};
										break;
									}
									context
								}
							};

							let row_count = context.imid_row_count;
							let native_mirror = context.imid_native_mirror;
							std::mem::drop(context);

							if row_count == 1 && !native_mirror {
								self.imid_scroll = (option) as i8;
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
								if option != 0 && display_selected > row_count && self.display_phone {
									self.display_phone = false;
									match self.get_display_menu_option(0) {
										Some(ai_msg) => {
											self.write_aibus_message(ai_msg);
										}
										None => {}
									}
									display_selected -= 1;
								}
								if option != 1 && display_selected > row_count && self.display_title {
									self.display_title = false;
									match self.get_display_menu_option(1) {
										Some(ai_msg) => {
											self.write_aibus_message(ai_msg);
										}
										None => {}
									}
									display_selected -= 1;
								}
								if option != 2 && display_selected > row_count && self.display_artist {
									self.display_artist = false;
									match self.get_display_menu_option(2) {
										Some(ai_msg) => {
											self.write_aibus_message(ai_msg);
										}
										None => {}
									}
									display_selected -= 1;
								}
								if option != 3 && display_selected > row_count && self.display_album {
									self.display_album = false;
									match self.get_display_menu_option(3) {
										Some(ai_msg) => {
											self.write_aibus_message(ai_msg);
										}
										None => {}
									}
									display_selected -= 1;
								}
								if option != 4 && display_selected > row_count && self.display_app {
									self.display_app = false;
									match self.get_display_menu_option(4) {
										Some(ai_msg) => {
											self.write_aibus_message(ai_msg);
										}
										None => {}
									}
								}
							}

							self.write_all_imid_text();
						}

						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								let context;
								loop {
									context = match self.context.try_lock() {
										Ok(context) => context,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								context
							}
						};
					} else if self.menu_open == AMirrorMenu::MenuFlash {
						if ai_msg.data[2] <= 0 {
							return;
						}

						let option = ai_msg.data[2] - 1;

						let flash_menu = match MenuList::new(MenuIndex::MirrorFlashSettings, Locale::LocaleEnglishUS) {
							Some(audio_menu) => audio_menu,
							None => {
								println!("Error loading menu option.");
								return;
							}
						};
						let menu_option = match flash_menu.get_global_index(option as usize) {
							Some(menu_option) => menu_option,
							None => {
								return;
							}
						};

						if menu_option == MenuIndex::MirrorFlashSettingsFlashTitle {
							self.flash_song_title = !self.flash_song_title;
							match self.get_flash_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}

							self.save_settings();
						} else if menu_option == MenuIndex::MirrorFlashSettingsFlashTitleMirror {
							self.flash_song_title_mirror = !self.flash_song_title_mirror;
							match self.get_flash_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}

							self.save_settings();
						}
					} else if self.menu_open == AMirrorMenu::MenuSettings {
						if ai_msg.data[2] <= 0 {
							return;
						}

						let option = ai_msg.data[2] - 1;

						let settings_menu = match MenuList::new(MenuIndex::MirrorMainSettings, Locale::LocaleEnglishUS) {
							Some(settings_menu) => settings_menu,
							None => {
								println!("Error loading menu option.");
								return;
							}
						};
						let menu_option = match settings_menu.get_global_index(option  as usize) {
							Some(menu_option) => menu_option,
							None => {
								return;
							}
						};

						if menu_option == MenuIndex::MirrorMainSettingsAutoStart {
							self.auto_mirror_start = !self.auto_mirror_start;
							match self.get_main_settings_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}
						} else if menu_option == MenuIndex::MirrorMainSettingsAutoMusicStart {
							self.auto_music_start = !self.auto_music_start;
							match self.get_main_settings_menu_option(option as usize) {
								Some(ai_msg) => {
									self.write_aibus_message(ai_msg);
								}
								None => {}
							}
						}
					}
				} else if ai_msg.l() >= 2 && ai_msg.data[1] == 0x45 {
					std::mem::drop(context);

					self.write_main_settings_menu();

					loop {
						context = match self.context.try_lock() {
							Ok(context) => context,
							Err(_) => {
								continue;
							}
						};
						break;
					}
				} else if ai_msg.l() >= 2 && (ai_msg.data[1] == 0x55 || ai_msg.data[1] == 0x56 || ai_msg.data[1] == 0x57) { //Menu request.
					let requested_menu;
					let command = ai_msg.data[1];
					if ai_msg.l() >= 3 {
						let text_bytes = ai_msg.data[2..].to_vec();
						let utf8_bytes = get_utf8_from_ascii(text_bytes);
						requested_menu = match std::str::from_utf8(&utf8_bytes) {
							Ok(text) => text.to_string(),
							Err(e) => {
								println!("Error: {}", e);
								return;
							}
						};
					} else {
						requested_menu = String::new();
					}

					if requested_menu.eq_ignore_ascii_case("SETTING") {
						if command == 0x55 { //Display the menu.
							std::mem::drop(context);

							self.write_main_settings_menu();

							loop {
								context = match self.context.try_lock() {
									Ok(context) => context,
									Err(_) => {
										continue;
									}
								};
								break;
							}
						} else {
							let mut command_response = [0x2B, command].to_vec();
							if command == 0x57 {
								let menu = match MenuList::new(MenuIndex::MirrorAudioSettings, Locale::LocaleEnglishUS) {
									Some(settings_menu) => settings_menu,
									None => {
										println!("Error loading menu option.");
										return;
									}
								};

								let menu_name = menu.title.as_bytes();
								for b in menu_name {
									command_response.push(*b);
								}
							}

							self.write_aibus_message(AIBusMessage {
								sender: AIBUS_DEVICE_AMIRROR,
								receiver: ai_msg.sender,
								data: command_response,
							});
						}
					}
				}
			}
		} else if ai_msg.sender == AIBUS_DEVICE_IMID {
			if ai_msg.l() >= 2 && ai_msg.data[0] == 0x3B {
				if ai_msg.data[1] == 0x23 && ai_msg.l() >= 4 { //Character count.
					if context.imid_text_len != ai_msg.data[2] || context.imid_row_count != ai_msg.data[3] {
						context.imid_text_len = ai_msg.data[2];
						context.imid_row_count = ai_msg.data[3];

						let display_sel = {
							let mut c = 0;

							if self.display_title {
								c += 1;
							}
							if self.display_artist {
								c += 1;
							}
							if self.display_album {
								c += 1;
							}
							if self.display_app {
								c += 1;
							}
							if self.display_phone {
								c += 1;
							}

							c
						};

						if context.imid_row_count >= 5 {
							if display_sel > context.imid_row_count {
								self.display_app = true;
								self.display_album = true;
								self.display_artist = true;
								self.display_title = true;
								self.display_phone = true;
							}
						} else if context.imid_row_count >= 4 {
							if display_sel > context.imid_row_count {
								self.display_album = true;
								self.display_artist = true;
								self.display_title = true;
								self.display_phone = true;
								self.display_app = false;
							}
						} else if context.imid_row_count >= 3 {
							if display_sel > context.imid_row_count {
								self.display_album = true;
								self.display_artist = true;
								self.display_title = true;
								self.display_phone = false;
								self.display_app = false;
							}
						} else if context.imid_row_count >= 2 {
							if display_sel > context.imid_row_count {
								self.display_artist = true;
								self.display_title = true;
								self.display_album = false;
								self.display_phone = false;
								self.display_app = false;
							}
						} else if context.imid_row_count >= 1 {
							if display_sel > context.imid_row_count {
								self.display_title = true;
								self.display_artist = false;
								self.display_album = false;
								self.display_app = false;
								self.display_phone = false;
							}
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
				if ai_msg.l() >= 3 && ai_msg.data[1] == 0x2 { //Key position.
					self.power_state = ai_msg.data[2]&0xF;
					if self.power_state == 0x0 && (self.door_state&0xC) != 0 && self.powered_on {
						self.run = false;

						match self.mpv_video.try_lock() {
							Ok(mut mpv) => {
								mpv.stop();
							}
							Err(_) => {
								let mut mpv;
								loop {
									mpv = match self.mpv_video.try_lock() {
										Ok(mpv) => mpv,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								mpv.stop();
							}
						}
					} else if self.power_state != 0x0 {
						self.powered_on = true;
					}
				} else if ai_msg.l() >= 3 && ai_msg.data[1] == 0x43 { //Door position.
					self.door_state = ai_msg.data[2];
					if self.power_state == 0x0 && (self.door_state&0xC) != 0 && (self.powered_on || (self.door_state&0x80) != 0) {
						self.run = false;

						match self.mpv_video.try_lock() {
							Ok(mut mpv) => {
								mpv.stop();
							}
							Err(_) => {
								let mut mpv;
								loop {
									mpv = match self.mpv_video.try_lock() {
										Ok(mpv) => mpv,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								mpv.stop();
							}
						}
					}
				} else if ai_msg.l() >= 4 && ai_msg.data[1] == 0x10 { //Light status message.
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
				} else if ai_msg.l() >= 4 && ai_msg.data[1] == 0x11 { //Headlights.
					let low_beam = ai_msg.data[2]&(1<<1) != 0;
					let high_beam = ai_msg.data[2]&(1<<2) != 0;

					if high_beam {
						context.headlight_state = HeadlightState::HeadlightStateHigh;
					} else if low_beam {
						context.headlight_state = HeadlightState::HeadlightStateLow;
					} else {
						context.headlight_state = HeadlightState::HeadlightStateOff;
					}
				} else if ai_msg.l() >= 3 && ai_msg.data[1] == 0x1F { //Common info parameter.
					if ai_msg.l() >= 5 && ai_msg.data[2] == 0x4 { //Speed.
						let speed_info = ai_msg.data[3];
						let mph = speed_info&(1<<7) != 0;
						let decimal_count = (speed_info>>4)&0x7;

						let mut speed: u64 = 0;
						for d in &ai_msg.data[4..] {
							speed <<= 8;
							speed += *d as u64;
						}

						if mph {
							speed = speed*16/10;
						}

						if decimal_count > 3 {
							for _i in 3..decimal_count {
								speed /= 10;
							}
						} else if decimal_count < 3 {
							for _i in decimal_count..3 {
								speed *= 10;
							}
						}

						context.speed_e3 = if speed < i32::MAX as u64 {
							speed as i32
						} else {
							i32::MAX
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

						if self.imid_scroll >= 0 {
							self.imid_scroll_header = true;
						}
					}

					self.imid_refresh = true;
					self.imid_scroll_pos = 0;
					self.scroll_timer = Instant::now();

					let mut overlay_text = "".to_string();

					if self.imid_scroll == 0 { //Phone.
						overlay_text = "Phone: ".to_string() + &context.phone_name;
					} else if self.imid_scroll == 1 { //Song.
						overlay_text = "Track: ".to_string() + &context.song_title;
					} else if self.imid_scroll == 2 { //Artist.
						overlay_text = "Artist: ".to_string() + &context.artist;
					} else if self.imid_scroll == 3 { //Album.
						overlay_text = "Album: ".to_string() + &context.album;
					} else if self.imid_scroll == 4 { //App.
						overlay_text = "App: ".to_string() + &context.app;
					}
					
					if overlay_text.len() > 0 {
						self.write_nav_overlay(overlay_text.replace("#", "##  "));
						let mut mpv_video = match self.mpv_video.try_lock() {
							Ok(mpv_video) => mpv_video,
							Err(_) => {
								let mpv;
								loop {
									mpv = match self.mpv_video.try_lock() {
										Ok(mpv) => mpv,
										Err(_) => {
											continue;
										}
									};
									break;
								}
								mpv
							}
						};

						mpv_video.set_header_overlay(overlay_text);
						self.header_overlay = true;
						self.overlay_timer = Instant::now();
					}
				} else if button == 0x26 { //Audio button.
					if state == 0x2 {
						if !self.audio_held {
							if !context.audio_selected {
								let mut mpv = match self.mpv_video.try_lock() {
									Ok(mpv) => mpv,
									Err(_) => {
										let mpv;
										loop {
											mpv = match self.mpv_video.try_lock() {
												Ok(mpv) => mpv,
												Err(_) => {
													continue;
												}
											};
											break;
										}
										mpv
									}
								};

								self.overlay_on = true;
								self.vol_overlay = false;
								self.overlay_timer = Instant::now();
								mpv.show_overlay();
							} else {
								if context.phone_type == 5 {
									self.aa_handler.show_audio_window();
								}
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
				} else if ((button == 0x46 && context.aibt_pause) || (button == 0x16 && !context.aibt_pause)) && state == 0x0 {
					if context.audio_selected {
						if context.phone_type == 5 {
							self.aa_handler.play_pause_audio();
						} else if context.phone_type == 3 {
							self.dongle_handler.send_carplay_command(PHONE_COMMAND_PLAYPAUSE);
						}
					}
				}
			} else if ai_msg.l() >= 3 && ai_msg.data[0] == 0x31 && ai_msg.data[1] == 0x30 { //Button list.
				let features = ai_msg.data[2];
				context.aibt_nav_knob = (features&0x2) != 0;
				context.aibt_touchscreen = (features&0x1) != 0;

				context.aibt_horizontal_toggle = false;
				context.aibt_vertical_toggle = false;

				context.aibt_audio = false;
				context.aibt_home = false;
				context.aibt_map = false;
				context.aibt_menu = false;
				context.aibt_phone = false;
				context.aibt_function = false;
				context.aibt_pause = false;

				for i in 3..ai_msg.l() {
					let d = ai_msg.data[i];
					if d == 0x28 || d == 0x29 {
						context.aibt_vertical_toggle = true;
					} else if d == 0x2A || d == 0x2B {
						context.aibt_horizontal_toggle = true;
					} else if d == 0x20 {
						context.aibt_home = true;
					} else if d == 0x51 {
						context.aibt_menu = true;
					} else if d == 0x26 {
						context.aibt_audio = true;
					} else if d == 0x50 {
						context.aibt_phone = true;
					} else if d == 0x55 {
						context.aibt_map = true;
					} else if d == 0x16 {
						context.aibt_function = true;
					} else if d == 0x46 {
						context.aibt_pause = true;
					}
				}

				self.screen_acknowledged = true;
			}
		} else if ai_msg.sender == AIBUS_DEVICE_ANTENNA {
			if ai_msg.data[0] == 0x55 && ai_msg.l() >= 14 { //Position message.
				let lat_degrees = ((ai_msg.data[2] as i16) << 8) | ai_msg.data[3] as i16;
				let lat_minutes = ai_msg.data[4]&0x7F;
				let lat_seconds = ai_msg.data[5];
				let lat_second_frac = ai_msg.data[6];
				let lat_negative = (ai_msg.data[4]&0x80) != 0;


				let long_degrees = ((ai_msg.data[7] as i16) << 8) | ai_msg.data[8] as i16;
				let long_minutes = ai_msg.data[9]&0x7F;
				let long_seconds = ai_msg.data[10];
				let long_second_frac = ai_msg.data[11];
				let long_negative = (ai_msg.data[9]&0x80) != 0;

				//Calculate latitude:
				let lat_sign = if lat_degrees >= 0 && !lat_negative{
					1
				} else {
					-1
				};
				let mut latitude = lat_degrees as f64;
				latitude += ((lat_minutes as f64)/60.0)*(lat_sign as f64);
				latitude += ((lat_seconds as f64)/3600.0)*(lat_sign as f64);
				latitude += ((lat_second_frac as f64)/(3600.0*255.0))*(lat_sign as f64);

				context.latitude = latitude;

				//Calculate longitude:
				let long_sign = if long_degrees >= 0 && !long_negative {
					1
				} else {
					-1
				};
				let mut longitude = long_degrees as f64;
				longitude += ((long_minutes as f64)/60.0)*(long_sign as f64);
				longitude += ((long_seconds as f64)/3600.0)*(long_sign as f64);
				longitude += ((long_second_frac as f64)/(3600.0*255.0))*(lat_sign as f64);

				context.longitude = longitude;

				context.altitude = ((ai_msg.data[12] as i32) << 8) | (ai_msg.data[13] as i32);
			}
		} else if ai_msg.sender == AIBUS_DEVICE_PHONE {
			if ai_msg.l() >= MAC_ADDR_LEN + 1 && ai_msg.data[0] == 0x12 { //MAC address.
				for i in 0..MAC_ADDR_LEN {
					context.mac_addr[i] = ai_msg.data[i+1];
				}
				context.mac_set = true;
			}
		}

		if !self.color_acknowledged && ai_msg.sender == AIBUS_DEVICE_NAV_COMPUTER {
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: [0x60, 0x20].to_vec()
			});
		}

		let phone_type = context.phone_type;

		std::mem::drop(context);
		if phone_type == 3 {
			self.dongle_handler.handle_aibus_message(ai_msg.clone());
		} else if phone_type == 5 {
			self.aa_handler.handle_aibus_message(ai_msg);
		}
	}

	///Save the settings file.
	pub fn save_settings(&self) {
		let mut settings = Ini::new();
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_DISPLAY_TITLE, bool::to_string(&self.display_title));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_DISPLAY_ARTIST, bool::to_string(&self.display_artist));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_DISPLAY_ALBUM, bool::to_string(&self.display_album));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_DISPLAY_APP, bool::to_string(&self.display_app));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_DISPLAY_PHONE, bool::to_string(&self.display_phone));

		settings.with_section(Some(SETTING_SECTION)).set(SETTING_AUTOMIRROR, bool::to_string(&self.auto_mirror_start));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_AUTOMUSIC, bool::to_string(&self.auto_music_start));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_FLASHSONG, bool::to_string(&self.flash_song_title));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_FLASHSONG_MIRROR, bool::to_string(&self.flash_song_title_mirror));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_SPLIT, bool::to_string(&self.imid_split));
		settings.with_section(Some(SETTING_SECTION)).set(SETTING_SCROLL_PARAM, i8::to_string(&self.imid_scroll));

		let _ = settings.write_to_file(SETTING_PATH);
	}

	///Write an AIBus message.
	fn write_aibus_message(&mut self, ai_msg: AIBusMessage) {
		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				self.aibus_cache.push(ai_msg);
				return;
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

	///Write multiple AIBus messages.
	fn write_aibus_messages(&mut self, msg_list: &[AIBusMessage]) {
		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				for ai_msg in msg_list {
					self.aibus_cache.push(ai_msg.clone());
				}
				return;
			}
		};

		let ai_tx = aibus_handler.get_ai_tx();
		
		for ai_msg in msg_list {
			ai_tx.push(ai_msg.clone());
		}
	}

	///Write the initial pings.
	pub fn write_init_ping(&mut self) {
		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_RADIO,
			data: [0x1].to_vec(),
		});

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_IMID,
			data: [0x4, 0xE6, 0x3B].to_vec(),
		});

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_SCREEN,
			data: [0x1].to_vec(),
		});

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_CANSLATOR,
			data: [0x4A, 0x1F].to_vec()
		});
	}

	///Write track time.
	fn write_time(&mut self, recipient: u8, time: u64) {
		let mut bytes = 1;
		let mut data_measure: u64 = 0x100;

		while time as u64 >= data_measure {
			bytes += 1;
			data_measure <<= 8;
		}

		let mut time_data = [0x3B].to_vec();

		for i in 0..bytes {
			time_data.push(((time>>(8*(bytes-i-1)))&0xFF) as u8);
		}

		let time_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: recipient,
			data: time_data,
		};

		self.write_aibus_message(time_msg);
	}

	///Write metadata.
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

	///Write metadata to the radio.
	fn write_radio_metadata(&mut self, text: String, position: u8) {
		self.write_metadata(AIBUS_DEVICE_RADIO, text, position);
	}

	///Write the radio handshake.
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

	///Write the device name.
	fn write_radio_name(&mut self) {
		let mut name_data = [0x1, 0x23, 0x0].to_vec();

		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
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

		std::mem::drop(context);
		self.write_radio_short_name();
	}

	///Write the short device name.
	fn write_radio_short_name(&mut self) {
		let mut name_data = [0x1, 0x22, 0x0].to_vec();

		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		let mut name_str = "Mirror".to_string();

		if context.phone_type == 3 {
			name_str = "Carplay".to_string();
		} else if context.phone_type == 5 {
			name_str = "Android".to_string();
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

	///Write metadata to the nav computer.
	fn write_nav_text(&mut self, text: String, position: u8, subtitle: u8, refresh: bool) {
		let mut meta_data = [0x23, 0x60|(subtitle&0xF), position].to_vec();
		
		if refresh {
			meta_data[1] |= 0x10;
		}

		let text_bytes = text.as_bytes();

		for t in text_bytes {
			meta_data.push(*t);
		}

		let meta_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: meta_data,
		};

		self.write_aibus_message(meta_msg);
	}

	///Write text to the nav screen overlay.
	fn write_nav_overlay(&mut self, text: String) {
		let mut text_data = [0x22, 0x61].to_vec();
		let text_bytes = text.as_bytes();

		for t in text_bytes {
			text_data.push(*t);
		}

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: text_data,
		});
	}

	///Write text to the IMID.
	fn write_imid_text(&mut self, text: String, xpos: u8, ypos: u8) {
		let mut meta_data = [0x23, 0x60, xpos, ypos].to_vec();

		let text_bytes = text.as_bytes();

		for t in text_bytes {
			meta_data.push(*t);
		}

		let meta_msg = AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_IMID,
			data: meta_data,
		};

		self.write_aibus_message(meta_msg);
	}

	///Write all metadata to the radio after being selected.
	fn write_all_metadata(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};
		
		self.write_radio_metadata(context.song_title.clone(), SONG_NAME);
		self.write_radio_metadata(context.artist.clone(), ARTIST_NAME);
		self.write_radio_metadata(context.album.clone(), ALBUM_NAME);
		self.write_radio_metadata(context.app.clone(), APP_NAME);
	}

	///Write all relevant text to the nav screen.
	fn write_all_text(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
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

		if !context.aibt_pause {
			self.write_nav_text("#FWD / ||".to_string(), 5, 2, true);
		}

		if context.playing {
			self.write_nav_text("#FWD".to_string(), 1, 1, true);
		} else {
			self.write_nav_text("||".to_string(), 1, 1, true);
		}

		std::mem::drop(context);
		self.write_all_imid_text();
	}

	///Write all relevant text to the IMID.
	fn write_all_imid_text(&mut self) {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		if context.imid_native_mirror {
			self.write_aibus_message(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_IMID,
				data: [0x30, context.phone_type].to_vec(),
			});

			if context.phone_type != 0 {
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
			}
		} else if context.imid_row_count > 0 && context.imid_text_len > 0 {
			if self.imid_scroll < 0 && context.phone_name.len() > 0 && context.phone_type != 0 {

				let mut row = 1;
				if self.display_phone {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.phone_name.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.phone_name.replace("#", "##  "), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_title {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.song_title.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.song_title.replace("#", "##  "), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_artist {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.artist.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.artist.replace("#", "##  "), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_album {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.album.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.album.replace("#", "##  "), imid_x as u8, row);
						row += 1;
					}
				}

				if self.display_app {
					let mut imid_x = (context.imid_text_len/2) as isize - (context.app.len()/2) as isize;
					if imid_x < 0 || imid_x > context.imid_text_len as isize {
						imid_x = 0;
					}

					if row <= context.imid_row_count {
						self.write_imid_text(context.app.replace("#", "##  "), imid_x as u8, row);
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

	///Reset the info display if the info state changed.
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

	///Send the menu creation message and wait for denial. Return true if successful (i.e. not denied).
	fn create_menu(&mut self, title: String, size: u8, audio: bool) -> bool {
		let y_pos: u16 = if audio {
			140
		} else {
			105
		};
		
		let menu_h: u16 = if audio {
			35
		 } else {
			40
		 };
		
		let mut menu_req_data = [0x2B, if audio { 0x5A } else { 0x50 }, size&0x7F, size, 0, 0, (y_pos>>8) as u8, (y_pos&0xFF) as u8, (self.w>>8) as u8, (self.w&0xFF) as u8, (menu_h>>8) as u8, (menu_h&0xFF) as u8].to_vec();

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
			if self.screen_acknowledged {
				self.aa_handler.process();
				self.dongle_handler.process();
			}
		}

		match self.client_aibus_handler.lock() {
			Ok(mut client_handler) => {
				client_handler.process();
			}
			Err(_) => {
				
			}
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

	///Clear the active menu.
	fn clear_menu(&mut self, ack: bool) -> bool {
		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x4A].to_vec(),
		});

		if !ack {
			return true;
		}

		let stream_timer = Instant::now();

		while Instant::now() - stream_timer < Duration::from_millis(100) {
			if self.screen_acknowledged {
				self.aa_handler.process();
				self.dongle_handler.process();
			}
		}

		match self.client_aibus_handler.lock() {
			Ok(mut client_handler) => {
				client_handler.process();
			}
			Err(_) => {
				
			}
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

	///Write the audio settings menu.
	fn write_audio_settings_menu(&mut self) {
		let audio_menu = match MenuList::new(MenuIndex::MirrorAudioSettings, Locale::LocaleEnglishUS) {
			Some(audio_menu) => audio_menu,
			None => {
				println!("Error loading menu.");
				return;
			}
		};
		
		if !self.create_menu(audio_menu.title.to_string(), audio_menu.size() as u8, true) {
			return;
		}

		let mut msg_list = Vec::new();

		for i in 0..audio_menu.size() {
			match self.get_audio_settings_menu_option(i) {
				Some(ai_msg) => {
					msg_list.push(ai_msg);
				}
				None => {}
			}
		}

		let mut sel = 0x3;
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		if !context.imid_row_count > 0 && context.imid_text_len > 0 && !context.imid_native_mirror {
			sel = 0x1;
		} else if !context.imid_row_count > 0 && context.imid_text_len > 0 && context.imid_native_mirror {
			sel = 0x2;
		}

		msg_list.push(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, sel].to_vec(),
		});

		self.write_aibus_messages(&msg_list);

		self.menu_open = AMirrorMenu::MenuAudioSettings;
	}

	///Write an audio settings menu option.
	fn get_audio_settings_menu_option(&mut self, option: usize) -> Option<AIBusMessage> {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		let audio_menu = match MenuList::new(MenuIndex::MirrorAudioSettings, Locale::LocaleEnglishUS) {
			Some(audio_menu) => audio_menu,
			None => {
				println!("Error loading menu option.");
				return None;
			}
		};
		let menu_option = match audio_menu.get_global_index(option) {
			Some(menu_option) => menu_option,
			None => {
				return None;
			}
		};

		let mut menu_req_data = [0x2B, 0x51, option as u8].to_vec();

		let mut req_text: String = "".to_string();

		if menu_option == MenuIndex::MirrorAudioSettingsClusterDisplayText {
			if !(context.imid_row_count > 0 && context.imid_text_len > 0 && !context.imid_native_mirror) {
				return None;
			}
		} else if menu_option == MenuIndex::MirrorAudioSettingsScrollInfoText {		
			if !(context.imid_row_count > 0 && context.imid_text_len > 0 && context.imid_native_mirror) {
				return None;
			}

			if self.imid_split {
				req_text = "#ROF ".to_string();
			} else {
				req_text = "#RON ".to_string();
			}
		} else if menu_option == MenuIndex::MirrorAudioSettingsAutoMusicStart {
			if self.auto_music_start {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		} /*else if menu_option == MenuIndex::MirrorAudioSettingsFlashTitle {
			if self.flash_song_title {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		}*/

		req_text += audio_menu.menu_str[option as usize];

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			return Some(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		} else {
			return None;
		}
	}

	///Write the main settings menu.
	fn write_main_settings_menu(&mut self) {
		let settings_menu = match MenuList::new(MenuIndex::MirrorMainSettings, Locale::LocaleEnglishUS) {
			Some(settings_menu) => settings_menu,
			None => {
				println!("Error loading menu option.");
				return;
			}
		};

		if !self.create_menu(settings_menu.title.to_string(), settings_menu.size() as u8, false) {
			return;
		}

		let mut msg_list = Vec::new();

		for i in 0..settings_menu.size() {
			match self.get_main_settings_menu_option(i) {
				Some(ai_msg) => {
					println!("Writing {}", ai_msg.l());
					msg_list.push(ai_msg);
				}
				None => {}
			}
		}

		let sel = 1;
		msg_list.push(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, sel].to_vec(),
		});

		self.write_aibus_messages(&msg_list);

		self.menu_open = AMirrorMenu::MenuSettings;
	}

	///Write a main setting menu option.
	fn get_main_settings_menu_option(&mut self, option: usize) -> Option<AIBusMessage> {
		let mut menu_req_data = [0x2B, 0x51, option as u8].to_vec();

		let settings_menu = match MenuList::new(MenuIndex::MirrorMainSettings, Locale::LocaleEnglishUS) {
			Some(settings_menu) => settings_menu,
			None => {
				println!("Error loading menu option.");
				return None;
			}
		};
		
		let menu_option = match settings_menu.get_global_index(option) {
			Some(menu_option) => menu_option,
			None => {
				return None;
			}
		};

		let mut req_text: String = "".to_string();

		if menu_option == MenuIndex::MirrorMainSettingsAutoStart {
			if self.auto_mirror_start {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		} else if menu_option == MenuIndex::MirrorMainSettingsAutoMusicStart {
			if self.auto_music_start {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		}

		req_text += settings_menu.menu_str[option];

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			return Some(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		} else {
			return None;
		}
	}

	///Write the display menu.
	fn write_display_menu(&mut self) {
		//self.print_debug("Display menu function called!");

		if self.menu_open != AMirrorMenu::MenuNone {
			self.next_menu = AMirrorMenu::MenuDisplay;
			self.clear_menu(false);
			return;
		}

		let cluster_menu = match MenuList::new(MenuIndex::MirrorClusterSettings, Locale::LocaleEnglishUS) {
			Some(cluster_menu) => cluster_menu,
			None => {
				self.print_debug("Error loading menu option.");
				return;
			}
		};
		
		if !self.create_menu(cluster_menu.title.to_string(), cluster_menu.size() as u8, true) {
			self.print_debug("Create menu failed...");
			return;
		}

		self.next_menu = AMirrorMenu::MenuNone;

		let mut msg_list = Vec::new();

		for i in 0..cluster_menu.size() {
			match self.get_display_menu_option(i) {
				Some(ai_msg) => {
					msg_list.push(ai_msg);
				}
				None => {}
			}
		}

		msg_list.push(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, 0x1].to_vec(),
		});

		self.write_aibus_messages(&msg_list);

		self.menu_open = AMirrorMenu::MenuDisplay;
	}

	///Write a display menu option.
	fn get_display_menu_option(&mut self, option: usize) -> Option<AIBusMessage> {
		let context = match self.context.try_lock() {
			Ok(context) => context,
			Err(_) => {
				let context;
				loop {
					 context = match self.context.try_lock() {
						Ok(context) => context,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				context
			}
		};

		let cluster_menu = match MenuList::new(MenuIndex::MirrorClusterSettings, Locale::LocaleEnglishUS) {
			Some(cluster_menu) => cluster_menu,
			None => {
				self.print_debug("Error loading menu option.");
				return None;
			}
		};
		let menu_option = match cluster_menu.get_global_index(option) {
			Some(menu_option) => menu_option,
			None => {
				self.print_debug(&format!("Menu option {} not found.", option));
				return None;
			}
		};

		let mut menu_req_data = [0x2B, 0x51, option as u8].to_vec();

		let mut req_text = "".to_string();
		let mut unchecked_text = "#ROF ";
		let mut checked_text = "#RON ";

		if context.imid_row_count == 1 {
			unchecked_text = "#COF ";
			checked_text = "#CON ";
		}

		if menu_option == MenuIndex::MirrorClusterSettingsPhone {
			if self.display_phone {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsSong {
			if self.display_title {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsArtist {
			if self.display_artist {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsAlbum {
			if self.display_album {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsApp {
			if self.display_app {
				req_text = checked_text.to_string();
			} else {
				req_text = unchecked_text.to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsScrollCluster {
			if context.imid_row_count != 1 {
				return None;
			}

			if self.imid_split {
				req_text = "#ROF ".to_string();
			} else {
				req_text = "#RON ".to_string();
			}
		} else if menu_option == MenuIndex::MirrorClusterSettingsScrollInfo {
			if context.imid_row_count == 1 {
				return None;
			}

			if self.imid_split {
				req_text = "#ROF ".to_string();
			} else {
				req_text = "#RON ".to_string();
			}
		}
		req_text += cluster_menu.menu_str[option];

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			return Some(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		} else {
			return None;
		}
	}

	///Write the title flash menu.
	fn write_flash_menu(&mut self) {
		if self.menu_open != AMirrorMenu::MenuNone {
			self.next_menu = AMirrorMenu::MenuFlash;
			self.clear_menu(false);
			return;
		}

		let flash_menu = match MenuList::new(MenuIndex::MirrorFlashSettings, Locale::LocaleEnglishUS) {
			Some(cluster_menu) => cluster_menu,
			None => {
				self.print_debug("Error loading menu option.");
				return;
			}
		};
		
		if !self.create_menu(flash_menu.title.to_string(), flash_menu.size() as u8, true) {
			self.print_debug("Create menu failed...");
			return;
		}

		self.next_menu = AMirrorMenu::MenuNone;

		let mut msg_list = Vec::new();

		for i in 0..flash_menu.size() {
			match self.get_flash_menu_option(i) {
				Some(ai_msg) => {
					msg_list.push(ai_msg);
				}
				None => {}
			}
		}

		msg_list.push(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_NAV_COMPUTER,
			data: [0x2B, 0x52, 0x1].to_vec(),
		});

		self.write_aibus_messages(&msg_list);

		self.menu_open = AMirrorMenu::MenuFlash;
	}

	//Write a flash menu option.
	fn get_flash_menu_option(&mut self, option: usize) -> Option<AIBusMessage> {
		let mut menu_req_data = [0x2B, 0x51, option as u8].to_vec();

		let settings_menu = match MenuList::new(MenuIndex::MirrorFlashSettings, Locale::LocaleEnglishUS) {
			Some(settings_menu) => settings_menu,
			None => {
				println!("Error loading menu option.");
				return None;
			}
		};
		
		let menu_option = match settings_menu.get_global_index(option) {
			Some(menu_option) => menu_option,
			None => {
				return None;
			}
		};

		let mut req_text: String = "".to_string();

		if menu_option == MenuIndex::MirrorFlashSettingsFlashTitle {
			if self.flash_song_title {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		} else if menu_option == MenuIndex::MirrorFlashSettingsFlashTitleMirror {
			if self.flash_song_title_mirror {
				req_text = "#RON ".to_string();
			} else {
				req_text = "#ROF ".to_string();
			}
		}

		req_text += settings_menu.menu_str[option];

		let req_text_b = req_text.as_bytes();
		for i in 0..req_text_b.len() {
			menu_req_data.push(req_text_b[i]);
		}

		if req_text_b.len() > 0 {
			return Some(AIBusMessage {
				sender: AIBUS_DEVICE_AMIRROR,
				receiver: AIBUS_DEVICE_NAV_COMPUTER,
				data: menu_req_data,
			});
		} else {
			return None;
		}
	}

	///Request the audio settings menu from the radio.
	fn request_audio_settings_menu(&mut self) {
		self.open_audio_menu = false;

		self.write_aibus_message(AIBusMessage {
			sender: AIBUS_DEVICE_AMIRROR,
			receiver: AIBUS_DEVICE_RADIO,
			data: [0x2B, 0x45].to_vec(),
		});
	}

	///Print a debug AIBus message.
	pub fn print_debug(&mut self, text: &str) {
		let mut data = [0xA1, 0xFF].to_vec();
		for b in text.as_bytes() {
			data.push(*b);
		}

		self.write_aibus_message(AIBusMessage { sender: AIBUS_DEVICE_AMIRROR, receiver: 0xFF, data });
	}
}

///Get a UTF8 string from ASCII.
fn get_utf8_from_ascii(text: Vec<u8>) -> Vec<u8> {
	let mut ret_text = text.clone();

	for i in 0..ret_text.len() {
		let c = ret_text[i];
		//Check whether this is already UTF-8 formatted. Following character should have the same condition.
		if (c&0x80) != 0 {
			let mut is_utf8 = false;
			if i <= ret_text.len() - 2 && (ret_text[i+1]&0x80) != 0 {
				is_utf8 = true;
			} else if i > 0 && (ret_text[i-1]&0x80 != 0) {
				is_utf8 = true;
			}

			//If there weren't any flags to indicate UTF8, correct it.
			if !is_utf8 {
				if (ret_text[i]&0x40) == 0 {
					ret_text.insert(i, 0xC2);
				} else {
					ret_text[i] &= !(0x40 as u8);
					ret_text.insert(i, 0xC3);
				}
			}
		}
	}

	return ret_text;
}
