// Usage: root -l -b -q 'plot_pt_reso.C("output.root","CUTNAME","figs/")'
// Needs the file produced by the PATCHED TANAactPtReso. Writes the PDFs and SVGs used in the thesis and
// momentum_numbers.txt with the values to copy into the LaTeX. CUTNAME = name of the cut-set directory.
#include "TFile.h"
#include "TDirectory.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TPaveText.h"
#include "TStyle.h"
#include "TLine.h"
#include "TKey.h"
#include "TSystem.h"
#include "TMath.h"
#include <fstream>
#include <iostream>

TDirectory *FindDir(TDirectory *d, const TString &name) {
  if (!d) return nullptr;
  if (TString(d->GetName()) == name) return d;
  TIter it(d->GetListOfKeys());
  while (TKey *k = (TKey *)it()) {
    if (TString(k->GetClassName()) != "TDirectoryFile" && TString(k->GetClassName()) != "TDirectory") continue;
    TDirectory *s = FindDir((TDirectory *)k->ReadObj(), name);
    if (s) return s;
  }
  return nullptr;
}

struct Fit { double mu=0, muE=0, sg=0, sgE=0, chi2=0; int ndf=0; bool ok=false; };

// peak fit with FWHM seed, +-2 sigma window iterated 3 times, optional lower limit (3He exclusion)
Fit GFit(TH1 *h, const char *nm, double lowLim = -1e30) {
  Fit r; if (!h || h->GetEntries() < 30) return r;
  TAxis *ax = h->GetXaxis();
  ax->SetRange(std::max(1, ax->FindBin(std::max(lowLim, ax->GetXmin() + 1e-9))), ax->GetNbins());
  int pk = h->GetMaximumBin(); ax->SetRange(0, 0);
  double mx = h->GetBinContent(pk), mu = h->GetBinCenter(pk);
  int bl = pk, br = pk;
  while (bl > 1 && h->GetBinContent(bl) > 0.5 * mx) --bl;
  while (br < ax->GetNbins() && h->GetBinContent(br) > 0.5 * mx) ++br;
  double sg = std::max((h->GetBinCenter(br) - h->GetBinCenter(bl)) / 2.355, 2 * h->GetBinWidth(pk));
  TF1 *f = new TF1(nm, "gaus", ax->GetXmin(), ax->GetXmax()); f->SetLineColor(kRed); f->SetLineWidth(2);
  for (int i = 0; i < 3; ++i) {
    double lo = std::max(lowLim, mu - 2 * sg), hi = mu + 2 * sg;
    f->SetRange(lo, hi); f->SetParameters(mx, mu, sg);
    if ((int)h->Fit(f, "QRS0") != 0) return r;
    mu = f->GetParameter(1); sg = fabs(f->GetParameter(2));
  }
  r.ok = true; r.mu = mu; r.muE = f->GetParError(1); r.sg = sg; r.sgE = f->GetParError(2);
  r.chi2 = f->GetChisquare(); r.ndf = f->GetNDF();
  h->GetListOfFunctions()->Clear(); h->GetListOfFunctions()->Add(f);  // keep fit for drawing
  return r;
}

void SaveBoth(TCanvas &c, const char *out) {   // writes <name>.pdf and <name>.svg
  c.SaveAs(out);
  TString s(out); s.ReplaceAll(".pdf", ".svg"); c.SaveAs(s.Data());
}

void Draw(TH1 *h, const Fit &f, const char *out, const char *xt, double x1, double x2, const char *extra = "") {
  TCanvas c("c", "c", 900, 650); c.SetLeftMargin(0.13); c.SetBottomMargin(0.13);
  gStyle->SetOptStat(0);
  h->SetTitle(""); h->GetXaxis()->SetTitle(xt); h->GetXaxis()->SetRangeUser(x1, x2);
  h->GetYaxis()->SetTitle("Entries"); h->SetLineColor(kBlack); h->SetMarkerStyle(20); h->SetMarkerSize(0.5);
  h->Draw("E");
  TF1 *fn = (TF1 *)h->GetListOfFunctions()->First(); if (fn) fn->Draw("same");
  TPaveText t(0.58, 0.62, 0.88, 0.88, "NDC"); t.SetFillColor(0); t.SetBorderSize(1); t.SetTextAlign(12);
  t.AddText(Form("#mu = %.4f #pm %.4f", f.mu, f.muE)); t.AddText(Form("#sigma = %.4f #pm %.4f", f.sg, f.sgE));
  t.AddText(Form("#chi^{2}/ndf = %.0f/%d", f.chi2, f.ndf)); t.AddText(Form("N = %.0f", h->GetEntries()));
  if (strlen(extra)) t.AddText(extra);
  t.Draw(); SaveBoth(c, out);
}

