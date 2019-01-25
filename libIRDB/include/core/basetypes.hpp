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


namespace libIRDB
{
	using  virtual_offset_t = IRDB_SDK::VirtualOffset_t;
	using  db_id_t = IRDB_SDK::DatabaseID_t;
	using  schema_version_t = IRDB_SDK::SchemaVersionID_t;

	// forward decls
	class AddressID_t;
	class ArchitectureDescription_t;
   	class BaseObj_t;
   	class DatabaseError_t;
   	class DBinterface_t;
   	class Doip_t;
   	class EhProgram_t;
   	class EhCallSite_t;
   	class File_t;
   	class FileIR_t;
   	class Function_t;
   	class ICFS_t;
   	class Instruction_t;
   	class IRDBObjects_t;
   	class pqxxDB_t;
   	class Relocation_t;
   	class DataScoop_t;
   	class Type_t;
   	class BasicType_t;
   	class PointerType_t;
   	class AggregateType_t;
   	class FuncType_t;
   	class VariantID_t;
	class DecodedInstructionDispatcher_t;
 	class DecodedOperandDispatcher_t;

}
