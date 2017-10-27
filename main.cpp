#include <iostream>
#include <fstream>

#include "elf/elf_file.h"
#include "event_object.h"

int main(int argc, char** argv) {
	if (argc < 2)
		return 1;

	std::string input(argv[1]);

	std::ifstream file;

	file.open(input, std::ios::in | std::ios::binary);

	if (!file.is_open())
		return 1;

	lyn::elf_file elfFile;
	elfFile.load_from_stream(file);

	file.close();

	std::cout << std::hex;

	lyn::event_object object;

	object.load_from_elf(elfFile);
	object.write_events(std::cout);

	return 0;
}
