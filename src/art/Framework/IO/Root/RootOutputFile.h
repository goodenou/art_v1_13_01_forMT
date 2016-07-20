#ifndef art_Framework_IO_Root_RootOutputFile_h
#define art_Framework_IO_Root_RootOutputFile_h

// ======================================================================
//
// RootOutputFile
//
// ======================================================================

// FIXME! There is an incestuous relationship between RootOutputFile and
// RootOutput that only works because the methods of RootOutput and
// OutputItem used by RootOutputFile are all inline. A correct and
// robust implementation would have a OutputItem defined in a separate
// file and the information (basket size, etc) in a different class in
// the main art/Framework/Root library accessed by both RootOutputFile
// and RootOutput. This has been entered as issue #2885.

#include "TROOT.h"
#include "art/Framework/Core/Frameworkfwd.h"
#include "art/Framework/IO/Root/RootOutput.h"
#include "art/Framework/IO/Root/RootOutputTree.h"
#include "art/Persistency/Provenance/BranchDescription.h"
#include "art/Persistency/Provenance/BranchID.h"
#include "art/Persistency/Provenance/BranchType.h"
#include "art/Persistency/Provenance/FileIndex.h"
#include "art/Persistency/Provenance/ParameterSetBlob.h"
#include "art/Persistency/Provenance/ParameterSetMap.h"
#include "art/Persistency/Provenance/ProductProvenance.h"
#include "art/Persistency/Provenance/Selections.h"
#include "art/Persistency/RootDB/SQLite3Wrapper.h"
#include "boost/filesystem.hpp"
#include "cpp0x/array"
#include "cpp0x/memory"

#include <map>
#include <string>
#include <vector>

class TFile;
class TTree;

namespace art {
  class RootOutput;
  class History;

  class RootOutputFile {
  public:
    typedef RootOutput::OutputItem OutputItem;
    typedef RootOutput::OutputItemList OutputItemList;
    typedef std::array<RootOutputTree *, NumBranchTypes> RootOutputTreePtrArray;
    explicit RootOutputFile(RootOutput * om, std::string const& fileName);
    // use compiler-generated copy c'tor, copy assignment, and d'tor
    void writeOne(EventPrincipal const& e);
    //void endFile();
    void writeSubRun(SubRunPrincipal const& sr);
    void writeRun(RunPrincipal const& r);
    void writeFileFormatVersion();
    void writeFileIndex();
    void writeEventHistory();
    void writeProcessConfigurationRegistry();
    void writeProcessHistoryRegistry();
    void writeParameterSetRegistry();
    void writeProductDescriptionRegistry();
    void writeParentageRegistry();
    void writeBranchIDListRegistry();
    void writeProductDependencies();
    void
    writeFileCatalogMetadata(FileStatsCollector const & stats,
                             FileCatalogMetadata::collection_type const & md,
                             FileCatalogMetadata::collection_type const & ssmd);

    void finishEndFile();
    void beginInputFile(FileBlock const& fb, bool fastClone);
    void respondToCloseInputFile(FileBlock const& fb);
    bool shouldWeCloseFile() const;

    std::string const & currentFileName() const;

  private:

    //-------------------------------
    // Private functions

    void fillBranches(BranchType const& branchType,
                      Principal const& principal,
                      std::vector<ProductProvenance> * productProvenanceVecPtr);

     void insertAncestors(ProductProvenance const& iGetParents,
                          Principal const& principal,
                          std::set<ProductProvenance>& oToFill);

    //-------------------------------
    // Member data

    std::string file_;
    RootOutput const* om_;
    bool currentlyFastCloning_;
    std::shared_ptr<TFile> filePtr_;
    FileIndex fileIndex_;
    FileIndex::EntryNumber_t eventEntryNumber_;
    FileIndex::EntryNumber_t subRunEntryNumber_;
    FileIndex::EntryNumber_t runEntryNumber_;
    TTree * metaDataTree_;
    TTree * parentageTree_;
    TTree * eventHistoryTree_;
    EventAuxiliary const*           pEventAux_;
    SubRunAuxiliary const* pSubRunAux_;
    RunAuxiliary const*             pRunAux_;
    ProductProvenances         eventProductProvenanceVector_;
    ProductProvenances         subRunProductProvenanceVector_;
    ProductProvenances         runProductProvenanceVector_;
    ProductProvenances *       pEventProductProvenanceVector_;
    ProductProvenances *       pSubRunProductProvenanceVector_;
    ProductProvenances *       pRunProductProvenanceVector_;
    History const*                  pHistory_;
    RootOutputTree eventTree_;
    RootOutputTree subRunTree_;
    RootOutputTree runTree_;
    RootOutputTreePtrArray treePointers_;
    bool dataTypeReported_;
    std::set<BranchID> branchesWithStoredHistory_;
    SQLite3Wrapper metaDataHandle_;
  };  // RootOutputFile

  inline
  std::string const &
  RootOutputFile::currentFileName() const {
    return file_;
  }

}  // art

// ======================================================================

#endif /* art_Framework_IO_Root_RootOutputFile_h */

// Local Variables:
// mode: c++
// End:
