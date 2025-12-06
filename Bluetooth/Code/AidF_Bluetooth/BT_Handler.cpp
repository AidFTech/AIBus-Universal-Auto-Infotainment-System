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

	connectKnownDevice();
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

//Get the connected device.
BTADevice* BTHandler::getConnectedDevice() {
	return *this->connected_device;
}

//Find and connect to a known-paired device.
void BTHandler::connectKnownDevice() {
	map<ObjectPath, map<string, map<string, Variant>>> reply;

	unique_ptr<IProxy> mgr_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath("/"));
	mgr_proxy->callMethod("GetManagedObjects").onInterface("org.freedesktop.DBus.ObjectManager").storeResultsTo(reply);

	for(auto path : reply) {
		if(path.first.find("dev_") != string::npos && path.first.find("player") == string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), path.first);
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

			if(paired && connected) {
				connectDevice(path.first);
				try {
					auto media_path = dev_proxy->getProperty("Player").onInterface("org.bluez.MediaControl1").get<ObjectPath>();
					setMediaProxy(media_path);
				} catch(Error e) {
					cout<<e.getMessage()<<endl;
				}
				break;
			}
		}
	}
}

//Connect a connected device to the program.
void BTHandler::connectDevice(ObjectPath device_path) {
	auto dev_proxy = createProxy(*connection, BusName("org.bluez"), device_path);
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

			*connected_device = new BTADevice(device_path, mac_addr, name);

			cout<<"Connected to "<<name<<endl;
			parameter_list->connection_changed = true;
		}
	}
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

//Get the media proxy.
unique_ptr<IProxy>* BTHandler::getMediaProxy() {
	while(!media_check_ok);
	return &this->media_proxy;
}

//Get the list of changed media properties.
map<string, Variant>* BTHandler::getChangedMediaProperties() {
	return &this->media_properties;
}

//Send a media control message.
void BTHandler::sendMediaControl(const media_control_t cmd) {
	if(*connected_device == NULL || *connected_device == nullptr)
		return;

	string method = "";
	switch(cmd) {
	case MEDIA_CONTROL_FF:
		method = "FastForward";
		break;
	case MEDIA_CONTROL_NEXT:
		method = "Next";
		break;
	case MEDIA_CONTROL_PAUSE:
		method = "Pause";
		break;
	case MEDIA_CONTROL_PLAY:
		method = "Play";
		break;
	case MEDIA_CONTROL_PREVIOUS:
		method = "Previous";
		break;
	case MEDIA_CONTROL_FR:
		method = "Rewind";
		break;
	case MEDIA_CONTROL_STOP:
		method = "Stop";
		break;
	case MEDIA_CONTROL_VOLUMEDOWN:
		method = "VolumeDown";
		break;
	case MEDIA_CONTROL_VOLUMEUP:
		method = "VolumeUp";
		break;
	default:
		return;
	}

	try {
		auto dev_proxy = createProxy(*connection, BusName("org.bluez"), (*connected_device)->getPath());
		dev_proxy->callMethod(method).onInterface("org.bluez.MediaControl1");
	} catch(Error e) {
		cout<<e.getMessage()<<endl;
	}
}

//Interface added.
void BTHandler::onInterfaceAdd(Message msg) {
	ObjectPath op;
	if(msg >> op) {
		if(op.find("/dev_") != string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), op);
			if(op.find("player") != string::npos) {
				const int index = op.find("player");
				bool sub_device = false;

				for(int i=index;i<op.length();i+=1) {
					if(op[i] == '/' || op[i] == '\\') {
						sub_device = true;
						break;
					}
				}

				if(!sub_device)
					setMediaProxy(op);
			} else {
				connectDevice(op);
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

			//TODO: Compare the connected device address with the object path.
			delete *connected_device;
			*connected_device = nullptr;

			parameter_list->connection_changed = true;
		}
	}
}

//Media property changed.
void BTHandler::onMediaPropertyChange(string interface, map<string, Variant> changed_properties, vector<string> invalidated_properties) {
	if(interface.compare("org.bluez.MediaPlayer1") == 0) {
		while(!media_check_ok);
		media_check_ok = false;
		for(auto property : changed_properties) {
			this->media_properties.emplace(property);
		}
		media_check_ok = true;
	}
}

//Set the media proxy.
void BTHandler::setMediaProxy(ObjectPath proxy_path) {
	media_proxy = createProxy(*connection, BusName("org.bluez"), proxy_path);

	auto on_property_change = [this](string interface, map<string, Variant> changed_properties, vector<string> invalidated_properties) {
		onMediaPropertyChange(interface, changed_properties, invalidated_properties);
	};

	media_proxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties").call(on_property_change);

	auto media_properties = media_proxy->getAllProperties().onInterface("org.bluez.MediaPlayer1");

	while(!media_check_ok);
	media_check_ok = false;
	for(auto property : media_properties) {
		this->media_properties.emplace(property);
	}
	media_check_ok = true;
}