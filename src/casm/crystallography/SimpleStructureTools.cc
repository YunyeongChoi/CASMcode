#include "casm/crystallography/SimpleStructure.hh"
#include "casm/crystallography/SimpleStructureTools.hh"
#include "casm/crystallography/BasicStructure_impl.hh"
#include "casm/crystallography/Structure.hh"
#include "casm/crystallography/Site.hh"
#include "casm/clex/Configuration.hh"
#include "casm/clex/Supercell.hh"
#include "casm/casm_io/jsonParser.hh"
#include "casm/basis_set/DoFTraits.hh"
#include "casm/casm_io/stream_io/container.hh"

namespace CASM {
  namespace xtal {

    SimpleStructure to_simple_structure(BasicStructure<Site> const &_struc, const std::string &_prefix) {
      SimpleStructure result(_prefix);
      result.lat_column_mat = _struc.lattice().lat_column_mat();
      result.selective_dynamics = _struc.selective_dynamics();

      result.mol_info.SD = Eigen::MatrixXi::Zero(3, _struc.basis().size());

      result.mol_info.coords.resize(3, _struc.basis().size());
      result.mol_info.names.reserve(_struc.basis().size());
      Eigen::VectorXi _mol_occ;
      _mol_occ.resize(_struc.basis().size());
      for(Index b = 0; b < _struc.basis().size(); ++b) {
        result.mol_info.coords.col(b) = _struc.basis(b).const_cart();
        result.mol_info.names.push_back(_struc.basis(b).occ_name());
        result.mol_info.permute.push_back(b);
        _mol_occ[b] = _struc.basis(b).occupant_dof().value();
      }
      _atomize(result, _mol_occ, _struc);
      return result;
    }

    //***************************************************************************

    SimpleStructure to_simple_structure(Supercell const &_scel,
                                        ConfigDoF const &_dof,
                                        const std::string &_prefix,
                                        std::vector<DoFKey> const &_which_dofs) {
      SimpleStructure result(_prefix);
      result.lat_column_mat = _scel.lattice().lat_column_mat();
      result.selective_dynamics = _scel.prim().selective_dynamics();

      if(result.selective_dynamics)
        result.mol_info.SD = Eigen::MatrixXi::Zero(3, _dof.size());

      result.mol_info.coords.resize(3, _dof.size());
      result.mol_info.names.reserve(_dof.size());

      for(Index b = 0, l = 0; b < _dof.n_sublat(); ++b) {
        for(Index v = 0; v < _dof.n_vol(); ++v, ++l) {
          result.mol_info.coords.col(l) = _scel.coord(l).const_cart();
          std::string mol_name = _scel.prim().basis()[ b ].occupant_dof()[_dof.occ(l)].name();
          result.mol_info.names.push_back(std::move(mol_name));
        }
      }

      _apply_dofs(result, _dof, _scel.prim(), _which_dofs);
      return result;
    }

    //***************************************************************************

    SimpleStructure to_simple_structure(Configuration const &_config, const std::string &_prefix, std::vector<DoFKey> const &_which_dofs) {
      return to_simple_structure(_config.supercell(), _config.configdof(), _prefix, _which_dofs);
    }

    //***************************************************************************

