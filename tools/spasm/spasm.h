/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */


#ifndef SPASM
#define SPASM

#include <string>
#include <exception>
#include <vector>

//void a2bspri(const std::string &input, const std::string &output, const std::string &elfFile) ;
void a2bspri(const std::vector<std::string> &input,const std::string &outFilename, 
	const std::string &exeFilename, const std::string &symbolFilename) ;

class SpasmException: public std::exception
{
    private:
        std::string message;

    public:
        SpasmException(const std::string &message) throw ()
        {
            this->message = message;
        }

        SpasmException(const char* message) throw ()
        {
            this->message = std::string(message);
        }

        ~SpasmException() throw()
        {

        }

        virtual const char* what() const throw()
        {
            return this->message.c_str();
        }
};

#endif
