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

	let mutex_run = Arc::new(Mutex::new(true));
	let mutex_run_clone = Arc::clone(&mutex_run);

	let radio_connected = Arc::new(Mutex::new(false));
	let radio_connected_aibus = Arc::clone(&radio_connected);

	let imid_connected = Arc::new(Mutex::new(false));
	let imid_connected_aibus = Arc::clone(&imid_connected);

	let screen_connected = Arc::new(Mutex::new(false));
	let screen_connected_aibus = Arc::clone(&screen_connected);

	let bluetooth_connected = Arc::new(Mutex::new(false));
	let bluetooth_connected_aibus = Arc::clone(&bluetooth_connected);

	let aibus_handle = thread::spawn( move || {
		let mut multiple_cache = Vec::new();
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
								ai_tx_vec.push(new_msg);
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
								ai_tx_vec.push(new_msg);
							}
						} else {
							ai_tx_vec.push(ai_data.clone());
						}
					}

					ai_tx.clear();
				}
				Err(_) => {
					//Continue.
				}
			}

			let mut multi_ack = true;

			for ai_msg in ai_tx_vec {
				if !multi_ack && ai_msg.l() > 0 && ai_msg.data[0] == 0x91 {
					continue;
				}

				if ai_msg.l() > 0 && ai_msg.data[0] != 0x91 {
					multi_ack = true;
				}

				write_aibus_message(&mut amirror_stream, ai_msg.clone());

				let msg_copy = ai_msg.clone();
				let mut send_ack = ai_msg.l() >=1 && ai_msg.data[0] != 0x80;
				
				if !send_ack {
					//Do nothing.
				} else if ai_msg.receiver == 0xFF {
					send_ack = false;
				} else if get_init_message(&ai_msg) {
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
				} else if ai_msg.receiver == AIBUS_DEVICE_NAV_SCREEN {
					match screen_connected_aibus.try_lock() {
						Ok(screen_connected) => {
							send_ack = *screen_connected;
						}
						Err(_) => {
							send_ack = true;
						}
					}
				} else if ai_msg.receiver == AIBUS_DEVICE_PHONE {
					match bluetooth_connected_aibus.try_lock() {
						Ok(bluetooth_connected) => {
							send_ack = *bluetooth_connected;
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
							let mut resend = msg_copy.clone();
							
							if resend.l() == 2 && resend.data[0] != 0xA1 {
								resend.data.push(0x0);
							}

							write_aibus_message(&mut amirror_stream, resend);
							num_tries += 1;
						}
					}

					//If the acknowledgment failed.
					if !ack {
						if ai_msg.receiver == AIBUS_DEVICE_RADIO {
							match radio_connected_aibus.try_lock() {
								Ok(mut radio_connected) => {
									*radio_connected = false;
								}
								Err(_) => {
									
								}
							}
						} else if ai_msg.receiver == AIBUS_DEVICE_NAV_SCREEN {
							match screen_connected_aibus.try_lock() {
								Ok(mut screen_connected) => {
									*screen_connected = false;
								}
								Err(_) => {
									
								}
							}
						} else if ai_msg.receiver == AIBUS_DEVICE_PHONE {
							match bluetooth_connected_aibus.try_lock() {
								Ok(mut bluetooth_connected) => {
									*bluetooth_connected = false;
								}
								Err(_) => {
									
								}
							}
						} else if ai_msg.receiver == AIBUS_DEVICE_IMID {
							match imid_connected_aibus.try_lock() {
								Ok(mut imid_connected) => {
									*imid_connected = false;
								}
								Err(_) => {
									
								}
							}
						}
					
						if ai_msg.l() > 0 && ai_msg.data[0] == 0x91 {
							multi_ack = false;
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
			
				//Determine whether to wait a bit for multi-messages.
				if msg_copy.l() > 0 && msg_copy.data[0] == 0x91 {
					thread::sleep(Duration::from_millis(5));
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
							if rx_msg.sender == AIBUS_DEVICE_AMIRROR {
								continue;
							}

							if rx_msg.receiver == AIBUS_DEVICE_AMIRROR && rx_msg.l() >= 1 && rx_msg.data[0] != 0x80 {
								write_aibus_message(&mut amirror_stream, AIBusMessage {
									sender: AIBUS_DEVICE_AMIRROR,
									receiver: rx_msg.sender,
									data: [0x80].to_vec(),
								});
							}

							if rx_msg.l() >= 3 && rx_msg.data[0] == 0x91 && (rx_msg.receiver == AIBUS_DEVICE_AMIRROR || rx_msg.receiver == 0xFF) {
								let expected_len = rx_msg.data[1] as usize;
								multiple_cache.push(rx_msg.clone());

								if multiple_cache.len() >= expected_len {
									println!("91 message complete!");
									let mut pos = 0;

									let mut full_data = Vec::new();

									while pos < expected_len {
										let mut found = false;
										for m in multiple_cache.clone() {
											if m.l() > 3 && m.data[2] == pos as u8 {
												for i in 3..m.l() {
													full_data.push(m.data[i]);
												}
												found = true;
												pos += 1;
											}
										}

										if !found {
											break;
										}
									}

									println!("91 data: {:X?}", full_data);

									if pos >= expected_len {
										let full_msg = AIBusMessage {
											sender: rx_msg.sender,
											receiver: rx_msg.receiver,
											data: full_data,
										};
										ai_rx.push(full_msg);
									}

									multiple_cache.clear();
								}
							} else {
								ai_rx.push(rx_msg.clone());
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

	let fullscreen = false; //TODO: Make true if running on a Pi.

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

	let resolution_start_time = Instant::now();

	while !resolution_response {
		if Instant::now() - resolution_start_time > Duration::from_millis(2000) {
			break;
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

	let mutex_context = Arc::new(Mutex::new(Context::new()));
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

				match screen_connected.lock() {
					Ok(mut connected) => {
						*connected = context.screen_connected;
					}
					Err(_) => {
						continue;
					}
				}

				match bluetooth_connected.lock() {
					Ok(mut connected) => {
						*connected = context.bluetooth_connected;
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
