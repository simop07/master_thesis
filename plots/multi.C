#include "PullComparePlots.C"

{
        gROOT->ProcessLine(".L PullComparePlots.C");
    

    std::vector<ComparisonSpec> specs = {{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "1", "Z=1"},{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "2", "Z=2"},{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "3", "Z=3"},{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "4", "Z=4"},{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "5", "Z=5"},{"./8028_postAlig_qualityPlots_OriginalMSDPos/Decoded_Analysis_CNAO2025_8028.root", "6", "Z=6"}};

    PullComparePlotsMulti(
        specs,
        "comparisonMSD.root",
        ".",
        false,
        "MSD",
        true
    );
}