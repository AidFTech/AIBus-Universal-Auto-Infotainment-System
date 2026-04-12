use int_enum::IntEnum;
use std::convert::TryFrom;

pub struct MenuList<'a> {
	pub start: MenuIndex,
	pub end: MenuIndex,
	pub menu_str: &'a [&'static str],
	pub title: &'a str,
}

impl<'a> MenuList<'a> {
	pub fn new(index: MenuIndex, locale: Locale) -> Option<Self> {
		let mut menu_index = -1;

		for i in 0..MENU_START_INDEX.len() {
			if MENU_START_INDEX[i] == index {
				menu_index = i as isize;
				break;
			}
		}

		if menu_index < 0 {
			return None;
		}

		let start = (MENU_START_INDEX[menu_index as usize] as usize) + 1;
		let end;
		if menu_index < (MenuIndex::MenuIndexLength as isize) - 1 {
			end = MENU_START_INDEX[menu_index as usize + 1];
		} else {
			end = MenuIndex::MenuIndexLength;
		}

		let menu_list = if locale == Locale::LocaleEnglishUS {
			MENUS_ENG
		} else {
			MENUS_ENG
		};

		let title = menu_list[MENU_START_INDEX[menu_index as usize] as usize];

		return Some(MenuList {
			start: match MenuIndex::try_from(start) {
				Ok(start) => start,
				Err(_) => {
					return None;
				}
			},
			end,
			menu_str: &menu_list[start..end as usize],
			title,
		});
	}

	///Get the menu length.
	pub fn size(&self) -> usize {
		return (self.end as usize) - (self.start as usize);
	}

	///Get the enumerated index at the specified index.
	pub fn get_global_index(&self, index: usize) -> Option<MenuIndex> {
		let start = self.start as usize;

		match MenuIndex::try_from(index + start) {
			Ok(index) => {
				return Some(index);
			}
			Err(_) => {
				return None;
			}
		}
	}
}

#[derive(Clone,Copy,PartialEq,Eq,Debug,Hash)]
pub enum Locale {
	LocaleEnglishUS,
}

#[derive(Clone,Copy,PartialEq,Eq,Debug,Hash,IntEnum)]
#[repr(usize)]
pub enum MenuIndex {
	//Audio settings menu:
	MirrorAudioSettings,
	MirrorAudioSettingsClusterDisplayText,
	MirrorAudioSettingsScrollInfoText,
	MirrorAudioSettingsAutoMusicStart,
	MirrorAudioSettingsFlashTitle,
	MirrorAudioSettingsAudioSettings,

	//Cluster display menu:
	MirrorClusterSettings,
	MirrorClusterSettingsPhone,
	MirrorClusterSettingsSong,
	MirrorClusterSettingsArtist,
	MirrorClusterSettingsAlbum,
	MirrorClusterSettingsApp,
	MirrorClusterSettingsScrollCluster,
	MirrorClusterSettingsScrollInfo,
	
	//Main settings menu:
	MirrorMainSettings,
	MirrorMainSettingsAutoStart,

	MenuIndexLength,
}

const MENU_START_INDEX: &[MenuIndex] = &[
	MenuIndex::MirrorAudioSettings,
	MenuIndex::MirrorClusterSettings,
	MenuIndex::MirrorMainSettings,
	MenuIndex::MenuIndexLength,
];

const MENUS_ENG: &[&'static str] = &[
	//Audio settings menu:
	"Mirror Settings",
	"Cluster Display Text",
	"Scroll Info Text",
	"Auto Music Start",
	"Flash Song Title on Change",
	"Audio Settings",

	//Cluster display menu:
	"Cluster Display Text",
	"Display Phone Name",
	"Display Song Title",
	"Display Artist",
	"Display Album",
	"Display App Name",
	"Scroll Cluster Display",
	"Scroll Info Text",
	
	//Main settings menu:
	"Phone Mirror Settings",
	"Auto Mirror Start",
];
