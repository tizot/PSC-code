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
#include <boost/lexical_cast.hpp>

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
        Vehicule *vehicule = new Vehicule(deltaT);
        cout << "Véhicule " << (i+1) << endl;
        int temps(0);
        while(temps < dureeSimulation) {
            if (temps > 0 && ((temps * deltaT) % 1440 == 0)) {
                vehicule->reinitJour(); // on réinitialise certaines données à 00h00 chaque jour
            }
            vehicule->simulation(temps, deltaT, puissanceReseau);
            temps++;
        }
        delete vehicule;
    }
    
    return puissanceReseau;
}

int main(int argc, const char * argv[]) {
    if (argc < 4) {
        cout << "Usage : modele_simulation deltaT duree taille" << endl;
        return -1;
    }
    
    try {
        int deltaT = boost::lexical_cast<int>(argv[1]);
        cout << "deltaT = " << deltaT << " minutes" << endl;
        try {
            int duree = boost::lexical_cast<int>(argv[2]);
            int nbJour = duree*deltaT/1440;
            int nbHeure = (duree*deltaT - nbJour*1440) / 60;
            int nbMinute = (duree*deltaT - nbJour*1440 - nbHeure*60);
            cout << "Durée = ";
            if (nbJour > 0)
                cout << nbJour << "j";
            cout << nbHeure << "h" << nbMinute << endl;
            try {
                int taille = boost::lexical_cast<int>(argv[3]);
                cout << "Nb de VE = " << taille << endl;
                
                cout << endl;
                
                vector<double> puissanceReseau = modele(duree, deltaT, taille);
                for (int i(0); i < duree; i++) {
                    cout << i << " : " << puissanceReseau[i] << endl;
                }
                cout << endl;
                
                cout << "Terminé avec +/- de succès..." << endl;
                return 0;
            } catch (const boost::bad_lexical_cast &) {
                cerr << "La taille doit être un nombre entiers de VE" << endl;
            }
        } catch (const boost::bad_lexical_cast &) {
            cerr << "La durée doit être un nombre entiers de deltaT" << endl;
        }
        
    } catch (const boost::bad_lexical_cast &) {
        cerr << "deltaT doit être un nombre entiers de minutes" << endl;
    }
}
