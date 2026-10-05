/*!
 \file PullComparePlots.C

 \brief Compare the mean and sigma of Pull distributions from MULTIPLE
        ROOT files and MULTIPLE charges.

 Each comparison series is defined by:
     - ROOT file
     - charge
     - label

 The histograms are read from:

     QualityPlots/Exp_All/Pull/<plane>/h_Z_<charge>

 The Gaussian fit is identical to PullSummaryPlots.C:
     fit range = [mean - 1*RMS, mean + 1*RMS]
     option    = "R Q M"

 -------------------------------------------------------------------------------
 EXAMPLES
 -------------------------------------------------------------------------------

 1) Compare Z=0 in before.root with Z=0 in after.root:

 root -l
 .x PullComparePlots.C("before.root","after.root")

 This uses the convenience two-file interface.

 -------------------------------------------------------------------------------

 2) Compare DIFFERENT charges from DIFFERENT files:

root -l -b -q 'PullComparePlots.C("before.root","after.root","2","6","comparison.root",".",false,"VT","Z=2","Z=6",true)'
 
PullComparePlotsCharges(
    "before.root",       // first ROOT file
    "after.root",        // second ROOT file
    "0",                 // charge selected from first file: Z=0
    "1",                 // charge selected from second file: Z=1
    "comparison.root",   // output ROOT file
    ".",                 // directory for plots
    true,                // save SVG images
    "VT",                // only VT planes
    "Before Z=0",        // legend label for first series
    "After Z=1",         // legend label for second series
    true                 // draw fit errors
)
Here:
      before.root -> charge 0
      after.root  -> charge 1

 -------------------------------------------------------------------------------

 3) General multi-file / multi-charge comparison:

 Create a vector of ComparisonSpec objects:

     std::vector<ComparisonSpec> comparisons = {
         {"before.root", "0", "Before Z=0"},
         {"after.root",  "0", "After Z=0"},
         {"before.root", "1", "Before Z=1"},
         {"after.root",  "1", "After Z=1"}
     };

 Then:

     PullComparePlotsMulti(comparisons,
                            "pull_comparison.root",
                            ".",
                            true,
                            "VT",
                            true);

 This produces one plot containing all requested series.

 -------------------------------------------------------------------------------
 */

#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLine.h"
#include "TString.h"
#include "TStyle.h"

#include <vector>
#include <iostream>
#include <cstdio>
#include <cctype>
#include <algorithm>

// -----------------------------------------------------------------------
// Plane list
// -----------------------------------------------------------------------
static const std::vector<TString> kPlanes = {
    "VT0x", "VT0y",
    "VT1x", "VT1y",
    "VT2x", "VT2y",
    "VT3x", "VT3y",
    "MSD0", "MSD1", "MSD2", "MSD3", "MSD4", "MSD5",
    "TWx", "TWy"
};

// -----------------------------------------------------------------------
// Fit result
// -----------------------------------------------------------------------
struct FitResult
{
    Double_t mean = 0.;
    Double_t meanErr = 0.;

    Double_t sigma = 0.;
    Double_t sigmaErr = 0.;

    Bool_t valid = false;
};

// -----------------------------------------------------------------------
// One comparison series.
//
// Example:
//
//     {"before.root", "0", "Before Z=0"}
//
// -----------------------------------------------------------------------
struct ComparisonSpec
{
    TString fileName;
    TString charge;
    TString label;

    ComparisonSpec(const char *file,
                   const char *z,
                   const char *name)
        : fileName(file), charge(z), label(name)
    {}
};

