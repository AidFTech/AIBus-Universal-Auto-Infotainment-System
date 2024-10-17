mod aibus;
mod ipc;
mod context;
mod amirror;
mod mirror;

use std::sync::{Arc, Mutex};

use aibus::*;
use ipc::*;
use context::*;
use amirror::*;

fn main() {
	let amirror_stream = match init_default_socket() {
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

	let mutex_stream = Arc::new(Mutex::new(amirror_stream));
	let mutex_context = Arc::new(Mutex::new(Context::new()));
	let mut amirror = AMirror::new(&mutex_context, &mutex_stream, 800, 480);

	while amirror.run {
		amirror.process();

		let mut stream = match mutex_stream.try_lock() {
			Ok(stream) => stream,
			Err(_) => {
				continue;
			}
		};

		let mut msg_list = Vec::new();
		let bytes_pending = read_socket_message(&mut stream, &mut msg_list);

		if bytes_pending > 0 {
			std::mem::drop(stream);
			while msg_list.len() > 0 {
				stream = match mutex_stream.try_lock() {
					Ok(stream) => stream,
					Err(_) => {
						break;
					}
				};
				let msg = &msg_list[0];

				if msg.opcode != OPCODE_AIBUS_RECV {
					break;
				}

				println!("{:X?}", msg.data);

				let ai_msg = get_aibus_message(msg.data.clone());

				if ai_msg.l() >= 1 && (ai_msg.sender == AIBUS_DEVICE_RADIO || ai_msg.receiver == AIBUS_DEVICE_AMIRROR || ai_msg.receiver == 0xFF) {
					std::mem::drop(stream);
					amirror.handle_aibus_message(ai_msg);
				}

				msg_list.remove(0);
			}
		}
	}
}
