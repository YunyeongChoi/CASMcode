#include "casm/database/Database_impl.hh"
#include "casm/database/DatabaseTypeDefs.hh"

// explicit template instantiations
#define INST_ValDatabase(r, data, type) template class ValDatabase<type>;

namespace CASM {
  namespace DB {
    BOOST_PP_SEQ_FOR_EACH(INST_ValDatabase, _, CASM_DB_TYPES)
  }
}