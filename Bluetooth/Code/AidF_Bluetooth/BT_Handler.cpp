#include "BT_Handler.h"

BTHandler::BTHandler(ParameterList* parameter_list, BTADevice** connected_device, TextHandler* text_handler, uint8_t* bt_mac) {
	this->parameter_list = parameter_list;
	this->connected_device = connected_device;
	this->bt_mac = bt_mac;
	this->text_handler = text_handler;
}

//Initialize Bluetooth.
void BTHandler::init(unsigned long* timer) {
	//Create the pipe.
	pipe(pipe_stdin);
	pipe(pipe_stdout);

	this->command_thread_params = new CommandThreadParameters;
	command_thread_params->self = this;
	command_thread_params->stdin_write = pipe_stdin[1];
	command_thread_params->stdout_read = pipe_stdout[0];

	pid_t pid = fork();
	if(pid == 0) { //Child process.
		dup2(pipe_stdin[0], STDIN_FILENO);
		dup2(pipe_stdout[1], STDOUT_FILENO);
		//dup2(pipe_stdout[1], STDERR_FILENO)<<endl;

		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);

		execlp("/usr/bin/bluetoothctl", "", NULL);

		return;
	} else if(pid > 0) { //Parent process.
		close(pipe_stdin[0]);
		close(pipe_stdout[1]);

		this->bluetoothctl_process = pid;

		const string power_on = "power on\n";
		write(pipe_stdin[1], power_on.c_str(), power_on.length());
	}

	pthread_create(&command_thread, NULL, readCommandThread, (void*)command_thread_params);

	//Create the direct DBUS connection.
	connection = createSystemBusConnection();

	this->timer = timer;
	start_time = *timer;
	
	adapter_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath("/org/bluez/hci0"));
	btCommand("pairable on");
	btCommand("system-alias \"AidF-BTA\"");

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

BTHandler::~BTHandler() {
	delete command_thread_params;

	pthread_cancel(command_thread);

	close(pipe_stdin[1]);
	close(pipe_stdout[0]);
}

//Loop function.
void BTHandler::loop() {
	if(!init_connect && *timer - start_time > INIT_WAIT) {
		init_connect = true;
		//connectKnownDevice();
	}
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

	const bool discovering = adapter_proxy->getProperty("Discovering").onInterface("org.bluez.Adapter1").get<bool>();
	if(discovering)
		return;

	btCommand("scan on");
}

//Disable discovery mode.
void BTHandler::stopDiscovery() {
	adapter_proxy->setProperty("Discoverable").onInterface("org.bluez.Adapter1").toValue(false);

	const bool discovering = adapter_proxy->getProperty("Discovering").onInterface("org.bluez.Adapter1").get<bool>();
	if(!discovering)
		return;

	btCommand("scan off");
}

//Return whether the device is discoverable.
bool BTHandler::getDiscoverable() {
	return adapter_proxy->getProperty("Discoverable").onInterface("org.bluez.Adapter1").get<bool>();
}

//Handle a message from Bluetoothctl.
void BTHandler::handleBTCTLMessage(const string msg) {
	if(msg.find("Confirm passkey") != string::npos) { //Passkey.
		btCommand("yes");

		cout<<msg<<endl;
	} else if(msg.find("Authorize service") != string::npos) { //Authorize service.
		btCommand("yes");
	}
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

	string attempt_connect = "";

	for(auto path: reply) {
		if(path.first.find("dev_") != string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), path.first);
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

			if(paired) {
				attempt_connect = path.first;
				break;
			}
		}
	}

	if(!attempt_connect.empty() && !isConnected()) {
		string dev_path = getDevicePath(attempt_connect);
		dev_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath(dev_path));

		const string addr = dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>();

		btCommand("connect " + addr);
	}
}

//Connect a connected device from the list.
void BTHandler::connectDevice(const int index) {
	if(index < 0 || index >= paired_address_list.size())
		return;

	if(isConnected())
		return;

	const string addr = paired_address_list[index];
	cout<<"Attempting connection to "<<addr<<endl;
	btCommand("trust " + addr);
	btCommand("connect " + addr);
}

