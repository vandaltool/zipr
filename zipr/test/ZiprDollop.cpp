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

/*
 * Test whether creating dollop from an instruction
 * with no fallthrough properly excludes the remaining
 * instructions.
 */
bool TestgetContainingDollopNoFallthrough() {
	ZiprDollopManager_t dollop_man;
	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	Dollop_t *dollop_a = nullptr;

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(dollop_a);

	return dollop_man.getContainingDollop(insn_b) == nullptr &&
	       dollop_man.getContainingDollop(insn_a) == dollop_a;
}

/*
 * Test whether creating a dollop from an instruction
 * with a fallthrough properly contains the linked
 * instructions.
 */
bool TestgetContainingDollopFallthrough(void) {
	ZiprDollopManager_t dollop_man;
	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	Dollop_t *dollop_a = nullptr;

	insn_a->setFallthrough(insn_b);

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(dollop_a);

	return dollop_man.getContainingDollop(insn_b) == dollop_a &&
	       dollop_man.getContainingDollop(insn_a) == dollop_a;
}

/*
 * Test whether getContainingDollop works
 * properly when there is more than one
 * dollop in the manager.
 */
bool TestgetContainingDollop(void) {
	ZiprDollopManager_t dollop_man;
	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	Dollop_t *dollop_a = Dollop_t::CreateNewDollop(insn_a);
	Dollop_t *dollop_b = Dollop_t::CreateNewDollop(insn_b);
	dollop_man.AddDollops(dollop_a);
	dollop_man.AddDollops(dollop_b);
	return dollop_man.getContainingDollop(insn_a) == dollop_a &&
	       dollop_man.getContainingDollop(insn_b) == dollop_b;
}

/*
 * Sanity check whether adding a dollop to the
 * dollop manager actually works.
 */
bool TestAddDollopEntry(void) {
	ZiprDollopManager_t dollop_man;
	IRDB_SDK::Instruction_t *insn = new IRDB_SDK::Instruction_t();
	dollop_man.AddDollops(Dollop_t::CreateNewDollop(insn));
	return 1 == dollop_man.Size();
}

bool TestDollopPatch(void) {
	Dollop_t *a = nullptr;
	DollopPatch_t patch;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	a = Dollop_t::CreateNewDollop(insn_a);

	patch.Target(a);

	return patch.Target() == a;
}

bool TestDollopPatchDollopManager(void) {
	ZiprDollopManager_t dollop_man;
	DollopPatch_t patch_a, patch_b;
	Dollop_t *dollop_a, *dollop_b;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();

	dollop_a = Dollop_t::CreateNewDollop(insn_a);
	dollop_b = Dollop_t::CreateNewDollop(insn_b);

	patch_a.Target(dollop_b);
	patch_b.Target(dollop_a);

	dollop_man.AddDollopPatch(&patch_a);
	dollop_man.AddDollopPatch(&patch_b);

	dollop_man.PrintDollopPatches(cout);
	return true;
}

/*
 * Test whether adding a new dollop that starts
 * with an instruction already in a dollop splits
 * the existing dollop.
 */
bool TestAddNewDollopSplitsExistingDollop(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	Dollop_t *a, *b;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_c = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_d = new IRDB_SDK::Instruction_t();

	/*
	 * a targets c
	 * c targets d (which will ultimately create a new dollop)
	 * a->b->c->d
	 *
	 * A: a, b, c, d
	 */

	insn_a->setFallthrough(insn_b);
	insn_b->setFallthrough(insn_c);
	insn_c->setFallthrough(insn_d);

	a = Dollop_t::CreateNewDollop(insn_a);
	dollop_man.AddDollops(a);
	success = (a->getDollopEntryCount() == 4) && dollop_man.Size() == 1;


	cout << "Before AddNewDollops()." << endl;
	cout << dollop_man << endl;

	b = dollop_man.AddNewDollops(insn_c);

	cout << "After AddNewDollops()." << endl;
	cout << dollop_man << endl;
	return success &&
	       a->getDollopEntryCount() == 2 &&
	       b->getDollopEntryCount() == 2 &&
				 dollop_man.Size() == 2 &&
				 a->FallthroughDollop() == b &&
				 b->FallbackDollop() == a;
}

