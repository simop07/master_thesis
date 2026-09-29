
/*!
 \file PullSummaryPlots.C
 \brief Standalone macro to reproduce the "Z_x mean/sigma for Exp_All" summary
        plots (mean and sigma of the Pull distribution vs. detector plane),
        for each charge (0-6) and for "All" charges combined.

 It reads directly from the histograms already produced and saved by
 TANAactQualityPlots, under the path:

     QualityPlots/Exp_All/Pull/<plane>/h_Z_<charge>

 e.g. QualityPlots/Exp_All/Pull/VT0x/h_Z_6

 The Gaussian fit performed here is IDENTICAL in logic to
 TANAactQualityPlots::GausFit(): range = [mean - 1*RMS, mean + 1*RMS],
 fit option "R Q M".

 -----------------------------------------------------------------------------
 USAGE
 -----------------------------------------------------------------------------
 Put this file in DECODED_analysis/z_analysis_post/ next to your ROOT files,
 then from a ROOT session in that folder:

     root -l
     .x PullSummaryPlots.C("Decoded_Analysis_CNAO2025_8040_TWBarCalib.root")

 or non-interactively:

     root -l -b -q 'PullSummaryPlots.C("Decoded_Analysis_CNAO2025_8040_TWBarCalib.root")'

 Optional 3rd/4th arguments let you change the output name and whether PNGs
 are saved (see function signature below).

 \author S. Pasquini
 */

#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TString.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TLine.h"
#include <vector>
#include <iostream>
#include <cstdio>
#include <algorithm>

// -----------------------------------------------------------------------
// EDIT HERE if your plane list / order differs from what is in the file.
// This matches the "Pull" subfolder names visible in your ROOT browser:
// VT0x VT0y VT1x VT1y VT2x VT2y VT3x VT3y MSD0 MSD1 MSD2 MSD3 MSD4 MSD5 TWx TWy
// -----------------------------------------------------------------------
static const std::vector<TString> kPlanes = {
	"VT0x", "VT0y", "VT1x", "VT1y", "VT2x", "VT2y", "VT3x", "VT3y"
	//,"MSD0", "MSD1", "MSD2", "MSD3", "MSD4", "MSD5", "TWx", "TWy"
};

// Charges present in the file: h_Z_0 ... h_Z_6, h_Z_All
static const std::vector<TString> kCharges = {
	"0", "1", "2", "3", "4", "5", "6", "All"
};

// Result of a single Gaussian fit
struct FitResult
{
	Double_t mean    = 0.;
	Double_t meanErr = 0.;
	Double_t sigma   = 0.;
	Double_t sigmaErr = 0.;
	Bool_t   valid   = false;
};

//------------------------------------------------------------------------------
//! \brief Same fitting recipe as TANAactQualityPlots::GausFit
//!        (auto range = mean +/- 1 RMS)
FitResult FitPullHisto(TH1D *h, const TString &uniqueTag)
{
	FitResult res;

	if (!h || h->GetEntries() < 20)
		return res;

	Double_t amplitude = h->GetMaximum();
	Double_t mean      = h->GetMean();
	Double_t rms       = h->GetRMS();

	Double_t fitMin = mean - 1.0 * rms;
	Double_t fitMax = mean + 1.0 * rms;

	// unique name per fit function to avoid clashes across many calls
	TF1 *fGauss = new TF1("fGauss_" + uniqueTag, "gaus", fitMin, fitMax);
	fGauss->SetParameter(0, amplitude);
	fGauss->SetParameter(1, mean);
	fGauss->SetParameter(2, rms);

	h->Fit(fGauss, "R Q M", "", fitMin, fitMax);

	res.mean     = fGauss->GetParameter(1);
	res.meanErr  = fGauss->GetParError(1);
	res.sigma    = fGauss->GetParameter(2);
	res.sigmaErr = fGauss->GetParError(2);
	res.valid    = true;

	delete fGauss;
	return res;
}

