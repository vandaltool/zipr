#include <iostream>
#include <fstream>
#include <MEDS_AnnotationParser.hpp>
#include <MEDS_SafeFuncAnnotation.hpp>

using namespace std;

int main(int argc, char *argv[] )
{

	if(argc!=2)
	{
		cout << "Usage: " << argv[0] << " <infile> " << endl;
		return 2;
	}

	ifstream fin(argv[1]);

	if(!fin)
	{
		cout << "Usage: " << argv[0] << " <infile> " << endl;
		return 2;
	}

	using namespace MEDS_Annotation;
	MEDS_AnnotationParser meds_ap(fin);

        cout << dec << meds_ap.getAnnotations().size()     <<  " instruction annotations "  << endl;
        cout << dec << meds_ap.getFuncAnnotations().size() <<  " function annotations "  << endl;
        return 0;

}
