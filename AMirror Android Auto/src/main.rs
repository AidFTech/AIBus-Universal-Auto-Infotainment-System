mod aibus;
mod aibus_handler;
mod ipc;
mod context;
mod amirror;
mod mirror;
mod aap;
mod text_split;

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
	let aibus_handler = Arc::new(Mutex::new(AIBusHandler::new()));
	let aibus_thread = Arc::clone(&aibus_handler);

	let mut mpv_video = None;
	let mut rd_audio = None;
	let mut nav_audio = None;
	let mut mpv_found = 0;

	while mpv_found < 3 {
		match MpvVideo::new(800, 480) {
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

	let mutex_context = Arc::new(Mutex::new(Context::new()));
	let mut amirror = AMirror::new(&mutex_context, aibus_handler, &mutex_mpv, &mutex_rdaudio, &mutex_navaudio, 800, 480);

	let mutex_run = Arc::new(Mutex::new(true));
	let mutex_run_clone = Arc::clone(&mutex_run);

	let radio_connected = Arc::new(Mutex::new(false));
	let radio_connected_aibus = Arc::clone(&radio_connected);

	let imid_connected = Arc::new(Mutex::new(false));
	let imid_connected_aibus = Arc::clone(&imid_connected);

	let aibus_handle = thread::spawn( move || {
		let mut amirror_stream = match init_default_socket() {
			Some(socket) => socket,
			None => {
				println!("Socket not connected.");
				let mut stream_test = None;
				let mut stream_connected = false;
				while !stream_connected {
					stream_test = match init_default_socket() {
						Some(socket) => {
							stream_connected = true;
							Some(socket)
						}
						None => {
							continue;
						}
					};
				}
				stream_test.unwrap()
			}
		};

		let mut run = true;

		while run {
			match mutex_run_clone.try_lock() {
				Ok(mrun) => {
					run = *mrun;
				}
				Err(_) => {

				}
			};

			let mut ai_tx_vec = Vec::new();

			match aibus_thread.try_lock() {
				Ok(mut aibus_thread) => {
					let ai_tx = aibus_thread.get_ai_tx();

					for ai_data in &mut *ai_tx {
						ai_tx_vec.push(ai_data.clone());
					}

					ai_tx.clear();
				}
				Err(_) => {
					//Continue.
				}
			}

			for ai_msg in ai_tx_vec {
				write_aibus_message(&mut amirror_stream, ai_msg.clone());

				let msg_copy = ai_msg.clone();
				let mut send_ack = ai_msg.l() >=1 && ai_msg.data[0] != 0x80;
				
				if !send_ack {
					//Do nothing.
				} else if ai_msg.receiver == 0xFF {
					send_ack = false;
				} else if ai_msg.receiver == AIBUS_DEVICE_RADIO {
					match radio_connected_aibus.try_lock() {
						Ok(radio_connected) => {
							send_ack = *radio_connected;
						}
						Err(_) => {
							send_ack = true;
						}
					}
				} else if ai_msg.receiver == AIBUS_DEVICE_IMID {
					match imid_connected_aibus.try_lock() {
						Ok(imid_connected) => {
							send_ack = *imid_connected;
						}
						Err(_) => {
							send_ack = true;
						}
					}
				}

				if send_ack { 
					let mut ack = false;
					let mut num_tries = 0;
					let mut last_try = Instant::now();

					let mut msg_cache = Vec::new();
					while !ack && num_tries < 15 {
						let mut msg_list = Vec::new();
		
						if read_socket_message(&mut amirror_stream, &mut msg_list) > 0 {
							for i in 0..msg_list.len() {
								let msg = &msg_list[i];
								if msg.opcode != OPCODE_AIBUS_RECV {
									continue;
								}
					
								let rx_msg = get_aibus_message(msg.data.clone());
								if rx_msg.receiver == msg_copy.sender && rx_msg.sender == msg_copy.receiver && rx_msg.l() >= 1 && rx_msg.data[0] == 0x80 {
									ack = true;
								} else if rx_msg.receiver == AIBUS_DEVICE_AMIRROR {
									write_aibus_message(&mut amirror_stream, AIBusMessage {
										sender: AIBUS_DEVICE_AMIRROR,
										receiver: rx_msg.sender,
										data: [0x80].to_vec(),
									});

									msg_cache.push(rx_msg);
								}
							}
						}
		
						if !ack && Instant::now() - last_try > Duration::from_millis(100) {
							last_try = Instant::now();
							let resend = msg_copy.clone();
							write_aibus_message(&mut amirror_stream, resend);
							num_tries += 1;
						}
					}

					match aibus_thread.try_lock() {
						Ok(mut aibus_thread) => {
							let thread_cache = aibus_thread.get_ai_rx();
							for ai_msg in msg_cache {
								thread_cache.push(ai_msg);
							}
						}
						Err(_) => {
							//Continue.
						}
					}
				}
			}
		
			let mut msg_list = Vec::new();
		
			if read_socket_message(&mut amirror_stream, &mut msg_list) > 0 {
				match aibus_thread.try_lock() {
					Ok(mut aibus_thread) => {
						let ai_rx = aibus_thread.get_ai_rx();

						for i in 0..msg_list.len() {
							let msg = &msg_list[i];
							if msg.opcode != OPCODE_AIBUS_RECV {
								continue;
							}

							println!("{:X?}", msg.data);
				
							let rx_msg = get_aibus_message(msg.data.clone());

							//if rx_msg.receiver != AIBUS_DEVICE_AMIRROR && rx_msg.receiver != 0xFF {
							//	continue;
							//}

							ai_rx.push(rx_msg.clone());

							if rx_msg.receiver == AIBUS_DEVICE_AMIRROR && rx_msg.l() >= 1 && rx_msg.data[0] != 0x80 {
								write_aibus_message(&mut amirror_stream, AIBusMessage {
									sender: AIBUS_DEVICE_AMIRROR,
									receiver: rx_msg.sender,
									data: [0x80].to_vec(),
								});
							}
						}
					}
					Err(_) => {
						continue;
					}
				}
			}
		
			thread::sleep(Duration::from_millis(10));
		}
	});

	while amirror.run {
		amirror.process();

		let context_ul = amirror.get_context();
		match context_ul.try_lock() {
			Ok(context) => {
				match radio_connected.lock() {
					Ok(mut connected) => {
						*connected = context.radio_connected;
					}
					Err(_) => {
						continue;
					}
				}

				match imid_connected.lock() {
					Ok(mut connected) => {
						*connected = context.imid_native_mirror || (context.imid_row_count > 0 && context.imid_text_len >= 8);
					}
					Err(_) => {
						continue;
					}
				}
			}
			Err(_) => {
				continue;
			}
		}
	}

	aibus_handle.join().unwrap();
}
