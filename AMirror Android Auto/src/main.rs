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
			return;
		}
	};

	let mutex_stream = Arc::new(Mutex::new(amirror_stream));
	let mutex_context = Arc::new(Mutex::new(Context::new()));
	let mut amirror = AMirror::new(&mutex_context, &mutex_stream, 800, 480);

	while amirror.run {
		amirror.process();

		let mut msg = SocketMessage {
			opcode: 0,
			data: Vec::new(),
		};

		let mut stream = match mutex_stream.try_lock() {
			Ok(stream) => stream,
			Err(_) => {
				continue;
			}
		};

		if read_socket_message(&mut stream, &mut msg) > 0 {
			if msg.opcode != OPCODE_AIBUS_RECV {
				continue;
			}

			let ai_msg = get_aibus_message(msg.data);

			if ai_msg.sender == AIBUS_DEVICE_RADIO || ai_msg.receiver == AIBUS_DEVICE_AMIRROR || ai_msg.receiver == 0xFF {
				std::mem::drop(stream);
				amirror.handle_aibus_message(ai_msg);
			}
		}
	}
}
