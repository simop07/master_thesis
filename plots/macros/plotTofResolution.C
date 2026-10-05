#include <TCanvas.h>
#include <TGraph.h>
#include <TF1.h>
#include <TLegend.h>
#include <TAxis.h>
#include <TStyle.h>

void plotTofResolution()
{
    // Canvas setup with margins matching ROOT style
    TCanvas *c1 = new TCanvas("c1", "TOF Resolution", 800, 600);
    c1->SetTopMargin(0.05);
    c1->SetRightMargin(0.05);
    c1->SetLeftMargin(0.13);
    c1->SetBottomMargin(0.13);

    // Estimated data points from the image
    double x_data[] = {7.0, 68.0, 84.0, 160.0};
    double y_data[] = {0.267, 0.076, 0.069, 0.056};
    int n_points = 4;

    TGraph *gr = new TGraph(n_points, x_data, y_data);
    gr->SetMarkerStyle(20); // Circle marker
    gr->SetMarkerSize(1.3);
    gr->SetMarkerColor(kBlack);

    // Axis titles using ROOT TLatex formatting
    gr->SetTitle(";\\mu(\\Delta E) [MeV];\\sigma(TOF) [ns]");
    gr->GetXaxis()->SetLimits(0.0, 200.0);
    gr->GetHistogram()->SetMinimum(0.0);
    gr->GetHistogram()->SetMaximum(0.28);

    // Axis styling
    gr->GetXaxis()->SetTitleSize(0.05);
    gr->GetXaxis()->SetLabelSize(0.045);
    gr->GetYaxis()->SetTitleSize(0.05);
    gr->GetYaxis()->SetLabelSize(0.045);
    gr->GetYaxis()->SetTitleOffset(1.1);

    // Define function from snippet
    TF1 *fTofResolution = new TF1("fTofResolution", "sqrt([0]/x + [1])", 1.0, 175.0);
    fTofResolution->SetParameter(0, 0.38);
    fTofResolution->SetParameter(1, 5.2e-4);
    fTofResolution->SetLineColor(kBlue + 1);
    fTofResolution->SetLineWidth(2);

    // Draw graph and function
    gr->Draw("AP");
    fTofResolution->Draw("SAME");

    // Add legend matching the reference image layout
    TLegend *leg = new TLegend(0.32, 0.72, 0.88, 0.90);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.042);
    leg->AddEntry(gr, "DATA", "p");
    leg->AddEntry(fTofResolution, "Parameterization y = #sqrt{#frac{A}{x} + B}", "l");
    leg->Draw();

    c1->Update();
}