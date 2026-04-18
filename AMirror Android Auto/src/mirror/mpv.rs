use core::str;
use std::fs::OpenOptions;
use std::io::Read;
use std::io::Write;
use std::os::unix::net::UnixStream;
use std::process::Command;
use std::process::Stdio;
use std::process::Child;
use std::time::Duration;
use std::time::Instant;

use ab_glyph::Font;
use ab_glyph::FontRef;
use ab_glyph::PxScale;
use ab_glyph::ScaleFont;
use image::imageops::flip_vertical;
use image::Rgba;
use image::RgbaImage;
use imageproc::drawing::draw_filled_rect;
use imageproc::drawing::draw_hollow_rect;
use imageproc::drawing::draw_polygon_mut;
use imageproc::drawing::draw_text_mut;

use imageproc::point::Point;
use imageproc::rect::Rect;

use crate::ipc;

const OVERLAY_STR_COUNT: usize = 5;
const EMPTY_STRING: String = String::new();

const MPV_PATH: &str = "/tmp/amirror_mpv";

pub struct MpvVideo {
	//process: Child,
	mpv_ipc: Option<UnixStream>,

	w: u16,
	h: u16,

	overlay_str: [String; OVERLAY_STR_COUNT],
	overlay_vol: u8,
	overlay_vol_limit: u8,

	text_color: Rgba<u8>,
	header_color: Rgba<u8>,

	fullscreen: bool,
	x_pan: f32,
}

impl MpvVideo {
	pub fn new(width: u16, height: u16, fullscreen: bool) -> Result<MpvVideo, String> {
		let mut full_w = width;
		let mut full_h = height;

		let mut pos = 0;

		let res_print = match Command::new("cat").arg("/sys/class/graphics/fb0/virtual_size").output() {
			Ok(output) => Some(output),
			Err(e) => {
				println!("{}", e);
				None
			}
		};

		match res_print {
			Some(res_print) => {
				let res_str = String::from_utf8_lossy(&res_print.stdout);
				for s in res_str.split(",") {
					let val = s.to_string();

					if pos == 0 {
						full_w = match val.parse::<u16>() {
							Ok(w) => w,
							Err(_) => width,
						};
					} else if pos == 1 {
						full_h = match val.parse::<u16>() {
							Ok(h) => h,
							Err(_) => width,
						};
					}

					pos += 1;
				}
			}
			None => {

			}
		}

		//Calculate pan.
		let mut pan_x = "".to_string();
		let mut pan_y = "".to_string();

		if fullscreen {
			if full_w > width {
				pan_x = "--video-pan-x=-".to_string();
				let pan_x_f = ((full_w - width) as f32)/(width as f32)/2.0;
				pan_x += &f32::to_string(&pan_x_f);
			}

			if full_h > height {
				pan_y = "--video-pan-y=-".to_string();
				let pan_y_f = ((full_h-height) as f32)/(height as f32)/2.0;
				pan_y += &f32::to_string(&pan_y_f);
			}
		}

		//Start FIFO.
		let _ = Command::new("mkfifo").arg(format!("{}", MPV_PATH)).spawn();

		//Start MPV.
		/*let mut mpv_cmd = Command::new("mpv");
		let process;
		mpv_cmd.arg("--vf=format=fmt=rgb24");
		mpv_cmd.arg("--of=rawvideo");
		mpv_cmd.arg("--ovc=rawvideo");
		mpv_cmd.arg("--no-cache");

		mpv_cmd.arg(format!("--geometry={}x{}+0+0", width, height));
		mpv_cmd.arg(pan_x);
		mpv_cmd.arg(pan_y);
		mpv_cmd.arg("--hwdec=no");
		mpv_cmd.arg("--demuxer-rawvideo-fps=60");
		mpv_cmd.arg("--untimed");

		if fullscreen {
			mpv_cmd.arg("--fs=yes");
		} else {
			mpv_cmd.arg("--osc=no");
		}

		mpv_cmd.arg("--fps=60");
		mpv_cmd.arg("--profile=low-latency");
		mpv_cmd.arg("--no-correct-pts");
		//mpv_cmd.arg(format!("--video-aspect-override={}/{}", width, height));
		mpv_cmd.arg("--video-unscaled=yes");
		mpv_cmd.arg("--input-ipc-server=/tmp/mka_cmd");
		mpv_cmd.arg("-");

		mpv_cmd.arg(format!("--o={}", MPV_PATH));

		match mpv_cmd.stdin(Stdio::piped()).stdout(Stdio::piped()).spawn() {
			Err(e) => return Err(format!("Could not start video Mpv: {} ", e)),
			Ok(match_process) => {
				process = Some(match_process);
			}
		}*/

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

		let x_pan = if full_w > width {
			((full_w - width) as f32)/(width as f32)/2.0
		} else {
			0.0
		};

		return Ok(MpvVideo {
			//process: process.unwrap(),
			mpv_ipc: sock,

			w: width,
			h: height,

			overlay_str: [EMPTY_STRING; OVERLAY_STR_COUNT],
			overlay_vol: 0,
			overlay_vol_limit: 64,

			text_color: Rgba([0,0,0,255]),
			header_color: Rgba([0xFF, 0xFF, 0x3A, 255]),

			fullscreen,
			x_pan,
		 });
	}

