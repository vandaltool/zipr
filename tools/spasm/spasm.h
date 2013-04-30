
#ifndef SPASM
#define SPASM

#include <string>
#include <exception>
#include <vector>

//void a2bspri(const std::string &input, const std::string &output, const std::string &elfFile) throw(std::exception);
void a2bspri(const std::vector<std::string> &input,const std::string &outFilename, const std::string &symbolFilename) throw(std::exception);

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