// -----------------------------------------------------------------------
// Fit one pull histogram
// Same recipe as TANAactQualityPlots::GausFit
// -----------------------------------------------------------------------
FitResult FitPullHisto(TH1D *h, const TString &uniqueTag)
{
    FitResult res;

    if (!h || h->GetEntries() < 20)
        return res;

    Double_t amplitude = h->GetMaximum();
    Double_t mean      = h->GetMean();
    Double_t rms       = h->GetRMS();

    if (rms <= 0.)
        return res;

    Double_t fitMin = mean - rms;
    Double_t fitMax = mean + rms;

    TF1 *fGauss =
        new TF1("fGauss_" + uniqueTag,
                "gaus",
                fitMin,
                fitMax);

    fGauss->SetParameter(0, amplitude);
    fGauss->SetParameter(1, mean);
    fGauss->SetParameter(2, rms);

    h->Fit(fGauss, "R Q M", "", fitMin, fitMax);

    res.mean     = fGauss->GetParameter(1);
    res.meanErr  = fGauss->GetParError(1);

    res.sigma    = fGauss->GetParameter(2);
    res.sigmaErr = fGauss->GetParError(2);

    res.valid = true;

    delete fGauss;

    return res;
}

// -----------------------------------------------------------------------
// Fit all planes for one file + one charge
// -----------------------------------------------------------------------
std::vector<FitResult>
FitAllPlanes(TFile *f,
             const std::vector<TString> &planes,
             const TString &charge,
             const TString &fileTag)
{
    const TString basePath =
        "QualityPlots/Exp_All/Pull";

    std::vector<FitResult> out(planes.size());

    for (size_t i = 0; i < planes.size(); ++i)
    {
        TString fullPath =
            basePath + "/" +
            planes[i] +
            "/h_Z_" +
            charge;

        TH1D *h = (TH1D *)f->Get(fullPath);

        if (!h)
        {
            std::cout
                << "[PullComparePlots] Missing histogram ("
                << fileTag << "): "
                << fullPath
                << " -> skipped"
                << std::endl;

            continue;
        }

        out[i] =
            FitPullHisto(
                h,
                fileTag + "_" +
                planes[i] +
                "_Z" +
                charge);

        if (!out[i].valid)
        {
            std::cout
                << "[PullComparePlots] Not enough statistics ("
                << fileTag << "): "
                << fullPath
                << std::endl;
        }
    }

    return out;
}

// -----------------------------------------------------------------------
// Build a readable plane label.
//
// VT0x -> VT0x
// etc.
//
// The labels are intentionally kept as the actual plane names.
// -----------------------------------------------------------------------
TString PlaneLabel(const TString &plane)
{
    return plane;
}

// -----------------------------------------------------------------------
// Multi-series comparison plot.
//
// quantity = "mean" or "sigma"
//
// Every ComparisonSpec becomes one graph on the same canvas.
// -----------------------------------------------------------------------
TCanvas *
MakeMultiComparePlot(
    const std::vector<TString> &labels,
    const std::vector<ComparisonSpec> &specs,
    const std::vector<std::vector<FitResult> > &results,
    const TString &quantity,
    const TString &prefix,
    Bool_t drawErrors,
    const TString &outDir,
    Bool_t saveImages)
{
    const Int_t n = (Int_t)labels.size();

    if (specs.empty())
        return nullptr;

    const Bool_t isMean =
        (quantity == "mean");

    // ------------------------------------------------------------------
    // Create one graph per comparison series
    // ------------------------------------------------------------------
    std::vector<TGraphErrors *> graphs;

    const Int_t nSeries = (Int_t)specs.size();

    for (Int_t s = 0; s < nSeries; ++s)
    {
        TGraphErrors *g =
            new TGraphErrors();

        g->SetName(
            Form("g_%s_%d",
                 quantity.Data(),
                 s));

        // Spread points horizontally around the bin centre.
        //
        // For many series the spacing is automatically reduced.
        Double_t totalWidth = 0.70;

        Double_t dx = 0.;

        if (nSeries > 1)
            dx = totalWidth / (nSeries - 1);

        for (Int_t i = 0; i < n; ++i)
        {
            if (!results[s][i].valid)
                continue;

            Double_t value =
                isMean
                    ? results[s][i].mean
                    : results[s][i].sigma;

            Double_t error =
                isMean
                    ? results[s][i].meanErr
                    : results[s][i].sigmaErr;

            Double_t x;

            if (nSeries == 1)
            {
                x = i + 0.5;
            }
            else
            {
                x =
                    i + 0.5
                    - totalWidth / 2.
                    + s * dx;
            }

            Int_t p = g->GetN();

            g->SetPoint(p, x, value);

            g->SetPointError(
                p,
                0.,
                drawErrors ? error : 0.);
        }

        graphs.push_back(g);
    }

    // ------------------------------------------------------------------
    // Find Y range
    // ------------------------------------------------------------------
    Double_t ymin = 1e30;
    Double_t ymax = -1e30;

    for (Int_t s = 0; s < nSeries; ++s)
    {
        for (Int_t i = 0; i < n; ++i)
        {
            if (!results[s][i].valid)
                continue;

            Double_t value =
                isMean
                    ? results[s][i].mean
                    : results[s][i].sigma;

            Double_t error =
                isMean
                    ? results[s][i].meanErr
                    : results[s][i].sigmaErr;

            Double_t e =
                drawErrors ? error : 0.;

            ymin = std::min(ymin, value - e);
            ymax = std::max(ymax, value + e);
        }
    }

    if (ymin > ymax)
        return nullptr;

    // Reference:
    //
    // mean  -> 0
    // sigma -> 1
    //
    const Double_t ref =
        isMean ? 0. : 1.;

    ymin = std::min(ymin, ref);
    ymax = std::max(ymax, ref);

    Double_t range = ymax - ymin;

    if (range <= 0.)
        range = 1.;

    Double_t lo =
        ymin - 0.15 * range;

    Double_t hi =
        ymax + 0.40 * range;

    // ------------------------------------------------------------------
    // Frame
    // ------------------------------------------------------------------
    TString title =
        Form("Pull comparison - %s",
             isMean ? "#mu" : "#sigma");

    if (!prefix.IsNull())
        title += " / " + prefix;

    TH1D *frame =
        new TH1D(
            Form("frame_%s_%s",
                 quantity.Data(),
                 prefix.Data()),
            title,
            n,
            0,
            n);

    frame->SetStats(0);

    for (Int_t i = 0; i < n; ++i)
    {
        frame->GetXaxis()->SetBinLabel(
            i + 1,
            labels[i]);
    }

    frame->GetXaxis()->LabelsOption("h");
    frame->GetXaxis()->SetLabelSize(0.045);
    frame->GetXaxis()->SetTitle("");

    frame->GetYaxis()->SetTitle(
        isMean
            ? "#mu_{pull}"
            : "#sigma_{pull}");

    frame->GetYaxis()->SetTitleSize(0.05);
    frame->GetYaxis()->SetTitleOffset(0.9);

    frame->SetMinimum(lo);
    frame->SetMaximum(hi);

    // ------------------------------------------------------------------
    // Canvas
    // ------------------------------------------------------------------
    TString canvasName =
        Form("c_%s_compare_%s",
             quantity.Data(),
             prefix.IsNull()
                 ? "All"
                 : prefix.Data());

    TCanvas *c =
        new TCanvas(
            canvasName,
            title,
            850,
            650);

    c->SetLeftMargin(0.13);
    c->SetBottomMargin(0.12);
    c->SetGridy();

    frame->Draw("AXIS");

    // ------------------------------------------------------------------
    // Reference line
    // ------------------------------------------------------------------
    TLine *lref =
        new TLine(
            0,
            ref,
            n,
            ref);

    lref->SetLineWidth(2);
    lref->SetLineColor(kBlack);
    lref->Draw();

    // ------------------------------------------------------------------
    // Styles
    //
    // ROOT has a finite number of pleasant basic colours.
    // Cycle through them if there are many series.
    // ------------------------------------------------------------------
    const Int_t colors[] = {
        kBlue + 1,
        kRed + 1,
        kGreen + 2,
        kMagenta + 1,
        kOrange + 7,
        kCyan + 1,
        kViolet + 1,
        kAzure + 2
    };

    const Int_t markers[] = {
        20, 21, 22, 23, 29, 33, 34, 47
    };

    const Int_t nStyles =
        sizeof(colors) / sizeof(colors[0]);

    // ------------------------------------------------------------------
    // Draw graphs
    // ------------------------------------------------------------------
    for (Int_t s = 0; s < nSeries; ++s)
    {
        TGraphErrors *g =
            graphs[s];

        Int_t styleIndex =
            s % nStyles;

        g->SetMarkerStyle(
            markers[styleIndex]);

        g->SetMarkerSize(1.5);

        g->SetMarkerColor(
            colors[styleIndex]);

        g->SetLineColor(
            colors[styleIndex]);

        g->SetLineWidth(2);

        if (drawErrors)
            g->Draw("P");
        else
            g->Draw("PX");
    }

    // ------------------------------------------------------------------
    // Legend
    // ------------------------------------------------------------------
    Double_t legendHeight =
        0.055 * std::min(nSeries, 5);

    Double_t y1 =
        0.88 - legendHeight;

    if (y1 < 0.50)
        y1 = 0.50;

    TLegend *leg =
        new TLegend(
            0.15,
            y1,
            0.55,
            0.88);

    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.038);

    for (Int_t s = 0; s < nSeries; ++s)
    {
        leg->AddEntry(
            graphs[s],
            specs[s].label,
            drawErrors ? "pe" : "lp");
    }

    leg->Draw();

    c->Modified();
    c->Update();

    // ------------------------------------------------------------------
    // Save SVG
    // ------------------------------------------------------------------
    if (saveImages)
    {
        TString outName =
            outDir + "/" +
            c->GetName() +
            ".svg";

        c->SaveAs(outName);
    }

    return c;
}

