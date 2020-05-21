#ifndef CASM_global_enum
#define CASM_global_enum

namespace CASM {

  enum class COORD_TYPE {
    FRAC, CART, INTEGRAL, COORD_DEFAULT
  };

  const COORD_TYPE FRAC = COORD_TYPE::FRAC;
  const COORD_TYPE CART = COORD_TYPE::CART;
  const COORD_TYPE INTEGRAL = COORD_TYPE::INTEGRAL;
  const COORD_TYPE COORD_DEFAULT = COORD_TYPE::COORD_DEFAULT;


  enum class PERIODICITY_TYPE : int {
    PERIODIC = 0, APERIODIC = 1, LOCAL = 1, PERIODICITY_DEFAULT = 2
  };

  const PERIODICITY_TYPE PERIODIC = PERIODICITY_TYPE::PERIODIC;
  const PERIODICITY_TYPE LOCAL = PERIODICITY_TYPE::LOCAL;
  const PERIODICITY_TYPE PERIODICITY_DEFAULT = PERIODICITY_TYPE::PERIODICITY_DEFAULT;


  enum class EQUIVALENCE_TYPE {
    PRIM, SCEL, CONFIG
  };


  enum class CELL_TYPE {
    PRIM, SCEL
  };

  const CELL_TYPE PRIM = CELL_TYPE::PRIM;
  const CELL_TYPE SCEL = CELL_TYPE::SCEL;


  /// Standard Error handling options:
  ///
  /// THROW: Throw an exception
  /// WARN: Print warning message, and continue
  /// CONTINUE: Continue quietly
  enum class OnError {
    THROW, WARN, CONTINUE
  };

}

#endif