#include "IEBus_Transceiver.h"
#include "elapsedMillis.h"

IEBusTransceiver transceiver;

//Global setup.
void setup() {
	delay(100);
	
	pinMode(IEBUS_ACTIVE, OUTPUT);
	pinMode(IEBUS_TX_BIT, OUTPUT);
	pinMode(TX_ENABLE, OUTPUT);
	pinMode(RX_ENABLE, OUTPUT);
	pinMode(IEBUS_RX, INPUT);
	pinMode(IEBUS_TX, INPUT);

	pinMode(IEBUS_CLK, INPUT);
	pinMode(IEBUS_START, OUTPUT);

	digitalWrite(IEBUS_ACTIVE, LOW);
	digitalWrite(IEBUS_TX_BIT, LOW);
	digitalWrite(TX_ENABLE, LOW);
	digitalWrite(RX_ENABLE, LOW);
	digitalWrite(IEBUS_START, LOW);

	Serial.begin(115200);
}

//Global loop.
void loop() {
	transceiver.loop();
}

IEBusTransceiver::IEBusTransceiver() {
	ie_cache_vec.setStorage(ie_cache);
	ie_ack_vec.setStorage(ie_ack_cache);
}

//Object loop.
void IEBusTransceiver::loop() {
	const bool ie_active = digitalRead(IEBUS_RX) == HIGH;

	if(test_mode)
		goto read_send;

	if(ie_active) { //A message is on its way.
		IE_Message rec;
		if(readMessage(&rec))
			ie_cache_vec.push_back(rec);
	}

	if(ie_cache_vec.size() > 0 && last_ie > 5) {
		for(IE_Message rec: ie_cache_vec) {
			uint8_t data[rec.l + 5];
			data[0] = rec.sender >> 8;
			data[1] = rec.sender&0xFF;
			data[2] = rec.receiver >> 8;
			data[3] = rec.receiver&0xFF;
			data[4] = rec.control;

			if(rec.direct)
				data[0] |= 0x80;

			for(int i=0;i<rec.l;i+=1)
				data[i+5] = rec.data[i];

			AIData rx_msg(sizeof(data), 0x11, 0x10, data);
			uint8_t tx_bytes[rx_msg.l + 4];
			rx_msg.getBytes(tx_bytes);
			Serial.write(tx_bytes, sizeof(tx_bytes));
			Serial.flush();
			delay(1);
		}
		
		ie_cache_vec.clear();
	}

	read_send:
	if(Serial.available() != avail) {
		avail = Serial.available();
		last_aibus = 0;
	}
	
	while(Serial.available() >= 4) { //Something to send.
		uint8_t sl[3];
		Serial.readBytes(sl, sizeof(sl));
		const uint8_t s = sl[0], r = sl[2];
		const int len = sl[1];
		
		while(Serial.available() < len - 1);
		
		uint8_t main_data[len - 1];
		Serial.readBytes(main_data, sizeof(main_data));

		uint8_t data[sizeof(sl) + sizeof(main_data)];
		for(int i=0;i<sizeof(sl);i+=1)
			data[i] = sl[i];

		for(int i=0;i<sizeof(main_data);i+=1)
			data[i+sizeof(sl)] = main_data[i];

		if(!checkValidity(data, sizeof(data)))
			continue;

		if(s != 0x10 && r != 0x11)
			continue;

		AIData rx;
		bytesToAIData(&rx, data);

		if(rx.sender == 0x10 && rx.receiver == 0x11) { //IEBus message to send.
			#ifdef LOOPBACK
			{ 
				AIData copy(rx.l, 0x11, 0x10, rx.data);
				uint8_t copy_bytes[copy.l + 4];
				copy.getBytes(copy_bytes);
				Serial.write(copy_bytes, sizeof(copy_bytes));
			}
			#endif

			if(rx.l > 5) {
				const uint16_t ie_sender = (uint16_t(rx[0]&0xF)<<8) | rx[1], ie_receiver = (uint16_t(rx[2]&0xF)<<8) | rx[3];
				const uint8_t ctl = rx[4]&0xF;
				const bool direct = (rx[0]&0x80) != 0, no_checksum = (rx[0]&0x40) != 0;

				IE_Message tx(rx.l - 5, ie_sender, ie_receiver, ctl, direct);
				for(int i=0;i<tx.l;i+=1)
					tx.data[i] = rx[i+5];

				if(no_checksum)
					writeMessage(&tx);
				else
					writeMessageChecksum(&tx);
			} else if(rx.l > 0) {
				digitalWrite(IEBUS_TX_BIT, LOW);
				digitalWrite(IEBUS_START, LOW);
				
				const bool rx_test = (rx[0]&0x20) != 0, tx_test = (rx[0]&0x10) != 0;
				if(!rx_test && !tx_test) {
					test_mode = false;
					digitalWrite(TX_ENABLE, LOW);
					digitalWrite(RX_ENABLE, LOW);
				} else if(tx_test) {
					test_mode = true;
					digitalWrite(TX_ENABLE, HIGH);
					digitalWrite(RX_ENABLE, LOW);
					digitalWrite(IEBUS_TX_BIT, rx_test ? HIGH : LOW);
				} else if(rx_test) {
					test_mode = true;
					digitalWrite(TX_ENABLE, LOW);
					digitalWrite(RX_ENABLE, HIGH);
				}
			}
		}
	}
	
	if(last_aibus > 300 && Serial.available() > 0) {
		uint8_t avail_bytes[avail];
		Serial.readBytes(avail_bytes, sizeof(avail_bytes));
		avail = 0;
	}
}

//Read an IEBus message. Return whether successful.
bool IEBusTransceiver::readMessage(IE_Message *rec) {
	digitalWrite(IEBUS_ACTIVE, HIGH);
	digitalWrite(RX_ENABLE, HIGH);
	noInterrupts();

	//Wait for the start bit to finish.
	while(digitalReadFast(IEBUS_RX) == HIGH);
	bool ack = false, send_ack = false;
	bool success = false;
	
	{
		const int16_t direct = readBits(1, false, false, false);
		if(direct < 0)
			goto read_exit;
		rec->direct = direct == 1;
	}

	{
		const int16_t sender = readBits(12, true, false, false);
		if(sender < 0)
			goto read_exit;
		rec->sender = sender;
	}

	{
		const int16_t receiver = readBits(12, true, false, false);
		if(receiver < 0)
			goto read_exit;
		rec->receiver = receiver;

		if(receiver == MY_ID) {
			while(digitalReadFast(IEBUS_CLK) == LOW);

			digitalWriteFast(IEBUS_TX_BIT, LOW);
			digitalWriteFast(TX_ENABLE, HIGH);
			while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);
			digitalWriteFast(TX_ENABLE, LOW);

			ack = true;
		}
	}

	{
		const int16_t ctl = readBits(4, true, true, ack);
		if(ctl < 0)
			goto read_exit;
		rec->control = ctl;
	}

	{
		const int16_t len = readBits(8, true, true, ack);
		if(len < 0)
			goto read_exit;
		rec->refreshIEData(len);
	}

	for(int i=0;i<rec->l;i+=1) {
		const int16_t data = readBits(8, true, true, ack);
		if(data < 0)
			goto read_exit;

		rec->data[i] = data&0xFF;
	}

	if(rec->checkVaildity())
		success = true;

	//Send Honda acknowledgment.
	if(rec->receiver == MY_ID && rec->checkVaildity())
		send_ack = true;

	if(ie_cache_vec.size() + 1 < ie_cache_vec.max_size())
		last_ie = 0;

	read_exit:
	interrupts();
	digitalWrite(RX_ENABLE, LOW);
	digitalWrite(IEBUS_ACTIVE, LOW);

	if(send_ack) {
		elapsedMicros wait;
		while(wait < 40);

		uint8_t ack_data[] = {0x80};
		IE_Message msg_ack(sizeof(ack_data), rec->receiver, rec->sender, 0xF, true);
		msg_ack.refreshIEData(ack_data);
		writeMessageChecksum(&msg_ack);
	}

	return success;
}

//Read a bit from IEBus.
bool IEBusTransceiver::readBit() {
	while(digitalReadFast(IEBUS_CLK) == HIGH);
	if(digitalReadFast(IEBUS_RX) == LOW) //Pulse shorter than clock. 1.
		return true;
	else
		return false;
}

//Read a set of bits from IEBus. Returns the number read if successful. 
int16_t IEBusTransceiver::readBits(int count, const bool use_parity, const bool use_ack, bool ack) {
	if(count < 0)
		count = 0;
	if(count > 15)
		count = 15;

	int16_t num = 0;
	bool parity = false;

	elapsedMicros timer;

	while(digitalReadFast(IEBUS_CLK) == LOW && timer < 200) {
		if(timer >= 200)
			return -1;
	}

	for(int i=0;i<count;i+=1) {
		num <<= 1;
		
		const bool bit = readBit();
		if(bit) {
			num |= 1;
			parity = !parity;
		}

		if(i < count - 1) {
			timer = 0;
			while(digitalReadFast(IEBUS_CLK) == LOW && timer < 200) {
				if(timer >= 200)
					return -1;
			}
		}
	}

	if(use_parity) {
		timer = 0;
		while(digitalReadFast(IEBUS_CLK) == LOW && timer < 200) {
			if(timer >= 200)
				return -1;
		}

		const bool bit = readBit();
		if(bit != parity)
			ack = false;

		if(bit != parity && !use_ack)
			return -1;
	}

	if(use_ack) {
		while(digitalReadFast(IEBUS_CLK) == LOW);

		if(ack) {
			while(digitalReadFast(IEBUS_CLK) == LOW);

			digitalWriteFast(IEBUS_TX_BIT, LOW);
			digitalWriteFast(TX_ENABLE, HIGH);
			while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);
			digitalWriteFast(TX_ENABLE, LOW);
		} else { //Parity failed.
			timer = 0;
			while(digitalRead(IEBUS_RX) == HIGH) {
				if(timer >= 200) {
					digitalWriteFast(TX_ENABLE, LOW);
					return -1;
				}
			}

			return -1;
		}
	}

	return num;
}

//Write a bit to IEBus.
void IEBusTransceiver::writeBit(const bool bit) {
	digitalWriteFast(IEBUS_TX_BIT, bit ? HIGH : LOW);
	while(digitalReadFast(IEBUS_CLK) == LOW);
	
	if(!bit) {
		digitalWriteFast(TX_ENABLE, LOW);
		while(digitalReadFast(IEBUS_TX) == HIGH);
		digitalWriteFast(TX_ENABLE, HIGH);
	}
}

//Write a set of bits to IEBus.
void IEBusTransceiver::writeBits(const int count, const uint16_t data, const bool use_parity, const bool use_ack) {
	bool parity = false;
	
	for(int i=count-1;i>=0;i-=1) {
		while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);
		const bool b = (data&(1<<i)) != 0;
		writeBit(b);
		if(b)
			parity = !parity;
	}

	if(use_parity) {
		while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);
		writeBit(parity);
	}

	if(use_ack) {
		while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);
		writeBit(true);
	}
}

//Write a full message to IEBus.
void IEBusTransceiver::writeMessage(IE_Message* ie_data, const bool ack) {
	const boolean was_active = digitalRead(IEBUS_ACTIVE) == HIGH;
	digitalWriteFast(IEBUS_ACTIVE, HIGH);
	digitalWriteFast(TX_ENABLE, HIGH);
	noInterrupts();

	elapsedMicros start_timer;
	digitalWriteFast(IEBUS_START, HIGH);
	digitalWriteFast(IEBUS_TX_BIT, HIGH);
	start_timer = 0;
	while(start_timer < 160 || digitalReadFast(IEBUS_CLK) == HIGH);
	digitalWriteFast(IEBUS_START, LOW);

	writeBits(1, ie_data->direct ? 1 : 0, false, false);
	writeBits(12, ie_data->sender, true, false);
	writeBits(12, ie_data->receiver, true, true);
	writeBits(4, ie_data->control, true, true);
	writeBits(8, ie_data->l&0xFF, true, true);
	for(int i=0;i<ie_data->l;i += 1)
		writeBits(8, ie_data->data[i], true, true);
	while(digitalReadFast(IEBUS_TX) == HIGH || digitalReadFast(IEBUS_CLK) == HIGH);

	if(!was_active) {
		interrupts();
		digitalWriteFast(IEBUS_ACTIVE, LOW);
	}
	digitalWriteFast(TX_ENABLE, LOW);

	if(ack && ie_data->direct && ie_data->sender == MY_ID) {
		int tries = 0;
		elapsedMillis send_timer;
		bool ack_rec = false;

		while(tries < 25) {
			const bool ie_active = digitalRead(IEBUS_RX) == HIGH;
			if(ie_active) {
				IE_Message rec;
				if(readMessage(&rec)) {
					if(rec.sender == ie_data->receiver && rec.receiver == ie_data->sender && rec.l > 0 && rec.data[0] == 0x80) {
						ack_rec = true;
						break;
					} else if(rec.receiver == ie_data->sender || rec.receiver == MY_ID)
						ie_cache_vec.push_back(rec);
				}
			}

			if(!ack_rec && send_timer > 200) {
				send_timer = 0;
				tries += 1;
				writeMessage(ie_data, false);
			}
		}
	}
}

//Write a message to IEBus with a Honda checksum.
void IEBusTransceiver::writeMessageChecksum(IE_Message* ie_data) {
	const uint8_t checksum = ie_data->getChecksum();

	IE_Message new_msg(ie_data->l+1, ie_data->sender, ie_data->receiver, ie_data->control, ie_data->direct);
	
	for(int i=0;i<ie_data->l;i+=1)
		new_msg.data[i] = ie_data->data[i];

	new_msg.data[new_msg.l-1] = checksum;
	writeMessage(&new_msg);
}