#include "art/Framework/IO/Root/RootTree.h"

#include "art/Framework/Principal/Principal.h"
#include "art/Framework/IO/Root/RootDelayedReader.h"
#include "art/Framework/Principal/Provenance.h"
#include "art/Persistency/Provenance/BranchDescription.h"
#include "art/Utilities/WrappedClassName.h"

#include "Rtypes.h"
#include "TFile.h"
#include "TTreeCache.h"
#include "TTreeIndex.h"
#include "TVirtualIndex.h"

#include <utility>
#include <iostream>


namespace art {
  namespace {

    TBranch * getAuxiliaryBranch(TTree * tree, BranchType const& branchType) {
      TBranch *branch = tree->GetBranch(BranchTypeToAuxiliaryBranchName(branchType).c_str());
      return branch;
    }
    TBranch * getProductProvenanceBranch(TTree * tree, BranchType const& branchType) {
      TBranch *branch = tree->GetBranch(productProvenanceBranchName(branchType).c_str());
      return branch;
    }
  }  // namespace

  RootTree::RootTree(std::shared_ptr<TFile> filePtr,
                     BranchType const& branchType,
                     int64_t saveMemoryObjectThreshold) :
    filePtr_(filePtr),
    tree_(dynamic_cast<TTree *>(filePtr_.get() != 0 ? filePtr->Get(BranchTypeToProductTreeName(branchType).c_str()) : 0)),
    metaTree_(dynamic_cast<TTree *>(filePtr_.get() != 0 ? filePtr->Get(BranchTypeToMetaDataTreeName(branchType).c_str()) : 0)),
    branchType_(branchType),
    saveMemoryObjectThreshold_(saveMemoryObjectThreshold),
    auxBranch_(tree_ ? getAuxiliaryBranch(tree_, branchType_) : 0),
    productProvenanceBranch_(metaTree_ ? getProductProvenanceBranch(metaTree_, branchType_) : 0),
    entries_(tree_ ? tree_->GetEntries() : 0),
    entryNumber_(-1),
    branchNames_(),
    branches_(new BranchMap)
  {
    if (!isValid()) {
      throw Exception(errors::FileReadError)
        << "RootTree for branch type "
        << BranchTypeToString(branchType)
        << "Could not be initialized correctly from input file.\n";
    }
  }

  bool
  RootTree::isValid() const {
    if (metaTree_ == 0 || metaTree_->GetNbranches() == 0)
      return tree_ != 0 && auxBranch_ != 0 && tree_->GetNbranches() == 1;
    else
      return tree_ && auxBranch_ && metaTree_ && productProvenanceBranch_;
  }

  bool
  RootTree::hasBranch(std::string const & branchName) const {
    return tree_->GetBranch(branchName.c_str()) != 0;
  }

  void
  RootTree::addBranch(BranchKey const& key,
                      BranchDescription const& prod,
                      std::string const& branchName) {
      assert(isValid());
      //use the translated branch name
      TBranch * branch = tree_->GetBranch(branchName.c_str());
      assert (prod.present() == (branch != 0));
      input::BranchInfo info(prod);
      info.productBranch_ = 0;
      if (prod.present()) {
        info.productBranch_ = branch;
        //we want the new branch name for the JobReport
        branchNames_.emplace_back(prod.branchName());
      }
      branches_->emplace(key, info);
  }

  void
  RootTree::dropBranch(std::string const& branchName) {
      //use the translated branch name
      TBranch * branch = tree_->GetBranch(branchName.c_str());
      if (branch != 0) {
        TObjArray * leaves = tree_->GetListOfLeaves();
        int entries = leaves->GetEntries();
        for (int i = 0; i < entries; ++i) {
          TLeaf *leaf = (TLeaf *)(*leaves)[i];
          if (leaf == 0) continue;
          TBranch* br = leaf->GetBranch();
          if (br == 0) continue;
          if (br->GetMother() == branch) {
            leaves->Remove(leaf);
          }
        }
        leaves->Compress();
        tree_->GetListOfBranches()->Remove(branch);
        tree_->GetListOfBranches()->Compress();
        delete branch;
      }
  }

  std::unique_ptr<DelayedReader>
  RootTree::makeDelayedReader() const {
    auto store =
      std::unique_ptr<DelayedReader>
      (new RootDelayedReader(entryNumber_,
                             branches_,
                             filePtr_,
                             saveMemoryObjectThreshold_));
    return store;
  }

  void
  RootTree::setCacheSize(unsigned int cacheSize) const {
    tree_->SetCacheSize(static_cast<Long64_t>(cacheSize));
  }

  void
  RootTree::setTreeMaxVirtualSize(int treeMaxVirtualSize) {
    if (treeMaxVirtualSize >= 0) tree_->SetMaxVirtualSize(static_cast<Long64_t>(treeMaxVirtualSize));
  }

  void
  RootTree::setEntryNumber(EntryNumber theEntryNumber) {
    if (TTreeCache *tc = dynamic_cast<TTreeCache *>(filePtr_->GetCacheRead(tree_))) {
      assert(tree_ == tc->GetTree());
      if (theEntryNumber >= 0 && tc->IsLearning()) {
        tc->SetLearnEntries(1);
        tc->SetEntryRange(0, tree_->GetEntries());
        for (BranchMap::const_iterator i = branches_->begin(), e = branches_->end(); i != e; ++i) {
          if (i->second.productBranch_) {
            tc->AddBranch(i->second.productBranch_, kTRUE);
          }
        }
        tc->StopLearningPhase();
      }
    }

    entryNumber_ = theEntryNumber;
    tree_->LoadTree(theEntryNumber);
  }

  std::unique_ptr<BranchMapper>
  RootTree::makeBranchMapper() const {
    return std::unique_ptr<BranchMapper>(new BranchMapperWithReader(productProvenanceBranch_, entryNumber_));
  }

  namespace input {

    Int_t
    getEntry(TBranch* branch, EntryNumber entryNumber) try {
      return branch->GetEntry(entryNumber);
    }
    catch(cet::exception &e) {
      throw art::Exception(art::errors::FileReadError) << e.explain_self() << "\n";
    }

    Int_t
    getEntry(TTree* tree, EntryNumber entryNumber) try {
      return tree->GetEntry(entryNumber);
    }
    catch(cet::exception &e) {
      throw art::Exception(art::errors::FileReadError) << e.explain_self() << "\n";
    }

  }  // input
}  // art
