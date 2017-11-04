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
		lyn::event_object object;

		for (int i=1; i<argc; ++i)
			object.append_from_elf(make_elf(argv[i]));

		object.make_trampolines();
		object.link();
		object.remove_local_symbols();

		object.write_events(std::cout);
	} catch (const std::exception& e) {
		std::cerr << "[lyn] err... " << e.what() << std::endl;
	}

	return 0;
}
