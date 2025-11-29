#include "BT_Handler.h"

BTHandler::BTHandler(ParameterList* parameter_list, BTADevice** connected_device, TextHandler* text_handler, uint8_t* bt_mac) {
	this->parameter_list = parameter_list;
	this->connected_device = connected_device;
	this->bt_mac = bt_mac;
	this->text_handler = text_handler;
}

//Initialize Bluetooth.
void BTHandler::init() {
	connection = createSystemBusConnection();
	adapter_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath("/org/bluez/hci0"));

	adapter_proxy->setProperty("Powered").onInterface("org.bluez.Adapter1").toValue(true);
	getPiMac();

	auto interface_add = [this](Message msg) {
		onInterfaceAdd(msg);
	};

	auto interface_remove = [this](Message msg) {
		onInterfaceRemove(msg);
	};

	connection->addMatch("type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'", interface_add);
	connection->addMatch("type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved'", interface_remove);
	connection->enterEventLoopAsync();
}

//Loop function.
void BTHandler::loop() {
	
}

//Get the Pi MAC address.
void BTHandler::getPiMac() {
	uint8_t mac_addr[MAC_LEN];
	getMacFromString(adapter_proxy->getProperty("Address").onInterface("org.bluez.Adapter1").get<string>(), mac_addr);

	for(int i=0;i<sizeof(mac_addr);i+=1)
		this->bt_mac[i] = mac_addr[i];

	parameter_list->bt_mac_set = true;

	parameter_list->pi_name = adapter_proxy->getProperty("Name").onInterface("org.bluez.Adapter1").get<string>();
}

//Enable discovery mode.
void BTHandler::startDiscovery() {
	adapter_proxy->setProperty("Discoverable").onInterface("org.bluez.Adapter1").toValue(true);
}

//Disable discovery mode.
void BTHandler::stopDiscovery() {
	adapter_proxy->setProperty("Discoverable").onInterface("org.bluez.Adapter1").toValue(false);
}

//Return whether the device is discoverable.
bool BTHandler::getDiscoverable() {
	return adapter_proxy->getProperty("Discoverable").onInterface("org.bluez.Adapter1").get<bool>();
}

//Print the object list.
void BTHandler::getObjectList() {
	map<ObjectPath, map<string, map<string, Variant>>> reply;

	unique_ptr<IProxy> mgr_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath("/"));
	mgr_proxy->callMethod("GetManagedObjects").onInterface("org.freedesktop.DBus.ObjectManager").storeResultsTo(reply);

	for(auto path : reply) {
		for(auto str : path.second) {
			cout<<path.first<<": "<<str.first<<endl;
			for(auto newstr : str.second) {
				if(string(newstr.second.peekValueType()).compare("s") == 0)
					cout<<'\t'<<newstr.first<<", "<<newstr.second.get<string>()<<endl;
			}
		}
	}
}

//Interface added.
void BTHandler::onInterfaceAdd(Message msg) {
	ObjectPath op;
	if(msg >> op) {
		if(op.find("/dev_") != string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), op);
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

			if(!paired) {
				//Display the PIN.
			} else if(!connected) { //Device is known but not connected.
				dev_proxy->callMethod("Connect").onInterface("org.bluez.Device1").dontExpectReply();
			} else { //Device was connected.
				if(*connected_device == nullptr || *connected_device == NULL) {
					uint8_t mac_addr[MAC_LEN];
					getMacFromString(dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>(), mac_addr);
					const string name = dev_proxy->getProperty("Name").onInterface("org.bluez.Device1").get<string>();

					*connected_device = new BTADevice(op, mac_addr, name);

					cout<<"Connected to "<<name<<endl;
					parameter_list->connection_changed = true;
				}
			}
		}
	}
}

//Interface removed.
void BTHandler::onInterfaceRemove(Message msg) {
	ObjectPath op;
	if(msg >> op) {
		if(op.find("/dev_") != string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), op);
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

			uint8_t mac_addr[MAC_LEN];
			getMacFromString(dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>(), mac_addr);

			delete *connected_device;
			*connected_device = nullptr;

			parameter_list->connection_changed = true;
		}
	}
}