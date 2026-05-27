use std::io::{BufReader, Write};
use std::io::{Read};
use std::os::unix::net::UnixStream;
use std::str;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use crate::AIBusMessage;
use crate::aibus::*;
use crate::aibus_handler::AIBusHandler;

const SOCKET_PATH: &str = "/tmp/amirror";
const SOCKET_START: &str = "AidFSock";

pub const OPCODE_AIBUS_SEND: u8 = 0x18;
pub const OPCODE_AIBUS_RECV: u8 = 0x68;

/*pub struct SocketMessage {
	pub opcode: u8,
	pub data: Vec<u8>,
}*/

pub struct ClientAIBusHandler {
	socket: Option<UnixStream>,
	aibus_handler: Arc<Mutex<AIBusHandler>>,

	multi_cache: Vec<AIBusMessage>,
}

impl ClientAIBusHandler {
	///Get an AIBus handler with the socket path.
	pub fn get(path: &str, ai_handler: Arc<Mutex<AIBusHandler>>) -> Self {
		let client_handler = if path.len() <= 0 {
			ClientAIBusHandler { socket: init_default_socket(),
								aibus_handler: ai_handler,
								multi_cache: Vec::new() }
		} else {
			ClientAIBusHandler { socket: init_socket(path.to_string()),
								aibus_handler: ai_handler,
								multi_cache: Vec::new() }
		};

		return client_handler;
	}

	///Get the socket path.
	pub fn get_socket(&self) -> &Option<UnixStream> {
		return &self.socket;
	}

	///Run the threaded loop.
	pub fn process(&mut self) {
		let mut stream = match &self.socket {
			Some(stream) => stream,
			None => {
				return;
			}
		};

		let mut receiver = BufReader::new(stream);
		let mut buffer = [0;1024];

		let ai_data_list;

		let _ = stream.set_read_timeout(Some(Duration::from_millis(5)));

		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				return;
			}
		};

		match receiver.read(&mut buffer) {
			Ok(size) => {
				let data = &buffer[0..size];
				ai_data_list = Self::get_aibus_messages(data);
			}
			Err(e) => {
				if e.raw_os_error() != Some(11) {
					println!("{}", e);
				}
				std::mem::drop(aibus_handler);
				return;
			}
		}

		let ai_rx = aibus_handler.get_ai_rx();

		for ai_data in ai_data_list {
			println!("{:X?}", ai_data.get_bytes());

			if ai_data.receiver != 0xFF && ai_data.receiver != AIBUS_DEVICE_AMIRROR {
				continue;
			}

			if ai_data.receiver == AIBUS_DEVICE_AMIRROR && (ai_data.l() > 0 && ai_data.data[0] != 0x80) {
				let ack = AIBusMessage {
					sender: AIBUS_DEVICE_AMIRROR,
					receiver: ai_data.sender,
					data: [0x80].to_vec(),
				};

				let ack_bytes = get_full_bytes(&ack);

				let _ = stream.write(&ack_bytes);
			}

			if ai_data.l() > 3 && ai_data.data[0] == 0x91 { //Multi message.
				self.multi_cache.push(ai_data.clone());

				let expected_len = ai_data.data[1] as usize;

				if ai_data.data[2] + 1 >= ai_data.data[1] {
					let mut pos = 0;
					let mut full_data = Vec::new();

					while pos < self.multi_cache.len() {
						let mut found = false;
						for m in &self.multi_cache {

							if m.l() > 3 && m.data[2] == pos as u8 {
								for i in 3..m.l() {
									full_data.push(m.data[i]);
								}
								found = true;
								pos += 1;
							}

							if !found {
								break;
							}
						}

						if !found {
							break;
						}
					}

					if pos >= expected_len {
						let full_msg = AIBusMessage {
							sender: ai_data.sender,
							receiver: ai_data.receiver,
							data: full_data,
						};
						ai_rx.push(full_msg);

						self.multi_cache.clear();
					}
				}
			} else {
				ai_rx.push(ai_data);
			}
		}
	
		std::mem::drop(aibus_handler);
	}

	///Get an AIBus message from a socket message.
	fn get_aibus_messages(orig_data: &[u8]) -> Vec<AIBusMessage> {
		let mut message_list = Vec::new();
		let mut data = orig_data.to_vec();

		while data.len() > SOCKET_START.len() + 2 {
			if data.len() <= SOCKET_START.len() {
				println!("Wrong data length: {}", data.len());
				return message_list;
			}

			for i in 0..SOCKET_START.len() {
				if data[i] != SOCKET_START.as_bytes()[i] {
					return message_list;
				}
			}

			let opcode = data[SOCKET_START.len()];
			let l = SOCKET_START.len() + 1;
			let start = l + 1;
			let len = data[l] as usize;

			if opcode != OPCODE_AIBUS_RECV {
				println!("Wrong opcode: {:X}", opcode);
				return message_list;
			}

			if start + len > data.len() {
				return message_list;
			}

			let mut ai_bytes = Vec::new();
			for i in start..start+len - 1 {
				ai_bytes.push(data[i]);
			}

			let ai_msg = get_aibus_message(ai_bytes);

			if ai_msg.l() > 0 || ai_msg.sender != 0 || ai_msg.receiver != 0 {
				message_list.push(ai_msg);
			}

			for _ in 0..start+len {
				data.remove(0);
			}
		}

		return message_list;
	}

	///Await an acknowledgment.
	pub fn await_acknowledgment(&self, ai_msg: AIBusMessage) -> (bool, Vec<AIBusMessage>) {
		let mut stream = match &self.socket {
			Some(stream) => stream,
			None => {
				return (false, Vec::new());
			}
		};

		let _ = stream.set_write_timeout(Some(Duration::from_millis(5)));

		let mut receiver = BufReader::new(stream);

		let mut ack = false;
		let mut tries = 0;
		let mut try_timer = Instant::now();

		let mut ai_data_list = Vec::new();
		let mut rec_data_list = Vec::new();

		while !ack && tries < 20 {
			let mut buffer = [0;1024];
			match receiver.read(&mut buffer) {
				Ok(size) => {
					let data = &buffer[0..size];
					ai_data_list = ClientAIBusHandler::get_aibus_messages(data);
				}
				Err(e) => {
					if e.raw_os_error() != Some(11) {
						println!("{}", e);
					}
				}
			}

			for rec_msg in &ai_data_list {
				if rec_msg.receiver != 0xFF && rec_msg.receiver != AIBUS_DEVICE_AMIRROR {
					continue;
				}

				println!("{:X?}", rec_msg.get_bytes());

				if rec_msg.receiver == ai_msg.sender && rec_msg.sender == ai_msg.receiver && rec_msg.l() > 0 && rec_msg.data[0] == 0x80 {
					ack = true;
				} else if rec_msg.receiver == AIBUS_DEVICE_AMIRROR && (rec_msg.l() > 0 && rec_msg.data[0] != 0x80 && rec_msg.data[0] != 0x91) {
					let ack = AIBusMessage {
						sender: AIBUS_DEVICE_AMIRROR,
						receiver: rec_msg.sender,
						data: [0x80].to_vec(),
					};

					let ack_bytes = get_full_bytes(&ack);
					let _ = stream.write(&ack_bytes);

					rec_data_list.push(rec_msg.clone());
				}
			}

			if ack {
				break;
			}
			
			if Instant::now() - try_timer > Duration::from_millis(100) {
				try_timer = Instant::now();
				let socket_data = get_full_bytes(&ai_msg);
				let _ = stream.write(&socket_data);
				tries += 1;
			}

			ai_data_list.clear();
		}

		return (ack, rec_data_list);
	}
}

///Get the default UnixStream object.
pub fn init_default_socket() -> Option<UnixStream> {
	return init_socket(SOCKET_PATH.to_string());
}

///Get a UnixStream object.
pub fn init_socket(socket_path: String) -> Option<UnixStream> {
	let stream = match UnixStream::connect(socket_path) {
		Ok(stream) => stream,
		Err(e) => {
			println!("Socket initialization error: {}", e);
			return None;
		}
	};

	let _ = stream.set_read_timeout(Some(Duration::from_millis(5)));
	//let _ = stream.set_nonblocking(true);
	return Some(stream);
}

///Get the full bytes to send through the socket.
pub fn get_full_bytes(ai_data: &AIBusMessage) -> Vec<u8> {
	let mut data = Vec::new();

	let start_data = SOCKET_START.as_bytes();
	for b in start_data {
		data.push(*b);
	}

	data.push(OPCODE_AIBUS_SEND);
	
	let ai_bytes = ai_data.get_bytes();
	data.push(ai_bytes.len() as u8 + 1);

	for b in ai_bytes {
		data.push(b);
	}

	let mut chex = 0;
	for b in &data {
		chex ^= *b;
	}

	data.push(chex);
	return data;
}