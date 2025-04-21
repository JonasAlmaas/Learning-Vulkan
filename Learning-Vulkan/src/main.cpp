#include "vk_app.hpp"

#include <iostream>
#include <stddef.h>

int main()
{
	vk_app app;

	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
