#ifndef IAPPLICATION_H
#define IAPPLICATION_H

import std;

namespace Fyuu {

	class IApplication {

	public:

		virtual void Init() = 0;
		virtual void Tick() = 0;
		virtual void Quit() = 0;

		virtual ~IApplication() = default;


	};


}

#endif // !IAPPLICATION_H
