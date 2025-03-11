use core::str;
use std::io::Cursor;
use std::io::Write;
use std::os::unix::net::UnixStream;
use std::process::Command;
use std::process::Stdio;
use std::process::Child;
use std::time::Duration;
use std::time::Instant;

use ab_glyph::FontRef;
use ab_glyph::PxScale;
use image::imageops::flip_vertical;
use image::Rgba;
use image::RgbaImage;
use imageproc::drawing::draw_filled_rect;
use imageproc::drawing::draw_text_mut;

use imageproc::rect::Rect;
use rodio::Decoder as AudioDecoder;
use rodio::OutputStream;
use rodio::OutputStreamHandle;
use rodio::Sink;

use crate::ipc;

const OVERLAY_STR_COUNT: usize = 5;
const EMPTY_STRING: String = String::new();

pub struct MpvVideo {
	process: Child,
	mpv_ipc: Option<UnixStream>,

	w: u16,
	h: u16,

	overlay_str: [String; OVERLAY_STR_COUNT],

	text_color: Rgba<u8>,
	header_color: Rgba<u8>,
}

impl MpvVideo {
	pub fn new(width: u16, height: u16, fullscreen: bool) -> Result<MpvVideo, String> {
		let mut mpv_cmd = Command::new("mpv");
		let process;
		mpv_cmd.arg(format!("--geometry={}x{}", width, height));
		mpv_cmd.arg("--hwdec=rpi");
		mpv_cmd.arg("--demuxer-rawvideo-fps=60");
		mpv_cmd.arg("--untimed");
		mpv_cmd.arg("--osc=no");

		if fullscreen {
			mpv_cmd.arg("--fs=yes");
		}

		mpv_cmd.arg("--fps=60");
		mpv_cmd.arg("--profile=low-latency");
		mpv_cmd.arg("--no-correct-pts");
		//mpv_cmd.arg(format!("--video-aspect-override={}/{}", width, height));
		mpv_cmd.arg("--video-unscaled=yes");
		mpv_cmd.arg("--input-ipc-server=/tmp/mka_cmd");
		mpv_cmd.arg("-");
		match mpv_cmd.stdin(Stdio::piped()).spawn() {
			Err(e) => return Err(format!("Could not start video Mpv: {} ", e)),
			Ok(match_process) => {
				process = Some(match_process);
			}
		}

		let sock_wait = Instant::now();
		while Instant::now() - sock_wait < Duration::from_millis(500) {

		}

		let sock = ipc::init_socket("/tmp/mka_cmd".to_string());

		match sock {
			None => {
				println!("Could not start mpv socket.");
			}
			Some(_) => {
				
			}
		}

		return Ok(MpvVideo {
			process: process.unwrap(),
			mpv_ipc: sock,
			w: width,
			h: height,
			overlay_str: [EMPTY_STRING; OVERLAY_STR_COUNT],

			text_color: Rgba([0,0,0,255]),
			header_color: Rgba([0xFF, 0xFF, 0x3A, 255]),
		 });
	}

	pub fn send_video(&mut self, data: &[u8]) {
		let mut child_stdin = self.process.stdin.as_ref().unwrap();
		let _ = child_stdin.write(&data);
	}

	//Set the overlay text color.
	pub fn set_overlay_text_color(&mut self, color: Rgba<u8>) {
		self.text_color = color;
		self.save_overlay_image();
	}

	//Set the header text color.
	pub fn set_header_text_color(&mut self, color: Rgba<u8>) {
		self.header_color = color;
		self.save_overlay_image();
	}

	//Set the overlay text.
	pub fn set_overlay_text(&mut self, text: String, index: usize) {
		if index >= OVERLAY_STR_COUNT {
			return;
		}

		self.overlay_str[index] = text;
		//TODO: Symbols.
	}

