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
		typedef std::array<char, 128> Buffer_t;

		CommandInterface(ResponseInterface &handle);

		ResponseId_t AwsCmdHandler(Buffer_t::const_iterator, Buffer_t::const_iterator);
		ResponseId_t ResetCmdHandler();
		ResponseId_t WifiCmdHandler(Buffer_t::const_iterator, Buffer_t::const_iterator);


    protected:
        virtual void Run();

    private:
        Buffer_t commandLineBuffer;
        ResponseInterface &responseHandle;
};


bool invalidChar (char c);

template <typename bufIter>
bufIter cleanLineBuffer(bufIter first, bufIter last);

template <typename bufIter>
bufIter validateBuffer(bufIter first, bufIter last);


#endif /* COMMANDINTERFACE_HPP_ */
