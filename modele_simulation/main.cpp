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
    vector<double> puissance(dureeSim);
    for (int i(0); i < dureeSim; i++) {
        puissance[i] = 0.0;
    }
    
    return puissance;
}

void modele(int dureeSimulation, int deltaT, int tailleEchantillon, vector<double> &puissanceReseau) { // dureeSimulation se mesure en deltaT
    //vector<double> puissanceReseau = initPuissanceReseau(dureeSimulation);
    cout << "Init" << endl;
    for (int i = 0; i < tailleEchantillon; i++) {
        cout << "VE " << (i+1) << ". " << endl;
        Vehicule *vehicule = new Vehicule(deltaT, true);
        int temps = 0;
        while(temps < dureeSimulation) {
            puissanceReseau[temps] += vehicule->simulation(temps, deltaT);
            temps++;
        }
        delete vehicule;
    }
}

int main(int argc, const char * argv[]) {
    if (argc < 4) {
        cout << "Usage : modele_simulation deltaT duree taille" << endl;
        return -1;
    }
    
    int duree = 1000;
    int deltaT = 10;
    int taille = 100;
    
    try {
        deltaT = boost::lexical_cast<int>(argv[1]);
        cout << "deltaT = " << deltaT << " minutes" << endl;
        try {
            duree = boost::lexical_cast<int>(argv[2]);
            int nbJour = duree*deltaT/1440;
            int nbHeure = (duree*deltaT - nbJour*1440) / 60;
            int nbMinute = (duree*deltaT - nbJour*1440 - nbHeure*60);
            cout << "Durée = ";
            if (nbJour > 0)
                cout << nbJour << " j ";
            cout << nbHeure << " h " << nbMinute << endl;
            try {
                taille = boost::lexical_cast<int>(argv[3]);
                cout << "Nb de VE = " << taille << endl;
                
                cout << endl;
                
                vector<double> puissanceReseau(duree, 0.0);
                modele(duree, deltaT, taille, puissanceReseau);
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
