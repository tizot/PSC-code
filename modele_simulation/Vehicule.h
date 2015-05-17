//
//  Vehicule.h
//  modele_simulation
//
//  Created by Camille MASSET on 09/03/2015.
//  Copyright (c) 2015 PSC. All rights reserved.
//

#ifndef __modele_simulation__Vehicule__
#define __modele_simulation__Vehicule__

#include <stdio.h>
#include <vector>

class Vehicule {
public:
    Vehicule();
    Vehicule(int deltaT,double wTc, double pCharge, bool debug); // constructeur
    ~Vehicule();
    
    // Données véhicule (constantes)
    int getTypeVehicule();
    int getModele();
    double getConsommation();
    double getVitesse();
    double getPuissanceCharge();
    double getPuissanceV2G();
    double getCapacite();
    bool getAccesBornes(int borne);
    void setAccesBornes(int borne, bool acces);
    int getHoraireDepart(int nbTrajetEffectues);
    
    // Données véhicule (variables)
    double getSoc();
    void setSoc(double newSoc);
    double getSocMin();
    void computeSocMin(int deltaT);
    double getSocV2G();
    int getEtatMouvActuel();
    void setEtatMouvActuel(int mouv);
    int getEtatMouvSuivant();
    void setEtatMouvSuivant(int mouv);
    bool getWillToCharge();
    void setWillToCharge(bool will);
    double getDistanceRestante();
    void setDistanceRestante(double distance);
    int getPosition();
    void setPosition(int pos);
    int getNbTrajetsEffectues();
    void incNbTrajetsEffectues();
    void resetNbTrajetsEffectues();
    int getNbTrajets();
    void setNbTrajets(int nb);
    double getLongueurTrajet();
    void setLongueurTrajet(double distance);
    int getDestination(int numTrajet);
    int getProchaineDestination();
    void addDestination(int position);
    void resetDestinations();
    bool getVehiculeToGrid();
    void setVehiculeToGrid(bool v2g);
    bool getAcceptSmartGrid();
    double getDebutSmartGrid();
    bool getAcceptV2G();
    
    // Autres fonctions membres utiles
    bool getNeedToReset();
    void setNeedToReset(bool need);
    void reinitJour();
    void printInfos(int deltaT) const;
    
    // Fonctions pour le modèle
    int transition(int temps, int deltaT, int useCase);
    double simulation(int temps, int deltaT, int useCase);
    int smartGrid(int temps, int deltaT, int useCase);
    
private:
    int typeVehicule; // VEP, VEE, VAP
    int modele;
    double consommation; // en kWh/km
    int comportementBranchement; // SMART GRID ???
    double vitesse; // en km/h
    double puissanceCharge;
    double puissanceV2G;
    double soc;
    double socMin;
    double socV2G;
    double capacite; // en kWh
    int etatMouvActuel;
    int etatMouvSuivant;
    bool willToCharge;
    double distanceRestante;
    int position;
    bool accesBornes[3]; // accesBornes[position]
    int nbTrajetsEffectues;
    int nbTrajets;
    double longueurTrajet; // tous les trajets ont la même longueur
    std::vector<int> destinations;
    std::vector<int> horaireDepart; // utilisation horaireDepart[nbTrajetsJournee]
    bool needToReset;
    bool vehiculeToGrid;
    bool acceptSmartGrid;
    double debutSmartGrid;
    bool acceptV2G;
};

#endif /* defined(__modele_simulation__Vehicule__) */
