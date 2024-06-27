#ifndef IAPPILICATION_H
#define IAPPILICATION_H

#include <functional>
#include <optional>
#include <stdexcept>
#include <memory>
#include <vector>

namespace Fyuu{

    class IApplication {

    public:

        virtual ~IApplication() noexcept = default;

        bool IsQuit() const { 
            return m_quit; 
        }
        void RequestQuit() {
            m_quit = true; 
        }

        virtual void Tick() = 0;

    protected:

        IApplication(int argc, char** argv) {

            if (argc < 0 || !argv) {
                throw std::invalid_argument("Invalid command line arguments");
            }
            m_argc = argc;
            m_argv = argv;

        }

    protected:

        char** m_argv = nullptr;
        int m_argc = 0;
        bool m_quit = false;

    };

}

#endif