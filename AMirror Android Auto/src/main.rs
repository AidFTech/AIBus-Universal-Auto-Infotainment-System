mod aibus;
mod aibus_handler;
mod ipc;
mod context;
mod amirror;
mod mirror;
mod aap;
mod text_split;
mod locale;

use std::io::Write;
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

	let aibus_handler = Arc::new(Mutex::new(AIBusHandler::new()));
	let aibus_main = Arc::clone(&aibus_handler);
	let aibus_thread = Arc::clone(&aibus_handler);

	let client_handler = Arc::new(Mutex::new(ClientAIBusHandler::get("", aibus_thread)));
	let client_thread = Arc::clone(&client_handler);

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
				}
				Err(_) => {
					continue;
				}
			}

			thread::sleep(Duration::from_millis(10));
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

		send_aibus_data(aibus_handler.clone(), mutex_context.clone(), client_handler.clone());
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
			send_aibus_data(aibus_handler.clone(), mutex_context.clone(), client_handler.clone());

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

	let mut amirror = AMirror::new(&mutex_context, aibus_handler, &mutex_mpv, &mutex_rdaudio, &mutex_navaudio, w, h);

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

		let mut send_aibus = false;

		match aibus_main.try_lock() {
			Ok(mut aibus_handler) => {
				if aibus_handler.get_ai_tx().len() > 0 {
					send_aibus = true;
				}
			}
			Err(_) => {
				send_aibus = false;
			}
		}

		if send_aibus {
			send_aibus_data(aibus_main.clone(), mutex_context.clone(), client_handler.clone());
		}
	}

	amirror.save_settings();
	aibus_handle.join().unwrap();
}

///Write AIBus data to the socket.
fn send_aibus_data(aibus_handler_ptr: Arc<Mutex<AIBusHandler>>, context: Arc<Mutex<Context>>, handler_ptr: Arc<Mutex<ClientAIBusHandler>>) {
	let mut aibus_handler = match aibus_handler_ptr.try_lock() {
		Ok(aibus_handler) => aibus_handler,
		Err(_) => {
			return;
		}
	};
	let tx_list = aibus_handler.get_ai_tx();
	let mut rx_list = Vec::new();

	let client_handler = match handler_ptr.try_lock() {
		Ok(handler) => handler,
		Err(_) => {
			std::mem::drop(aibus_handler);
			return;
		}
	};

	let socket = client_handler.get_socket();
	let mut stream = match socket {
		Some(stream) => stream,
		None => {
			std::mem::drop(aibus_handler);
			return;
		}
	};

	let _ = stream.set_write_timeout(Some(Duration::from_millis(5)));
	let mut ai_tx_list = Vec::new();

	for ai_data in &mut *tx_list {
		if ai_data.l() > AIDATA_LIMIT + 3 {
			let count = ai_data.l() / AIDATA_LIMIT;
			let r = ai_data.l() % AIDATA_LIMIT;

			let total_count = if r > 0 {
				count + 1
			} else {
				count
			};

			for m in 0..count {
				let mut data = [0x91, (total_count&0xFF) as u8, (m&0xFF) as u8].to_vec();
				for i in 0..AIDATA_LIMIT {
					data.push(ai_data.data[i+m*AIDATA_LIMIT]);
				}

				let new_msg = AIBusMessage {
					sender: ai_data.sender,
					receiver: ai_data.receiver,
					data: data,
				};
				ai_tx_list.push(new_msg);
			}

			if r > 0 {
				let mut data = [0x91, (total_count&0xFF) as u8, ((total_count-1)&0xFF) as u8].to_vec();
				for i in 0..r {
					data.push(ai_data.data[i+(total_count-1)*AIDATA_LIMIT]);
				}

				let new_msg = AIBusMessage {
					sender: ai_data.sender,
					receiver: ai_data.receiver,
					data: data,
				};
				ai_tx_list.push(new_msg);
			}
		} else {
			ai_tx_list.push(ai_data.clone());
		}
	}

	for ai_msg in &ai_tx_list {
		let socket_data = get_full_bytes(ai_msg);
		let _ = stream.write(&socket_data);
		println!("Wrote {:X?}", socket_data);

		let (radio_connected, screen_connected, bluetooth_connected, imid_connected) = match context.try_lock() {
			Ok(context) => {
				(context.radio_connected, context.screen_connected, context.bluetooth_connected, context.imid_connected)
			}
			Err(_) => {
				(true, true, true, true)
			}
		};
		
		let mut ack = true;
		if ai_msg.receiver == 0xFF {
			ack = false;
		} else if ai_msg.receiver == AIBUS_DEVICE_RADIO {
			ack = radio_connected;
		} else if ai_msg.receiver == AIBUS_DEVICE_NAV_SCREEN {
			ack = screen_connected;
		} else if ai_msg.receiver == AIBUS_DEVICE_PHONE {
			ack = bluetooth_connected;
		} else if ai_msg.receiver == AIBUS_DEVICE_IMID {
			ack = imid_connected;
		}

		if ack {
			let (rec_ack, rec_msg) = client_handler.await_acknowledgment(ai_msg.clone());

			if !rec_ack {
				//TODO: If the intended recipient was radio, Bluetooth, or screen, turn connection mode off.
			}

			for r in rec_msg {
				rx_list.push(r.clone());
			}
		}
	}

	tx_list.clear();

	for r in rx_list {
		aibus_handler.get_ai_rx().push(r);
	}

	std::mem::drop(aibus_handler);
}

#[cfg(not(target_arch = "aarch64"))]
fn is_rpi() -> bool {
	return false;
}

#[cfg(target_arch = "aarch64")]
fn is_rpi() -> bool {
	return true;
}