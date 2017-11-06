/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */


#ifndef __PNTRANSFORMDRIVER
#define __PNTRANSFORMDRIVER

#include <vector>
#include <set>
#include <exeio.h>
#include "PNStackLayoutInference.hpp"
#include "PNRegularExpressions.hpp"
#include <csignal>
#include "Rewrite_Utility.hpp"
#include <libIRDB-cfg.hpp>
#include "canary.h"
#include <libIRDB-core.hpp>


//TODO: I should use the types defined by beaengine
//#define RetType 13
//#define JmpType 11
//#define CallType 12


struct finalize_record
{
	Function_t *func;
	PNStackLayout* layout;
	FileIR_t *firp;	
};

struct validation_record
{
	unsigned int hierarchy_index;
	unsigned int layout_index;
	vector<PNStackLayout*> layouts;
	Function_t *func;
};


class PNTransformDriver
{
	protected:

    	libIRDB::VariantID_t *pidp;
    	libIRDB::FileIR_t *orig_virp;
	EXEIO::exeio* elfiop;
    	std::string BED_script;
    	int orig_progid;
    	bool do_canaries;
    	bool do_floating_canary;
    	bool do_align;
	//TODO: coverage map should not use function name as the key, since
	//we may want to support coverage for shared objects. 
	std::map<std::string,std::map<std::string,double> > coverage_map;
	int no_validation_level;
	double coverage_threshold;
	bool do_shared_object_protection;
  
    	std::vector< std::vector<PNStackLayoutInference*> > transform_hierarchy;
    	PNRegularExpressions *pn_regex;
    	std::set<std::string> blacklist;
	std::set<libIRDB::Function_t*> sanitized;
    	std::set<std::string> only_validate_list;
    	//std::map<libIRDB::Instruction_t*,std::string> undo_list;
    	//std::map<libIRDB::Instruction_t*,libIRDB::Instruction_t*> undo_list;
    	std::map<libIRDB::Function_t*, std::map<libIRDB::Instruction_t*,libIRDB::Instruction_t*> > undo_list;
    	std::map< std::string,std::vector<PNStackLayout*> > transformed_history;
    	int blacklist_funcs;
	int sanitized_funcs;
	int push_pop_sanitized_funcs;
	int cond_frame_sanitized_funcs;
	int bad_variadic_func_sanitized;
	int jump_table_sanitized;
  	int pic_jump_table_sanitized;
  	int eh_sanitized;
    	int total_funcs;
	int dynamic_frames;
    	std::vector<std::string> not_transformable;
    	std::vector<libIRDB::Function_t*> failed;
	std::vector<finalize_record> finalization_registry;
	std::set<FileIR_t*> registered_firps;
	int high_coverage_count, low_coverage_count, no_coverage_count, validation_count;

    	// write stack objects to IRDB
    	bool write_stack_ir_to_db;

	mitigation_policy m_mitigation_policy;

	// a way to map an instruction to it's set of predecessors. 
  	std::map< Instruction_t* , set<Instruction_t*> > preds;

//virtual bool Rewrite(PNStackLayout *layout, libIRDB::Function_t *func);
//virtual bool LayoutValidation(PNStackLayout *layout);
//virtual void undo(std::map<libIRDB::Instruction_t*,std::string> undo_list, libIRDB::Function_t *func);
//virtual void reset_undo(std::string func);

    	virtual bool Validate(libIRDB::FileIR_t *virp, std::string name);
    	virtual void undo( libIRDB::Function_t *func);
    	virtual std::vector<PNStackLayout*> GenerateInferences(libIRDB::Function_t *func, int level);
    	virtual bool ShuffleValidation(int reps, PNStackLayout *layout,libIRDB::Function_t *func);
	virtual void ShuffleValidation(std::vector<validation_record> &vrs);
//virtual void GenerateTransforms2(libIRDB::FileIR_t *virp,std::vector<libIRDB::Function_t*> funcs,std::string BED_script, int progid);
//virtual bool ValidateLayout(PNStackLayout *layout,std::string BED_script,int progid);
//virtual bool Canary_Rewrite(FileIR_t *virp, PNStackLayout *orig_layout,libIRDB::Function_t *func);

    	//altered for TNE hack for dyn array padding, assuming all virp is orig_virp
    	virtual bool Canary_Rewrite( PNStackLayout *orig_layout,libIRDB::Function_t *func);
    	virtual bool Sans_Canary_Rewrite(PNStackLayout *orig_layout, libIRDB::Function_t *func);
    	inline bool Instruction_Rewrite(PNStackLayout *layout, libIRDB::Instruction_t *instr, ControlFlowGraph_t* cfg);
	inline bool FunctionCheck(libIRDB::Function_t* a, libIRDB::Function_t* b);
	inline bool TargetFunctionCheck(libIRDB::Instruction_t* a, libIRDB::Instruction_t* b);
	inline bool FallthroughFunctionCheck(libIRDB::Instruction_t* a, libIRDB::Instruction_t* b);

	virtual PNStackLayout* Get_Next_Layout(validation_record &vr);

    	virtual void Print_Report();
    	virtual void Print_Map();
//    	virtual bool CanaryTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func,bool validate);
    	virtual bool PaddingTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func,bool validate);
    	virtual bool LayoutRandTransformHandler(PNStackLayout *layout, libIRDB::Function_t *func, bool validate);
    	virtual void GenerateTransformsInit();
    	virtual bool IsBlacklisted(libIRDB::Function_t *func);
    	virtual unsigned int GetRandomCanary();
	virtual void GenerateTransformsHidden(std::map<std::string,double> &file_coverage_map);
	void SanitizeFunctions();
//virtual bool WriteToDB();
    	virtual bool WriteStackIRToDB();

	virtual void Finalize_Transformation();
	void Register_Finalized(std::vector<validation_record> &vrs,unsigned int start, int length);
	bool Validate_Recursive(std::vector<validation_record> &vrs, unsigned int start, int length);//,bool suspect=false);
//bool Validate_Linear(std::vector<validation_record> &vrs, unsigned int start, int length);

	// see .cpp
	int prologue_offset_to_actual_offset(ControlFlowGraph_t* cfg, Instruction_t *instr,int offset);
	bool check_jump_tables(Instruction_t* insn);
  	bool check_jump_table_entries(std::set<int> insn,Function_t *func);
  	bool check_for_PIC_switch_table64(Instruction_t* insn, DISASM disasm);
  	bool backup_until(const char* insn_type, Instruction_t *& prev, Instruction_t* orig);
  	void calc_preds();
	void InitNewFileIR(File_t* this_file);


	pqxxDB_t *pqxx_interface;

public:
    	static bool timeExpired;
    	//TODO: use unsigned int?
    
    	PNTransformDriver(libIRDB::VariantID_t *pidp, std::string BED_script, pqxxDB_t *pqxx_if);
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
    	virtual void SetDoFloatingCanary(bool do_floating_canary);
    	virtual void SetDoAlignStack(bool align_stack);
	virtual void SetCoverageMap(std::map<std::string,std::map<std::string,double> > coverage_map);
	virtual void SetNoValidationLevel(unsigned int no_validation_level);
	virtual void SetCoverageThreshold(double threshold);
	virtual void SetProtectSharedObjects(bool do_protection);

    	virtual void GenerateTransforms();
    	virtual void SetWriteStackIrToDb(bool setting) { write_stack_ir_to_db = setting; }

		inline virtual mitigation_policy GetMitigationPolicy() const { return m_mitigation_policy; }
		virtual void SetMitigationPolicy(mitigation_policy policy) { m_mitigation_policy = policy; }
};

#endif