// -----------------------------------------------------------------------
// Print comparison table
// -----------------------------------------------------------------------
void PrintComparisonTable(
    const std::vector<TString> &planes,
    const std::vector<ComparisonSpec> &specs,
    const std::vector<std::vector<FitResult> > &results)
{
    std::cout << "\n";
    std::cout
        << "============================================================================================\n";

    std::cout
        << " Pull comparison\n";

    std::cout
        << "============================================================================================\n";

    for (size_t s = 0; s < specs.size(); ++s)
    {
        std::cout
            << "  [" << s << "] "
            << specs[s].label
            << "   file = "
            << specs[s].fileName
            << "   charge = Z="
            << specs[s].charge
            << "\n";
    }

    std::cout
        << "--------------------------------------------------------------------------------------------\n";

    // Header
    printf("%-8s", "Plane");

    for (size_t s = 0; s < specs.size(); ++s)
    {
        printf(" | %-20s", specs[s].label.Data());
    }

    std::cout << "\n";

    std::cout
        << "--------------------------------------------------------------------------------------------\n";

    // Mean + sigma
    for (size_t i = 0; i < planes.size(); ++i)
    {
        printf("%-8s", planes[i].Data());

        for (size_t s = 0; s < specs.size(); ++s)
        {
            if (!results[s][i].valid)
            {
                printf(" | %-20s", "n/a");
                continue;
            }

            TString text =
                Form("%7.4f +- %6.4f",
                     results[s][i].mean,
                     results[s][i].meanErr);

            printf(
                " | %-20s",
                text.Data());
        }

        std::cout << "\n";
    }

    std::cout
        << "--------------------------------------------------------------------------------------------\n";

    std::cout
        << "Sigma:\n";

    for (size_t i = 0; i < planes.size(); ++i)
    {
        printf("%-8s", planes[i].Data());

        for (size_t s = 0; s < specs.size(); ++s)
        {
            if (!results[s][i].valid)
            {
                printf(" | %-20s", "n/a");
                continue;
            }

            TString text =
                Form("%7.4f +- %6.4f",
                     results[s][i].sigma,
                     results[s][i].sigmaErr);

            printf(
                " | %-20s",
                text.Data());
        }

        std::cout << "\n";
    }

    std::cout
        << "============================================================================================\n";
}

