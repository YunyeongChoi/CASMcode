#include "casm/kinetics/DiffusionTransformation.hh"
#include "casm/clex/Configuration.hh"
#include "casm/clex/Supercell.hh"
#include "casm/symmetry/Orbit_impl.hh"
#include "casm/kinetics/DiffTransConfiguration.hh"

namespace CASM {

  namespace Kinetics {


    DiffTransConfiguration::DiffTransConfiguration(const Configuration &_from_config,
                                                   const DiffusionTransformation &_diff_trans) :
      m_diff_trans(_diff_trans), m_from_config(_from_config) {
      ScelPeriodicDiffTransSymCompare symcompare(m_from_config.supercell().prim_grid(),
                                                 m_from_config.supercell().crystallography_tol());
      m_diff_trans = symcompare.prepare(m_diff_trans);
    }

    /// \brief sort DiffTransConfiguration in place
    DiffTransConfiguration &DiffTransConfiguration::sort() {
      Configuration to = to_config();
      if(to < m_from_config) {
        m_from_config = to;
        m_diff_trans.reverse();
      }
      return *this;
    }

    /// \brief Returns a sorted version of this DiffTransConfiguration
    DiffTransConfiguration DiffTransConfiguration::sorted() const {
      DiffTransConfiguration tmp {*this};
      return tmp.sort();
    }

    bool DiffTransConfiguration::is_sorted() const {
      Configuration to = to_config();
      return m_from_config < to;
    }

    PermuteIterator DiffTransConfiguration::to_canonical() const {
      // check which supercell factor group operations
      // when applied to m_diff_trans results in the greatest
      // DiffusionTransformation

      std::vector<PermuteIterator> checklist;
      ScelPeriodicDiffTransSymCompare symcompare(m_from_config.supercell().prim_grid(),
                                                 m_from_config.supercell().crystallography_tol());
      DiffusionTransformation greatest = symcompare.prepare(m_diff_trans);
      for(auto it = m_from_config.supercell().permute_begin();
          it != m_from_config.supercell().permute_end(); ++it) {
        DiffusionTransformation tmp = symcompare.prepare(copy_apply(it.sym_op(), m_diff_trans));

        if(tmp == greatest) {
          checklist.push_back(it);
        }
        else if(tmp > greatest) {
          checklist.clear();
          greatest = tmp;
          checklist.push_back(it);
        }
      }

      // of these operations check which one maximizes
      // the result of applying to m_from_config
      auto it = checklist.begin();
      DiffTransConfiguration max_dtc(copy_apply(*it, sorted().from_config()), greatest);
      max_dtc.sort();
      std::cout << "max_dtc sorted?" << max_dtc.is_sorted() << std::endl;

      PermuteIterator canon_op_it {*it};
      ++it;
      for(; it != checklist.end(); ++it) {
        Configuration tmp = copy_apply(*it, sorted().from_config());

        //it->sym_op().print(std::cout);
        DiffTransConfiguration dtc_tmp(tmp, greatest);
        //std::cout << dtc_tmp.sorted().from_config();
        //std::cout << max_dtc.sorted().from_config();
        //std::cout << (dtc_tmp.sorted().from_config() == max_dtc.sorted().from_config()) << "configs eq?" << std::endl;
        //std::cout << (dtc_tmp.sorted().from_config() < max_dtc.sorted().from_config()) << "config lt max?" << std::endl;
        if(dtc_tmp > max_dtc) {
          std::cout << "max_dtc changes " << checklist.size() << std::endl;
          max_dtc = dtc_tmp.sorted();
          canon_op_it = *it;
        }
      }
      canon_op_it.sym_op().print(std::cout);
      // return the operation that transforms this to canonical form
      return canon_op_it;
    }

    DiffTransConfiguration DiffTransConfiguration::canonical_form() const {
      return copy_apply(this->to_canonical(), *this).sorted();
    }

    bool DiffTransConfiguration::is_canonical() const {
      return std::all_of(m_from_config.supercell().permute_begin(),
                         m_from_config.supercell().permute_end(),
      [&](const PermuteIterator & p) {
        return copy_apply(p, *this) <= *this;
      });
    }

    DiffTransConfiguration &DiffTransConfiguration::apply_sym(const PermuteIterator &it) {
      m_from_config = apply(it, m_from_config);

      ScelPeriodicDiffTransSymCompare symcompare(m_from_config.supercell().prim_grid(),
                                                 m_from_config.supercell().crystallography_tol());
      m_diff_trans.apply_sym(it.sym_op());

      m_diff_trans = symcompare.prepare(m_diff_trans);

      return *this;
    }

    /// \brief prints this DiffTransConfiguration
    std::ostream &operator<<(std::ostream &sout, const DiffTransConfiguration &dtc) {
      sout << dtc.diff_trans();
      sout << dtc.from_config();
      return sout;
    }

  }
}