	///Loop function.
	pub fn process(&mut self) {
		
	}

	///Send video bytes.
	pub fn send_video(&mut self, data: &[u8]) {
		/*let mut child_stdin = self.process.stdin.as_ref().unwrap();
		let _ = child_stdin.write(&data);*/

		match OpenOptions::new().write(true).open(MPV_PATH) {
			Ok(mut stdin) => {
				let _ = stdin.write_all(data);
				let _ = stdin.flush();
			}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}

	///Set the overlay text color.
	pub fn set_overlay_text_color(&mut self, color: Rgba<u8>) {
		self.text_color = color;
		self.save_overlay_image();
	}

	///Set the header text color.
	pub fn set_header_text_color(&mut self, color: Rgba<u8>) {
		self.header_color = color;
		self.save_overlay_image();
	}

	///Set the overlay text.
	pub fn set_overlay_text(&mut self, text: String, index: usize) {
		if index >= OVERLAY_STR_COUNT {
			return;
		}

		self.overlay_str[index] = text;
	}

	///Set the volume overlay.
	pub fn set_volume_overlay(&mut self, new_vol: u8, new_max: u8) {
		self.overlay_vol = new_vol;
		self.overlay_vol_limit = new_max;
		self.save_volume_image();
	}

	///Set a single-entry header overlay.
	pub fn set_header_overlay(&mut self, text: String) {
		self.save_header_image(text);

		let mut mpv_ipc = match &self.mpv_ipc {
			Some(mpv_ipc) => mpv_ipc,
			None => return,
		};

		let overlay_height = self.h/10;
		let file_size = self.w*4;

		let overlay_str = "overlay-add 0 0 0 \"/tmp/overlay_header.bmp\" 122 bgra ".to_string() +
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

	///Show the overlay.
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

	///Show the volume overlay.
	pub fn show_volume_overlay(&mut self) {
		let mut mpv_ipc = match &self.mpv_ipc {
			Some(mpv_ipc) => mpv_ipc,
			None => return,
		};

		let overlay_height = self.h/10;
		let file_size = self.w*4;

		let overlay_str = "overlay-add 0 0 0 \"/tmp/overlay_volume.bmp\" 122 bgra ".to_string() +
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

	///Clear the overlay.
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
	
	///Start video playback.
	pub fn start(&mut self) {
		//Start video playback.
	}
	
	///Stop video playback.
	pub fn stop(&mut self) {
		/*match self.process.kill() {
			Ok(_) => {

			}
			Err(e) => {
				println!("Error: {}", e);
			}
		}*/
	}
	
	///Minimize or maximize the video window.
	pub fn set_minimize(&mut self, minimize: bool) {
		if self.fullscreen {
			let pan_str = f32::to_string(&-self.x_pan);

			let minimize_str = "set video-pan-x ".to_string() + if minimize {
				"1"
			} else {
				&pan_str
			} + "\n";

			let mut mpv_ipc = match &self.mpv_ipc {
				Some(mpv_ipc) => mpv_ipc,
				None => return,
			};

			match mpv_ipc.write(minimize_str.as_bytes()) {
				Ok(_) => {
					
				}
				Err(e) => {
					println!("Error: {}", e);
				}
			}
		 } else {
			/*let pid = self.process.id();
			let wid_cmd = Command::new("xdotool").arg("search").arg("--pid").arg(format!("{}", pid)).output();

			let wid_vec = match wid_cmd {
				Ok(wid) => wid.stdout,
				Err(_) => {
					println!("No window found for process {}.", pid);
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
			}*/
		}
	}
	

	///Save an overlay image.
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

			let overlay_text: Vec<char> = Self::get_symbol(self.overlay_str[i].clone()).chars().collect();

			let mut displayed_text = "".to_string();
			let mut str_x = (i*(self.w as usize/OVERLAY_STR_COUNT)) as i32;

			for j in 0..overlay_text.len() {
				let c = overlay_text[j];
				if c != '\u{25B2}' && c != '\u{25BC}' && c != '\u{25BA}' && c != '\u{25C4}' {
					displayed_text += &c.to_string();
				} else {
					draw_text_mut(&mut overlay_image, self.text_color, str_x, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &displayed_text);
					str_x += Self::get_text_width(displayed_text.clone(), font.clone(), height) as i32;
					displayed_text = "".to_string();
					
					let triangle_height = height*0.8;

					Self::draw_triangle(&mut overlay_image, c, &mut str_x, (overlay_h/2 - (triangle_height as u16)/2) as i32, triangle_height as i32, self.text_color);
				}

				if j >= overlay_text.len() - 1 {
					draw_text_mut(&mut overlay_image, self.text_color, str_x, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &displayed_text);
				}
			}
		}

		overlay_image = flip_vertical(&mut overlay_image);

		match overlay_image.save("/tmp/overlay.bmp") {
			Ok(_) => {}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}

	///Draw a triangle.
	fn draw_triangle(image: &mut RgbaImage, c: char, x_pos: &mut i32, y_pos: i32, height: i32, color: Rgba<u8>) {
		let mut triangle = [Point::new(0,0);3];
		let mut draw_triangle = false;

		if c == '\u{25B2}' { //Up.
			triangle[0].x = *x_pos;
			triangle[0].y = y_pos + height;
			triangle[1].x = *x_pos + height/2;
			triangle[1].y = y_pos;
			triangle[2].x = *x_pos + height;
			triangle[2].y = y_pos + height;
			draw_triangle = true;
		} else if c == '\u{25BC}' { //Down.
			triangle[0].x = *x_pos;
			triangle[0].y = y_pos;
			triangle[1].x = *x_pos + height/2;
			triangle[1].y = y_pos + height;
			triangle[2].x = *x_pos + height;
			triangle[2].y = y_pos;
			draw_triangle = true;
		} else if c == '\u{25BA}' { //Right.
			triangle[0].x = *x_pos;
			triangle[0].y = y_pos;
			triangle[1].x = *x_pos;
			triangle[1].y = y_pos + height;
			triangle[2].x = *x_pos + height;
			triangle[2].y = y_pos + height/2;
			draw_triangle = true;
		} else if c == '\u{25C4}' { //Left.
			triangle[0].x = *x_pos + height;
			triangle[0].y = y_pos;
			triangle[1].x = *x_pos + height;
			triangle[1].y = y_pos + height;
			triangle[2].x = *x_pos;
			triangle[2].y = y_pos + height/2;
			draw_triangle = true;
		}

		if draw_triangle {
			draw_polygon_mut(image, &triangle, color);
			*x_pos += height;
		}
	}

	///Calculate the text width.
	fn get_text_width(text: String, font: FontRef, size: f32) -> f32 {
		let text_chars: Vec<char> = text.chars().collect();
		let mut total_w = 0.0;

		for c in text_chars {
			let w = font.as_scaled(size).h_advance(font.glyph_id(c));
			total_w += w;
		}

		return total_w;
	}


	///Save the overlay header image.
	fn save_header_image(&self, text: String) {
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

		let height = (overlay_h) as f32*3.0/5.0;
		let scale = PxScale {
			x: height,
			y: height,
		};

		let overlay_text: Vec<char> = Self::get_symbol(text.clone()).chars().collect();

		let mut displayed_text = "".to_string();
		let mut str_x = (self.w as i32)/2 - (Self::get_text_width(text, font.clone(), height) as i32)/2;
	
		for j in 0..overlay_text.len() {
			let c = overlay_text[j];
			if c != '\u{25B2}' && c != '\u{25BC}' && c != '\u{25BA}' && c != '\u{25C4}' {
				displayed_text += &c.to_string();
			} else {
				draw_text_mut(&mut overlay_image, self.text_color, str_x, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &displayed_text);
				str_x += Self::get_text_width(displayed_text.clone(), font.clone(), height) as i32;
				displayed_text = "".to_string();
				
				let triangle_height = height*0.8;

				Self::draw_triangle(&mut overlay_image, c, &mut str_x, (overlay_h/2 - (triangle_height as u16)/2) as i32, triangle_height as i32, self.text_color);
			}

			if j >= overlay_text.len() - 1 {
				draw_text_mut(&mut overlay_image, self.text_color, str_x, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &displayed_text);
			}
		}

		overlay_image = flip_vertical(&mut overlay_image);

		match overlay_image.save("/tmp/overlay_header.bmp") {
			Ok(_) => {}
			Err(e) => {
				println!("Error: {}", e);
			}
		}
	}

	///Save the overlay image for volume.
	fn save_volume_image(&self) {
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

		let height = (overlay_h) as f32*3.0/5.0;
		let scale = PxScale {
			x: height,
			y: height,
		};

		let vol_bar_spacing = 2;

		overlay_image = draw_hollow_rect(&mut overlay_image, Rect::at((self.w/4) as i32, (overlay_h/2 - (height as u16)/2) as i32).of_size((self.w/4) as u32, height as u32), self.text_color);

		if (self.w/4 - 2*vol_bar_spacing as u16)*(self.overlay_vol as u16)/(self.overlay_vol_limit as u16) > 0 {
			overlay_image = draw_filled_rect(&mut overlay_image, Rect::at((self.w/4) as i32 + vol_bar_spacing as i32, (overlay_h/2 - (height as u16)/2) as i32 + vol_bar_spacing as i32).of_size(((self.w/4 - 2*vol_bar_spacing as u16)*(self.overlay_vol as u16)/(self.overlay_vol_limit as u16)) as u32, height as u32 - 2*vol_bar_spacing), self.text_color);
		}

		draw_text_mut(&mut overlay_image, self.text_color, 0, (overlay_h/2 - (height as u16)/2) as i32, scale, &font, &("Volume: ".to_string() + &self.overlay_vol.to_string()));

		overlay_image = flip_vertical(&mut overlay_image);

		match overlay_image.save("/tmp/overlay_volume.bmp") {
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
	//_stream: OutputStream,
	//_handler: OutputStreamHandle,
	//sink: Sink,
	process: Child,
	data: Vec<u8>,
	sample: u32,
	bits: u16,
	channels: u16,
	send_header: bool,
}

impl RdAudio {
	pub fn new() -> Result<RdAudio, String> {
		/*let (stream, handler) = match OutputStream::try_default() {
			Ok(sh) => sh,
			Err(e) => {
				return Err(e.to_string());
			}
		};
		let sink = match Sink::try_new(&handler) {
			Ok(sh) => sh,
			Err(e) => {
				return Err(e.to_string());
			}
		};

		let data = Vec::new();
		return Ok(RdAudio{_stream: stream, _handler: handler, sink, data, sample: 48000, bits: 16, channels: 2});*/

		let mut mpv_cmd = Command::new("mpv");
		let process;
		
		mpv_cmd.arg("-");
		mpv_cmd.arg("--no-config");
		mpv_cmd.arg("--profile=low-latency");
		mpv_cmd.arg("--audio-samplerate=48000");
		mpv_cmd.arg("--audio-format=s32");
		mpv_cmd.arg("--audio-channels=2");
		match mpv_cmd.stdin(Stdio::piped()).spawn() {
			Err(e) => return Err(format!("Could not start audio Mpv: {} ", e)),
			Ok(match_process) => {
				process = Some(match_process);
			}
		}

		let mut this = RdAudio { process: process.unwrap(), data: Vec::new(), sample: 48000, bits: 32, channels: 2, send_header: true };
		this.send_audio(&[]);

		return Ok(this);
	}

	///Send audio bytes.
	pub fn send_audio(&mut self, data: &[u8]) {
		for i in 0..data.len() {
			self.data.push(data[i]);
		}

		let mut new_data = if self.send_header {
			self.send_header = false;
			get_wav_header(self.data.len(), self.sample, self.channels, self.bits)
		} else {
			Vec::new()
		};

		for i in 0..self.data.len() {
			new_data.push(self.data[i]);
		}
		self.data.clear(); 

		/*let cursor = Cursor::new(new_data);
		let source = match AudioDecoder::new_wav(cursor) {
			Ok(source) => source,
			Err(err) => {
				println!("Decoder Error: {}", err);
				return;
			}
		};*/
		
		//self.sink.append(source);
		let mut child_stdin = self.process.stdin.as_ref().unwrap();
		let _ = child_stdin.write(&new_data);
	}
	
	///Set the audio sample rate, bits, and channel count. Restart the audio stream as needed.
	pub fn set_audio_profile(&mut self, sample: u32, bits: u16, channels: u16) {
		let last_sample = self.sample;
		let last_bits = self.bits;
		let last_channels = self.channels;

		self.sample = sample;
		self.bits = bits;
		self.channels = channels;

		if last_sample != sample || last_bits != bits || last_channels != channels {
			self.send_header = true;

			let _ = self.process.kill();

			let process = match self.get_audio_process() {
				Some(process) => process,
				None => {
					return;
				}
			};

			self.process = process;
			self.send_audio(&[]);
		}
	}
	
	///Get the audio sample rate, bits, and channel count.
	pub fn get_audio_profile(&mut self) -> (u32, u16, u16) {
		return (self.sample, self.bits, self.channels);
	}

	///Get an audio process.
	fn get_audio_process(&self) -> Option<Child> {
		let mut mpv_cmd = Command::new("mpv");
		let process;

		let mut audio_format = "".to_string();
		if self.bits == 8 {
			audio_format = "--audio-format=u8".to_string();
		} else if self.bits == 16 {
			audio_format = "--audio-format=s16".to_string();
		} else if self.bits == 32 {
			audio_format = "--audio-format=s32".to_string();
		} else if self.bits == 64 {
			audio_format = "--audio-format=s64".to_string();
		}

		mpv_cmd.arg("-");
		mpv_cmd.arg("--no-config");
		mpv_cmd.arg("--profile=low-latency");
		mpv_cmd.arg(format!("--audio-samplerate={}", self.sample));
		mpv_cmd.arg(audio_format);
		mpv_cmd.arg(format!("--audio-channels={}", self.channels));
		match mpv_cmd.stdin(Stdio::piped()).spawn() {
			Err(e) => {
				println!("Error: {}", e);
				return None;
			}
			Ok(match_process) => {
				process = Some(match_process);
			}
		}

		return process;
	}
}

///Get the WAV byte header.
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
