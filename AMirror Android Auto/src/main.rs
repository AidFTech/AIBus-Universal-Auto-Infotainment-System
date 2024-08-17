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
}
