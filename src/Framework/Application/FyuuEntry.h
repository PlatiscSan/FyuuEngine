#ifndef FYUU_ENTRY_H
#define FYUU_ENTRY_H

#include "Application.h"

namespace Fyuu {

	std::shared_ptr<Application> InitWindowsApp(int& argc, char**& argv);
	std::shared_ptr<Application> InitLinuxApp(int& argc, char**& argv);

}

#endif // !FYUU_ENTRY_H
