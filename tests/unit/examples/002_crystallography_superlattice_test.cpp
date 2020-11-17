#include "gtest/gtest.h"

#include "casm/crystallography/Superlattice.hh"
#include "casm/crystallography/SuperlatticeEnumerator.hh"
#include "casm/crystallography/SymTools.hh"
#include "casm/misc/CASM_Eigen_math.hh" // for almost_equal of Eigen types
#include "casm/misc/CASM_math.hh" // for almost_equal of other types

TEST(ExampleCrystallographySuperlattice, SuperlatticeConstruction) {

  CASM::xtal::Lattice unit_lattice {Eigen::Matrix3d::Identity()};

  // make a 2x2x2 supercell of the unit lattice
  Eigen::Matrix3l T;
  T << 2, 0, 0,
  0, 2, 0,
  0, 0, 2;

  CASM::xtal::Lattice basic_superlattice = make_superlattice(unit_lattice, T);
  EXPECT_TRUE(almost_equal(basic_superlattice.lat_column_mat(), unit_lattice.lat_column_mat() * T.cast<double>()));

  // There is also a class, Superlattice, which can be used to hold the unit lattice,
  //   the super lattice, and the transformation matrix
  CASM::xtal::Superlattice superlattice_1 {unit_lattice, T};
  EXPECT_EQ(superlattice_1.transformation_matrix_to_super(), T);
  EXPECT_TRUE(almost_equal(superlattice_1.superlattice().lat_column_mat(), basic_superlattice.lat_column_mat()));

  // The 'size' of a Superlattice is the number of unit lattices that fit inside the super lattice
  EXPECT_EQ(superlattice_1.size(), T.determinant());

  // A Superlattice, can also be constructed with a unit lattice and a super lattice,
  //   in which case the transformation matrix is calculated
  CASM::xtal::Superlattice superlattice_2 {unit_lattice, basic_superlattice};
  EXPECT_EQ(superlattice_1.transformation_matrix_to_super(), T);

  // The Superlattice constructor will throw if the provided super lattice is not an integer
  //   multiple of the unit lattice
  CASM::xtal::Lattice not_a_superlattice = basic_superlattice;
  not_a_superlattice[0] = basic_superlattice[0] * 1.1;
  ASSERT_THROW((CASM::xtal::Superlattice {unit_lattice, not_a_superlattice}), std::runtime_error);

}

// The SuperlatticeEnumerator class implements an algorithm for generating super lattices.
// - The algorithm generates transformation matrices, T, such that S = U*G*P*T, where:
//   - S = the super lattice vectors, a 3x3 column matrix
//   - U = the unit lattice vectors, a 3x3 column matrix
//   - G = a "generating matrix", a 3x3 integer matrix with det>0, by default the identity matrix,
//         generates a "unit lattice for enumeration" from the original unit lattice. (Note that
//         the algorithm could just let the user input U*G as the unit lattice. This construction
//         comes from the practice of making supercells of configurations, where U is the
//         primitive lattice vectors and G is the primitive to supercell transformation matrix of
//         the configuration's supercell.)
//   - P = a permutation matrix, generated by indicating which lattice vectors to enumerate over,
//         enabling enumeration of 1D, 2D, or 3D super lattices of the unit lattice
//   - T = the unit-lattice-to-super-lattice transformation matrix, a 3x3 integer matrix in Hermite
//         normal form
// - There exists a unique Hermite normal form transformation matrix, T, for every unique
//   super lattice. All potential super lattices can be considered in turn by generating all possible
//   Hermite normal form matrices that result in a particular volume super lattice. The desired
//   volume limits which values can be on the diagonal, and the Hermite normal form limits which
//   values need to be considered in the off diagonal elements. The algorithm iterates over
//   volumes specified by the user, then Hermite normal form matrices corresponding to that volume,
//   and then uses symmetry operations to transform T and identify a unique T from the set that are
//   equivalent under transformation by point group operations.
// - The HermiteCounter class is used to enumerate Hermite normal form matrices at a particular
//   volume.
// - The generated super lattice vectors are in the form generated directly from the transformation
//   matrices, and not in any reduced/Niglli/canonical form
//
// ScelEnumProps holds SuperlatticeEnumerator algorithm parameters:
// - begin_volume: The beginning volume to enumerate
// - end_volume: The past-the-last volume to enumerate
// - dirs: String indicating which lattice vectors to enumerate over. Some combination of
//   'a', 'b', and 'c', where 'a' indicates the first lattice vector of the unit cell, 'b' the
//   second, and 'c' the third. Used to generate the P matrix described above.
// - generating_matrix: This matrix, G, transforms the primitive lattice vectors into the
//   unit lattice with the vectors which are used to generate supercells.

