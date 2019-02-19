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
using IRDB_Type    = IRDB_SDK::IRDBType_t;
using TypeSet_t    = IRDB_SDK::TypeSet_t;
using TypeVector_t = IRDB_SDK::TypeVector_t;

class Type_t : public BaseObj_t, virtual public IRDB_SDK::Type_t
{
	public:
		Type_t() : BaseObj_t(NULL) {
			typeID = IRDB_SDK::itUnknown;
		}	
		Type_t(db_id_t dbid, IRDB_Type t, std::string n) : BaseObj_t(NULL) {
			setBaseID(dbid);
			typeID = t;
			name = n;
		}	
		virtual ~Type_t() {}

		virtual std::string WriteToDB(File_t *fid, db_id_t newid) = 0;

		std::string getName() const { return name; }
		void setName(const std::string& newname) { name=newname; }
		IRDB_Type getTypeID() const { return typeID; }
		void setTypeID(IRDB_Type t) { typeID = t; }

		virtual bool isUnknownType() const { return typeID == IRDB_SDK::itUnknown; }
		virtual bool isVariadicType() const { return typeID == IRDB_SDK::itVariadic; }
		virtual bool isAggregateType() const { return typeID == IRDB_SDK::itAggregate; }
		virtual bool isFuncType() const { return typeID == IRDB_SDK::itFunc; }
		virtual bool isBasicType() const { return false; }
		virtual bool isPointerType() const { return false; }
		virtual bool isNumericType() const { return false; }

	private:
		std::string name;
		IRDB_Type typeID;
};

class BasicType_t : public Type_t, virtual public IRDB_SDK::BasicType_t
{
	public:
		BasicType_t() : Type_t() {}	
		BasicType_t(db_id_t id, IRDB_Type type, std::string p_name) : Type_t(id, type, p_name) {}
		virtual ~BasicType_t() {}

		virtual bool isBasicType() const { return true; }
		virtual bool isNumericType() const;

		std::string WriteToDB(File_t *fid, db_id_t newid);
};

class PointerType_t : public Type_t, virtual public IRDB_SDK::PointerType_t
{
	public:
		PointerType_t() : Type_t() { setTypeID(IRDB_SDK::itPointer); referentType = NULL; }	
		PointerType_t(db_id_t id, Type_t* ref, std::string p_name) : Type_t(id, IRDB_SDK::itPointer, p_name) {
			setReferentType(ref);
		}
		virtual ~PointerType_t() {}
		virtual bool isPointerType() const { return true; }

		IRDB_SDK::Type_t* getReferentType() const { return referentType; }
		void setReferentType(IRDB_SDK::Type_t* r) { referentType = dynamic_cast<Type_t*>(r); if(r) assert(referentType); }

		std::string WriteToDB(File_t *fid, db_id_t newid);

	private:
		Type_t* referentType; 
};

class AggregateType_t : public Type_t, virtual public IRDB_SDK::AggregateType_t
{
	public:
		AggregateType_t() : Type_t() { setTypeID(IRDB_SDK::itAggregate); }	
		AggregateType_t(db_id_t id, std::string p_name) : Type_t(id, IRDB_SDK::itAggregate, p_name) {}
		virtual ~AggregateType_t() {}
		virtual bool isAggregateType() const { return true; }

		void addAggregatedType(IRDB_SDK::Type_t *t, int pos);
		virtual size_t getNumAggregatedTypes() const { return refTypes.size(); } 
		IRDB_SDK::Type_t* getAggregatedType(unsigned int pos) const { 
			return (pos < (unsigned int)refTypes.size()) ? refTypes.at(pos) : NULL;
		}

		std::string toString() {
			return getName();
		}

		std::string WriteToDB(File_t *fid, db_id_t newid);

	private:
		TypeVector_t refTypes;
};

class FuncType_t : public Type_t, virtual public IRDB_SDK::FuncType_t
{
	public:
		FuncType_t() : Type_t() { setTypeID(IRDB_SDK::itFunc); _init(); }	
		FuncType_t(db_id_t id, std::string p_name) : Type_t(id, IRDB_SDK::itFunc, p_name) { _init(); }
		virtual ~FuncType_t() {}

		virtual bool isFuncType() const { return true; }

		std::string WriteToDB(File_t *fid, db_id_t newid);

		IRDB_SDK::Type_t* getReturnType() const { return returnType; }
		void setReturnType(IRDB_SDK::Type_t *t) { returnType = dynamic_cast<Type_t*>(t); if(t) assert(returnType); }
		IRDB_SDK::AggregateType_t* getArgumentsType() const { return argsType; }
		void setArgumentsType(IRDB_SDK::AggregateType_t *t) { argsType = dynamic_cast<AggregateType_t*>(t); if(t) assert(argsType);  }

	private:
		void _init() {
			returnType = NULL;
			argsType = NULL;
		}

	private:
		Type_t*            returnType;
		AggregateType_t*   argsType;
};

}
