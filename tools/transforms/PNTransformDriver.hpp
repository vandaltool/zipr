
#ifndef __PNTRANSFORMDRIVER
#define __PNTRANSFORMDRIVER

#include <vector>
#include <set>
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include <csignal>
#include "Rewrite_Utility.hpp"

struct canary
{
    unsigned int canary_val;
    int ret_offset;//Should be negative, the value to subtract from esp if esp is at ret addr
    int esp_offset;
};

class PNTransformDriver
{
protected:
    libIRDB::VariantID_t *pidp;
    libIRDB::FileIR_t *orig_virp;
    std::string BED_script;
    int orig_progid;
    bool do_canaries;
    bool do_align;
	std::map<std::string,double> coverage_map;
	int no_validation_level;
	double coverage_threshold;
	bool do_shared_object_protection;
  
    std::vector< std::vector<PNStackLayoutInference*> > transform_hierarchy;
    PNRegularExpressions pn_regex;
    std::set<std::string> blacklist;
    std::set<std::string> only_validate_list;
    //std::map<libIRDB::Instruction_t*,std::string> undo_list;
    //std::map<libIRDB::Instruction_t*,libIRDB::Instruction_t*> undo_list;
    std::map<std::string, std::map<libIRDB::Instruction_t*,libIRDB::Instruction_t*> > undo_list;
    std::map< std::string,std::vector<PNStackLayout*> > transformed_history;
    int blacklist_funcs;
    int total_funcs;
    std::vector<std::string> not_transformable;
    std::vector<libIRDB::Function_t*> failed;

    //   virtual bool Rewrite(PNStackLayout *layout, libIRDB::Function_t *func);
//    virtual bool LayoutValidation(PNStackLayout *layout);
    virtual bool Validate(libIRDB::FileIR_t *virp, libIRDB::Function_t *func);
    //virtual void undo(std::map<libIRDB::Instruction_t*,std::string> undo_list, libIRDB::Function_t *func);
    virtual void undo( libIRDB::Function_t *func);
    //virtual void reset_undo(std::string func);
    virtual std::vector<PNStackLayout*> GenerateInferences(libIRDB::Function_t *func, int level);
    virtual bool ShuffleValidation(int reps, PNStackLayout *layout,libIRDB::Function_t *func);
    //virtual void GenerateTransforms2(libIRDB::FileIR_t *virp,std::vector<libIRDB::Function_t*> funcs,std::string BED_script, int progid);
    //virtual bool ValidateLayout(PNStackLayout *layout,std::string BED_script,int progid);
//    virtual bool Canary_Rewrite(FileIR_t *virp, PNStackLayout *orig_layout,libIRDB::Function_t *func);
    //altered for TNE hack for dyn array padding, assuming all virp is orig_virp
    virtual bool Canary_Rewrite( PNStackLayout *orig_layout,libIRDB::Function_t *func);
    virtual bool Sans_Canary_Rewrite(PNStackLayout *orig_layout, libIRDB::Function_t *func);
    inline bool Instruction_Rewrite(PNStackLayout *layout, libIRDB::Instruction_t *instr);
    virtual void Print_Report();
    virtual bool CanaryTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func,bool validate);
    virtual bool PaddingTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func,bool validate);
    virtual bool LayoutRandTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func, bool validate);
    virtual void GenerateTransformsInit();
    virtual bool IsBlacklisted(libIRDB::Function_t *func);
    virtual unsigned int GetRandomCanary();
	virtual void GenerateTransformsHidden();

public:
    static bool timeExpired;
    //TODO: use unsigned int?
    
    PNTransformDriver(libIRDB::VariantID_t *pidp, std::string BED_script);
    virtual ~PNTransformDriver();

    //Level indicates the priority of the layout when attempting
    //the transform. Levels start at 1 (the highest priority). A level less than 1 raises
    //an exception (for now an assert will fail). If not level is
    //provided, the default is level 1.
    virtual void AddInference(PNStackLayoutInference *inference, int level=0);
    virtual void AddBlacklist(std::set<std::string> &blacklist);
    virtual void AddBlacklistFunction(std::string func_name);
    virtual void AddOnlyValidateList(std::set<std::string> &only_validate_list);
    virtual void SetDoCanaries(bool do_canaries);
    virtual void SetDoAlignStack(bool align_stack);
	virtual void SetCoverageMap(std::map<std::string,double> coverage_map);
	virtual void SetNoValidationLevel(unsigned int no_validation_level);
	virtual void SetCoverageThreshold(double threshold);
	virtual void SetProtectSharedObjects(bool do_protection);

    virtual void GenerateTransforms();
};

#endif
