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

#ifndef _IRDB_TYPE_H_
#define _IRDB_TYPE_H_

// add as many types as you want here
// MUST MATCH code in meds2pdb!!!
typedef enum IRDB_Type {
	T_UNKNOWN = 0,     // must be 0, leave this value alone
	T_NUMERIC = 1, T_POINTER = 2, 
	T_VOID = 10, T_VARIADIC = 11, T_INT = 12, T_CHAR = 13, T_FLOAT = 14, T_DOUBLE = 15, 
	T_TYPEDEF = 21, T_SUBTYPE = 22, 
	T_FUNC = 100, T_AGGREGATE = 101
} IRDB_Type;

class Type_t;

typedef std::set<Type_t*> TypeSet_t;
typedef std::vector<Type_t*> TypeVector_t;

class Type_t : public BaseObj_t
{
	public:
		Type_t() : BaseObj_t(NULL) {
			typeID = T_UNKNOWN;
		}	
		Type_t(db_id_t dbid, IRDB_Type t, std::string n) : BaseObj_t(NULL) {
			SetBaseID(dbid);
			typeID = t;
			name = n;
		}	
		virtual ~Type_t() {}

		virtual std::string WriteToDB(File_t *fid, db_id_t newid) = 0;

		std::string GetName() const { return name; }
		void SetName(std::string newname) { name=newname; }
		IRDB_Type GetTypeID() const { return typeID; }
		void SetTypeID(IRDB_Type t) { typeID = t; }

		virtual bool IsUnknownType() const { return typeID == T_UNKNOWN; }
		virtual bool IsVariadicType() const { return typeID == T_VARIADIC; }
		virtual bool IsAggregateType() const { return typeID == T_AGGREGATE; }
		virtual bool IsFuncType() const { return typeID == T_FUNC; }
		virtual bool IsBasicType() const { return false; }
		virtual bool IsPointerType() const { return false; }
		virtual bool IsNumericType() const { return false; }

	private:
		std::string name;
		IRDB_Type typeID;
};

class BasicType_t : public Type_t
{
	public:
		BasicType_t() : Type_t() {}	
		BasicType_t(db_id_t id, IRDB_Type type, std::string name) : Type_t(id, type, name) {}
		virtual ~BasicType_t() {}

		virtual bool IsBasicType() const { return true; }
		virtual bool IsNumericType() const;

		std::string WriteToDB(File_t *fid, db_id_t newid);
};

class PointerType_t : public Type_t
{
	public:
		PointerType_t() : Type_t() { SetTypeID(T_POINTER); referentType = NULL; }	
		PointerType_t(db_id_t id, Type_t* ref, std::string name) : Type_t(id, T_POINTER, name) {
			SetReferentType(ref);
		}
		virtual ~PointerType_t() {}
		virtual bool IsPointerType() const { return true; }

		Type_t* GetReferentType() const { return referentType; }
		void SetReferentType(Type_t* r) { referentType = r; }

		std::string WriteToDB(File_t *fid, db_id_t newid);

	private:
		Type_t* referentType; 
};

class AggregateType_t : public Type_t
{
	public:
		AggregateType_t() : Type_t() { SetTypeID(T_AGGREGATE); }	
		AggregateType_t(db_id_t id, std::string name) : Type_t(id, T_AGGREGATE, name) {}
		virtual ~AggregateType_t() {}
		virtual bool IsAggregateType() const { return true; }

		void AddAggregatedType(Type_t *t, int pos);
		virtual int GetNumAggregatedTypes() const { return refTypes.size(); } 
		Type_t* GetAggregatedType(int pos) const { 
			return (pos >= 0 && pos < (int)refTypes.size()) ? refTypes.at(pos) : NULL;
		}

		std::string toString() {
			return GetName();
		}

		std::string WriteToDB(File_t *fid, db_id_t newid);

	private:
		TypeVector_t refTypes;
};

class FuncType_t : public Type_t
{
	public:
		FuncType_t() : Type_t() { SetTypeID(T_FUNC); _init(); }	
		FuncType_t(db_id_t id, std::string name) : Type_t(id, T_FUNC, name) { _init(); }
		virtual ~FuncType_t() {}

		virtual bool IsFuncType() const { return true; }

		std::string WriteToDB(File_t *fid, db_id_t newid);

		Type_t* GetReturnType() const { return returnType; }
		void SetReturnType(Type_t *t) { returnType = t; }
		AggregateType_t* GetArgumentsType() const { return argsType; }
		void SetArgumentsType(AggregateType_t *t) { argsType = t; }

	private:
		void _init() {
			returnType = NULL;
			argsType = NULL;
		}

	private:
		Type_t*            returnType;
		AggregateType_t*   argsType;
};

#endif
