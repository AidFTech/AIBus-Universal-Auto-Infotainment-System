#include "Ini_Context.h"

IniList::IniList() : IniList(0,0) {

}

IniList::IniList(const int l_n, const int l_s) {
	this->l_n = l_n;
	this->l_s = l_s;
	
	this->num_vars = new std::string[this->l_n];
	this->num_values = new int[this->l_n];
	
	this->str_vars = new std::string[this->l_s];
	this->str_values = new std::string[this->l_s];
}

IniList::~IniList() {
	delete[] this->num_vars;
	delete[] this->num_values;
	
	delete[] this->str_vars;
	delete[] this->str_values;
}

IniList::IniList(const IniList &copy) {
	this->l_n = copy.l_n;
	this->l_s = copy.l_s;
	this->title = copy.title;
	
	this->num_vars = new std::string[this->l_n];
	this->num_values = new int[this->l_n];
	
	this->str_vars = new std::string[this->l_s];
	this->str_values = new std::string[this->l_s];

	for(int i=0;i<this->l_n;i+=1) {
		this->num_vars[i] = copy.num_vars[i];
		this->num_values[i] = copy.num_values[i];
	}
	
	for(int i=0;i<this->l_s;i+=1) {
		this->str_vars[i] = copy.str_vars[i];
		this->str_values[i] = copy.str_values[i];
	}
}

//Generate a set of lists from an INI file.
std::vector<IniList> loadIniFile(const char* fpath) {
	std::ifstream file(fpath);
	std::vector<std::string> text_lines(0);

	std::string file_text;
	while(getline(file, file_text)) {
		if(file_text.compare("\n") == 0)
			continue;

		if(file_text.length() >= 1)
			text_lines.push_back(file_text);
	}

	std::vector<IniList> ini_file(0);

	std::string title = "";

	std::vector<std::string> num_vars(0);
	std::vector<int> num_values(0);

	std::vector<std::string> str_vars(0);
	std::vector<std::string> str_values(0);

	for(int t=0;t<text_lines.size();t+=1) {
		if(text_lines[t].at(0) != '[') {
			const int eq_pos = text_lines[t].find('=');

			if(eq_pos == std::string::npos) {
				str_vars.push_back(text_lines[t]);
				str_values.push_back("");
				continue;
			}

			std::string var = text_lines[t].substr(0, eq_pos);
			std::string value = text_lines[t].substr(eq_pos + 1, text_lines[t].length() - eq_pos - 1);

			try {
				const int num_value = std::stoi(value);

				num_vars.push_back(var);
				num_values.push_back(num_value);
			} catch(std::invalid_argument &e) {
				str_vars.push_back(var);
				str_values.push_back(value);
			}
		}

		if(text_lines[t].at(0) == '[' || t == text_lines.size() - 1) {
			if(num_vars.size() > 0 || str_vars.size() > 0) {
				IniList item(num_vars.size(), str_vars.size());
				item.title = title;

				for(int i=0;i<num_vars.size();i+=1) {
					item.num_vars[i] = num_vars[i];
					item.num_values[i] = num_values[i];
				}

				for(int i=0;i<str_vars.size();i+=1) {
					item.str_vars[i] = str_vars[i];
					item.str_values[i] = str_values[i];
				}

				ini_file.push_back(item);
			}

			title = text_lines[t].substr(1, text_lines[t].length() - 2);

			num_vars.clear();
			num_values.clear();
			str_vars.clear();
			str_values.clear();
		}
	}

	return ini_file;
}

//Save an INI file.
void saveIniFile(const char* fpath, std::vector<IniList> ini_file) {
	std::ofstream file(fpath);

	for(int f=0;f<ini_file.size();f+=1) {
		IniList ini_list = ini_file[f];

		file<<'['<<ini_list.title<<"]\n";
		for(int i=0;i<ini_list.l_n;i+=1)
			file<<ini_list.num_vars[i]<<'='<<ini_list.num_values[i]<<'\n';

		for(int i=0;i<ini_list.l_s;i+=1)
			file<<ini_list.str_vars[i]<<'='<<ini_list.str_values[i]<<'\n';

		if(f<ini_file.size()-1)
			file<<"\n\n";
	}
}