void Draw2D(TH2 *h, const char *out, double lo, double hi, const char *xt, const char *yt) {
  gStyle->SetPalette(kRainBow); gStyle->SetNumberContours(255);
  TCanvas c("c2", "c2", 900, 750); c.SetRightMargin(0.16); c.SetLeftMargin(0.13); c.SetBottomMargin(0.13);
  gStyle->SetOptStat(0); c.SetLogz();
  h->SetTitle(""); h->GetXaxis()->SetTitle(xt); h->GetYaxis()->SetTitle(yt);
  h->GetXaxis()->SetRangeUser(lo, hi); h->GetYaxis()->SetRangeUser(lo, hi); h->Draw("COLZ");
  gPad->Update();
  TLine l(lo, lo, hi, hi); l.SetLineColor(kRed); l.SetLineStyle(2); l.SetLineWidth(2); l.Draw();
  SaveBoth(c, out);
}

void plot_pt_reso(const char *file = "output.root", const char *cut = "CUTNAME", const char *od = "figs/") {
  TFile *F = TFile::Open(file); if (!F || F->IsZombie()) { printf("cannot open %s\n", file); return; }
  TDirectory *cd = FindDir(F, cut); if (!cd) { printf("cut dir %s not found\n", cut); return; }
  TDirectory *dr = FindDir(cd, "Data_reso");
  gStyle->SetPalette(kRainBow);
  TString o(od); gSystem->mkdir(od, kTRUE);
  std::ofstream tx((o + "momentum_numbers.txt").Data());
  auto G = [&](int Z, const char *n) { return (TH1 *)dr->Get(Form("Z%d/%s", Z, n)); };

  // ---- Carbon (Z=6) and helium (Z=2)
  TH1 *eC = G(6, "h_RelMomentumResidual"), *pC = G(6, "h_TwMomentum"), *kC = G(6, "h_TrkMomentum");
  TH1 *eH = G(2, "h_RelMomentumResidual"), *kH = G(2, "h_TrkMomentum");
  Fit fe_C = GFit(eC, "fEC"), fp_C = GFit(pC, "fPC"), fe_H = GFit(eH, "fEH", -0.12), fk_H = GFit(kH, "fKH");
  Fit fk_C = GFit(kC, "fKC");
  if (eC) Draw(eC, fe_C, (o + "fig_eps_data_C.pdf").Data(), "#epsilon_{data} = (p_{fit}-p_{#beta})/p_{#beta}", fe_C.mu - 10 * fe_C.sg, fe_C.mu + 10 * fe_C.sg);
  if (pC) Draw(pC, fp_C, (o + "fig_pbeta_C.pdf").Data(), "p_{#beta} [GeV/c]", fp_C.mu - 12 * fp_C.sg, fp_C.mu + 12 * fp_C.sg);
  if (eH) {
    // draw full range (3He visible) and show the 4He fit window only
    Draw(eH, fe_H, (o + "fig_eps_data_He.pdf").Data(), "#epsilon_{data} = (p_{fit}-p_{#beta})/p_{#beta}", -0.9, 1, "^{3}He peak excluded (#epsilon>-0.12)");
  }
  if (kH) Draw(kH, fk_H, (o + "fig_pfit_He.pdf").Data(), "p_{fit} [GeV/c]", fk_H.mu - 8 * fk_H.sg, fk_H.mu + 8 * fk_H.sg);
  if (TH2 *h = (TH2 *)dr->Get("Z6/h_MomentumVsTwMomentum")) Draw2D(h, (o + "fig_pbeta_vs_pfit_C.pdf").Data(), 4.0, 13., "p_{fit} [GeV/c]", "p_{#beta} [GeV/c]");
  if (TH2 *h = (TH2 *)dr->Get("Z2/h_MomentumVsTwMomentum")) Draw2D(h, (o + "fig_pbeta_vs_pfit_He.pdf").Data(), 0.5, 5.5, "p_{fit} [GeV/c]", "p_{#beta} [GeV/c]");

  // ---- ToF inputs for the nominal term (data-driven)
  TF1 tofRes("tofRes", "sqrt([0]/x+[1])", 1, 200); tofRes.SetParameters(0.38, 5.2e-4);
  auto Nominal = [&](int Z, double pbeta_mu, double mA) {
    TH1 *he = G(Z, "h_TwEloss"), *ht = G(Z, "h_TofFromTarget"); if (!he || !ht) return -1.;
    Fit e = GFit(he, Form("fE%d", Z)), t = GFit(ht, Form("fT%d", Z)); if (!e.ok || !t.ok) return -1.;
    double bg = pbeta_mu / mA, g2 = 1 + bg * bg, st = tofRes.Eval(e.mu);
    tx << Form("Z=%d  mu(dE)=%.2f MeV  sigma_ToF=%.1f ps  mu(ToF)=%.3f ns  gamma^2=%.3f  nominal sigma_pb/p=%.4f\n", Z, e.mu, 1e3 * st, t.mu, g2, g2 * st / t.mu);
    return g2 * st / t.mu;
  };
  // carbon: p_beta peak from data; helium: p_beta peak from the beta-gamma histogram of the 4He peak
  double nomC = fp_C.ok ? Nominal(6, fp_C.mu, 11.1779) : -1;
  double pbHe = 0; if (TH1 *hh = G(2, "h_TwMomentum")) { Fit f = GFit(hh, "fPHe"); pbHe = f.ok ? f.mu : 0; }
  double nomH = pbHe > 0 ? Nominal(2, pbHe, 3.7274) : -1;

  auto Res = [&](const char *nm, Fit &e, double nom) {
    if (!e.ok || nom < 0) return;
    double r = sqrt(std::max(0., e.sg * e.sg - nom * nom)), re = e.sg * e.sgE / r;
    tx << Form("%s: <eps>=%.4f+-%.4f  sigma(eps)=%.5f+-%.5f  nominal=%.4f  sigma(pfit)/p=%.4f+-%.4f\n", nm, e.mu, e.muE, e.sg, e.sgE, nom, r, re);
  };
  tx << Form("C: <pbeta>=%.4f GeV/c  sigma(pbeta)=%.4f  observed rel. width=%.4f (includes physical spread)\n", fp_C.mu, fp_C.sg, fp_C.sg / fp_C.mu);
  Res("C", fe_C, nomC); Res("He", fe_H, nomH);
  tx << Form("He: <pfit>=%.4f+-%.4f GeV/c sigma=%.4f\n", fk_H.mu, fk_H.muE, fk_H.sg);
  tx << Form("C: <pfit>=%.4f GeV/c; expected p(200 MeV/u)=7.708 -> <pbeta>/p_nom-1=%.4f\n", fk_C.mu, fp_C.mu / 7.708 - 1);
  tx.close(); printf("wrote %smomentum_numbers.txt\n", od);

  // ---- extra: eps_data vs beta-gamma (bias / sigma trend) for C and He
  for (int Z : {6, 2}) if (TH2 *h = (TH2 *)dr->Get(Form("Z%d/h_RelMomentumResidualVsBetaGamma", Z))) {
    gStyle->SetPalette(kRainBow); gStyle->SetNumberContours(255);
    TCanvas c("c3", "c3", 900, 650); c.SetLeftMargin(0.13); c.SetBottomMargin(0.13); c.SetRightMargin(0.16); c.SetLogz(); gStyle->SetOptStat(0);
    h->SetTitle(""); h->GetXaxis()->SetRangeUser(-0.6, 0.6); h->Draw("COLZ"); SaveBoth(c, Form("%sfig_eps_vs_bg_Z%d.pdf", od, Z));
  }
}