    void _atomize(SimpleStructure &_sstruc,
                  Eigen::Ref<const Eigen::VectorXi> const &_mol_occ,
                  BasicStructure<Site> const &_reference) {
      /*if(m_atomized)
        return;
        m_atomized = true;*/
      Index N_atoms(0);

      Index nb = _reference.basis().size();
      Index nv = _sstruc.mol_info.names.size() / nb;
      Index s = 0;
      for(Index b = 0; b < nb; ++b) {
        for(Index v = 0; v < nv; ++v, ++s) {
          N_atoms += _reference.basis(b).occupant_dof()[_mol_occ[s]].size();
        }
      }

      //std::cout << "Atomizing with N_atom = " << N_atoms << "; nv = " << nv << "; nb = " << nb << "\n";
      _sstruc.atom_info.coords.resize(3, N_atoms);
      _sstruc.atom_info.names.resize(N_atoms);

      _sstruc.atom_info.SD = Eigen::MatrixXi::Zero(3, N_atoms);

      // a indexes atom, s indexes site (molecule)
      Index a = 0;
      s = 0;
      for(Index b = 0; b < nb; ++b) {
        for(Index v = 0; v < nv; ++v, ++s) {
          Molecule const &molref = _reference.basis(b).occupant_dof()[_mol_occ[s]];
          //std::cout << "(b,v): (" << b << ", " << v << "); molref.size() = " << molref.size() << "\n";
          for(Index ms = 0; ms < molref.size(); ++ms, ++a) {
            _sstruc.atom_info.coords.col(a) = _sstruc.mol_info.coords.col(s) + molref.atom(ms).cart();
            _sstruc.atom_info.names[a] = molref.atom(ms).name();
            _sstruc.atom_info.permute.push_back(a);
            if(_sstruc.selective_dynamics) {
              _sstruc.atom_info.SD.col(a) = _sstruc.mol_info.SD.col(s);
              for(Index i = 0; i < 3; ++i) {
                if(molref.atom(i).sd_flag()[i])
                  _sstruc.atom_info.SD(i, a) = 1;
              }
            }
          }
        }
      }
    }

    //***************************************************************************

    std::vector<std::set<Index> > mol_site_compatibility(SimpleStructure const &sstruc,
                                                         BasicStructure<Site> const &_prim) {
      std::vector<std::set<Index> > result;
      result.reserve(sstruc.mol_info.names.size());
      for(std::string const &sp : sstruc.mol_info.names) {
        result.push_back({});
        for(Index b = 0; b < _prim.basis().size(); ++b) {
          if(_prim.basis()[b].contains(sp)) {
            result.back().insert(b);
          }
        }
      }
      return result;
    }

    //***************************************************************************

    std::vector<std::set<Index> > mol_site_compatibility(SimpleStructure const &sstruc,
                                                         Configuration const &_config) {
      std::vector<std::set<Index> > result;
      result.reserve(sstruc.mol_info.names.size());
      for(std::string const &sp : sstruc.mol_info.names) {
        result.push_back({});
        for(Index l = 0; l < _config.size(); ++l) {
          if(_config.mol(l).name() == sp) {
            result.back().insert(l);
          }
        }
      }
      return result;
    }

    //***************************************************************************

    std::vector<std::set<Index> > atom_site_compatibility(SimpleStructure const &sstruc,
                                                          BasicStructure<Site> const &_prim) {

      std::vector<std::set<Index> > result;
      result.reserve(sstruc.atom_info.names.size());
      for(std::string const &sp : sstruc.atom_info.names) {
        result.push_back({});
        for(Index b = 0; b < _prim.basis().size(); ++b) {
          for(Molecule const &mol : _prim.basis(b).occupant_dof().domain()) {
            if(mol.contains(sp)) {
              result.back().insert(b);
              break;
            }
          }
        }
      }
      return result;
    }

    //***************************************************************************

    std::vector<std::set<Index> > atom_site_compatibility(SimpleStructure const &sstruc,
                                                          Configuration const &_config) {
      std::vector<std::set<Index> > result;
      result.reserve(sstruc.atom_info.names.size());
      for(std::string const &sp : sstruc.atom_info.names) {
        result.push_back({});
        for(Index l = 0; l < _config.size(); ++l) {
          if(_config.mol(l).contains(sp)) {
            result.back().insert(l);
          }
        }
      }
      return result;
    }

    //***************************************************************************

