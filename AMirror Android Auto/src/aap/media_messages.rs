pub struct VideoMsg {
	data: Vec<u8>,
	msg_type: u16,
}

impl VideoMsg {
	pub fn get_data(self) -> Vec<u8> {
		return self.data;
	}

	pub fn get_type(self) -> u16 {
		return self.msg_type;
	}

	pub fn set_data(&mut self, data: &[u8]) {
		if data.len() < 2 {
			return;
		}

		self.msg_type = u16::from_be_bytes([data[0], data[1]]);
		self.data = data[2..].to_vec();
	}

	pub fn new() -> Self {
		return Self {
			data: Vec::new(),
			msg_type: 1,
		}
	}
}