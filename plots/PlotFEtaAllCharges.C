// PlotFEtaAllCharges.C
//
// Legge il file "Decoded_Analysis_CNAO2025_8028_TWBarCalib_ALL_PER_CHARGE.root",
// entra nelle cartelle Z_1 ... Z_6 (una per carica), recupera l'istogramma
// "hEtaVsX_EtaTot" per ciascuna carica, estrae la funzione di fit associata
// (fFunct, o comunque la prima funzione trovata nella lista) e la disegna
// su un unico canvas, una curva per carica.
//
// Uso:
//   root -l -q 'PlotFEtaAllCharges.C("Decoded_Analysis_CNAO2025_8028_TWBarCalib_ALL_PER_CHARGE.root")'
//
// oppure, se il file si chiama esattamente come sopra ed e' nella stessa
// cartella della macro:
//   root -l -q PlotFEtaAllCharges.C

#include "TFile.h"
#include "TDirectory.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TList.h"
#include "TString.h"
#include "TKey.h"
#include "TSystem.h"
#include <iostream>

// Cerca ricorsivamente, a partire da 'top', una sottocartella con nome 'name'.
// Ritorna il puntatore alla TDirectory se trovata, altrimenti nullptr.
TDirectory* FindSubDir(TDirectory* top, const char* name)
{
    if (!top) return nullptr;

    TObject* obj = top->Get(name);
    if (obj && obj->InheritsFrom(TDirectory::Class())) {
        return (TDirectory*) obj;
    }

    TList* keys = top->GetListOfKeys();
    if (!keys) return nullptr;

    TIter next(keys);
    TKey* key;
    while ((key = (TKey*) next())) {
        if (TString(key->GetClassName()).Contains("TDirectory")) {
            TDirectory* sub = (TDirectory*) top->Get(key->GetName());
            if (!sub) continue;
            if (TString(sub->GetName()) == name) return sub;
            TDirectory* found = FindSubDir(sub, name);
            if (found) return found;
        }
    }
    return nullptr;
}

void PlotFEtaAllCharges(const char* filename = "Decoded_Analysis_CNAO2025_8028_TWBarCalib_ALL_PER_CHARGE.root",
                         const char* histoName = "hEtaVsX_EtaTot",
                         const char* funcName  = "fFunct",
                         int chargeMin = 1,
                         int chargeMax = 6,
                         const char* topDirName = "MSDetaFunc")
{
    TFile* f = TFile::Open(filename, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "ERRORE: impossibile aprire il file " << filename << std::endl;
        return;
    }

    // Cartella principale che contiene Z_1 ... Z_6 (es. "MSDetaFunc")
    TDirectory* topDir = (TDirectory*) f->Get(topDirName);
    if (!topDir) {
        std::cerr << "Attenzione: cartella top-level '" << topDirName
                   << "' non trovata, cerco Z_q direttamente nel file." << std::endl;
        topDir = f;
    }

    // Canvas finale con tutte le funzioni sovrapposte
    TCanvas* cAll = new TCanvas("cAllFEta", "f(#eta) per tutte le cariche", 900, 700);
    cAll->SetGrid();

    TLegend* leg = new TLegend(0.15, 0.65, 0.4, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);

    int colors[6] = {kBlack, kRed+1, kBlue+1, kGreen+2, kMagenta+1, kOrange+1};

    bool firstDrawn = false;

    for (int q = chargeMin; q <= chargeMax; ++q) {

        TString dirName = TString::Format("Z_%d", q);

        // Prima cerco dentro topDir (es. MSDetaFunc/Z_q), poi in tutto il file
        TDirectory* dir = FindSubDir(topDir, dirName);
        if (!dir) dir = FindSubDir(f, dirName);

        if (!dir) {
            std::cerr << "Attenzione: cartella " << dirName << " non trovata, salto." << std::endl;
            continue;
        }

        TH1* h = (TH1*) dir->Get(histoName);
        if (!h) {
            std::cerr << "Attenzione: istogramma " << histoName
                       << " non trovato in " << dirName << ", salto." << std::endl;
            continue;
        }

        // Prova a prendere la funzione per nome, altrimenti la prima disponibile
        TF1* fFit = h->GetFunction(funcName);
        if (!fFit) {
            TList* funcList = h->GetListOfFunctions();
            if (funcList && funcList->GetSize() > 0) {
                fFit = dynamic_cast<TF1*>(funcList->At(0));
            }
        }

        if (!fFit) {
            std::cerr << "Attenzione: nessuna funzione di fit trovata per " << dirName << std::endl;
            continue;
        }

        // Clono la funzione cosi' posso ridisegnarla liberamente sul canvas comune
        TF1* fClone = (TF1*) fFit->Clone(TString::Format("fFEta_charge%d", q));
        fClone->SetLineColor(colors[(q - 1) % 6]);
        fClone->SetLineWidth(2);
        fClone->SetLineStyle(1);

        cAll->cd();
        if (!firstDrawn) {
            fClone->SetTitle("f(#eta) - confronto tra cariche;#eta;x_{n}");
            fClone->Draw();
            firstDrawn = true;
        } else {
            fClone->Draw("same");
        }

        leg->AddEntry(fClone, TString::Format("Carica %d", q), "l");

        std::cout << "Carica " << q << ": funzione '" << fClone->GetName()
                  << "' aggiunta al plot." << std::endl;
    }

    if (!firstDrawn) {
        std::cerr << "ERRORE: nessuna funzione trovata per nessuna carica, nulla da disegnare." << std::endl;
        return;
    }

    leg->Draw();
    cAll->Update();

    // Salvo il canvas nella stessa cartella del file .root di input
    TString inputDir = gSystem->DirName(filename);
    TString outPath = (inputDir == ".") ? TString("fEta_allCharges.root")
                                         : inputDir + "/fEta_allCharges.root";
    cAll->SaveAs(outPath);

    std::cout << "Fatto. Canvas salvato come " << outPath << std::endl;
}