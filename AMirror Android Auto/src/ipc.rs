use std::io::{BufReader, Write};
use std::io::{Read};
use std::os::unix::net::UnixStream;
use std::str;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use crate::AIBusMessage;
use crate::aibus::*;
use crate::aibus_handler::AIBusHandler;
use crate::context::Context;

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
	multi_expected_len: i32,
	last_multi_message: Instant,

	recent_tx: Vec<AIBusMessage>,

	use_ack_cache: bool,
	auto_ack_cache_off: bool,
	ack_cache: Vec<(AIBusMessage, u32)>,

	process_active: bool,

	context: Arc<Mutex<Context>>,
}

impl ClientAIBusHandler {
	///Get an AIBus handler with the socket path.
	pub fn get(path: &str, ai_handler: Arc<Mutex<AIBusHandler>>, context: Arc<Mutex<Context>>) -> Self {
		let client_handler = if path.len() <= 0 {
			ClientAIBusHandler { socket: init_default_socket(),
								aibus_handler: ai_handler,
								multi_cache: Vec::new(),
								recent_tx: Vec::new(),
								multi_expected_len: -1,
								last_multi_message: Instant::now(),

								use_ack_cache: false,
								auto_ack_cache_off: false,
								ack_cache: Vec::new(),

								process_active: false,
								context }
		} else {
			ClientAIBusHandler { socket: init_socket(path.to_string()),
								aibus_handler: ai_handler,
								multi_expected_len: -1,
								multi_cache: Vec::new(),
								recent_tx: Vec::new(),
								last_multi_message: Instant::now(),

								use_ack_cache: false,
								auto_ack_cache_off: false,
								ack_cache: Vec::new(),

								process_active: false,
								context }
		};

		return client_handler;
	}

	///Run the threaded loop.
	pub fn process(&mut self) {
		self.read_stream();
		self.write_stream();
	}

