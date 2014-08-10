#include <stdlib.h>
#include <fstream>
#include <libIRDB-core.hpp>
#include <libgen.h>

#include "MEDS_AnnotationParser.hpp"
#include "rss_instrument.hpp"

using namespace std;
using namespace libIRDB;
using namespace MEDS_Annotation;


#define BINARY_NAME "a.ncexe"
#define SHARED_OBJECTS_DIR "shared_objects"


void usage(char* name)
{
	cerr<<"Usage: "<<name<<" <variant_id>\n"; 
}

int main(int argc, char **argv)
{
        if(argc != 2)
        {
                usage(argv[0]);
                exit(1);
        }

        string programName(argv[0]);
        int variantID = atoi(argv[1]);

        VariantID_t *pidp=NULL;

        /* setup the interface to the sql server */
        pqxxDB_t pqxx_interface;
        BaseObj_t::SetInterface(&pqxx_interface);

        pidp=new VariantID_t(variantID);
        assert(pidp->IsRegistered()==true);

	cout<<"ret_shadow_stack.exe started\n";

        bool one_success = false;
        for(set<File_t*>::iterator it=pidp->GetFiles().begin();
            it!=pidp->GetFiles().end();
                ++it)
        {
                File_t* this_file = *it;
                FileIR_t *firp = new FileIR_t(*pidp, this_file);
                char *fileBasename = basename((char*)this_file->GetURL().c_str());

		cout<<"Transforming "<<this_file->GetURL()<<endl;

                assert(firp && pidp);

                try
                {
                        string annotationFilename;
                        // need to map filename to integer annotation file produced by STARS
                        // this should be retrieved from the IRDB but for now, we use files to store annotations
                        // convention from within the peasoup subdirectory is:
                        //      a.ncexe.infoannot
                        //      shared_objects/<shared-lib-filename>.infoannot
                        if (strcmp(fileBasename, BINARY_NAME) == 0)
                                annotationFilename = string(BINARY_NAME) + string(".annot");
                        else
                                annotationFilename = string(SHARED_OBJECTS_DIR) + "/" + fileBasename + ".annot";

                        cerr << "annotation file: " << annotationFilename << endl;

                        // parse MEDS integer annotations
                        ifstream annotationFile(annotationFilename.c_str(), ifstream::in);
                        if (!annotationFile.is_open())
                        {
                                cerr << "annotation file not found: " << annotationFilename.c_str() << endl;
                                continue;
                        }

                        MEDS_AnnotationParser annotationParser(annotationFile);

//                        std::multimap<VirtualOffset, MEDS_AnnotationBase> annotations = annotationParser.getAnnotations();

			RSS_Instrument rssi(firp, &annotationParser);


			int exitcode=rssi.execute();

                        if (exitcode == 0)
                        {
				cout<<"Writing changes for "<<this_file->GetURL()<<endl;
                                one_success = true;
                                firp->WriteToDB();
                                delete firp;
                        }
			else
			{
				cout<<"Skipping (no changes) "<<this_file->GetURL()<<endl;
			}
                }
                catch (DatabaseError_t pnide)
                {
                        cerr << programName << ": Unexpected database error: " << pnide << "file url: " << this_file->GetURL() << endl;
                }
                catch (...)
                {
                        cerr << programName << ": Unexpected error file url: " << this_file->GetURL() << endl;
                }
        } // end file iterator

        // if any integer transforms for any files succeeded, we commit
        if (one_success)
	{
		cout<<"Commiting changes...\n";
                pqxx_interface.Commit();
	}

        return 0;
}

