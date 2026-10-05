#include "PullComparePlots.C"

{
        gROOT->ProcessLine(".L PullComparePlots.C");
    

    std::vector<ComparisonSpec> specs = {{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "1", "Z=1"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "2", "Z=2"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "3", "Z=3"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "4", "Z=4"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "5", "Z=5"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "6", "Z=6"}};

    PullComparePlotsMulti(specs,"comparisonALLSFTWPOST.root", ".", false,"VT", true);
}
// This macro can be used as follows:
// root [0] .L PullComparePlots.C
// root [1] std::vector<ComparisonSpec> specs = {{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "1", "Z=1"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "2", "Z=2"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "3", "Z=3"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "4", "Z=4"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "5", "Z=5"},{"./8028_postAlig_qualityPlots/Decoded_Analysis_CNAO2025_8028_TWBarCalib_SIGNAL_FETA_ALL_events_new.root", "6", "Z=6"}};
// root [2]     PullComparePlotsMulti(specs,"comparisonALLSFTWPOST.root", ".", false,"VT", true);