//------------------------------------------------------------------------------
//! \brief Build one summary plot (mean or sigma vs plane) and draw/save it.
//
//! The plotting cosmetics are matched to PullComparePlots.C:
//!   - 750 x 650 canvas
//!   - left margin = 0.13
//!   - horizontal dotted grid
//!   - dynamic y-axis range
//!   - reference line at 0 for mean and 1 for sigma
//!   - #mu_{pull} / #sigma_{pull} y-axis labels
//!   - marker style/size matching PullComparePlots
TCanvas *MakeSummaryPlot(const std::vector<Double_t> &values,
                          const std::vector<Double_t> &errors,
                          const std::vector<TString> &planeNames,
                          const TString &quantity,
                          const TString &charge,
                          const TString &outDir,
                          Bool_t savePNG)
{
	Int_t nPlanes = (Int_t)planeNames.size();
	if (nPlanes == 0)
		return nullptr;

	const Bool_t isMean = (quantity == "mean");

	// ------------------------------------------------------------------
	// Determine y-axis limits from the plotted values and their errors,
	// following the same logic as PullComparePlots.C.
	// ------------------------------------------------------------------
	Double_t ymin = 1e30;
	Double_t ymax = -1e30;

	for (Int_t i = 0; i < nPlanes; ++i)
	{
		Double_t value = values[i];
		Double_t error = errors[i];

		ymin = std::min(ymin, value - error);
		ymax = std::max(ymax, value + error);
	}

	// Reference line:
	//   mean  -> 0
	//   sigma -> 1 (ideal pull)
	const Double_t ref = isMean ? 0. : 1.;

	ymin = std::min(ymin, ref);
	ymax = std::max(ymax, ref);

	// Same margins used by PullComparePlots.C
	Double_t range = ymax - ymin;
	if (range <= 0)
		range = 1.;

	Double_t lo = ymin - 0.15 * range;
	Double_t hi = ymax + 0.40 * range;

	// ------------------------------------------------------------------
	// Histogram used only as an axis/frame.
	// The data points are still stored as bin contents as in the
	// original PullSummaryPlots.C.
	// ------------------------------------------------------------------
	TString hname  = Form("h_%s_Z_%s", quantity.Data(), charge.Data());
	TString title  = Form("Pull_Z_%s", charge.Data());

	TH1D *h = new TH1D(hname, title, nPlanes, 0, nPlanes);
	h->SetStats(0);

	for (Int_t i = 0; i < nPlanes; ++i)
	{
		h->SetBinContent(i + 1, values[i]);
		h->SetBinError(i + 1, 0.);
		h->GetXaxis()->SetBinLabel(i + 1, planeNames[i]);
	}

	// ------------------------------------------------------------------
	// Axis cosmetics matched to PullComparePlots.C
	// ------------------------------------------------------------------
	h->GetXaxis()->LabelsOption("h");
	h->GetXaxis()->SetLabelSize(0.045);
	h->GetXaxis()->SetTitle("");

	h->GetYaxis()->SetTitle(isMean ? "#mu_{pull}" : "#sigma_{pull}");
	h->GetYaxis()->SetTitleSize(0.05);
	h->GetYaxis()->SetTitleOffset(0.9);

	h->SetMinimum(lo);
	h->SetMaximum(hi);

	// ------------------------------------------------------------------
	// Canvas cosmetics matched to PullComparePlots.C
	// ------------------------------------------------------------------
	TString cname = Form("c_%s_Z_%s", quantity.Data(), charge.Data());

	TCanvas *c = new TCanvas(cname, title, 750, 650);
	c->SetLeftMargin(0.13);
	c->SetGridy();

	// Draw axes/frame
	h->Draw("AXIS");

	// ------------------------------------------------------------------
	// Reference line:
	//   mean  -> 0
	//   sigma -> 1
	// ------------------------------------------------------------------
	TLine *lref = new TLine(0, ref, nPlanes, ref);
	lref->SetLineWidth(2);
	lref->SetLineColor(kBlack);
	lref->Draw();

	// ------------------------------------------------------------------
	// Data points
	// Same marker cosmetics as the "Before" series in PullComparePlots:
	// blue filled circles, marker size 1.6.
	// ------------------------------------------------------------------
	const Int_t color = kBlue + 1;

	// Use a TGraphErrors so that the fit uncertainties can be displayed
	// vertically, consistently with the comparison plots.
	TGraphErrors *g = new TGraphErrors(nPlanes);

	for (Int_t i = 0; i < nPlanes; ++i)
	{
		g->SetPoint(i, i + 0.5, values[i]);
		g->SetPointError(i, 0., errors[i]);
	}

	g->SetName(Form("g_%s_Z_%s", quantity.Data(), charge.Data()));
	g->SetMarkerStyle(20);
	g->SetMarkerSize(1.6);
	g->SetMarkerColor(color);
	g->SetLineColor(color);
	g->SetLineWidth(2);

	g->Draw("P");

	c->Modified();
	c->Update();

	if (savePNG)
	{
		TString imageName = outDir + "/" + cname + ".svg";
		c->SaveAs(imageName);
	}

	return c;
}