// This class constructs input variables used in the following examples
class ExampleCrystallographySuperlatticeEnumerator: public testing::Test {
protected:

  CASM::xtal::Lattice unit_lattice;
  std::vector<CASM::xtal::SymOp> point_group;
  int begin_volume;
  int end_volume;
  std::string dirs;
  Eigen::Matrix3i generating_matrix;
  CASM::xtal::ScelEnumProps enumeration_params;

  ExampleCrystallographySuperlatticeEnumerator():
    unit_lattice(Eigen::Matrix3d::Identity()),
    point_group(CASM::xtal::make_point_group(unit_lattice)),
    begin_volume(1),
    end_volume(5),
    dirs("abc"),
    generating_matrix(Eigen::Matrix3i::Identity()),
    enumeration_params(begin_volume, end_volume, dirs, generating_matrix) {}

};

// Example1 demonstrates using SuperlatticeEnumerator to iterate through unique super lattices
TEST_F(ExampleCrystallographySuperlatticeEnumerator, Example1) {

  // Construct the SuperlatticeEnumerator
  CASM::xtal::SuperlatticeEnumerator enumerator {unit_lattice, point_group, enumeration_params};

  std::vector<CASM::xtal::Lattice> superlattices;

  int i = 0;
  auto it = enumerator.begin();
  auto end = enumerator.end();
  for(; it != end; ++it) {
    // The SuperlatticeIterator, it, provides the integral volume and transformation matrix directly
    EXPECT_TRUE(it.volume() >= begin_volume);
    EXPECT_TRUE(it.volume() < end_volume);
    EXPECT_TRUE(it.matrix().determinant() == it.volume());

    // Dereferencing provides the super lattice
    CASM::xtal::Lattice super_lattice = *it;
    EXPECT_TRUE(almost_equal(super_lattice.lat_column_mat(), unit_lattice.lat_column_mat() * it.matrix().cast<double>(), CASM::TOL));

    // // Print the generated super lattices
    // // - print volume
    // std::cout << i++ << " (volume=" << it.volume() << ")\n";
    // // - print the transformation matrix
    // std::cout << "transformation matrix:\n" << it.matrix() << "\n\n";
    // // - print the super lattice vectors
    // std::cout << "super lattice column vector matrix:\n" << super_lattice.lat_column_mat() << "\n\n";
    // // - print the reduced form (compact) super lattice vectors
    // std::cout << "super lattice (reduced form) column vector matrix:\n" << super_lattice.reduced_cell().lat_column_mat() << "\n\n";

    superlattices.push_back(super_lattice);
  }

  EXPECT_EQ(superlattices.size(), 16);

  // As an exercise, check if the results are unique and complete given the lattice point group
}

// Example2 demonstrates directly populating a vector<Lattice> from SuperlatticeEnumerator
TEST_F(ExampleCrystallographySuperlatticeEnumerator, Example2) {

  // Construct the SuperlatticeEnumerator
  CASM::xtal::SuperlatticeEnumerator enumerator {unit_lattice, point_group, enumeration_params};

  // Generate the super lattices and immdiately store them in a vector
  std::vector<CASM::xtal::Lattice> superlattices {enumerator.begin(), enumerator.end()};

  EXPECT_EQ(superlattices.size(), 16);

}