#ifndef TSS_H
#define TSS_H

class IRDB_SDK::TransformStepState_t
{
        public:

                TransformStepState_t(DatabaseID_t v, IRDBObjects_t* i)
                        :
                                vid(v),
                                irdb_objects(i)
                {
                }



        DatabaseID_t vid;
        IRDBObjects_t* irdb_objects;

};



#endif