//------------------------------------------------------------------------------
//! \brief Main entry point.
//
//! \param inputFile   path to the Decoded_Analysis_*.root file produced by TAG
//! \param outputFile  name of a new .root file where all summary canvases and
//!                    histograms are stored (default: appends "_pullsummary")
//! \param outDir      folder for optional svg output (default: ".")
//! \param savePNG     also dump each canvas as svg (default: false)
void PullSummaryPlots(const char *inputFile,
                       const char *outputFile = "",
                       const char *outDir = ".",
                       Bool_t savePNG = false)
{
	gStyle->SetOptStat(0);
	gStyle->SetOptFit(0);

	// ------------------------------------------------------------------
	// Plot cosmetics copied from PullComparePlots.C
	// ------------------------------------------------------------------
	gStyle->SetGridStyle(3);       // dotted
	gStyle->SetGridColor(kBlack);
	gStyle->SetGridWidth(2);

	TFile *fin = TFile::Open(inputFile, "READ");
	if (!fin || fin->IsZombie())
	{
		std::cerr << "[PullSummaryPlots] ERROR: cannot open input file "
		          << inputFile << std::endl;
		return;
	}

	TString outName = outputFile;
	if (outName.IsNull())
	{
		outName = inputFile;
		outName.ReplaceAll(".root", "_pullsummary.root");
	}

	TFile *fout = TFile::Open(outName, "RECREATE");

	const TString basePath = "QualityPlots/Exp_All/Pull";

	for (const auto &charge : kCharges)
	{
		std::vector<Double_t> means, meanErrs, sigmas, sigmaErrs;
		std::vector<TString> planeNamesFilled;

		for (const auto &plane : kPlanes)
		{
			TString histName = "h_Z_" + charge;
			TString fullPath = basePath + "/" + plane + "/" + histName;

			TH1D *h = (TH1D *)fin->Get(fullPath);
			if (!h)
			{
				std::cout << "[PullSummaryPlots] Missing histogram: "
				          << fullPath << " -> skipped" << std::endl;
				continue;
			}

			FitResult r = FitPullHisto(h, plane + "_Z" + charge);

			if (!r.valid)
			{
				std::cout << "[PullSummaryPlots] Fit failed / not enough stats: "
				          << fullPath << std::endl;
				continue;
			}

			planeNamesFilled.push_back(plane);
			means.push_back(r.mean);
			meanErrs.push_back(r.meanErr);
			sigmas.push_back(r.sigma);
			sigmaErrs.push_back(r.sigmaErr);
		}

		if (planeNamesFilled.empty())
			continue;

		// ---- Print mean/sigma table for this charge ----
		printf("\n=====================================================================\n");

		if (charge == "All")
			printf(" Pull fit summary (Exp_All) -- Charge: All charges combined\n");
		else
			printf(" Pull fit summary (Exp_All) -- Charge: Z = %s\n",
			       charge.Data());

		printf("=====================================================================\n");
		printf(" %-8s | %10s | %10s | %10s | %10s\n",
		       "Plane", "Mean", "MeanErr", "Sigma", "SigmaErr");
		printf("---------------------------------------------------------------------\n");

		for (size_t i = 0; i < planeNamesFilled.size(); ++i)
		{
			printf(" %-8s | %10.4f | %10.4f | %10.4f | %10.4f\n",
			       planeNamesFilled[i].Data(),
			       means[i], meanErrs[i],
			       sigmas[i], sigmaErrs[i]);
		}

		printf("=====================================================================\n");

		TCanvas *cMean = MakeSummaryPlot(
			means,
			meanErrs,
			planeNamesFilled,
			"mean",
			charge,
			outDir,
			savePNG
		);

		TCanvas *cSigma = MakeSummaryPlot(
			sigmas,
			sigmaErrs,
			planeNamesFilled,
			"sigma",
			charge,
			outDir,
			savePNG
		);

		fout->cd();

		if (cMean)
			cMean->Write();

		if (cSigma)
			cSigma->Write();
	}

	fout->Close();
	fin->Close();

	std::cout << "[PullSummaryPlots] Done. Summary saved to: "
	          << outName << std::endl;
}
