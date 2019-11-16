/*
 * Copyright (C) 2019 Andrew Bonneville.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COMMANDINTERFACE_HPP_
#define COMMANDINTERFACE_HPP_

#include <array>
#include "thread.hpp"
#include "queue.hpp"
#include "ResponseInterface.hpp"

class UserConfig;

/**
 * @brief Implements a persistent thread responsible for; command reception, parsing, and
 * 		  execution
 */
class CommandInterface : public cpp_freertos::Thread
{

    public:
		typedef std::array<char, 128> Buffer_t;

		CommandInterface(cpp_freertos::Queue &, UserConfig &);

		ResponseId_t CloudCmdHandler(Buffer_t::const_iterator, Buffer_t::const_iterator);
		ResponseId_t ResetCmdHandler();
		ResponseId_t WifiCmdHandler(Buffer_t::const_iterator, Buffer_t::const_iterator);


    protected:
        virtual void Run();

    private:
		uint16_t RxPEMObject(uint8_t *, uint16_t );


        Buffer_t commandLineBuffer;
		cpp_freertos::Queue &msgHandle;
		UserConfig &userConfigHandle;
};


bool invalidChar (char c);

template <typename bufIter>
bufIter cleanLineBuffer(bufIter first, bufIter last);

template <typename bufIter>
bufIter validateBuffer(bufIter first, bufIter last);


#endif /* COMMANDINTERFACE_HPP_ */