    void _apply_dofs(SimpleStructure &_sstruc, ConfigDoF const &_config, BasicStructure<Site> const &_reference, std::vector<DoFKey> which_dofs) {
      std::set<TransformDirective> tformers({TransformDirective("atomize")});
      if(which_dofs.empty()) {
        for(std::string const &dof : continuous_local_dof_types(_reference))
          which_dofs.push_back(dof);
        for(std::string const &dof : global_dof_types(_reference))
          which_dofs.push_back(dof);
      }

      for(DoFKey const &dof : which_dofs) {
        if(dof != "none" && dof != "occ")
          tformers.insert(dof);
      }

      //std::cout << "About to transform!!!\n";
      for(TransformDirective const &tformer : tformers) {
        tformer.transform(_config, _reference, _sstruc);
      }
    }

    //***************************************************************************
    /*
    void _dofs_to_json(jsonParser &json, ConfigDoF const &_config, BasicStructure<Site> const &_reference, std::vector<DoFKey> which_dofs) {
      std::set<TransformDirective> tformers({TransformDirective("atomize")});
      if(which_dofs.empty()){
        for(std::string const &dof : continuous_local_dof_types(_reference))
          which_dofs.push_back(dof);
        for(std::string const &dof : global_dof_types(_reference))
          which_dofs.push_back(dof);
      }

      for(DoFKey const & dof : which_dofs){
        if(dof != "none" && dof != "occ")
          tformers.insert(dof);
      }

      //std::cout << "About to transform!!!\n";
      for(TransformDirective const &tformer : tformers) {
        tformer.dofs_to_json(json, _config, _reference);
      }
    }
    */

    //***************************************************************************
    TransformDirective::TransformDirective(std::string const &_name) :
      m_name(_name),
      m_traits_ptr(nullptr) {
      if(name() != "atomize") {
        m_traits_ptr = &DoFType::traits(name());
        _accumulate_before({_name}, m_before);
        _accumulate_after({_name}, m_after);
        if(m_after.count("atomize") == 0)
          m_before.insert("atomize");

      }
    }

    //***************************************************************************
    bool TransformDirective::operator<(TransformDirective const &_other) const {
      if(m_before.count(_other.name()) || _other.m_after.count(name()))
        return false;
      if(m_after.count(_other.name()) || _other.m_before.count(name()))
        return true;
      return name() < _other.name();
    }

    //***************************************************************************

    void TransformDirective::_accumulate_before(std::set<std::string>const &_queue, std::set<std::string> &_result) const {
      for(std::string const &el : _queue) {
        if(el != name())
          _result.insert(el);
        if(el != "atomize")
          _accumulate_before(DoF::traits(el).before_dof_apply(), _result);
      }
    }

    //***************************************************************************

    void TransformDirective::_accumulate_after(std::set<std::string>const &_queue, std::set<std::string> &_result) const {
      for(std::string const &el : _queue) {
        if(el != name())
          _result.insert(el);
        if(el != "atomize")
          _accumulate_after(DoF::traits(el).after_dof_apply(), _result);
      }
    }

    //***************************************************************************

    void TransformDirective::transform(ConfigDoF const  &_dof, BasicStructure<Site> const &_reference, SimpleStructure &_struc) const {
      //std::cout << "Applying transformation: " << m_name << "\n";
      if(m_traits_ptr) {
        if(m_traits_ptr->global())
          _struc.dofs[m_traits_ptr->type_name()] = _dof.global_dof(m_traits_ptr->type_name()).standard_values();
        else
          _struc.mol_info.dofs[m_traits_ptr->type_name()] = _dof.local_dof(m_traits_ptr->type_name()).standard_values();

        m_traits_ptr->apply_dof(_dof, _reference, _struc);
      }
      else {
        _atomize(_struc, _dof.occupation(), _reference);
      }
    }

