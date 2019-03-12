#include <irdb-core>
#include <transform_step_state.hpp>

using namespace IRDB_SDK;

File_t* TransformStep_t::getMainFile()
{
	auto variantID=getVariantID();
	auto irdb_objects=m_state->irdb_objects;

	/* setup the interface to the sql server */
	auto &pqxx_interface=*irdb_objects->getDBInterface();;
	BaseObj_t::setInterface(&pqxx_interface);

	const auto pidp = irdb_objects->addVariant(variantID);
	assert(pidp->isRegistered()==true);

	auto this_file=pidp->getMainFile();
	return this_file;

}
FileIR_t* TransformStep_t::getMainFileIR()
{
	auto variantID=getVariantID();
	auto irdb_objects=m_state->irdb_objects;
	auto this_file=getMainFile();
	auto firp = irdb_objects->addFileIR(variantID, this_file->getBaseID());

	return firp;
}

DatabaseID_t TransformStep_t::getVariantID()
{
	auto variantID=m_state->vid;
	return variantID;
}

void TransformStep_t::setState(TransformStepState_t* p_state) 
{
	m_state=p_state;

}

IRDB_SDK::IRDBObjects_t*const  TransformStep_t::getIRDBObjects()
{
	return m_state->irdb_objects;
}
	
