#include <zipr_all.h>

using namespace zipr;

void zipr::PrintStat(std::ostream &out, std::string description, double value)
{
	out << description << ": " << std::dec << value << std::endl;
}