// -----------------------------------------------------------------------
// MAIN MULTI-COMPARISON FUNCTION
// -----------------------------------------------------------------------
void PullComparePlotsMulti(
    const std::vector<ComparisonSpec> &specs,
    const char *outputFile = "pull_comparison.root",
    const char *outDir = ".",
    Bool_t saveImages = false,
    const char *planePrefix = "VT",
    Bool_t drawErrors = true)
{
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);

    gStyle->SetGridStyle(3);
    gStyle->SetGridColor(kBlack);
    gStyle->SetGridWidth(2);

    if (specs.empty())
    {
        std::cerr
            << "[PullComparePlots] ERROR: "
            << "no comparison series specified."
            << std::endl;

        return;
    }

    // ------------------------------------------------------------------
    // Open all ROOT files
    // ------------------------------------------------------------------
    std::vector<TFile *> files;

    for (size_t i = 0; i < specs.size(); ++i)
    {
        TFile *f =
            TFile::Open(
                specs[i].fileName,
                "READ");

        if (!f || f->IsZombie())
        {
            std::cerr
                << "[PullComparePlots] ERROR: "
                << "cannot open "
                << specs[i].fileName
                << std::endl;

            // Close files already opened
            for (TFile *ff : files)
            {
                if (ff)
                    ff->Close();
            }

            return;
        }

        files.push_back(f);
    }

    // ------------------------------------------------------------------
    // Select planes
    // ------------------------------------------------------------------
    TString prefix =
        planePrefix;

    std::vector<TString> planes;
    std::vector<TString> labels;

    for (const auto &p : kPlanes)
    {
        if (prefix.IsNull() ||
            p.BeginsWith(prefix))
        {
            planes.push_back(p);
            labels.push_back(
                PlaneLabel(p));
        }
    }

    if (planes.empty())
    {
        std::cerr
            << "[PullComparePlots] ERROR: "
            << "no plane matches prefix '"
            << prefix
            << "'"
            << std::endl;

        for (TFile *f : files)
            f->Close();

        return;
    }

    // ------------------------------------------------------------------
    // Fit every file/charge combination
    // ------------------------------------------------------------------
    std::vector<std::vector<FitResult> > results;

    for (size_t s = 0; s < specs.size(); ++s)
    {
        TString tag =
            Form("series%zu_%s_Z%s",
                 s,
                 specs[s].label.Data(),
                 specs[s].charge.Data());

        std::vector<FitResult> r =
            FitAllPlanes(
                files[s],
                planes,
                specs[s].charge,
                tag);

        results.push_back(r);
    }

    // ------------------------------------------------------------------
    // Print table
    // ------------------------------------------------------------------
    PrintComparisonTable(
        planes,
        specs,
        results);

    // ------------------------------------------------------------------
    // Create output file
    // ------------------------------------------------------------------
    TFile *fout =
        TFile::Open(
            outputFile,
            "RECREATE");

    if (!fout || fout->IsZombie())
    {
        std::cerr
            << "[PullComparePlots] ERROR: "
            << "cannot create output file "
            << outputFile
            << std::endl;

        for (TFile *f : files)
            f->Close();

        return;
    }

    // ------------------------------------------------------------------
    // Mean comparison
    // ------------------------------------------------------------------
    TCanvas *cMean =
        MakeMultiComparePlot(
            labels,
            specs,
            results,
            "mean",
            prefix,
            drawErrors,
            outDir,
            saveImages);

    // ------------------------------------------------------------------
    // Sigma comparison
    // ------------------------------------------------------------------
    TCanvas *cSigma =
        MakeMultiComparePlot(
            labels,
            specs,
            results,
            "sigma",
            prefix,
            drawErrors,
            outDir,
            saveImages);

    // ------------------------------------------------------------------
    // Save canvases
    // ------------------------------------------------------------------
    fout->cd();

    if (cMean)
        cMean->Write();

    if (cSigma)
        cSigma->Write();

    fout->Close();

    // ------------------------------------------------------------------
    // Close input files
    // ------------------------------------------------------------------
    for (TFile *f : files)
        f->Close();

    std::cout
        << "[PullComparePlots] Done."
        << std::endl;

    std::cout
        << "Comparison saved to: "
        << outputFile
        << std::endl;
}

