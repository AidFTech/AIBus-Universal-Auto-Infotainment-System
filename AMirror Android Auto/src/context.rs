pub struct Context {
	pub audio_selected: bool, //True if the mirror is selected as the active audio device.
	pub audio_text: bool, //True if audio text handling is allowed.
	pub phone_active: bool, //True if phone mirroring is active.
	pub fullscreen: bool, //True on the final Raspberry Pi, false for testing.
	pub playing: bool, //True if the phone is playing music.

	pub phone_type: u8, //The phone type, as defined by the dongle.
	pub phone_name: String, //The name of the phone.

	pub song_title: String, //The song title.
	pub artist: String, //The artist name.
	pub album: String, // The album name.
	pub app: String, //The app name.

	pub night: bool, //True if the system is in night mode.

	pub phone_req_off: bool, //True if the phone has requested a return to the factory UI.
	pub home_held: bool, //True if "home" was held by the user.

	//IMID Parameters:
	pub imid_row_count: u8,
	pub imid_text_len: u8,
	pub imid_native_mirror: bool,

	//Nav screen buttons:
	pub vertical_toggle: bool, //Vertical toggle options present.
	pub horizontal_toggle: bool, //Horizontal toggle options present.
	pub nav_knob: bool, //Nav knob present.

	//Devices:
	pub radio_connected: bool,
}

impl Context {
	pub fn new() -> Self {

		return Self {
			audio_selected: false,
			audio_text: false,
			phone_active: true,
			fullscreen: false,
			playing: false,

			phone_type: 0,
			phone_name: "".to_string(),

			song_title: "".to_string(),
			artist: "".to_string(),
			album: "".to_string(),
			app: "".to_string(),

			night: false,

			phone_req_off: false,
			home_held: false,
			
			imid_row_count: 0,
			imid_text_len: 0,
			imid_native_mirror: false,

			vertical_toggle: false,
			horizontal_toggle: false,
			nav_knob: false,
			
			radio_connected: false,
		};
	}
}
