use std::io::prelude::*;
use std::os::unix::net::UnixStream;
use std::str;
use std::time::Duration;

use crate::AIBusMessage;

const SOCKET_PATH: &str = "/tmp/amirror";
const SOCKET_START: &str = "AidFSock";

pub const OPCODE_AIBUS_SEND: u8 = 0x18;
pub const OPCODE_AIBUS_RECV: u8 = 0x68;

pub struct SocketMessage {
	pub opcode: u8,
	pub data: Vec<u8>,
}

//Get the default UnixStream object.
pub fn init_default_socket() -> Option<UnixStream> {
	return init_socket(SOCKET_PATH.to_string());
}

//Get a UnixStream object.
pub fn init_socket(socket_path: String) -> Option<UnixStream> {
	let stream = match UnixStream::connect(socket_path) {
		Ok(stream) => stream,
		Err(e) => {
			println!("Socket initialization error: {}", e);
			return None;
		}
	};

	let _ = stream.set_read_timeout(Some(Duration::from_millis(1)));
	let _ = stream.set_nonblocking(true);
	return Some(stream);
}

//Read bytes from a socket.
fn read_socket_bytes(stream: &mut UnixStream, data: &mut [u8]) -> usize {
	let l = stream.read(data);
	match l {
		Ok(l) => {
			return l;
		}
		Err(_err) => {
			return 0;
		}
	}
}

//Write bytes to a socket.
fn write_socket_bytes(stream: &mut UnixStream, data: &mut Vec<u8>) -> usize {

	let bytes_written = stream.write(data);
	match bytes_written {
		Ok(bytes_written) => {
			return bytes_written;
		}
		Err(_err) => {
			return 0;
		}
	}
}

//Read a full message from the socket.
pub fn read_socket_message(stream: &mut UnixStream, message_list: &mut Vec<SocketMessage>) -> usize {
	let mut data : [u8; 1024] = [0; 1024];
	let full_l = read_socket_bytes(stream, &mut data);

	if full_l < SOCKET_START.len() {
		return 0;
	}

	let mut byte_vec = Vec::new();
	for i in 0..full_l {
		byte_vec.push(data[i]);
	}
	
	//println!("{:X?}", byte_vec);

	while byte_vec.len() > SOCKET_START.len() + 3 {
		let socket_start_msg = SOCKET_START.as_bytes();
		for i in 0..socket_start_msg.len() {
			if byte_vec[i] != socket_start_msg[i] {
				return 0;
			}
		}

		let mut message = SocketMessage {
			opcode: 0,
			data: Vec::new(),
		};

		message.opcode = byte_vec[socket_start_msg.len()];
		let data_l: u8 = byte_vec[socket_start_msg.len() + 1]-1;
		let start = socket_start_msg.len() + 2;
		let msg_l = data_l as usize + socket_start_msg.len() + 3;

		let mut checksum = 0;
		for i in 0..msg_l-1 {
			checksum ^= byte_vec[i];
		}

		if checksum != byte_vec[msg_l-1] {
			return 0;
		}

		message.data = Vec::new();

		for i in start..msg_l-1 {
			message.data.push(byte_vec[i]);
		}

		message_list.push(message);
		
		for _i in 0..msg_l {
			byte_vec.remove(0);
		}
	}

	return full_l as usize;
}

//Write a full message to the socket.
pub fn write_socket_message(stream: &mut UnixStream, message: SocketMessage) {
	let socket_start_msg = SOCKET_START.as_bytes();
	let mut data: Vec<u8> = vec![0; message.data.len() + socket_start_msg.len() + 3];

	for i in 0..socket_start_msg.len() {
		data[i] = socket_start_msg[i];
	}

	data[socket_start_msg.len()] = message.opcode;
	data[socket_start_msg.len() + 1] = (message.data.len() + 1) as u8;

	for i in 0..message.data.len() {
		data[i + socket_start_msg.len() + 2] = message.data[i];
	}

	let mut checksum: u8 = 0;
	for i in 0..data.len() - 1 {
		checksum ^= data[i];
	}

	let checksum_index = data.len() - 1;
	data[checksum_index] = checksum;

	let _ = write_socket_bytes(stream, &mut data);
}

pub fn write_aibus_message(stream: &mut UnixStream, message: AIBusMessage) {
	let bytes = message.get_bytes();

	let mut socket_msg = SocketMessage {
		opcode: OPCODE_AIBUS_SEND,
		data: vec![0;bytes.len()],
	};

	for i in 0..bytes.len() {
		socket_msg.data[i] = bytes[i];
	}

	write_socket_message(stream, socket_msg);
}
