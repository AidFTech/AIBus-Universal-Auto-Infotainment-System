import os

def getFolders(start_directory: str) -> list:
	paths = []
	for root, dirs, files in os.walk(start_directory):
		for d in dirs:
			paths.append(root + '/' + d)
	
	return(paths)
	
def writeLibraryPaths(f, paths: list):
	for i in range(len(paths)):
		f.write("    - \"-I" + paths[i] + '\"')
		if i < len(paths):
			f.write('\n')

library_paths = getFolders("/home/aidan/Arduino/libraries")
main_paths = getFolders("/home/aidan/.arduinocdt/packages/arduino/")
sec_paths = getFolders("/home/aidan/.arduino15/packages/arduino/")

file = open(r"./Arduino Clangd", 'w')
file.write("CompileFlags:\n  Add:\n")

writeLibraryPaths(file, main_paths)
writeLibraryPaths(file, sec_paths)
writeLibraryPaths(file, library_paths)
