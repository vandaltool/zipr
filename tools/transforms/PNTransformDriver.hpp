
#ifndef __PNTRANSFORMDRIVER
#define __PNTRANSFORMDRIVER

#include <vector>
#include <set>
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include <csignal>

class PNTransformDriver
{
protected:
    std::vector< std::vector<PNStackLayoutInference*> > transform_hierarchy;
    PNRegularExpressions pn_regex;
    std::set<std::string> blacklist;
    std::map<libIRDB::Instruction_t*,std::string> undo_list;
    std::map< std::string,std::vector<PNStackLayout*> > transformed_history;
    
    
    virtual bool Rewrite(PNStackLayout *layouts, libIRDB::Function_t *func);
    virtual bool Validate(libIRDB::VariantIR_t *virp,libIRDB::Function_t *func,std::string BED_script,int progid);
    virtual void undo(std::map<libIRDB::Instruction_t*,std::string> undo_list, libIRDB::Function_t *func);
    virtual std::vector<PNStackLayout*> GenerateInferences(libIRDB::Function_t *func, int level);
    virtual bool ShuffleValidation(int reps, PNStackLayout *layout,libIRDB::VariantIR_t *virp,libIRDB::Function_t *func, std::string BED_script, int progid);
    //virtual void GenerateTransforms2(libIRDB::VariantIR_t *virp,std::vector<libIRDB::Function_t*> funcs,std::string BED_script, int progid);

public:
    static bool timeExpired;
    //TODO: use unsigned int?
    
    //Level indicates the priority of the layout when attempting
    //the transform. Levels start at 1 (the highest priority). A level less than 1 raises
    //an exception (for now an assert will fail). If not level is
    //provided, the default is level 1.
    virtual void AddInference(PNStackLayoutInference *inference, int level=1);
    virtual void AddBlacklist(std::set<std::string> &blacklist);
    virtual void AddBlacklistFunction(std::string &func_name);
    
    virtual void GenerateTransforms(libIRDB::VariantIR_t *virp, std::string BED_script, int progid, std::map<std::string,double>coverage_map, double p1threshold);
};

#endif
