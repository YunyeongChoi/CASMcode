#ifndef UNITCELLWITHIN_HH
#define UNITCELLWITHIN_HH

#include <vector>
#include "casm/external/Eigen/Core"
#include "casm/external/Eigen/src/Core/Matrix.h"
#include "casm/global/eigen.hh"
#include "casm/misc/CASM_Eigen_math.hh"

namespace CASM {
  namespace xtal {
    class UnitCell;
    class UnitCellCoord;
    class Lattice;

    //This is just a forward declaration. The routine is implemented in Lattice.cc
    Eigen::Matrix3l make_transformation_matrix(const Lattice &tiling_unit, const Lattice &superlattice, double tol);

    /**
     * Handles bringing a UnitCell (i,j,k values) within a particular
     * superlattice. Provide the tiling unit and superlattice at construction,
     * and use this class to bring any external UnitCell within the
     * superlattice
     */

    class LatticePointWithin_f {
    public:
      typedef Eigen::Matrix<long, 3, 3> matrix_type;
      typedef Eigen::Matrix<long, 3, 1> vector_type;

      /// Specify the integer transformation matrix that turns the tiling unit
      /// into the desired superlattice. Lattice points (UnitCell) with fractional
      /// coordinates relative to the tiling unit will then be brought into
      /// the superlattice.
      LatticePointWithin_f(const matrix_type &superlattice_transformation_matrix)
        : m_transformation_matrix(superlattice_transformation_matrix),
          m_transformation_matrix_adjugate(adjugate(superlattice_transformation_matrix)),
          m_total_lattice_points_in_superlattice(superlattice_transformation_matrix.determinant()) {
        _throw_if_bad_transformation_matrix(this->m_transformation_matrix);
      }

      LatticePointWithin_f(const Eigen::Matrix3i &superlattice_transformation_matrix)
        : LatticePointWithin_f(matrix_type(superlattice_transformation_matrix.cast<long>())) {
      }

      /// Specify the tiling unit, and the superlattice into which lattice points (UnitCells)
      /// should be brought within. The superlattice must be an integer transformation of
      /// the tiling unit
      LatticePointWithin_f(const Lattice &tiling_unit, const Lattice &superlattice)
        : LatticePointWithin_f(make_transformation_matrix(tiling_unit, superlattice, TOL)) {
      }

      /// Brings the given lattice point within the superlattice
      vector_type operator()(const vector_type &ijk) const;

      template <typename UnitCellType>
      UnitCellType operator()(const UnitCellType &ijk) const {
        return UnitCellType(this->operator()(static_cast<vector_type>(ijk)));
      }

      UnitCellCoord operator()(const UnitCellCoord &bijk) const;

    private:
      /// Integer matrix that converts the tiling unit into the superlattice.
      /// For a tiling unit U, superlattice S, and transformation matrix T -> S=U*T
      matrix_type m_transformation_matrix;

      /// The adjugate matrix of the transformation matrix.
      /// adjugate(T)=det(T)*inverse(T)
      matrix_type m_transformation_matrix_adjugate;

      long int m_total_lattice_points_in_superlattice;

      /// Throws exception if the transformation matrix has determinant 0
      static void _throw_if_bad_transformation_matrix(const matrix_type &transformation_matrix);
    };

    //********************************************************************************************************************************//

    /**
     * The Smith Normal Form of a transformation matrix allows the enumeration of lattice points to
     * be generated in a particular order. This class can generate any lattice point given its index
     * in the list in constant time.
     *
     * If you want to convert between linear index and bijk (UnitCellCoord) quickly, you probably
     * want to be using the LinearIndexConverter class instead of this.
     */

    class OrderedLatticePointGenerator {
    public:
      typedef LatticePointWithin_f::matrix_type matrix_type;
      typedef LatticePointWithin_f::vector_type vector_type;

      /// Construct with the transformation matrix that takes the tiling unit to the superlattice
      OrderedLatticePointGenerator(const matrix_type &transformation_matrix);

      /// Given the index into the list of lattice points, return its corresponding lattice point, with
      /// guaranteed order
      vector_type operator()(Index ix) const;

      /// Returns the total number of unique lattice points that can be generated
      long int size() const {
        return m_total_lattice_points;
      }

    private:
      long int m_total_lattice_points;

      // Can map ijk values within the supercell
      LatticePointWithin_f m_bring_within_f;

      /// The Smith Normal Form decomposition is: trans_mat = U*S*V, with det(U)=det(V)=1; S is diagonal
      matrix_type m_smith_normal_U;
      /// The Smith Normal Form decomposition is: trans_mat = U*S*V, with det(U)=det(V)=1; S is diagonal
      matrix_type m_smith_normal_S;
      /// The Smith Normal Form decomposition is: trans_mat = U*S*V, with det(U)=det(V)=1; S is diagonal
      matrix_type m_smith_normal_V;

      /// stride maps canonical 3d index (m,n,p) onto linear index -- l = m + n*stride[0] + p*stride[1]
      int m_stride[2];

      /// Convert UnitCell created with strides (mnp) to a "normal" UnitCell in terms of the superlattice (ijk)
      /// U*mnp = ijk
      vector_type _normalize_lattice_point(const vector_type &mnp) const;

      /// Create a lattice point from the provided index, in the Smith Normal Form diagonalized space
      vector_type _make_smith_normal_form_lattice_point(Index ix) const;
    };

    //********************************************************************************************************************************//

    ///Returns all the lattice points that exists within the superlattice that has the given transformation matrix
    std::vector<UnitCell> make_lattice_points(const OrderedLatticePointGenerator::matrix_type &transformation_matrix);
    std::vector<UnitCell> make_lattice_points(const Eigen::Matrix3i &transformation_matrix);
    ///Returns all the lattice points that exists when tiling the tiling unit inside the superlattice
    std::vector<UnitCell> make_lattice_points(const Lattice &tiling_unit, const Lattice &superlattice, double tol);

  } // namespace xtal
} // namespace CASM

#endif
