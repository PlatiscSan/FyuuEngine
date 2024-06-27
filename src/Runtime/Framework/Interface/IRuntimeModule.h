#ifndef IRUNTIME_MODULE_H
#define IRUNTIME_MODULE_H

#include "IApplication.h"
#include "FyuuError.h"

#include <stdexcept>

namespace Fyuu {

	class IRuntimeModule {

    public:

        IRuntimeModule(IApplication* app) {
            if (!app) {
                throw std::invalid_argument("null app pointer");
            }
            m_app = app;
        }
        virtual ~IRuntimeModule() = default;

        virtual void Tick() = 0;

        void SetOwner(IApplication* app) {
            if (!app) {
                throw std::invalid_argument("Invalid app pointer");
            }
            m_app = app;
        }

    protected:

        IApplication* m_app = nullptr;

	};

}

#endif // !IRUNTIME_MODULE_H
