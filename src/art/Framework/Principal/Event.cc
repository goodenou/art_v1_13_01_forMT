// ======================================================================
//
// Event - This is the primary interface for accessing EDProducts from a
//         single collision and inserting new derived products.
//
// ======================================================================

#include "art/Framework/Principal/Event.h"

#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Persistency/Provenance/BranchType.h"
#include "art/Persistency/Provenance/ProcessHistoryRegistry.h"
#include "art/Framework/Principal/Provenance.h"
#include "fhiclcpp/ParameterSetRegistry.h"

using namespace std;
using namespace fhicl;

namespace art {

  namespace {
    SubRun * newSubRun(EventPrincipal& ep, ModuleDescription const& md) {
      return (ep.subRunPrincipalSharedPtr() ? new SubRun(ep.subRunPrincipal(), md) : 0);
    }
  }

  Event::Event(EventPrincipal& ep, ModuleDescription const& md)
  : DataViewImpl ( ep, md, InEvent )
  , aux_         ( ep.aux() )
  , subRun_      ( newSubRun(ep, md) )
  , gotBranchIDs_( )
  { }

  EDProductGetter const *
  Event::productGetter(ProductID const & pid) const {
    return eventPrincipal().productGetter(pid);
  }

  ProductID
  Event::branchIDToProductID(BranchID const &bid) const {
    return eventPrincipal().branchIDToProductID(bid);
  }

  EventPrincipal &
  Event::eventPrincipal() {
    return dynamic_cast<EventPrincipal &>(principal());
  }

  EventPrincipal const &
  Event::eventPrincipal() const {
    return dynamic_cast<EventPrincipal const&>(principal());
  }

  ProductID
  Event::makeProductID(BranchDescription const& desc) const {
    return eventPrincipal().branchIDToProductID(desc.branchID());
  }

  SubRun const&
  Event::getSubRun() const {
    if (!subRun_) {
      throw Exception(errors::NullPointerError)
        << "Tried to obtain a NULL subRun.\n";
    }
    return *subRun_;
  }

  Run const&
  Event::getRun() const {
    return getSubRun().getRun();
  }

//   History const&
//   Event::history() const {
//     DataViewImpl const& dvi = me();
//     EDProductGetter const* pg = dvi.prodGetter(); // certain to be non-null
//     assert(pg);
//     EventPrincipal const& ep = dynamic_cast<EventPrincipal const&>(*pg);
//     return ep.history();
//   }
  History const&
  Event::history() const {
    return eventPrincipal().history();
  }

  ProcessHistoryID const&
  Event::processHistoryID() const {
    return eventPrincipal().history().processHistoryID();
  }


  bool
  Event::getProcessParameterSet(string const& processName,
                                ParameterSet& ps) const
  {
    // Get the ProcessHistory for this event.
    ProcessHistory ph;
    if (!ProcessHistoryRegistry::get(processHistoryID(), ph))
      {
        throw Exception(errors::NotFound)
          << "ProcessHistoryID " << processHistoryID()
          << " is claimed to describe " << id()
          << "\nbut is not found in the ProcessHistoryRegistry.\n"
             "This file is malformed.\n";
      }

    ProcessConfiguration config;
    bool process_found = ph.getConfigurationForProcess(processName, config);
    if (process_found)
      ParameterSetRegistry::get(config.parameterSetID(), ps);
    return process_found;
  }

  GroupQueryResult
  Event::getByProductID_(ProductID const& oid) const
  {
    return eventPrincipal().getByProductID(oid);
  }


  void
  Event::commit_() {
    commit_aux(putProducts(), true);
    commit_aux(putProductsWithoutParents(), false);
  }

  void
  Event::commit_aux(Base::ProductPtrVec& products, bool record_parents) {
    // fill in guts of provenance here
    EventPrincipal & ep = eventPrincipal();

    ProductPtrVec::iterator pit(products.begin());
    ProductPtrVec::iterator pie(products.end());

    vector<BranchID> gotBranchIDVector;

    // Note that gotBranchIDVector will remain empty if
    // record_parents is false (and may be empty if record_parents is
    // true).

    if (record_parents && !gotBranchIDs_.empty()) {
      gotBranchIDVector.reserve(gotBranchIDs_.size());
      for (BranchIDSet::const_iterator it = gotBranchIDs_.begin(), itEnd = gotBranchIDs_.end();
          it != itEnd; ++it) {
        gotBranchIDVector.push_back(*it);
      }
    }

    while(pit!=pie) {
        unique_ptr<EDProduct> pr(pit->first);
        // note: ownership has been passed - so clear the pointer!
        pit->first = 0;

        // set provenance
        unique_ptr<ProductProvenance const> productProvenancePtr(
                new ProductProvenance(pit->second->branchID(),
                                      productstatus::present(),
                                      gotBranchIDVector));
        ep.put(std::move(pr),
               *pit->second,
               std::move(productProvenancePtr));
        ++pit;
    }

    // the cleanup is all or none
    products.clear();
  }

  void
  Event::addToGotBranchIDs(Provenance const& prov) const {
    if (prov.branchDescription().transient()) {
      // If the product retrieved is transient, don't use its branch ID.
      // use the branch ID's of its parents.
      vector<BranchID> const& bids = prov.parents();
      for (vector<BranchID>::const_iterator it = bids.begin(), itEnd = bids.end(); it != itEnd; ++it) {
        gotBranchIDs_.insert(*it);
      }
    } else {
      gotBranchIDs_.insert(prov.branchID());
    }
  }

// ----------------------------------------------------------------------

  void
    Event::ensure_unique_product( std::size_t         nFound
                                , TypeID      const & typeID
                                , std::string const & moduleLabel
                                , std::string const & productInstanceName
                                , std::string const & processName
                                ) const
  {
    if( nFound == 1 )
      return;

    art::Exception e(art::errors::ProductNotFound);
    e << "getView: Found "
      << (nFound == 0 ? "no products"
                      : "more than one product"
         )
      << " matching all criteria\n"
      << "Looking for sequence of type: " << typeID << "\n"
      << "Looking for module label: " << moduleLabel << "\n"
      << "Looking for productInstanceName: " << productInstanceName << "\n";
    if( ! processName.empty() )
      e << "Looking for processName: "<< processName <<"\n";
    throw e;
  }  // ensure_unique_product()

}  // art

// ======================================================================
