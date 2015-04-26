//
//  main.cpp
//  modele_simulation
//
//  Created by Camille MASSET on 09/03/2015.
//  Copyright (c) 2015 PSC. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "Vehicule.h"
#include <vector>
#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>
#include "gnuplot_i.hpp"

using namespace std;

// FONCTIONS
vector<double> initPuissanceReseau(int dureeSim) {
    vector<double> puissance(dureeSim);
    for (int i(0); i < dureeSim; i++) {
        puissance[i] = 0.0;
    }

    return puissance;
}

void modele(int dureeSimulation, int deltaT, int tailleEchantillon, int useCase, vector<double> &puissanceReseau) { // dureeSimulation se mesure en deltaT
    //cout << "Init" << endl;
    for (int i = 0; i < tailleEchantillon; i++) {
        cout << "Véhicule " << (i+1) << " sur " << boost::lexical_cast<std::string>(tailleEchantillon) << ".          \r" << flush;
        Vehicule *vehicule = new Vehicule(deltaT, false);
        //cout << "Véhicule " << (i+1) << endl;
        //vehicule->printInfos(deltaT);
        int temps = 0;
        while(temps < dureeSimulation) {
            puissanceReseau[temps] += vehicule->simulation(temps, deltaT, useCase);
            temps++;
        }
        delete vehicule;
    }
}

int main(int argc, const char * argv[]) {
    double t1, t2;
    t1 = clock();
    if (argc < 5) {
        cout << "Usage : modele_simulation deltaT(min) duree(j) taille iterations" << endl;
        return -1;
    }

    int duree = 2; // en jours
    int deltaT = 10; // in minutes
    int taille = 100;
    int iterations = 10;

    try {
        deltaT = boost::lexical_cast<int>(argv[1]);
        cout << "deltaT = " << deltaT << " min" << endl;
    } catch (const boost::bad_lexical_cast &) {
        cerr << "deltaT doit être un nombre entier de minutes." << endl;
        cout << "Valeur par défaut utilisée : 10 min." << endl;
    }

    try {
        duree = boost::lexical_cast<int>(argv[2]);
        cout << "Durée = " << duree << " j " << endl;
    } catch (const boost::bad_lexical_cast &) {
        cerr << "La durée doit être un nombre entier de jours." << endl;
        cout << "Valeur par défaut utilisée : 2 jours." << endl;
    }

    try {
        taille = boost::lexical_cast<int>(argv[3]);
        cout << "Nb de VE = " << taille << endl;
    } catch (const boost::bad_lexical_cast &) {
        cerr << "La taille doit être un nombre entier de VE." << endl;
        cout << "Valeur par défaut utilisée : 100." << endl;
    }

    try {
        iterations = boost::lexical_cast<int>(argv[4]);
        cout << "Nb d'itérations = " << iterations << endl;
    } catch (const boost::bad_lexical_cast &) {
        cerr << "Le nombre d'itérations doit être un entier strictement positif." << endl;
        cout << "Valeur par défaut utilisée : 10." << endl;
    }

    cout << endl;

    cout << "Choisir un Use Case : \n";
    cout << "  1. Pas de SmartGrid : on charge tant qu'on peut et on ne s'arrête que quand le véhicule est complètement chargé.\n";
    cout << "  2. On ne recharge pas entre 18h et 21h.\n";
    cout << "  3. On ne recharge pas entre 18h et 21h et on fait du Vehicle2Grid.";
    cout << endl << "Votre choix : ";

    string useCase_str;
    int useCase;
    cin >> useCase_str;
    try {
        useCase = boost::lexical_cast<int>(useCase_str) - 1;
        if (useCase < 0 || useCase > 2)
            useCase = 0;
    } catch (const boost::bad_lexical_cast &) {
        useCase = 0;
    }

    cout << endl << "Use Case choisi : " << (useCase + 1) << endl << endl;

    const int dureeDeltaT = duree * 1440 / deltaT;
    vector<double> puissanceReseau(dureeDeltaT, 0.0);
    vector<double> puissanceMoyenne(dureeDeltaT, 0.0);
    for (int j(0); j < iterations; j++) {
        cout << "\033[FItération " << (j+1) << " sur " << boost::lexical_cast<std::string>(iterations) << ".                    \n" << flush;
        modele(dureeDeltaT, deltaT, taille, useCase, puissanceReseau);
        for (int i(0); i < dureeDeltaT; i++) {
            puissanceMoyenne[i] += puissanceReseau[i] / (double)iterations;
            puissanceReseau[i] = 0.0;
        }
    }

    cout << endl << "Calculs terminés" << endl;

    // On exporte le dernier jour entier
    string fileName = "courbe_deltaT" + boost::lexical_cast<std::string>(deltaT) + "_duree" + boost::lexical_cast<std::string>(duree) + "_taille" + boost::lexical_cast<std::string>(taille) + "_iterations" + boost::lexical_cast<std::string>(iterations) + "_usecase" + useCase_str + "";
    int index_min = (duree-1) * 1440 / deltaT;
    int index_max = duree * 1440 / deltaT;
    ofstream donnees(fileName + ".csv");

    if (donnees) {
        double tps = 0.0;
        for (int i = index_min; i < index_max; i++) {
            tps = (((i - index_min) * deltaT) % 1440) / 60.0;
            donnees << tps << "\t" << puissanceMoyenne[i] << endl;
        }
        cout << "Ecriture terminée" << endl;
    } else {
        cerr << "ERREUR : impossible d'ouvrir le fichier 'puissance.dat'." << endl;
    }

    try {
        Gnuplot::set_terminal_std("postscript");
        Gnuplot g1("courbe");
        g1.reset_all();
        const string title = "PUISSANCE NECESSAIRE A LA RECHARGE DE " + boost::lexical_cast<std::string>(taille) + " VE";
        g1.set_title(title);
        g1.set_xlabel("Temps (h)");
        g1.set_ylabel("Puissance (kW)");

        vector<double> x, y;
        double tps = 0.0;
        for (int i = index_min; i < index_max; i++) {
            tps = (((i - index_min) * deltaT) % 1440) / 60.0;
            x.push_back(tps);
            y.push_back(puissanceMoyenne[i]);
        }

        g1.set_xrange(0.0, 24.0);
        g1.savetops(fileName);
        g1.set_style("histeps").plot_xy(x, y, "");

        cout << endl << "Graphique terminé" << endl;
    } catch (GnuplotException ge) {
        cout << ge.what() << endl;
    }

    t2 = clock();

    cout << "Terminé en " << (t2 - t1)/float(CLOCKS_PER_SEC) << " s" << endl;
    return 0;
}