// -----------------------------------------------------------------------
// CONVENIENCE FUNCTION
//
// Keeps your original two-file syntax:
//
//     .x PullComparePlots.C("before.root","after.root")
//
// By default it compares:
//
//     before.root -> Z=0
//     after.root  -> Z=0
//
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// MAIN ENTRY POINT
//
// Compare arbitrary charges from TWO ROOT files.
//
// Example:
//
// root -l -b -q 'PullComparePlots.C("before.root","after.root","2","6","comparison.root",".",true,"VT","Z=2","Z=6",true)'
//
// This compares:
//
//     before.root -> h_Z_2
//     after.root  -> h_Z_6
//
// for every selected plane.
//
// -----------------------------------------------------------------------
void PullComparePlots(
    const char *fileBefore,
    const char *fileAfter,
    const char *chargeBefore = "0",
    const char *chargeAfter = "0",
    const char *outputFile = "",
    const char *outDir = ".",
    Bool_t saveImages = false,
    const char *planePrefix = "VT",
    const char *labelBefore = "Before",
    const char *labelAfter = "After",
    Bool_t drawErrors = true)
{
    TString outName = outputFile;

    if (outName.IsNull())
    {
        outName = fileAfter;

        // Remove .root if present
        if (outName.EndsWith(".root"))
            outName.ReplaceAll(".root", "");

        outName +=
            Form("_Z%s_vs_Z%s_pullcompare.root",
                 chargeBefore,
                 chargeAfter);
    }

    std::vector<ComparisonSpec> specs = {

        ComparisonSpec(
            fileBefore,
            chargeBefore,
            labelBefore),

        ComparisonSpec(
            fileAfter,
            chargeAfter,
            labelAfter)
    };

    PullComparePlotsMulti(
        specs,
        outName.Data(),
        outDir,
        saveImages,
        planePrefix,
        drawErrors);
}


// -----------------------------------------------------------------------
// OPTIONAL CONVENIENCE FUNCTION
//
// Compare MORE THAN TWO file/charge combinations.
//
// Example:
//
// std::vector<ComparisonSpec> specs = {
//     {"runA.root", "0", "Run A Z=0"},
//     {"runA.root", "1", "Run A Z=1"},
//     {"runB.root", "2", "Run B Z=2"},
//     {"runC.root", "6", "Run C Z=6"}
// };
//
// PullComparePlotsMulti(
//     specs,
//     "comparison.root",
//     ".",
//     true,
//     "VT",
//     true
// );
//
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// SPECIAL CONVENIENCE FUNCTION
//
// Compare arbitrary charges between TWO files.
//
// Example:
//
//     .x PullComparePlotsCharges(
//          "before.root",
//          "after.root",
//          "0",
//          "1",
//          "comparison.root",
//          ".",
//          true,
//          "VT",
//          "Before Z=0",
//          "After Z=1",
//          true)
//
// -----------------------------------------------------------------------
void PullComparePlotsCharges(
    const char *file1,
    const char *file2,
    const char *charge1,
    const char *charge2,
    const char *outputFile = "pull_charge_comparison.root",
    const char *outDir = ".",
    Bool_t saveImages = false,
    const char *planePrefix = "VT",
    const char *label1 = "File 1",
    const char *label2 = "File 2",
    Bool_t drawErrors = true)
{
    std::vector<ComparisonSpec> specs = {
        ComparisonSpec(
            file1,
            charge1,
            label1),

        ComparisonSpec(
            file2,
            charge2,
            label2)
    };

    PullComparePlotsMulti(
        specs,
        outputFile,
        outDir,
        saveImages,
        planePrefix,
        drawErrors);
}
