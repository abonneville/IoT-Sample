/*
 * CommandInterface.hpp
 *
 *  Created on: Mar 27, 2019
 *      Author: Andrew
 */

#ifndef COMMANDINTERFACE_HPP_
#define COMMANDINTERFACE_HPP_

#include <array>
#include "thread.hpp"
#include "ResponseInterface.hpp"

/**
 * @brief Implements a persistent thread responsible for; command reception, parsing, and
 * 		  execution
 */
class CommandInterface : public cpp_freertos::Thread
{

    public:
		CommandInterface(ResponseInterface &handle);

    protected:
        virtual void Run();

    private:
        std::array<char,128> commandLineBuffer;
        ResponseInterface &responseHandle;
};





#endif /* COMMANDINTERFACE_HPP_ */
