#include <iostream>
#include <fstream>

#include "elf/elf_file.h"
#include "event_object.h"

lyn::elf_file make_elf(const std::string& fName) {
	lyn::elf_file result;
	std::ifstream file;

	file.open(fName, std::ios::in | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error(std::string("Couldn't open file for read: ").append(fName));

	result.load_from_stream(file);
	file.close();

	return result;
}

int main(int argc, char** argv) {
	if (argc < 2)
		return 1;

	try {
		std::cout << std::hex;

		lyn::event_object object;

		object.load_from_elf(make_elf(argv[1]));
		object.write_events(std::cout);
	} catch (const std::exception& e) {
		std::cerr << "err... " << e.what() << std::endl;
	}

	return 0;
}
