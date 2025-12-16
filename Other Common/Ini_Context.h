#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>

#ifndef ini_context_h
#define ini_context_h

struct IniList {
	std::string title;

	std::string* num_vars;
	int* num_values;
	
	std::string* str_vars;
	std::string* str_values;
	
	int l_n, l_s;
	
	IniList();
	IniList(const int l_n, const int l_s);
	IniList(const IniList &copy);
	~IniList();
};

std::vector<IniList> loadIniFile(const char* fpath);
void saveIniFile(const char* fpath, std::vector<IniList> ini_file);

#endif
