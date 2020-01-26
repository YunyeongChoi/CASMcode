#ifndef CASM_ClusterSymCompare_impl
#define CASM_ClusterSymCompare_impl

#include "casm/clusterography/ClusterSymCompare.hh"
#include "casm/symmetry/SymPermutation.hh"
#include "casm/crystallography/UnitCellCoord.hh"

namespace CASM {

  /// \brief Orders 'prepared' elements in the same orbit
  ///
  /// - Returns 'true' to indicate A < B
  /// - Equivalence is indicated by \code !compare(A,B) && !compare(B,A) \endcode
  /// - Assumes elements are 'prepared' before being compared
  /// Implementation:
  /// - First compares by number of sites in cluster
  /// - Then compare all displacements, from longest to shortest
  template<typename Base>
  bool ClusterSymCompare<Base>::invariants_compare_impl(const Element &A, const Element &B) const {
    return CASM::compare(A.invariants(), B.invariants(), this->derived().tol());
  }

  /// \brief Compares 'prepared' elements
  ///
  /// - Returns 'true' to indicate A < B
  /// - Equivalence is indicated by \code !compare(A,B) && !compare(B,A) \endcode
  /// - Assumes elements are 'prepared' before being compared
  template<typename Base>
  bool ClusterSymCompare<Base>::compare_impl(const Element &A, const Element &B) const {
    return A < B;
  }


  /// \brief Returns transformation that takes 'obj' to its prepared (canonical) form
  ///
  /// - For now returns pointer to SymPermutation object that encodes permutation due to sorting elements
  template<typename Base>
  std::unique_ptr<SymOpRepresentation> ClusterSymCompare<Base>::canonical_transform_impl(Element const &obj)const {
    return std::unique_ptr<SymOpRepresentation>(new SymPermutation(obj.sort_permutation()));
  }

  /// \brief type-specific way to get position of element
  ///
  /// - Returns traits<Element>::position(el)
  template<typename Base>
  UnitCellCoord ClusterSymCompare<Base>::position(const Element &el) {
    return traits<Element>::position(el);
  }


  // -- LocalSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  AperiodicSymCompare<Element>::
  AperiodicSymCompare(PrimType_ptr prim_ptr, double tol): m_prim(prim_ptr), m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - Returns sorted
  template<typename Element>
  Element AperiodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    return obj.sort();
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Returns sorted
  template<typename Element>
  Element AperiodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {

    return obj;
  }


  // -- PrimPeriodicSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  PrimPeriodicSymCompare<Element>::
  PrimPeriodicSymCompare(PrimType_ptr prim_ptr, double tol): m_prim(prim_ptr), m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - translates so that obj[0] is in the origin unit cell
  template<typename Element>
  Element PrimPeriodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }

    const auto pos = position(obj);
    this->m_spatial_transform = SymOp::translation(-this->m_prim->lattice().lat_column_mat() * pos.unitcell().template cast<double>());
    return obj - pos.unitcell();
  }


  /// \brief Prepare an element for comparison
  ///
  /// - Sorts so that, after translation obj[0] is in the origin unit cell
  template<typename Element>
  Element PrimPeriodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    obj.sort();

    return obj;
  }


  // -- ScelPeriodicSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  ScelPeriodicSymCompare<Element>::
  ScelPeriodicSymCompare(PrimType_ptr prim_ptr, const xtal::IntegralCoordinateWithin_f &bring_within_f, double tol):
    m_bring_within_f(bring_within_f),
    m_prim(prim_ptr),
    m_tol(tol) {}

  /// \brief Prepare an element for comparison
  ///
  /// - translates so that obj[0] is within the supercell
  template<typename Element>
  Element ScelPeriodicSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    const auto pos = position(obj);
    this->m_spatial_transform = SymOp::translation(this->m_prim->lattice().lat_column_mat() * (m_bring_within_f(pos).unitcell() - pos.unitcell()).template cast<double>());
    return obj + (m_bring_within_f(pos).unitcell() - pos.unitcell());
  }


  /// \brief Prepare an element for comparison
  ///
  /// - Sorts UnitCellCoord
  template<typename Element>
  Element ScelPeriodicSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    obj.sort();

    return obj;
  }


  // -- WithinScelSymCompare<IntegralCluster> -------------------------------------

  /// \brief Constructor
  ///
  /// \param tol Tolerance for invariants_compare of site-to-site distances
  ///
  template<typename Element>
  WithinScelSymCompare<Element>::
  WithinScelSymCompare(PrimType_ptr prim_ptr, const xtal::IntegralCoordinateWithin_f &bring_within_f, double tol):
    m_bring_within_f(bring_within_f),
    m_prim(prim_ptr),
    m_tol(tol) {}

  /// \brief Returns transformation that takes 'obj' to its prepared (canonical) form
  ///
  /// - For now returns pointer to SymPermutation object that encodes permutation due to sorting elements
  template<typename Element>
  std::unique_ptr<SymOpRepresentation> WithinScelSymCompare<Element>::
  canonical_transform_impl(Element const &obj)const {
    Element tobj = obj;
    for(Index i = 0; i < tobj.size(); ++i) {
      tobj[i] = m_bring_within_f(tobj[i]);
    }

    return std::unique_ptr<SymOpRepresentation>(new SymPermutation(tobj.sort_permutation()));
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Does nothing, since fully prepared version is just sorted and 'within'-ed version of the cluster
  template<typename Element>
  Element WithinScelSymCompare<Element>::
  spatial_prepare_impl(Element obj) const {
    return obj;
  }

  /// \brief Prepare an element for comparison
  ///
  /// - Puts all sites within the supercell, then sorts
  template<typename Element>
  Element WithinScelSymCompare<Element>::
  representation_prepare_impl(Element obj) const {
    if(!obj.size()) {
      return obj;
    }
    for(Index i = 0; i < obj.size(); ++i) {
      obj[i] = m_bring_within_f(obj[i]);
    }
    obj.sort();
    return obj;
  }
}

#endif
