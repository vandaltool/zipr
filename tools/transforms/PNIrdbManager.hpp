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

#ifndef __PN_IRDB_MANAGER_HPP__
#define __PN_IRDB_MANAGER_HPP__

#include <libIRDB-core.hpp>
#include <map>

class PNIrdbManager
{
public:

    enum IRSource {
        PEASOUP,
        DWARF,
    };

    PNIrdbManager(libIRDB::db_id_t variant_db_id);
    virtual ~PNIrdbManager();

    // Create the IRDB table managed by this class
    virtual void CreateTable();

    // Check the existence of the IRDB table managed by this class
    virtual bool TableExists();

    // Delete the contents of the IRDB table managed by this class
    virtual void ClearTable();

    // Delete all stack objects from a given source
    virtual void DeleteSource(IRSource source);

    // Add a stack object
    virtual libIRDB::db_id_t InsertStackObject(
        std::string function_name,
        int offset,
        unsigned int size,
        IRSource source);

protected:

    /// @brief Variant ID associated with this table manager
    libIRDB::VariantID_t * m_variant_id;

    /// @brief File associated with this table manager
    libIRDB::FileIR_t * m_file_ir;

    /// @brief Functions identified in this variant
    std::map<std::string, libIRDB::Function_t*> m_function_map;

    /// @brief Name of the table managed by this class
    std::string m_table_name;
};

#endif // __PN_IRDB_MANAGER_HPP__
