// root -l 'CompareChi2AndPValue.C(6)'


#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TDirectory.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TString.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

// Recursively look for a histogram called hname whose path contains pathKey
static TH1* FindHist(TDirectory* dir, const TString& hname, const TString& pathKey,
                     const TString& cur = "", TString* foundPath = nullptr)
{
    TIter next(dir->GetListOfKeys());
    while (TKey* key = (TKey*)next()) {
        TObject* obj = key->ReadObj();
        if (!obj) continue;
        TString p = cur + "/" + key->GetName();
        if (obj->InheritsFrom(TDirectory::Class())) {
            TH1* h = FindHist((TDirectory*)obj, hname, pathKey, p, foundPath);
            if (h) return h;
        } else if (obj->InheritsFrom(TH1::Class()) &&
                   hname == key->GetName() && p.Contains(pathKey)) {
            if (foundPath) *foundPath = p;
            return (TH1*)obj;
        }
    }
    return nullptr;
}

struct Config { TString file, label, tag; Color_t color; };

static TH1* Load(const Config& c, const TString& hname, const TString& pathKey)
{
    TFile* f = TFile::Open(c.file, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error opening " << c.file << std::endl;
        return nullptr;
    }
    TString found;
    TH1* h = FindHist(f, hname, pathKey, "", &found);
    if (!h) {
        std::cerr << "[" << c.label << "] " << hname << " with '" << pathKey
                  << "' NOT FOUND in " << c.file << " (use f->ls() to inspect)\n";
        f->Close();
        return nullptr;
    }
    std::cout << "[" << c.label << "] using " << found << std::endl;
    TH1* hc = (TH1*)h->Clone(Form("%s_%s_%s", hname.Data(), pathKey.Data(), c.tag.Data()));
    hc->SetDirectory(0);          // detach before closing the file
    hc->SetLineColor(c.color);
    hc->SetLineWidth(2);
    f->Close();
    return hc;
}

static void PrintInfo(const char* label, TH1* h)
{
    if (!h) { std::cout << std::left << std::setw(32) << label << " -> [NOT FOUND]\n"; return; }
    int maxBin = h->GetMaximumBin();
    std::cout << std::left << std::setw(32) << label
              << " entries=" << std::setw(9) << h->GetEntries()
              << " mean=" << std::setw(8) << h->GetMean()
              << " rms=" << std::setw(8) << h->GetStdDev()
              << " peak at x=" << h->GetXaxis()->GetBinCenter(maxBin)
              << " (content " << h->GetMaximum() << ")\n";
}

static void Overlay(std::vector<TH1*>& hs, const std::vector<Config>& cfg,
                    const char* cname, const char* title, bool logy, bool norm,
                    double ymin, const char* outName)
{
    TCanvas* c = new TCanvas(cname, title, 800, 600);
    c->SetGrid();
    if (logy) c->SetLogy();

    double ymax = 0;
    for (auto* h : hs) {
        if (!h) continue;
        if (norm && h->Integral() > 0) h->Scale(1. / h->Integral());
        ymax = std::max(ymax, h->GetMaximum());
    }

    TLegend* leg = new TLegend(0.50, 0.70, 0.88, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);

    bool first = true;
    for (size_t i = 0; i < hs.size(); ++i) {
        if (!hs[i]) continue;
        if (first) {
            hs[i]->SetTitle(title);
            hs[i]->SetMaximum(ymax * (logy ? 3. : 1.2));
            hs[i]->SetMinimum(ymin);
            hs[i]->Draw("HIST");
            first = false;
        } else {
            hs[i]->Draw("HIST SAME");
        }
        leg->AddEntry(hs[i], cfg[i].label, "l");
    }
    leg->Draw();
    c->Update();
    c->SaveAs(outName);
}

void CompareChi2AndPValue(int Z = 6, bool normalize = false)
{
    gStyle->SetOptStat(0);

    // Order matches the thesis panels (a), (b), (c)
    std::vector<Config> cfg = {
        {"8028_preAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028_TWBarCalib_ALL_POINTS.root",
         "Original, pre-alignment",  "pre",  kBlue + 1},
        {"8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root",
         "Original, post-alignment", "orig", kRed + 1},
        // CHECK: your screenshot of the f(eta) panel came from the ..._SIGNAL_FETA_ALL_events_new.root file
        {"8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root",
         "f(#eta), post-alignment",  "feta", kGreen + 2}
    };

    const TString hname = Form("h_Z_%d", Z);

    std::vector<TH1*> chi, pv;
    for (auto& c : cfg) {
        chi.push_back(Load(c, hname, "Exp_All/ReducedChi2"));
        pv .push_back(Load(c, hname, "Exp_All/Pvalue"));
    }

    std::cout << "\n=========== Reduced chi2, Z=" << Z << " ===========\n";
    for (size_t i = 0; i < cfg.size(); ++i) PrintInfo(cfg[i].label, chi[i]);
    std::cout << "\n=========== P-value, Z=" << Z << " ===========\n";
    for (size_t i = 0; i < cfg.size(); ++i) PrintInfo(cfg[i].label, pv[i]);
    std::cout << std::endl;

    const char* ytitle = normalize ? "Fraction of events" : "Events";
    Overlay(chi, cfg, "c_chi", Form("#chi^{2}_{red} comparison, Z=%d;#chi^{2}_{red};%s", Z, ytitle),
            true, normalize, normalize ? 1e-6 : 0.5, Form("chi2_comparison_Z%d.pdf", Z));
    Overlay(pv, cfg, "c_p", Form("p-value comparison, Z=%d;p-value;%s", Z, ytitle),
            true, normalize, normalize ? 1e-6 : 0.5, Form("pvalue_comparison_Z%d.pdf", Z));

    // Single-panel exports for the thesis subfigures (a), (b), (c)
    for (size_t i = 0; i < cfg.size(); ++i) {
        if (chi[i]) {
            TCanvas* c = new TCanvas(Form("cc_%zu", i), "", 600, 600);
            chi[i]->SetTitle(";#chi^{2}_{red};Events");
            chi[i]->Draw("HIST");
            c->SaveAs(Form("chi2_%s_Z%d.pdf", cfg[i].tag.Data(), Z));
        }
        if (pv[i]) {
            TCanvas* c = new TCanvas(Form("cp_%zu", i), "", 600, 600);
            c->SetLogy();
            pv[i]->SetTitle(";p-value;Events");
            pv[i]->Draw("HIST");
            c->SaveAs(Form("pvalue_%s_Z%d.pdf", cfg[i].tag.Data(), Z));
        }
    }
}