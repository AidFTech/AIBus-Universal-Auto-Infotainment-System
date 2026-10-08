mod aibus;
mod aibus_handler;
mod ipc;
mod context;
mod amirror;
mod mirror;
mod aap;
mod text_split;
mod locale;

use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, Instant};

use aibus::*;
use aibus_handler::AIBusHandler;
use ipc::*;
use context::*;
use amirror::*;
use mirror::mpv::{MpvVideo, RdAudio};

fn main() {
	let mutex_context = Arc::new(Mutex::new(Context::new()));
	let thread_context = Arc::clone(&mutex_context);

	let aibus_handler = Arc::new(Mutex::new(AIBusHandler::new()));
	let aibus_thread = Arc::clone(&aibus_handler);

	let client_handler = Arc::new(Mutex::new(ClientAIBusHandler::get("", aibus_thread, thread_context)));
	let client_thread = Arc::clone(&client_handler);
	let client_amirror = Arc::clone(&client_handler);

	let mut mpv_video = None;
	let mut rd_audio = None;
	let mut nav_audio = None;
	let mut mpv_found = 0;

	let mutex_run = Arc::new(Mutex::new(true));
	let mutex_run_clone = Arc::clone(&mutex_run);

	let aibus_handle = thread::spawn( move || {
		let mut run = true;

		while run {
			match mutex_run_clone.try_lock() {
				Ok(mrun) => {
					run = *mrun;
				}
				Err(_) => {

				}
			};
			match client_thread.try_lock() {
				Ok(mut client_handler) => {
					client_handler.process();
					client_handler.await_cached_acknowledgment(Duration::from_millis(10));
				}
				Err(_) => {
					continue;
				}
			}

			thread::sleep(Duration::from_millis(2));
		}
	});

	match client_handler.lock() {
		Ok(mut client_handler) => {
			client_handler.activate();
		}
		Err(_) => {
			
		}
	}

	let fullscreen = is_rpi();

	let mut resolution_response = false;
	let mut resolution_request = false;

	let mut w = 800;
	let mut h = 480;

	while !resolution_request {
		match aibus_handler.try_lock() {
			Ok(mut aibus_handler) => {
				let tx_list = aibus_handler.get_ai_tx();
				tx_list.push(AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: AIBUS_DEVICE_NAV_COMPUTER,
					data: [0x2C, 0xF0].to_vec(),
				});

				resolution_request = true;
			}
			Err(_) => {
				thread::sleep(Duration::from_millis(20));
				continue;
			}
		}
	}

	let mut resolution_start_time = Instant::now();

	while !resolution_response {
		if Instant::now() - resolution_start_time > Duration::from_millis(2000) {
			match aibus_handler.try_lock() {
				Ok(mut aibus_handler) => {
					let tx_list = aibus_handler.get_ai_tx();
					tx_list.push(AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: AIBUS_DEVICE_NAV_COMPUTER,
						data: [0x2C, 0xF0].to_vec(),
					});
				}
				Err(_) => {
					thread::sleep(Duration::from_millis(20));
					continue;
				}
			}

			resolution_start_time = Instant::now();
		}

		match aibus_handler.try_lock() {
			Ok(mut aibus_handler) => {
				let rx_list = aibus_handler.get_ai_rx();

				for msg in rx_list {
					if msg.sender == AIBUS_DEVICE_NAV_COMPUTER && msg.receiver == AIBUS_DEVICE_AMIRROR && msg.l() >= 5 && msg.data[0] == 0x2C { //Resolution info.
						let x_array = [msg.data[1], msg.data[2]];
						let y_array = [msg.data[3], msg.data[4]];

						w = u16::from_be_bytes(x_array);
						h = u16::from_be_bytes(y_array);

						resolution_response = true;
					}
				}
			}
			Err(_) => {
				continue;
			}
		}
	}

	while mpv_found < 3 {
		match MpvVideo::new(w, h, fullscreen) {
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

	let mutex_mpv = Arc::new(Mutex::new(mpv_video.unwrap()));
	let mutex_rdaudio = Arc::new(Mutex::new(rd_audio.unwrap()));
	let mutex_navaudio = Arc::new(Mutex::new(nav_audio.unwrap()));

	let mut amirror = AMirror::new(&mutex_context, aibus_handler, client_amirror, &mutex_mpv, &mutex_rdaudio, &mutex_navaudio, w, h);

	amirror.write_init_ping();

	{
		let mut run_set = false;
		while !run_set {
			match mutex_run.try_lock() {
				Ok(mut run) => {
					*run = amirror.run;
					run_set = true;
				}
				Err(_) => {
					continue;
				}
			}
		}
	}

	while amirror.run {
		amirror.process();

		let mut run_set = false;
		while !run_set {
			match mutex_run.try_lock() {
				Ok(mut run) => {
					*run = amirror.run;
					run_set = true;
				}
				Err(_) => {
					continue;
				}
			}
		}
	}

	amirror.save_settings();
	aibus_handle.join().unwrap();
}

#[cfg(not(target_arch = "aarch64"))]
fn is_rpi() -> bool {
	return false;
}

#[cfg(target_arch = "aarch64")]
fn is_rpi() -> bool {
	return true;
}