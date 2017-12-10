#include <iostream>
#include <fstream>

#include "elf/elf_file.h"
#include "core/event_object.h"

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

	struct {
		bool doLink          = true;
		bool longCall        = false;
		bool applyHooks      = true;
		bool printTemporary  = false;
	} options;

	std::vector<std::string> elves;

	for (int i=1; i<argc; ++i) {
		std::string argument(argv[i]);

		if (argument.size() == 0)
			continue;

		if (argument[0] == '-') { // option
			if (argument        == "-nolink") {
				options.doLink         = false;
			} else if (argument == "-link") {
				options.doLink         = true;
			} else if (argument == "-longcalls") {
				options.longCall       = true;
			} else if (argument == "-nolongcalls") {
				options.longCall       = false;
			} else if (argument == "-raw") {
				options.doLink         = false;
				options.longCall       = false;
				options.applyHooks     = false;
			} else if (argument == "-temp") {
				options.printTemporary = true;
			} else if (argument == "-notemp") {
				options.printTemporary = false;
			} else if (argument == "-hook") {
				options.applyHooks     = true;
			} else if (argument == "-nohook") {
				options.applyHooks     = false;
			}
		} else { // elf
			elves.push_back(std::move(argument));
		}
	}

	try {
		lyn::event_object object;

		for (auto& elf : elves)
			object.append_from_elf(make_elf(elf));

		if (options.doLink)
			object.try_relocate_relatives();

		if (options.longCall)
			object.try_transform_relatives();

		if (options.doLink)
			object.try_relocate_absolutes();

		if (!options.printTemporary)
			object.remove_unnecessary_symbols();

		object.cleanup();

		if (options.applyHooks) {
			for (auto& hook : object.get_hooks()) {
				lyn::event_object temp;

				std::cout << "PUSH" << std::endl;
				std::cout << "ORG $" << std::hex << (hook.originalOffset & (~1)) << std::endl;

				temp.combine_with(lyn::arm_relocator::make_thumb_veneer(hook.name, 0));
				temp.write_events(std::cout);

				std::cout << "POP" << std::endl;
			}
		}

		object.write_events(std::cout);
	} catch (const std::exception& e) {
		std::cerr << "[lyn] ERROR: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