    //***************************************************************************
    /*
    void TransformDirective::dofs_to_json(ConfigDoF const  &_config, BasicStructure<Site> const &_reference){
      //std::cout << "Applying transformation: " << m_name << "\n";
      if(m_traits_ptr)
        m_traits_ptr->dof_to_json(_json, _config, _reference);
        }*/
  }
  //***************************************************************************
  /// \brief Construct from Configuration and specify prefix for output quantities
  jsonParser json_supplement(Configuration const &_config,
                             std::string const &_prefix,
                             std::vector<DoFKey> const &_which_dofs) {


    return json_supplement(_config.configdof(), _config.prim(), _prefix, _which_dofs);
  }

  //***************************************************************************
  /// \brief Construct from Configuration and specify prefix for output quantities
  jsonParser json_supplement(ConfigDoF const &_dof,
                             BasicStructure<Site> const &_reference,
                             std::string const &_prefix,
                             std::vector<DoFKey> which_dofs) {
    jsonParser result;
    if(which_dofs.empty()) {
      for(std::string const &dof : continuous_local_dof_types(_reference))
        which_dofs.push_back(dof);
      for(std::string const &dof : global_dof_types(_reference))
        which_dofs.push_back(dof);
    }

    for(DoFKey const &dof : which_dofs) {
      if(dof != "none" && dof != "occ") {
        auto traits_ptr = &DoFType::traits(dof);
        if(traits_ptr->global())
          result[_prefix + "global_dofs"][dof] = traits_ptr->dof_to_json(_dof, _reference);
        else
          result[_prefix + "mol_dofs"][dof] = traits_ptr->dof_to_json(_dof, _reference);

      }
    }
    return result;
  }

  //***************************************************************************

  jsonParser &to_json(SimpleStructure const &_struc,
                      jsonParser &supplement,
                      std::set<std::string> const &excluded_species) {

    std::string prefix = _struc.prefix();
    if(!prefix.empty())
      prefix.push_back('_');


    std::vector<Index> atom_permute, mol_permute;
    jsonParser &ajson = supplement["atom_type"].put_array();

    for(Index i = 0; i < _struc.atom_info.names.size(); ++i) {
      if(excluded_species.count(_struc.atom_info.names[i]))
        continue;
      ajson.push_back(_struc.atom_info.names[i]);
      atom_permute.push_back(i);
    }

    jsonParser &mjson = supplement["mol_type"].put_array();
    for(Index i = 0; i < _struc.mol_info.names.size(); ++i) {
      if(excluded_species.count(_struc.mol_info.names[i]))
        continue;
      mjson.push_back(_struc.mol_info.names[i]);
      mol_permute.push_back(i);
    }

    supplement[prefix + "lattice"] = _struc.lat_column_mat.transpose();

    for(auto const &dof : _struc.dofs) {
      to_json_array(dof.second, supplement[prefix + "global_dofs"][dof.first]["value"]);
    }

    for(auto const &dof : _struc.atom_info.dofs) {
      jsonParser &tjson = supplement[prefix + "atom_dofs"][dof.first]["value"].put_array();
      for(Index i : atom_permute)
        tjson.push_back(dof.second.col(i), jsonParser::as_array());
    }

    for(auto const &dof : _struc.mol_info.dofs) {
      jsonParser &tjson = supplement[prefix + "mol_dofs"][dof.first]["value"].put_array();
      for(Index i : mol_permute)
        tjson.push_back(dof.second.col(i), jsonParser::as_array());
    }

    if(_struc.selective_dynamics) {
      supplement["selective_dynamics"] = _struc.selective_dynamics;
      supplement["atom_selective_dynamics"].put_array();
      supplement["mol_selective_dynamics"].put_array();
    }


    {
      jsonParser &tjson = supplement[prefix + "atom_coords"].put_array();
      for(Index i : atom_permute) {
        tjson.push_back(_struc.atom_info.coords.col(i), jsonParser::as_array());
        if(_struc.selective_dynamics)
          supplement["atom_selective_dynamics"].push_back(_struc.atom_info.SD.col(i), jsonParser::as_array());
      }
    }

    {
      jsonParser &tjson = supplement[prefix + "mol_coords"].put_array();
      for(Index i : mol_permute) {
        tjson.push_back(_struc.mol_info.coords.col(i), jsonParser::as_array());
        if(_struc.selective_dynamics)
          supplement["mol_selective_dynamics"].push_back(_struc.mol_info.SD.col(i), jsonParser::as_array());
      }
    }
    return supplement;
  }

  //***************************************************************************

  void from_json(SimpleStructure &_struc, const jsonParser &json) {
    std::string prefix = _struc.prefix();
    if(!prefix.empty())
      prefix.push_back('_');

    try {
      std::string tstr;
      CASM::from_json(tstr, json["coord_mode"]);

      COORD_TYPE mode = CART;
      if(tstr == "direct" || tstr == "Direct")
        mode = FRAC;

      _struc.lat_column_mat = json[prefix + "lattice"].get<Eigen::Matrix3d>().transpose();
      {
        auto it = json.find(prefix + "global_dofs");
        if(it != json.end()) {
          for(auto it2 = it->begin(); it2 != it->end(); ++it2) {
            _struc.dofs[it2.name()] = (*it2)["value"].get<Eigen::MatrixXd>();
          }
        }
      }


      if(json.contains("atoms_per_type")) {
        std::vector<Index> ntype = json["atoms_per_type"].get<std::vector<Index> >();
        std::vector<std::string> type = json["atoms_type"].get<std::vector<std::string> >();

        for(Index i = 0; i < ntype.size(); ++i) {
          for(Index j = 0; j < ntype[i]; ++j) {
            _struc.atom_info.names.push_back(type[i]);
          }
        }

        if(mode == FRAC)
          _struc.atom_info.coords = _struc.lat_column_mat * json[prefix + "atom_coords"].get<Eigen::MatrixXd>().transpose();
        else
          _struc.atom_info.coords = json[prefix + "atom_coords"].get<Eigen::MatrixXd>().transpose();
      }

      if(json.contains("mols_per_type")) {
        std::vector<Index> ntype = json["mols_per_type"].get<std::vector<Index> >();
        std::vector<std::string> type = json["mols_type"].get<std::vector<std::string> >();

        for(Index i = 0; i < ntype.size(); ++i) {
          for(Index j = 0; j < ntype[i]; ++j) {
            _struc.mol_info.names.push_back(type[i]);
          }
        }

        if(mode == FRAC)
          _struc.mol_info.coords = _struc.lat_column_mat * json[prefix + "mol_coords"].get<Eigen::MatrixXd>().transpose();
        else
          _struc.mol_info.coords = _struc.lat_column_mat * json[prefix + "mol_coords"].get<Eigen::MatrixXd>().transpose();
      }

      {
        auto it = json.find(prefix + "atom_dofs");
        if(it != json.end()) {
          for(auto it2 = it->begin(); it2 != it->end(); ++it2) {
            _struc.atom_info.dofs[it2.name()] = (*it2)["value"].get<Eigen::MatrixXd>().transpose();
          }
        }
      }

      {
        auto it = json.find(prefix + "mol_dofs");
        if(it != json.end()) {
          for(auto it2 = it->begin(); it2 != it->end(); ++it2) {
            _struc.mol_info.dofs[it2.name()] = (*it2)["value"].get<Eigen::MatrixXd>().transpose();
          }
        }
      }

      json.get_if(_struc.selective_dynamics, "selective_dynamics");
      if(_struc.selective_dynamics) {
        if(json.contains("atom_selective_dynamics")) {
          _struc.atom_info.SD = json["atom_selective_dynamics"].get<Eigen::MatrixXi>().transpose();
        }
        if(json.contains("mol_selective_dynamics")) {
          _struc.mol_info.SD = json["mol_selective_dynamics"].get<Eigen::MatrixXi>().transpose();
        }
      }

    }
    catch(const std::exception &ex) {
      throw std::runtime_error(std::string("Unable to parse Structure from JSON object.  One or more tags were improperly specified:\n") + ex.what());
    }
  }

}