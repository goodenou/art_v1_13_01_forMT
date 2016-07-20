#ifndef art_Framework_IO_Root_RootTree_h
#define art_Framework_IO_Root_RootTree_h

// ======================================================================
//
// RootTree - used by ROOT input sources
//
// ======================================================================

#include "TBranch.h"
#include "TTree.h"
#include "art/Framework/Core/Frameworkfwd.h"
#include "art/Framework/IO/Root/BranchMapperWithReader.h"
#include "art/Framework/IO/Root/Inputfwd.h"
#include "art/Persistency/Provenance/BranchKey.h"
#include "art/Persistency/Provenance/BranchType.h"
#include "art/Persistency/Provenance/ProductProvenance.h"
#include "art/Persistency/Provenance/ProvenanceFwd.h"
#include "cpp0x/memory"
#include <string>
#include <vector>

// ----------------------------------------------------------------------

class TFile;

namespace art {
  class DelayedReader;

  class RootTree {
  public:
    RootTree(RootTree const&) = delete;
    RootTree& operator=(RootTree const&) = delete;

    typedef input::BranchMap BranchMap;
    typedef input::EntryNumber EntryNumber;
    RootTree(std::shared_ptr<TFile> filePtr,
             BranchType const& branchType,
             int64_t saveMemoryObjectThreshold);

    // use compiler-generated copy c'tor, copy assignment, and d'tor

    bool isValid() const;
    bool hasBranch(std::string const & branchName) const;
    void addBranch(BranchKey const& key,
                   BranchDescription const& prod,
                   std::string const& branchName);
    void dropBranch(std::string const& branchName);
    bool next() {return ++entryNumber_ < entries_;}
    bool previous() {return --entryNumber_ >= 0;}
    bool current() {return entryNumber_ < entries_ && entryNumber_ >= 0;}
    void rewind() {entryNumber_ = 0;}
    EntryNumber const& entryNumber() const {return entryNumber_;}
    EntryNumber const& entries() const {return entries_;}
    void setEntryNumber(EntryNumber theEntryNumber);
    std::vector<std::string> const& branchNames() const {return branchNames_;}
    template <typename T>
    void fillGroups(T& item);
    std::unique_ptr<DelayedReader> makeDelayedReader() const;
    std::unique_ptr<BranchMapper> makeBranchMapper() const;
    //TBranch *auxBranch() {return auxBranch_;}
    template <typename T>
    void fillAux(T *& pAux) const {
      auxBranch_->SetAddress(&pAux);
      input::getEntry(auxBranch_, entryNumber_);
    }
    TTree const* tree() const {return tree_;}
    TTree const* metaTree() const {return metaTree_;}
    void setCacheSize(unsigned int cacheSize) const;
    void setTreeMaxVirtualSize(int treeMaxVirtualSize);
    BranchMap const& branches() const {return *branches_;}

    TBranch *productProvenanceBranch() const {return productProvenanceBranch_;}

  private:
    std::shared_ptr<TFile> filePtr_;
// We use bare pointers for pointers to some ROOT entities.
// Root owns them and uses bare pointers internally.
// Therefore,using smart pointers here will do no good.
    TTree *const tree_;
    TTree *const metaTree_;
    BranchType branchType_;
    int64_t const saveMemoryObjectThreshold_;
    TBranch *const auxBranch_;
    TBranch *const productProvenanceBranch_;
    EntryNumber entries_;
    EntryNumber entryNumber_;
    std::vector<std::string> branchNames_;
    std::shared_ptr<BranchMap> branches_;
  };  // RootTree

  template <typename T>
  void
  RootTree::fillGroups(T& item) {
    if (metaTree_ == 0 || metaTree_->GetNbranches() == 0) return;
    // Loop over provenance
    for (BranchMap::const_iterator pit = branches_->begin(), pitEnd = branches_->end(); pit != pitEnd; ++pit) {
      item.addGroup(pit->second.branchDescription_);
    }
  }

}  // art

// ======================================================================

#endif /* art_Framework_IO_Root_RootTree_h */

// Local Variables:
// mode: c++
// End:
