//AIBus implementation

// Devices
pub const AIBUS_DEVICE_NAV_COMPUTER: u8 = 0x1;
pub const AIBUS_DEVICE_NAV_SCREEN: u8 = 0x7;
pub const AIBUS_DEVICE_RADIO: u8 = 0x10;
pub const AIBUS_DEVICE_AMIRROR: u8 = 0x8E;
pub const AIBUS_DEVICE_CANSLATOR: u8 = 0x57;
pub const AIBUS_DEVICE_IMID: u8 = 0x11;

pub struct AIBusMessage {
	pub sender: u8,
	pub receiver: u8,
	pub data: Vec<u8>
}

impl Clone for AIBusMessage {
	fn clone(&self) -> Self {
		let mut new_data: Vec<u8> = vec![0;self.data.len()];
		for i in 0..self.data.len() {
			new_data[i] = self.data[i];
		}

		return AIBusMessage {
			sender: self.sender,
			receiver: self.receiver,
			data: new_data,
		}
	}
}

impl AIBusMessage {
	//Message length.
	pub fn l(&self) -> usize {
		return self.data.len();
	}
	
	//Get bytes from an AIBus message.
	pub fn get_bytes(&self) -> Vec<u8> {
		let mut data: Vec<u8> = vec![0; self.data.len() + 4];
		
		data[0] = self.sender;
		data[1] = (self.data.len() + 2) as u8;
		data[2] = self.receiver;
		
		for i in 0..self.data.len() {
			data[i+3] = self.data[i];
		}
		
		let mut checksum: u8 = 0;
		
		for i in 0..data.len() - 1 {
			checksum ^= data[i];
		}
		
		let checksum_index = data.len() - 1;
		data[checksum_index] = checksum;
		
		return data;
	}
}

//Get an AIBus message from a vector of bytes.
pub fn get_aibus_message(data: Vec<u8>) -> AIBusMessage {
	if data.len() < 4 {
		return AIBusMessage {
			sender: 0,
			receiver: 0,
			data: Vec::new(),
		};
	}

	let mut checksum = 0;
	for i in 0..data.len() - 1 {
		checksum ^= data[i];
	}

	if checksum != data[data.len()-1] {
		return AIBusMessage {
			sender: 0,
			receiver: 0,
			data: Vec::new(),
		};
	}
	
	if data[1] != (data.len() - 2) as u8 {
		return AIBusMessage {
			sender: 0,
			receiver: 0,
			data: Vec::new(),
		};
	}

	let mut the_return = AIBusMessage {
		sender: data[0],
		receiver: data[2],
		data: vec![0;data.len() - 4],
	};

	for i in 0..the_return.l() {
		the_return.data[i] = data[i+3];
	}

	return the_return;
}