bool TestUpdateTargetsDollopManager(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	Dollop_t *a, *c;
	Dollop_t *original_insn_d_container = nullptr;
	Dollop_t *updated_insn_d_container = nullptr;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_c = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_d = new IRDB_SDK::Instruction_t();

	/*
	 * a targets c
	 * c targets d (which will ultimately create a new dollop)
	 * a->b
	 * c->d
	 *
	 * A: a, b
	 * C: c, d
	 */

	insn_a->setFallthrough(insn_b);
	insn_c->setFallthrough(insn_d);

	insn_a->setTarget(insn_c);
	insn_b->setTarget(insn_d);

	a = Dollop_t::CreateNewDollop(insn_a);
	c = Dollop_t::CreateNewDollop(insn_c);

	cout << "&a: " << std::hex << a << endl;
	cout << "&c: " << std::hex << c << endl;

	dollop_man.AddDollops(a);
	dollop_man.AddDollops(c);

	original_insn_d_container = dollop_man.getContainingDollop(insn_d);

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
	/* UpdateTargets(c) will notice that d is a target that is
	 * not at the head of a dollop. It will subsequently create
	 * a new dollop from that instruction by splitting
	 * the existing dollop.
	 */
	dollop_man.UpdateTargets(c);

	cout << "After UpdateTargets([ac])" << endl;

	cout << dollop_man << endl;
/*
	cout << "A: " << endl;
	cout << (*a) << endl;
	cout << "C: " << endl;
	cout << (*c) << endl;
*/
	updated_insn_d_container = dollop_man.getContainingDollop(insn_d);
	return c->getDollopEntryCount() == 1 &&
	       a->getDollopEntryCount() == 2 &&
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

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_c = new IRDB_SDK::Instruction_t();

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

bool TestDollopFallthroughDollopEntry(void) {
	Dollop_t *a;
	DollopEntry_t *aa, *bb;

	a = new Dollop_t();

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	
	aa = new DollopEntry_t(insn_a, a);
	bb = new DollopEntry_t(insn_b, a);

	a->push_back(aa);
	a->push_back(bb);

	return bb == a->FallthroughDollopEntry(aa) &&
	       nullptr == a->FallthroughDollopEntry(bb) &&
	       nullptr == a->FallthroughDollopEntry(nullptr);
}

bool TestDollopSplit(void) {
	Dollop_t *a, *b;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_c = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_d = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_e = new IRDB_SDK::Instruction_t();

	insn_a->setFallthrough(insn_b);
	insn_b->setFallthrough(insn_c);
	insn_c->setFallthrough(insn_d);
	insn_d->setFallthrough(insn_e);

	a = Dollop_t::CreateNewDollop(insn_a);

	cout << "Dollop A: " << endl;
	cout << *a << endl;

	b = a->Split(insn_b);

	cout << "Dollop A: " << endl;
	cout << *a << endl;
	cout << "Dollop B: " << endl;
	cout << *b << endl;

	return a->getDollopEntryCount() == 1 && b->getDollopEntryCount() == 4 &&
	       a->FallthroughDollop() == b && b->FallbackDollop() == a;
}

bool TestDollopEntryEquals(void) {
	DollopEntry_t *a, *b, *c, *d;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();

	a = new DollopEntry_t(insn_a, nullptr);
	b = new DollopEntry_t(insn_b, nullptr);
	c = new DollopEntry_t(insn_a, nullptr);
	d = new DollopEntry_t(insn_a, nullptr);
	d->TargetDollop((Dollop_t*)0x5000);

	return *a != *b &&
	       *b != *c &&
	       *a == *a &&
				 *b == *b &&
				 *a == *c &&
				 *a != *d;
}

/*
 * Test whether or not 
 * 1. dollops created from overlapping
 * instructions are correctly split
 * 2. whether or not the dollop manager realizes
 * that dollops might be getting readded
 * 3. whether or not dollop entries are reassigned
 * to the proper containing dollops.
 *
 * Instruction layout:
 * e\
 * b->c->d
 * a/
 *
 * Ultimate (correct) dollop layout: 
 * B_fallthrough: bf
 * A: a,
 * B: b,
 * E: e,
 * Anon: c->d
 * A->Anon
 * B->Anon
 * C->Anon
 * Anon->B_fallthrough
 */
bool TestCreateDollopsFromOverlappingInstructions(void) {
	bool success = true;
	ZiprDollopManager_t dollop_man;
	Dollop_t *a, *b, *e, *b_fallthrough;

	IRDB_SDK::Instruction_t *insn_a = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_b = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_bf= new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_c = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_d = new IRDB_SDK::Instruction_t();
	IRDB_SDK::Instruction_t *insn_e = new IRDB_SDK::Instruction_t();

	insn_a->setFallthrough(insn_c);
	insn_b->setFallthrough(insn_c);
	insn_e->setFallthrough(insn_c);
	insn_c->setFallthrough(insn_d);

	b_fallthrough = Dollop_t::CreateNewDollop(insn_bf);
	b = Dollop_t::CreateNewDollop(insn_b);
	b->FallthroughDollop(b_fallthrough);
	dollop_man.AddDollops(b);
	cout << "b (before transforming): " << endl << *b << endl;
	success = (dollop_man.Size() == 2 && b->FallthroughDollop() == b_fallthrough);

	a = dollop_man.AddNewDollops(insn_a);
	e = dollop_man.AddNewDollops(insn_e);

	cout << "a->FallthroughDollop(): " 
	     << std::hex << a->FallthroughDollop() << endl;
	cout << "b->FallthroughDollop(): " 
	     << std::hex << a->FallthroughDollop() << endl;
	cout << "e->FallthroughDollop(): " 
	     << std::hex << e->FallthroughDollop() << endl;
	cout << "Common tail: " << endl << *(b->FallthroughDollop()) << endl;
	cout << "# of Dollops: " << dollop_man.Size() << endl;

	return success &&
	       dollop_man.Size() == 5 &&
	       dollop_man.getContainingDollop(insn_b) == b &&
	       dollop_man.getContainingDollop(insn_a) == a &&
	       a->FallthroughDollop() == b->FallthroughDollop() &&
				 dollop_man.getContainingDollop(insn_c)->FallthroughDollop() ==
				 b_fallthrough;
}

int main(int argc, char *argv[])
{
	INVOKE(TestAddDollopEntry);
	INVOKE(TestDollopEntryEquals);
	INVOKE(TestDollopSplit);
	INVOKE(TestgetContainingDollop);
	INVOKE(TestgetContainingDollopFallthrough);
	INVOKE(TestgetContainingDollopNoFallthrough);
	INVOKE(TestDollopPatch);
	INVOKE(TestDollopPatchDollopManager);
	INVOKE(TestDollopPatchMapDollopManager);
	INVOKE(TestUpdateTargetsDollopManager);
	INVOKE(TestAddNewDollopSplitsExistingDollop);
	INVOKE(TestDollopFallthroughDollopEntry);
	INVOKE(TestCreateDollopsFromOverlappingInstructions);
	return 0;
}
