use crate::AIBusMessage;

pub struct AIBusHandler {
	ai_rx: Vec<AIBusMessage>,
	ai_tx: Vec<AIBusMessage>,
}

impl AIBusHandler {
	pub fn new() -> Self {
		return Self {
			ai_rx: Vec::new(),
			ai_tx: Vec::new(),
		}
	}

	///Get the mutable AIBus RX list.
	pub fn get_ai_rx(&mut self) -> &mut Vec<AIBusMessage> {
		return &mut self.ai_rx;
	}

	///Get the mutable AIBus TX list.
	pub fn get_ai_tx(&mut self) -> &mut Vec<AIBusMessage> {
		return &mut self.ai_tx;
	}
}