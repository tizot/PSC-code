//
//  main.cpp
//  modele_simulation
//
//  Created by Camille MASSET on 09/03/2015.
//  Copyright (c) 2015 PSC. All rights reserved.
//

#include <iostream>
#include "Vehicule.h"
#include <vector>
#include <boost/random.hpp>

using namespace std;

// FONCTIONS
vector<double> initPuissanceReseau(int dureeSim) {
    vector<double> puissance;
    for (int i(0); i < dureeSim; i++) {
        puissance.push_back(0.0);
    }
    
    return puissance;
}

vector<Vehicule> initialisationFlotte(int tailleEchantillon, int deltaT) {
    vector<Vehicule> flotte;
    for (int i(0); i < tailleEchantillon; i++) {
        flotte.push_back(Vehicule(deltaT));
    }
    
    return flotte;
}

vector<double> modele(int dureeSimulation, int deltaT, int tailleEchantillon) { // dureeSimulation se mesure en deltaT
    vector<Vehicule> flotte = initialisationFlotte(tailleEchantillon, deltaT);
    vector<double> puissanceReseau = initPuissanceReseau(dureeSimulation);
    
    for (int i(0); i < tailleEchantillon; i++) {
        Vehicule vehicule = flotte[i];
//        vehicule.printInfos();
        int temps(0);
        while(temps < dureeSimulation) {
            if (temps > 0 && ((temps * deltaT) % 86400 == 0)) {
                vehicule.reinitJour(); // on réinitialise certaines données à 00h00 chaque jour
            }
            vehicule.simulation(temps, deltaT, puissanceReseau);
            cout << endl << "Temps : " << temps << endl;
            vehicule.printInfos();
            cout << endl;
            temps++;
        }
    }
    
    return puissanceReseau;
}

int main(int argc, const char * argv[]) {
    vector<double> puissanceReseau = modele(92, 15, 1);
    
    return 0;
}
