#ifndef art_Framework_Core_ProductRegistryHelper_h
#define art_Framework_Core_ProductRegistryHelper_h

// -----------------------------------------------------------------
//
// ProductRegistryHelper: This class provides the produces<> templates
// and the reconstitutes<> templates used by other classes to register
// what types of products are put into the Event/SubRun/Run principals
// by the actions of the classes which invoke those templates.
//
// EDProducers' and EDFilters' constructors should call one of the
// 'produces' functions for each product put into the event, subrun,
// or run. Instance names should be provided only when the producer or
// filter in question makes more than one instance of the same type
// per event.
//
// InputSources' constructors should call 'reconstitutes' for each
// product read from an external source if and only if that source
// does not already carry the metadata that art data files carry.
//
// -----------------------------------------------------------------

#include "art/Framework/Principal/fwd.h"
#include "art/Persistency/Common/Assns.h"
#include "art/Persistency/Provenance/BranchType.h"
#include "art/Persistency/Provenance/ProductList.h"
#include "art/Persistency/Provenance/TypeLabel.h"
#include "art/Utilities/Exception.h"
#include "art/Utilities/TypeID.h"
#include "cetlib/exception.h"

#include <memory>
#include <set>
#include <string>

namespace
{
  inline
  void verifyInstanceName(std::string const& instanceName)
  {
    if (instanceName.find('_') != std::string::npos)
      {
        throw art::Exception(art::errors::Configuration)
          << "Instance name \""
          << instanceName
          << "\" is illegal: underscores are not permitted in instance names."
          << '\n';
      }
  }

  inline
  void verifyFriendlyClassName(std::string const& fcn)
  {
    if (fcn.find('_') != std::string::npos)
      {
        throw art::Exception(art::errors::LogicError)
          << "Class \""
          << fcn
          << "\" is not suitable for use as a product due to the presence of "
          << "underscores which are not allowed anywhere in the class name "
          << "(including namespace and enclosing classes).\n";
      }
  }
}


namespace art
{
  class EDProduct;
  class ModuleDescription;
  class MasterProductRegistry;
  class ProductRegistryHelper;
}

class art::ProductRegistryHelper
{
public:

  // Used by an input source to provide a product list
  // to be merged into the master product registry
  // later by registerProducts().
  void productList(ProductList* p) { productList_.reset(p); }

  // Record the production of an object of type P, with optional
  // instance name, in the Event (by default), Run, or SubRun.
  template <class P, BranchType B = InEvent>
  void produces(std::string const& instanceName=std::string());

  // Record the reconstitution of an object of type P, in either the
  // Run, SubRun, or Event, recording that this object was
  // originally created by a module with label modLabel, and with an
  // optional instance name.
  template <class P, BranchType B>
  art::TypeLabel const &
  reconstitutes(std::string const& modLabel,
                std::string const& instanceName=std::string());

  void registerProducts(MasterProductRegistry &,
                        ModuleDescription const&);

private:
  typedef std::set<art::TypeLabel> TypeLabelList;

  template <BranchType B>
  void
  produces_in_branch(TypeID const& productType,
                     std::string const& instanceName=std::string());

  template <BranchType B>
  art::TypeLabel const &
  reconstitutes_to_branch(TypeID const& productType,
                          std::string const& moduleLabel,
                          std::string const& instanceName=std::string());

  art::TypeLabel const & insertOrThrow(TypeLabel const &tl);

  TypeLabelList typeLabelList_;

  // Set by an input source for merging into the
  // master product registry by registerProducts().
  std::unique_ptr<ProductList> productList_;

};

template <typename P, art::BranchType B>
inline
void
art::ProductRegistryHelper::produces(std::string const& instanceName)
{
  verifyInstanceName(instanceName);
  TypeID productType(typeid(P));
  verifyFriendlyClassName(productType.friendlyClassName());
  produces_in_branch<B>(productType, instanceName);
}

template <typename P, art::BranchType B>
art::TypeLabel const &
art::ProductRegistryHelper::reconstitutes(std::string const& emulatedModule,
                                     std::string const& instanceName)
{
  verifyInstanceName(instanceName);
  TypeID productType(typeid(P));
  verifyFriendlyClassName(productType.friendlyClassName());
  return reconstitutes_to_branch<B>(productType,
                             emulatedModule,
                             instanceName);
}

template <art::BranchType B>
void
art::ProductRegistryHelper::produces_in_branch(TypeID const& productType,
                                          std::string const& instanceName)
{
  insertOrThrow(TypeLabel(B, productType, instanceName));
}

template <art::BranchType B>
art::TypeLabel const &
art::ProductRegistryHelper::reconstitutes_to_branch(TypeID const& productType,
                                               std::string const& emulatedModule,
                                               std::string const& instanceName)
{
  if (emulatedModule.empty())
    throw Exception(errors::Configuration)
      << "Input sources must call reconstitutes with a non-empty "
      << "module label.\n";
  return insertOrThrow(TypeLabel(B, productType, instanceName, emulatedModule));
}

#endif /* art_Framework_Core_ProductRegistryHelper_h */

// Local Variables:
// mode: c++
// End:
