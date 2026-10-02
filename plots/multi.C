#include "PullComparePlots.C"

{
        gROOT->ProcessLine(".L PullComparePlots.C");
    

    std::vector<ComparisonSpec> specs = {{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "1", "Z=1"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "2", "Z=2"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "3", "Z=3"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "4", "Z=4"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "5", "Z=5"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_fEtaERROR_ANALYSIS.root", "6", "Z=6"}};

    PullComparePlotsMulti(specs,"comparisonMSDNOTORIGINAL.root", ".", false,"MSD", true);
}