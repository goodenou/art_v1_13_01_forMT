// ======================================================================
//
// FileDumperOutput.cc: "dump contents of a file"
//
// Proposed output format (Feature #941):
// Process Name | Module Label | Process Label | Data Product type | size
//
// ======================================================================

#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/OutputModule.h"
#include "art/Framework/Principal/EventPrincipal.h"
#include "art/Framework/Principal/RunPrincipal.h"
#include "art/Framework/Principal/SubRunPrincipal.h"
#include "art/Utilities/Exception.h"
#include "cetlib/column_width.h"
#include "cetlib/lpad.h"
#include "cetlib/rpad.h"
#include "cpp0x/algorithm"
#include "fhiclcpp/ParameterSet.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace art {
  class FileDumperOutput;
}

using fhicl::ParameterSet;

class art::FileDumperOutput : public OutputModule {
public:
  explicit FileDumperOutput(ParameterSet const &);

private:
  void write(EventPrincipal const & e) override;
  void writeRun(RunPrincipal const & r) override;
  void writeSubRun(SubRunPrincipal const & sr) override;

  template <typename P>
  void printPrincipal(P const & p);

  bool wantOnDemandProduction_;
  bool wantProductFullClassName_;
  bool wantProductFriendlyClassName_;
  bool wantResolveProducts_;
  bool wantPresentOnly_;
};  // FileDumperOutput

art::FileDumperOutput::
FileDumperOutput(fhicl::ParameterSet const & ps)
  :
  OutputModule(ps),
  wantOnDemandProduction_(ps.get<bool>("onDemandProduction", false)),
  wantProductFullClassName_(ps.get<bool>("wantProductFullClassName", true)),
  wantProductFriendlyClassName_(ps.get<bool>("wantProductFriendlyClassName",
                                ! wantProductFullClassName_)),
  wantResolveProducts_(ps.get<bool>("resolveProducts", true)),
  wantPresentOnly_(ps.get<bool>("onlyIfPresent", false))
{
}

void
art::FileDumperOutput::
write(EventPrincipal const & e)
{
  printPrincipal(e);
}

void
art::FileDumperOutput::
writeRun(RunPrincipal const & r)
{
  printPrincipal(r);
}

void
art::FileDumperOutput::
writeSubRun(SubRunPrincipal const & sr)
{
  printPrincipal(sr);
}

template <typename P>
void
art::FileDumperOutput::
printPrincipal(P const & p)
{
  if (!p.size()) { return; } // Nothing to do.
  // prepare the data structure, a sequence of columns:
  typedef  std::vector<std::string>  column;
  unsigned int ncols = 6;
  std::vector<column> col(ncols);
  // provide column headings:
  col[0].push_back("PROCESS NAME");
  col[1].push_back("MODULE LABEL");
  col[2].push_back("PRODUCT INSTANCE NAME");
  col[3].push_back("DATA PRODUCT TYPE");
  col[4].push_back("PRODUCT FRIENDLY TYPE");
  col[5].push_back("SIZE");
  size_t present = 0;
  size_t not_present = 0;
  // insert the per-product data:
  for (typename P::const_iterator
       it  = p.begin(),
       end = p.end();
       it != end;
       ++it) {
    Group const & g = *(it->second);
    if (wantResolveProducts_) {
      try {
        if (!g.resolveProduct(wantOnDemandProduction_, g.producedWrapperType()))
        { throw Exception(errors::DataCorruption, "data corruption"); }
      }
      catch (art::Exception const & e) {
        if (e.category() != "ProductNotFound")
        { throw; }
        if (g.anyProduct())
          throw art::Exception(errors::LogicError, "FileDumperOutput module", e)
              << "Product reported as not present, but is pointed to nevertheless!";
      }
    }
    if (g.anyProduct()) {
      ++present;
    }
    else {
      ++not_present;
    }
    if ((!wantPresentOnly_) || g.anyProduct()) {
      col[0].push_back(g.processName());
      col[1].push_back(g.moduleLabel());
      col[2].push_back(g.productInstanceName());
      col[3].push_back(g.productDescription().producedClassName());
      col[4].push_back(g.productDescription().friendlyClassName());
      if (g.anyProduct()) {
        col[5].push_back(g.anyProduct()->productSize());
      }
      else {
        col[5].push_back(g.onDemand() ? "o/d" : "?");
      }
    }
  }
  // determine each column's width:
  std::vector<unsigned> width(ncols);
  std::transform(col.begin(),
                 col.end(),
                 width.begin(),
                 &cet::column_width);
  // prepare and emit the per-product information:
  for (unsigned row = 0, end = col[0].size(); row != end; ++row) {
    std::string s;
    for (unsigned c = 0, end = ncols - 1; c != end; ++c) {
      if (c == 3 && ! wantProductFullClassName_) { continue; }
      if (c == 4 && ! wantProductFriendlyClassName_) { continue; }
      s.append(cet::rpad(col[c][row], width[c], '.'))
      .append(" | ");
    }
    s.append(cet::lpad(col[ncols - 1][row], width[ncols - 1], '.'));
    std::cout << s << '\n';
  }
  std::cout << "\nTotal products (present, not present): "
            << present + not_present
            << " ("
            << present
            << ", "
            << not_present
            << ").\n\n";
}

DEFINE_ART_MODULE(art::FileDumperOutput)
