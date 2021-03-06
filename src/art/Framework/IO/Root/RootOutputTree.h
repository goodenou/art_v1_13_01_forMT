#ifndef art_Framework_IO_Root_RootOutputTree_h
#define art_Framework_IO_Root_RootOutputTree_h

/*----------------------------------------------------------------------

RootOutputTree.h // used by ROOT output modules

----------------------------------------------------------------------*/

#include "art/Framework/Core/Frameworkfwd.h"
#include "art/Persistency/Provenance/BranchType.h"
#include "art/Persistency/Provenance/ProductProvenance.h"
#include "cpp0x/memory"

#include <set>
#include <string>
#include <vector>

#include "TTree.h"

class TFile;
class TBranch;

namespace art {

  class RootOutputTree {
  public:
    RootOutputTree(RootOutputTree const&) = delete;
    RootOutputTree& operator=(RootOutputTree const&) = delete;

    // Constructor for trees with no fast cloning
    template <typename T>
    RootOutputTree(T* , // first argument is a dummy so that the compiler can resolve the match.
                   std::shared_ptr<TFile> filePtr,
                   BranchType const& branchType,
                   typename T::Auxiliary const*& pAux,
                   ProductProvenances *& pProductProvenanceVector,
                   int bufSize,
                   int splitLevel,
                   int64_t treeMaxVirtualSize,
                   int64_t saveMemoryObjectThreshold
                  ) :
      filePtr_(filePtr),
      tree_(makeTTree(filePtr.get(), BranchTypeToProductTreeName(branchType), splitLevel)),
      metaTree_(makeTTree(filePtr.get(), BranchTypeToMetaDataTreeName(branchType), 0)),
      auxBranch_(0),
      producedBranches_(),
      metaBranches_(),
      readBranches_(),
      unclonedReadBranches_(),
      unclonedReadBranchNames_(),
      currentlyFastCloning_(),
      basketSize_(bufSize),
      splitLevel_(splitLevel),
      saveMemoryObjectThreshold_(saveMemoryObjectThreshold) {

      if (treeMaxVirtualSize >= 0) tree_->SetMaxVirtualSize(treeMaxVirtualSize);
      auxBranch_ = tree_->Branch(BranchTypeToAuxiliaryBranchName(branchType).c_str(), &pAux, bufSize, 0);
      readBranches_.push_back(auxBranch_);

      productProvenanceBranch_ = metaTree_->Branch(productProvenanceBranchName(branchType).c_str(),
                                                 &pProductProvenanceVector, bufSize, 0);
      metaBranches_.push_back(productProvenanceBranch_);
  }

    // use compiler-generated copy c'tor, copy assignment, and d'tor

    static void fastCloneTTree(TTree *in, TTree *out);

    static TTree * makeTTree(TFile *filePtr, std::string const& name, int splitLevel);

    static TTree * assignTTree(TFile *file, TTree * tree);

    static void writeTTree(TTree *tree);

    bool isValid() const;

    void addBranch(BranchDescription const& prod,
                   void const*& pProd);

    bool checkSplitLevelAndBasketSize(TTree *inputTree) const;

    void fastCloneTree(TTree *tree);

    void fillTree() const;

    void writeTree() const;

    TTree * tree() const {
      return tree_;
    }

    TTree * metaTree() const {
      return metaTree_;
    }

    void setEntries() {
      if (tree_->GetNbranches() != 0) tree_->SetEntries(-1);
      if (metaTree_->GetNbranches() != 0) metaTree_->SetEntries(-1);
    }

    void beginInputFile(bool fastCloning) {
      currentlyFastCloning_ = fastCloning;
    }

    bool
    uncloned(std::string const& branchName) const {
        return unclonedReadBranchNames_.find(branchName) != unclonedReadBranchNames_.end();
    }

  private:
    void fillTTree(TTree *tree,
                   std::vector<TBranch *> const& branches,
                   bool saveMemory = false) const;
// We use bare pointers for pointers to some ROOT entities.
// Root owns them and uses bare pointers internally.
// Therefore,using smart pointers here will do no good.
    std::shared_ptr<TFile> filePtr_;
    TTree *const tree_;
    TTree *const metaTree_;
    TBranch * auxBranch_;
    TBranch * productProvenanceBranch_;
    std::vector<TBranch *> producedBranches_; // does not include cloned branches
    std::vector<TBranch *> metaBranches_;
    std::vector<TBranch *> readBranches_;
    std::vector<TBranch *> unclonedReadBranches_;
    std::set<std::string> unclonedReadBranchNames_;
    bool currentlyFastCloning_;
    int basketSize_;
    int splitLevel_;
    int64_t saveMemoryObjectThreshold_;
  };  // RootOutputTree

}  // art

#endif /* art_Framework_IO_Root_RootOutputTree_h */

// Local Variables:
// mode: c++
// End:
