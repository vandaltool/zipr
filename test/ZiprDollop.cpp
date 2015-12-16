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

#include <zipr_all.h>
#include <zipr_sdk.h>

using namespace zipr;
using namespace std;
using namespace Zipr_SDK;

#define INVOKE(a) \
bool __ ## a ## _result = false; \
printf("Invoking " #a ":\n"); \
__ ## a ## _result = a(); \
printf(#a ":"); \
if (__ ## a ## _result) \
{ \
printf(" pass\n"); \
} \
else \
{ \
printf(" fail\n"); \
}

bool TestGetContainingDollopNoFallthrough() {
	ZiprDollopManager_t dollop_man;
	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	Dollop_t *dollop_a = NULL;

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(dollop_a);

	return dollop_man.GetContainingDollop(insn_b) == NULL &&
	       dollop_man.GetContainingDollop(insn_a) == dollop_a;
}

bool TestGetContainingDollopFallthrough(void) {
	ZiprDollopManager_t dollop_man;
	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	Dollop_t *dollop_a = NULL;

	insn_a->SetFallthrough(insn_b);

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(dollop_a);

	return dollop_man.GetContainingDollop(insn_b) == dollop_a &&
	       dollop_man.GetContainingDollop(insn_a) == dollop_a;
}

bool TestGetContainingDollop(void) {
	ZiprDollopManager_t dollop_man;
	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	Dollop_t *dollop_a = Dollop_t::CreateNewDollop(insn_a);
	Dollop_t *dollop_b = Dollop_t::CreateNewDollop(insn_b);
	dollop_man.AddDollops(dollop_a);
	dollop_man.AddDollops(dollop_b);
	return dollop_man.GetContainingDollop(insn_a) == dollop_a &&
	       dollop_man.GetContainingDollop(insn_b) == dollop_b;
}

bool TestAddDollopEntry(void) {
	ZiprDollopManager_t dollop_man;
	libIRDB::Instruction_t *insn = new libIRDB::Instruction_t();
	dollop_man.AddDollops(Dollop_t::CreateNewDollop(insn));
	return 1 == dollop_man.Size();
}

bool TestDollopPatch(void) {
	Dollop_t *a = NULL;
	DollopPatch_t patch;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	a = Dollop_t::CreateNewDollop(insn_a);

	patch.Target(a);

	return patch.Target() == a;
}

bool TestDollopPatchDollopManager(void) {
	ZiprDollopManager_t dollop_man;
	DollopPatch_t patch_a, patch_b;
	Dollop_t *dollop_a, *dollop_b;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_b = Dollop_t::CreateNewDollop(insn_b);

	patch_a.Target(dollop_b);
	patch_b.Target(dollop_a);

	dollop_man.AddDollopPatch(&patch_a);
	dollop_man.AddDollopPatch(&patch_b);

	dollop_man.PrintDollopPatches(cout);
	return true;
}

bool TestAddNewDollopSplitsExistingDollop(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	Dollop_t *a, *b;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_c = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_d = new libIRDB::Instruction_t();

	/*
	 * a targets c
	 * c targets d (which will ultimately create a new dollop)
	 * a->b->c->d
	 *
	 * A: a, b, c, d
	 */

	insn_a->SetFallthrough(insn_b);
	insn_b->SetFallthrough(insn_c);
	insn_c->SetFallthrough(insn_d);

	a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(a);
	success = (a->GetDollopEntryCount() == 4) && dollop_man.Size() == 1;


	cout << "Before AddNewDollops()." << endl;
	cout << dollop_man << endl;

	b = dollop_man.AddNewDollops(insn_c);

	cout << "After AddNewDollops()." << endl;
	cout << dollop_man << endl;
	return success &&
	       a->GetDollopEntryCount() == 2 &&
	       b->GetDollopEntryCount() == 2 &&
				 dollop_man.Size() == 2;
}

bool TestUpdateTargetsDollopManager(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	Dollop_t *a, *c;
	Dollop_t *original_insn_d_container = NULL;
	Dollop_t *updated_insn_d_container = NULL;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_c = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_d = new libIRDB::Instruction_t();

	/*
	 * a targets c
	 * c targets d (which will ultimately create a new dollop)
	 * a->b
	 * c->d
	 *
	 * A: a, b
	 * C: c, d
	 */

	insn_a->SetFallthrough(insn_b);
	insn_c->SetFallthrough(insn_d);

	insn_a->SetTarget(insn_c);
	insn_b->SetTarget(insn_d);

	a = Dollop_t::CreateNewDollop(insn_a);
	c = Dollop_t::CreateNewDollop(insn_c);

	cout << "&a: " << std::hex << a << endl;
	cout << "&c: " << std::hex << c << endl;

	dollop_man.AddDollops(a);
	dollop_man.AddDollops(c);

	original_insn_d_container = dollop_man.GetContainingDollop(insn_d);

	success &= (original_insn_d_container == c);

	cout << "Before UpdateTargets([ac])" << endl;

	cout << dollop_man << endl;
/*
	cout << "A: " << endl;
	cout << (*a) << endl;
	cout << "C: " << endl;
	cout << (*c) << endl;
*/

	dollop_man.UpdateTargets(a);
	dollop_man.UpdateTargets(c);

	cout << "After UpdateTargets([ac])" << endl;

	cout << dollop_man << endl;
/*
	cout << "A: " << endl;
	cout << (*a) << endl;
	cout << "C: " << endl;
	cout << (*c) << endl;
*/
	updated_insn_d_container = dollop_man.GetContainingDollop(insn_d);
	return c->GetDollopEntryCount() == 1 &&
	       a->GetDollopEntryCount() == 2 &&
				 dollop_man.Size() == 3 &&
				 updated_insn_d_container != original_insn_d_container &&
				 success;
}

bool TestDollopPatchMapDollopManager(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	DollopPatch_t patch_a, patch_aa, patch_c;
	Dollop_t *dollop_a, *dollop_c;
	std::list<DollopPatch_t *>::const_iterator patches_it, patches_it_end;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_c = new libIRDB::Instruction_t();

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_c = Dollop_t::CreateNewDollop(insn_c);

	patch_a.Target(dollop_a);
	patch_aa.Target(dollop_a);
	patch_c.Target(dollop_c);

	dollop_man.AddDollops(dollop_a);
	dollop_man.AddDollops(dollop_c);

	dollop_man.AddDollopPatch(&patch_a);
	dollop_man.AddDollopPatch(&patch_aa);
	dollop_man.AddDollopPatch(&patch_c);

	cout << "&dollop_a: " << std::hex << dollop_a << endl;
	cout << "&dollop_c: " << std::hex << dollop_c << endl;

	std::list<DollopPatch_t *> patches = dollop_man.PatchesToDollop(dollop_a);
	for (patches_it = patches.begin(), patches_it_end = patches.end();
	     patches_it != patches.end();
			 patches_it++) {
		success &= ((*patches_it)->Target()) == dollop_a;
		cout << *(*patches_it) << endl;
	}
	patches = dollop_man.PatchesToDollop(dollop_c);
	for (patches_it = patches.begin(), patches_it_end = patches.end();
	     patches_it != patches.end();
			 patches_it++) {
		success &= ((*patches_it)->Target()) == dollop_c;
		cout << *(*patches_it) << endl;
	}
	return success;
}

bool TestDollopSplit(void) {
	Dollop_t *a, *b;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_c = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_d = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_e = new libIRDB::Instruction_t();

	insn_a->SetFallthrough(insn_b);
	insn_b->SetFallthrough(insn_c);
	insn_c->SetFallthrough(insn_d);
	insn_d->SetFallthrough(insn_e);

	a = Dollop_t::CreateNewDollop(insn_a);

	cout << "Dollop A: " << endl;
	cout << *a << endl;

	b = a->Split(insn_b);

	cout << "Dollop A: " << endl;
	cout << *a << endl;
	cout << "Dollop B: " << endl;
	cout << *b << endl;

	return a->GetDollopEntryCount() == 1 && b->GetDollopEntryCount() == 4;
}

bool TestDollopEntryEquals(void) {
	DollopEntry_t *a, *b, *c, *d;

	libIRDB::Instruction_t *insn_a = new libIRDB::Instruction_t();
	libIRDB::Instruction_t *insn_b = new libIRDB::Instruction_t();

	a = new DollopEntry_t(insn_a);
	b = new DollopEntry_t(insn_b);
	c = new DollopEntry_t(insn_a);
	d = new DollopEntry_t(insn_a);
	d->TargetDollop((Dollop_t*)0x5000);

	return *a != *b &&
	       *b != *c &&
	       *a == *a &&
				 *b == *b &&
				 *a == *c &&
				 *a != *d;
}

int main(int argc, char *argv[])
{
	INVOKE(TestAddDollopEntry);
	INVOKE(TestDollopEntryEquals);
	INVOKE(TestDollopSplit);
	INVOKE(TestGetContainingDollop);
	INVOKE(TestGetContainingDollopFallthrough);
	INVOKE(TestGetContainingDollopNoFallthrough);
	INVOKE(TestDollopPatch);
	INVOKE(TestDollopPatchDollopManager);
	INVOKE(TestDollopPatchMapDollopManager);
	INVOKE(TestUpdateTargetsDollopManager);
	INVOKE(TestAddNewDollopSplitsExistingDollop);
	return 0;
}