//Connect a connected device to the program.
void BTHandler::connectDevice(ObjectPath device_path) {
	if(isConnected())
		return;

	if(dev_proxy == nullptr || dev_proxy->getObjectPath().compare(device_path) != 0)
		dev_proxy = createProxy(*connection, BusName("org.bluez"), device_path);

	const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
	const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

	if(!paired) {
		//TODO: Display the PIN.
		dev_proxy->callMethod("Pair").onInterface("org.bluez.Device1").dontExpectReply();
		dev_proxy->setProperty("Trusted").onInterface("org.bluez.Device1").toValue(true);
		return;
	} 
	
	if(!connected) { //Device is known but not connected.
		const string addr = dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>();
		btCommand("connect " + addr);

		stopDiscovery();
	} else { //Device was connected.
		stopDiscovery();
		if(*connected_device == nullptr || *connected_device == NULL) {
			uint8_t mac_addr[MAC_LEN];
			getMacFromString(dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>(), mac_addr);
			const string name = dev_proxy->getProperty("Name").onInterface("org.bluez.Device1").get<string>();

			*connected_device = new BTADevice(device_path, mac_addr, name);

			cout<<"Connected to "<<name<<endl;
			parameter_list->connection_changed = true;

			const auto uuid_list = dev_proxy->getProperty("UUIDs").onInterface("org.bluez.Device1").get<vector<string>>();
			for(string uuid: uuid_list) {
				dev_proxy->callMethod("ConnectProfile").onInterface("org.bluez.Device1").withArguments(uuid).dontExpectReply();
			}
		}
	}
}

//Disconnect a device.
void BTHandler::disconnectDevice() {
	if(dev_proxy == nullptr)
		dev_proxy = createProxy(*connection, BusName("org.bluez"), (*connected_device)->getPath());

	dev_proxy->callMethod("Disconnect").onInterface("org.bluez.Device1");

	delete *connected_device;
	*connected_device = nullptr;

	parameter_list->connection_changed = true;
	media_set = false;
}

//Return whether a device is connected.
bool BTHandler::isConnected() {
	return *connected_device != NULL && *connected_device != nullptr;
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
		bool connect = false;

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

				if(!sub_device && !media_set) {
					const string device_path = getDevicePath(op);
					auto test_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath(device_path));

					const bool paired = test_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
					const bool connected = test_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

					if(paired && connected)
						setMediaProxy(op);
				}
			} else {
				const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
				const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

				if(paired || connected)
					connect = true;	
			}

			if(!isConnected()) {
				const string device_path = getDevicePath(op);
				auto test_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath(device_path));

				const bool paired = test_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
				const bool connected = test_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

				if(device_path.compare(op) != 0 && paired) {
					connect = true;
					op = ObjectPath(device_path);
				}
			}
		}

		if(connect)
			connectDevice(op);
	}
}

//Interface removed.
void BTHandler::onInterfaceRemove(Message msg) {
	ObjectPath op;
	if(msg >> op) {
		if(op.find("/dev_") != string::npos) {
			string dev_path = op;

			const int start_index = op.find("/dev_") + string("/dev_").length();
			int end_index = start_index;
			for(int i=start_index;i<op.length();i+=1) {
				if(op[i] == '/' || op[i] == '\\') {
					end_index = i;
					break;
				}
			}

			if(end_index > start_index)
				dev_path = op.substr(0,end_index);

			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath(dev_path));
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();
			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();

			uint8_t mac_addr[MAC_LEN];
			getMacFromString(dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>(), mac_addr);

			if(*connected_device != nullptr && *connected_device != NULL) {
				uint8_t* connected_mac = (*connected_device)->getMacAddress();
				bool match = true;

				for(int i=0;i<sizeof(mac_addr);i+=1) {
					if(mac_addr[i] != connected_mac[i]) {
						match = false;
						break;
					}
				}

				bool sub_device = false;
				if(op.find("player") != string::npos) {
					const int index = op.find("player");

					for(int i=index;i<op.length();i+=1) {
						if(op[i] == '/' || op[i] == '\\') {
							sub_device = true;
							break;
						}
					}
				}

				if(match && !sub_device) {
					delete *connected_device;
					*connected_device = nullptr;

					parameter_list->connection_changed = true;
					media_set = false;
				}
			}
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
	cout<<"Media proxy set! "<<proxy_path<<endl;
	media_proxy = createProxy(*connection, BusName("org.bluez"), proxy_path);

	auto on_property_change = [this](string interface, map<string, Variant> changed_properties, vector<string> invalidated_properties) {
		onMediaPropertyChange(interface, changed_properties, invalidated_properties);
	};

	media_proxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties").call(on_property_change);

	auto media_properties = media_proxy->getAllProperties().onInterface("org.bluez.MediaPlayer1");

	while(!media_check_ok);
	media_check_ok = false;
	for(auto property: media_properties) {
		if(this->media_properties.find(property.first) != this->media_properties.end())
			cout<<"Contains "<<property.first<<": "<<property.second.peekValueType()<<endl;
		else
			cout<<"Added "<<property.first<<": "<<property.second.peekValueType()<<endl;
		
		this->media_properties.emplace(property);
	}
	media_check_ok = true;

	media_set = true;
}

//Call a command on the Bluetoothctl proxy.
void BTHandler::btCommand(const string command) {
	const string cmd_str = command + '\n';
	write(pipe_stdin[1], cmd_str.c_str(), cmd_str.length());
}

//Create a device list menu on screen.
void BTHandler::createDeviceListMenu() {
	paired_address_list.clear();
	vector<string> device_names;

	map<ObjectPath, map<string, map<string, Variant>>> reply;

	unique_ptr<IProxy> mgr_proxy = createProxy(*connection, BusName("org.bluez"), ObjectPath("/"));

	try {
		mgr_proxy->callMethod("GetManagedObjects").onInterface("org.freedesktop.DBus.ObjectManager").storeResultsTo(reply);
	} catch(Error e) {
		cout<<e.getMessage()<<endl;
		return;
	}

	for(auto path: reply) {
		if(path.first.find("dev_") != string::npos) {
			auto dev_proxy = createProxy(*connection, BusName("org.bluez"), path.first);
			const bool paired = dev_proxy->getProperty("Paired").onInterface("org.bluez.Device1").get<bool>();

			if(!paired)
				continue;

			const bool connected = dev_proxy->getProperty("Connected").onInterface("org.bluez.Device1").get<bool>();
			const string name = dev_proxy->getProperty("Name").onInterface("org.bluez.Device1").get<string>();
			const string address = dev_proxy->getProperty("Address").onInterface("org.bluez.Device1").get<string>();

			paired_address_list.push_back(address);
			device_names.push_back(name);
		}
	}

	text_handler->createDeviceListMenu(device_names);
}

//Get the original device path.
string getDevicePath(string path) {
	if(path.find("/dev_") == string::npos)
		return path;

	const int start = path.find("/dev_") + string("/dev_").length();
	int end = -1;
	for(int i=start;i<path.length();i+=1) {
		if(path[i] == '/') {
			end = i;
			break;
		}
	}

	if(end < 0)
		return path;

	return path.substr(0, end);
}

//Read command thread function.
void* readCommandThread(void* parameters_v){
	CommandThreadParameters* parameters = (CommandThreadParameters*)parameters_v;
	char buf[1024];

	while(true) {
		int l = 0;
		if((l = read(parameters->stdout_read, &buf, sizeof(buf))) > 0) {
			const string str = string(buf).substr(0, l);
			//cout<<str<<endl;
			parameters->self->handleBTCTLMessage(str);
		}
	}

	void* result;
	return result;
}