	//Show the overlay.
	pub fn show_overlay(&mut self) {
		let mut mpv_ipc = match &self.mpv_ipc {
			Some(mpv_ipc) => mpv_ipc,
			None => return,
		};

		self.save_overlay_image();

		let overlay_height = self.h/10;
		let file_size = self.w*4;

		let overlay_str = "overlay-add 0 0 0 \"/tmp/overlay.bmp\" 122 bgra ".to_string() +
			&self.w.to_string() + &" ".to_string() + 
			&overlay_height.to_string() + &" ".to_string() +
			&file_size.to_string() + &" \n".to_string();
			
		match mpv_ipc.write(overlay_str.as_bytes()) {
			Ok(_) => {
				
			}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}

	//Clear the overlay.
	pub fn clear_overlay(&mut self) {
		let mut mpv_ipc = match &self.mpv_ipc {
			Some(mpv_ipc) => mpv_ipc,
			None => return,
		};

		let overlay_str = "overlay-remove 0\n";
		match mpv_ipc.write(overlay_str.as_bytes()) {
			Ok(_) => {
				
			}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}
	
	pub fn start(&mut self) {
		//Start video playback.
	}
	
	pub fn stop(&mut self) {
		//Stop video playback.
	}
	
	pub fn set_minimize(&mut self, minimize: bool) {
		let pid = self.process.id();
		let wid_cmd = Command::new("xdotool").arg("search").arg("--pid").arg(format!("{}", pid)).output();

		let wid_vec = match wid_cmd {
			Ok(wid) => wid.stdout,
			Err(_) => {
				return;
			}
		};

		let wid_str = match str::from_utf8(&wid_vec) {
			Ok(wid_str) => wid_str,
			Err(_) => {
				return;
			}
		};

		if minimize {
			let minimize_cmd = Command::new("xdotool").arg("windowminimize").arg(wid_str).output();
			match minimize_cmd {
				Ok(_) => {

				}
				Err(_) => {
					return;
				}
			}
		} else {
			let minimize_cmd = Command::new("xdotool").arg("windowactivate").arg(wid_str).output();
			match minimize_cmd {
				Ok(_) => {

				}
				Err(_) => {
					return;
				}
			}
		}
	}

	//Save an overlay image.
	fn save_overlay_image(&self) {
		let overlay_h = self.h/10;
		let mut overlay_image = RgbaImage::new(self.w as u32, overlay_h as u32);

		overlay_image = draw_filled_rect(&mut overlay_image, Rect::at(0, 0).of_size(self.w as u32, overlay_h as u32), self.header_color);

		let font = match FontRef::try_from_slice(include_bytes!("AidF Font.ttf")) {
			Ok(font) => font,
			Err(e) => {
				println!("Error: {}", e);
				return;
			}
		};

		for i in 0..OVERLAY_STR_COUNT {
			let height;
			
			if i == 0 {
				height = (overlay_h) as f32*6.0/7.0;
			} else {
				height = (overlay_h) as f32*3.0/5.0;
			}
			let scale = PxScale {
				x: height,
				y: height,
			};

			let overlay_text = Self::get_symbol(self.overlay_str[i].clone());
			draw_text_mut(&mut overlay_image, self.text_color, (i*(self.w as usize/OVERLAY_STR_COUNT)) as i32, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &overlay_text);
		}

		overlay_image = flip_vertical(&mut overlay_image);

		match overlay_image.save("/tmp/overlay.bmp") {
			Ok(_) => {}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}

	fn get_symbol(text: String) -> String {
		let mut ret_text = text.clone();
		
		ret_text = ret_text.replace("#UP ", "\u{25B2}");
		ret_text = ret_text.replace("#DN ", "\u{25BC}");
		ret_text = ret_text.replace("#FWD", "\u{25BA}");
		ret_text = ret_text.replace("#REV", "\u{25C4}");

		ret_text = ret_text.replace("#REW", "\u{25C4}\u{25C4}");
		ret_text = ret_text.replace("#FF ", "\u{25BA}\u{25BA}");

		ret_text = ret_text.replace("##  ", "#");

		return ret_text;
	}
}

pub struct RdAudio {
	_stream: OutputStream,
	_handler: OutputStreamHandle,
	sink: Sink,
	data: Vec<u8>,
	sample: u32,
	bits: u16,
	channels: u16,
}

impl RdAudio {
	pub fn new() -> Result<RdAudio, String> {
		let (stream, handler) = OutputStream::try_default().unwrap();
		let sink = Sink::try_new(&handler).unwrap();

		let data = Vec::new();
		return Ok(RdAudio{_stream: stream, _handler: handler, sink, data, sample: 48000, bits: 16, channels: 2});
	}

	pub fn send_audio(&mut self, data: &[u8]) {
		for i in 0..data.len() {
			self.data.push(data[i]);
		}

		let mut new_data = get_wav_header(self.data.len(), self.sample, self.channels, self.bits);

		for i in 0..self.data.len() {
			new_data.push(self.data[i]);
		}
		self.data.clear(); 

		let cursor = Cursor::new(new_data);
		let source = match AudioDecoder::new_wav(cursor) {
			Ok(source) => source,
			Err(err) => {
				println!("Decoder Error: {}", err);
				return;
			}
		};
		
		self.sink.append(source);
	}
	
	pub fn set_audio_profile(&mut self, sample: u32, bits: u16, channels: u16) {
		self.sample = sample;
		self.bits = bits;
		self.channels = channels;
	}
	
	pub fn get_audio_profile(&mut self) -> (u32, u16, u16) {
		return (self.sample, self.bits, self.channels);
	}
}

fn get_wav_header(len: usize, sample: u32, channels: u16, bits: u16) -> Vec<u8> {
	let mut wav_header = Vec::new();

	let riff_str = "RIFF".as_bytes();
	for i in 0..riff_str.len() {
		wav_header.push(riff_str[i]);
	}

	let full_size = ((len + 36) as u32).to_le_bytes();
	for d in full_size {
		wav_header.push(d);
	}

	let wave_str = "WAVEfmt ".as_bytes();
	for i in 0..wave_str.len() {
		wav_header.push(wave_str[i]);
	}

	let meta_size = (16 as u32).to_le_bytes();
	for d in meta_size {
		wav_header.push(d);
	}

	let pcm = (1 as u16).to_le_bytes();
	for d in pcm {
		wav_header.push(d);
	}

	let channels_le = channels.to_le_bytes();
	for d in channels_le {
		wav_header.push(d);
	}

	let sample_le = sample.to_le_bytes();
	for d in sample_le {
		wav_header.push(d);
	}

	let check1 = ((bits as u32)*(channels as u32)*sample/8).to_le_bytes();
	for d in check1 {
		wav_header.push(d);
	}

	let check2 = (bits*channels/8).to_le_bytes();
	for d in check2 {
		wav_header.push(d);
	}

	let bits_le = bits.to_le_bytes();
	for d in bits_le {
		wav_header.push(d);
	}

	let data_str = "data".as_bytes();
	for i in 0..data_str.len() {
		wav_header.push(data_str[i]);
	}

	let len_le = (len as u32).to_le_bytes();
	for d in len_le {
		wav_header.push(d);
	}

	return wav_header;
}

pub fn get_decode_type(decode_num: u32) -> (u32, u16, u16) {
	if decode_num == 1 || decode_num == 2 {
		return (44100, 16, 2);
	} else if decode_num == 3 {
		return(8000, 16, 1);
	} else if decode_num == 4 {
		return (48000, 16, 2);
	} else if decode_num == 5 {
		return(16000, 16, 1);
	} else if decode_num == 6 {
		return(24000, 16, 1);
	} else if decode_num == 7 {
		return(16000, 16, 2);
	} else {
		return (44100, 16, 2);
	}
}