	///Read the AIBus socket.
	fn read_stream(&mut self) {
		let mut stream = match &self.socket {
			Some(stream) => stream,
			None => {
				return;
			}
		};

		let mut receiver = BufReader::new(stream);
		let mut buffer = [0;1024];

		let ai_data_list;

		let _ = stream.set_read_timeout(Some(Duration::from_millis(12)));

		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				let aibus_handler;
				loop {
					aibus_handler = match self.aibus_handler.try_lock() {
						Ok(aibus_handler) => aibus_handler,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				aibus_handler
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
				return;
			}
		}

		if !self.process_active {
			return;
		}

		let ai_rx = aibus_handler.get_ai_rx();

		for ai_data in ai_data_list {
			if self.multi_expected_len > 0 && self.multi_cache.len() > 0 && Instant::now() - self.last_multi_message > Duration::from_millis(500) {
				self.multi_expected_len = -1;
				self.multi_cache.clear();
			}

			println!("{:X?}", ai_data.get_bytes());

			if ai_data.receiver != 0xFF && ai_data.receiver != AIBUS_DEVICE_AMIRROR {
				continue;
			}

			if ai_data.l() >= 3 && ai_data.data[0] == 0x91 && self.multi_cache.len() > 0 && (self.multi_cache[0].sender != ai_data.sender || self.multi_cache[0].receiver != ai_data.receiver) {
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

			if ai_data.l() >= 1 && ai_data.data[0] == 0x92 { //Resend request.
				for last_tx in &self.recent_tx {
					if last_tx.sender == ai_data.receiver && last_tx.receiver == ai_data.sender {
						let resend_bytes = get_full_bytes(last_tx);
						let _ = stream.write(&resend_bytes); //TODO: Acknowledge?

						break;
					}
				}
			} else if ai_data.l() >= 3 && ai_data.data[0] == 0x91 { //Multi message.
				self.last_multi_message = Instant::now();

				if self.multi_expected_len <= 0 {
					self.multi_expected_len = ai_data.data[1] as i32;
				}

				self.multi_cache.push(ai_data.clone());

				if self.multi_cache.len() as i32 >= self.multi_expected_len {
					let mut full_data = Vec::new();

					for m in 0..self.multi_expected_len {
						for test_msg in &self.multi_cache {
							if test_msg.l() < 3 || test_msg.data[0] != 0x91 || test_msg.data[1] as i32 != self.multi_expected_len {
								continue;
							}

							if test_msg.data[2] as i32 != m {
								continue;
							}

							if test_msg.sender != ai_data.sender || test_msg.receiver != ai_data.receiver {
								continue;
							}

							let mut i = 3;
							while i < test_msg.l() {
								full_data.push(test_msg.data[i]);
								i += 1;
							}
						}
					}

					let full_msg = AIBusMessage {
						sender: ai_data.sender,
						receiver: ai_data.receiver,
						data: full_data,
					};
					ai_rx.push(full_msg);

					self.multi_cache.clear();
					self.multi_expected_len = -1;
				}
			} else {
				ai_rx.push(ai_data);
			}
		}
	
		std::mem::drop(aibus_handler);
	}

	///Write cached data to the AIBus socket.
	fn write_stream(&mut self) {
		let mut stream = match &self.socket {
			Some(stream) => stream,
			None => {
				return;
			}
		};

		let mut aibus_handler = match self.aibus_handler.try_lock() {
			Ok(aibus_handler) => aibus_handler,
			Err(_) => {
				let aibus_handler;
				loop {
					aibus_handler = match self.aibus_handler.try_lock() {
						Ok(aibus_handler) => aibus_handler,
						Err(_) => {
							continue;
						}
					};
					break;
				}
				aibus_handler
			}
		};

		let tx_list = aibus_handler.get_ai_tx();
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

		let use_ack_cache = self.use_ack_cache;
		let mut ack_cache = Vec::new();
		let mut long_indices = Vec::new();

		let mut first_long = 0;
		let mut long_set = false;

		let mut sent_list = Vec::new();
		let mut rx_list = Vec::new();

		for m in 0..ai_tx_list.len() {
			let ai_msg = &ai_tx_list[m];
			let socket_data = get_full_bytes(ai_msg);
			let _ = stream.write(&socket_data);
			println!("Wrote {:X?}", socket_data);

			if !long_set && ai_msg.l() > 0 && ai_msg.data[0] == 0x91 {
				first_long = m;
				long_set = true;
			} else if ai_msg.l() <= 0 || ai_msg.data[0] != 0x91 {
				long_set = false;
			}

			sent_list.push(ai_msg.clone());

			let (radio_connected, screen_connected, bluetooth_connected, imid_connected) = match self.context.try_lock() {
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
				let (rec_ack, rec_msg) = self.await_acknowledgment(ai_msg.clone());

				if !rec_ack && use_ack_cache {
					if ai_msg.l() > 0 && ai_msg.data[0] == 0x91 && long_indices.len() <= 0 {
						for i in first_long..ai_tx_list.len() {
							if ai_tx_list[i].l() > 0 && ai_tx_list[i].data[0] == 0x91 {
								long_indices.push(i);
							}
						}
					} else {
						if long_indices.len() > 0 {
							long_indices.clear();
						}
						ack_cache.push(ai_msg.clone());
					}
				} else if !rec_ack {
					//TODO: If the intended recipient was radio, Bluetooth, or screen, turn connection mode off.
				}

				for r in rec_msg {
					rx_list.push(r.clone());
				}
			}
		}

		tx_list.clear();

		for l in long_indices {
			ack_cache.push(ai_tx_list[l].clone());
		}

		for r in rx_list {
			aibus_handler.get_ai_rx().push(r);
		}

		std::mem::drop(aibus_handler);

		for ai_msg in sent_list {
			self.cache_tx(ai_msg);
		}

		if use_ack_cache {
			for msg in ack_cache {
				self.add_ack_cache(msg);
			}
		}
	}

	///Set the acknowledgment cache.
	pub fn set_ack_cache(&mut self, use_ack_cache: bool, auto_off: bool) {
		self.use_ack_cache = use_ack_cache;
		self.auto_ack_cache_off = auto_off;

		if !self.use_ack_cache {
			self.ack_cache.clear();
		}
	}

	///Add a message to the acknowledgment cache.
	pub fn add_ack_cache(&mut self, ai_msg: AIBusMessage) {
		if !self.use_ack_cache {
			return;
		}

		self.ack_cache.push((ai_msg, 0));
	}

	///Activate the socket handler.
	pub fn activate(&mut self) {
		self.process_active = true;
	}

	///Cache a message to be sent in case it needs to be resent later.
	pub fn cache_tx(&mut self, ai_msg: AIBusMessage) {
		for i in 0..self.recent_tx.len() {
			let test_msg = &self.recent_tx[i];
			if test_msg.sender == ai_msg.sender && test_msg.receiver == ai_msg.receiver {
				self.recent_tx.remove(i);
				break;
			}
		}

		self.recent_tx.push(ai_msg);
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

		let limit = if self.use_ack_cache {
			2
		} else {
			20
		};

		while !ack && tries < limit {
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

	///Call during the main loop- send all cached messages awaiting acknowledgment and wait for acknowledgment in the specified wait period in ms.
	pub fn await_cached_acknowledgment(&mut self, wait: Duration) {
		if !self.use_ack_cache {
			return;
		}

		let mut stream = match &self.socket {
			Some(stream) => stream,
			None => {
				return;
			}
		};

		let aibus_rx = match self.aibus_handler.try_lock() {
			Ok(mut handler) => {
				let dyn_rx = handler.get_ai_rx();
				dyn_rx.clone()
			}	
			Err(_) => {
				return;
			}
		};

		for new_msg in &aibus_rx {
			if new_msg.sender == AIBUS_DEVICE_AMIRROR {
				continue;
			}

			if new_msg.l() <= 0 || new_msg.data[0] != 0x80 {
				continue;
			}

			for i in 0..self.ack_cache.len() {
				let test_msg = &self.ack_cache[i].0;

				if test_msg.sender == new_msg.receiver && test_msg.receiver == new_msg.sender {
					self.ack_cache.remove(i);
					break;
				}
			}
		}

		if self.ack_cache.len() <= 0 {
			if self.auto_ack_cache_off {
				self.use_ack_cache = false;
			}
			return;
		}

		std::mem::drop(aibus_rx);

		let start = Instant::now();
		while Instant::now() - start < wait {
			let mut i = 0;
			while i < self.ack_cache.len() {
				let tx = self.ack_cache[i].0.clone();

				let tx_bytes = tx.get_bytes();
				let _ = stream.write(&tx_bytes);

				let ack = self.await_acknowledgment(tx.clone());
				let add_rx = ack.1;

				match self.aibus_handler.lock() {
					Ok(mut aibus_handler) => {
						let rx = aibus_handler.get_ai_rx();
						for msg in add_rx {
							rx.push(msg);
						}
					}
					Err(_) => {

					}
				}

				if ack.0 {
					self.ack_cache.remove(i);
					continue;
				} else {
					self.ack_cache[i].1 += 1;
				}

				let read_start = Instant::now();
				let mut found = false;
				while Instant::now() - read_start < wait {
					let aibus_rx = match self.aibus_handler.try_lock() {
						Ok(mut handler) => {
							let dyn_rx = handler.get_ai_rx();
							dyn_rx.clone()
						}	
						Err(_) => {
							continue;
						}
					};

					for new_msg in &aibus_rx {
						if new_msg.sender == AIBUS_DEVICE_AMIRROR {
							continue;
						}

						if new_msg.sender == tx.receiver && new_msg.receiver == tx.sender && new_msg.l() > 0 && new_msg.data[0] == 0x80 {
							found = true;
							break;
						}
					}

					if found {
						break;
					}
				}

				if found || self.ack_cache[i].1 > 25 {
					self.ack_cache.remove(i);
					continue;
				}

				if Instant::now() - start >= wait {
					break;
				}

				i += 1;
			}

			if self.ack_cache.len() <= 0 {
				if self.auto_ack_cache_off {
					self.use_ack_cache = false;
				}
				break;
			}
		}
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