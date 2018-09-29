#ifndef CASM_global_enum
#define CASM_global_enum

#include "casm/casm_io/EnumIO.hh"

namespace CASM {

  enum class COORD_TYPE {
    FRAC, CART, INTEGRAL, COORD_DEFAULT
  };

  const COORD_TYPE FRAC = COORD_TYPE::FRAC;
  const COORD_TYPE CART = COORD_TYPE::CART;
  const COORD_TYPE INTEGRAL = COORD_TYPE::INTEGRAL;
  const COORD_TYPE COORD_DEFAULT = COORD_TYPE::COORD_DEFAULT;

  ENUM_IO_DECL(COORD_TYPE)
  ENUM_TRAITS(COORD_TYPE)


  enum class PERIODICITY_TYPE : int {
    PERIODIC = 0, APERIODIC = 1, LOCAL = 1, PERIODICITY_DEFAULT = 2
  };

  const PERIODICITY_TYPE PERIODIC = PERIODICITY_TYPE::PERIODIC;
  const PERIODICITY_TYPE LOCAL = PERIODICITY_TYPE::LOCAL;
  const PERIODICITY_TYPE PERIODICITY_DEFAULT = PERIODICITY_TYPE::PERIODICITY_DEFAULT;

  ENUM_IO_DECL(PERIODICITY_TYPE)
  ENUM_TRAITS(PERIODICITY_TYPE)


  enum class EQUIVALENCE_TYPE {
    PRIM, SCEL, CONFIG
  };

  ENUM_IO_DECL(EQUIVALENCE_TYPE)
  ENUM_TRAITS(EQUIVALENCE_TYPE)


  enum class CELL_TYPE {
    PRIM, SCEL
  };

  const CELL_TYPE PRIM = CELL_TYPE::PRIM;
  const CELL_TYPE SCEL = CELL_TYPE::SCEL;

  ENUM_IO_DECL(CELL_TYPE)
  ENUM_TRAITS(CELL_TYPE)

  /// Standard Error handling options:
  ///
  /// THROW: Throw an exception
  /// WARN: Print warning message, and continue
  /// CONTINUE: Continue quietly
  enum class OnError {
    THROW, WARN, CONTINUE
  };

  ENUM_IO_DECL(OnError)
  ENUM_TRAITS(OnError)

}

#endif