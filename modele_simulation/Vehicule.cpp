//
//  Vehicule.cpp
//  modele_simulation
//
//  Created by Camille MASSET on 09/03/2015.
//  Copyright (c) 2015 PSC. All rights reserved.
//

#include "Vehicule.h"
#include <ctime>
#include <vector>
#include <iostream>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

// CONSTANTES
// Etat de mouvement
const int GARE_NON_BRANCHE = 0;
const int BRANCHE_PAS_EN_CHARGE = 1;
const int BRANCHE_EN_CHARGE = 2;
const int EN_TRAIN_DE_ROULER = 3;
const std::string listeEtatMouvement[4] = {"Garé non branché", "Branché pas en charge", "Branché en charge", "En train de rouler"};
// Position
const int MAISON = 0;
const int TRAVAIL = 1;
const int REPAS = 2;
const std::string listePosition[3] = {"Maison", "Travail", "Repas"};
// Type de véhicule
const int VE_PARTICULIER = 0;
const int VE_ENTREPRISE = 1;
const int VE_PARTAGE = 2;
const std::string listeTypeVehicule[3] = {"VE particulier", "VE d'entreprise", "VE autopartage"};
// Modèles de véhicule
const int NB_MODELE(5);
const int ZOE(0);
const int LEAF(1);
const int BOLLORE(2);
const int SMART(3);
const int TESLA(4);
const std::string listeModeleVehicule[NB_MODELE] = {"Renault Zoé", "Nissan Leaf", "Bolloré", "Smart", "Tesla"};


/* *********************************************** *
 * Données statistiques et fonctions de génération *
 * *********************************************** */

// Vrai modulo (pour gérer les nombres négatifs)
int real_mod(int n, int p) {
    int r = n % p;
    if (r < 0) {
        return r + p;
    } else {
        return r;
    }
}

// Générateur aléatoire
boost::random::mt19937 gen((int)time(NULL));

// Disponibilité des bornes de recharge
boost::random::bernoulli_distribution<> distAccesBorneHOME(0.96); // HOME
boost::random::bernoulli_distribution<> distAccesBorneWORK(0.33); // WORK
boost::random::bernoulli_distribution<> distAccesBorneLUNCH(0.14); // LUNCH

// Vitesse moyenne
boost::random::normal_distribution<> distVitesse(38.75, 10.0); // km/h

// Nombre de trajets par jour
boost::random::normal_distribution<> distNbTrajets(2.5, 1);

// Distance d'un trajet
boost::random::lognormal_distribution<> distDistanceTrajet(12.75, 1.0);
const double stats_distanceTrajet[7] = {0.13, 0.162, 0.206, 0.242, 0.19, 0.06, 0.011}; // 0-2, 2-5, 5-10, 10-20, 20-40, 40-80, >80 km
const double stats_distanceMoyenneTrajet(12.75); // km

// Horaires de départ
const int classeHoraire[8] = {5, 7, 8, 9, 12, 16, 20, 24}; // max de la classe horaire
boost::random::discrete_distribution<> distClasseHoraireDepart{0.01, 0.15, 0.31, 0.34, 0.13, 0.3, 0.2, 0.01};

// Type de véhicule (VEP, VEE, VAP)
boost::random::discrete_distribution<> distType{70, 20, 10};

// Modèle du véhicule
boost::random::discrete_distribution<> distModele{0.6389, 0.18, 0, 0.0573, 0.0161};
const double stats_puissanceVehicule[NB_MODELE] = {65, 80, 50, 55, 222}; // kW
const double stats_capaciteVehicule[NB_MODELE] = {22, 24, 30, 17.6, 60}; // kWh
const double stats_autonomieVehicule[NB_MODELE] = {125, 150, 200, 100, 370}; // km
const double stats_tempsRechargeVehicule[NB_MODELE] = {6, 8, 6, 7, 3}; // h

// Fonctions de détermination des variables issues de données statistiques
int initType() {
    return distType(gen);
}

int initModele(int typeVE) {
    switch (typeVE) {
        case VE_PARTAGE:
            return BOLLORE;
            break;
            
        default:
            return distModele(gen);
            break;
    }
}

double initVitesse() {
    return distVitesse(gen);
}

int initNbTrajets() {
    int nbTrajets;
    do {
        nbTrajets = std::floor(distNbTrajets(gen));
    } while (nbTrajets <= 0);
    
    return nbTrajets;
}

void initAccesBornes(bool accesBornes[], int typeVE) {
    do { // pour éviter qu'un véhicule n'ait pas de borne du tout
        switch (typeVE) {
            case VE_PARTICULIER:
                accesBornes[MAISON] = distAccesBorneHOME(gen);
                accesBornes[TRAVAIL] = distAccesBorneWORK(gen);
                accesBornes[REPAS] = distAccesBorneLUNCH(gen);
                break;
                
            default:
                accesBornes[MAISON] = false; // VEE et VAP n'ont pas de maison
                accesBornes[TRAVAIL] = true; // Ils sont toujours des bornes de recharge (AutoLib' ou locaux de l'entreprise)
                accesBornes[REPAS] = (boost::random::bernoulli_distribution<> (0.5))(gen); // 1 chance sur 2 d'avoir une borne lors d'un déplacement
                break;
        }
    } while (!(accesBornes[MAISON] && accesBornes[TRAVAIL] && accesBornes[REPAS]));
}

int initHoraireDepart(int deltaT, int minDepart) {
    int depart;
    do {
        // Déterminer la classe
        int classe(distClasseHoraireDepart(gen));
        int tailleClasse(real_mod(classeHoraire[classe] - classeHoraire[real_mod(classe-1, 8)], 24));
        // Répartition uniforme sur la classe
        boost::random::uniform_smallint<> distInClasseHoraire(0, tailleClasse * 60 / deltaT);
        depart = classeHoraire[real_mod(classe-1, 8)] * 60 / deltaT + distInClasseHoraire(gen);
    } while (depart < minDepart);
    
    return depart;
}

int giveDestination(int typeVE, int positionActuelle, int temps, int deltaT) {
    int res(-1);
    switch (typeVE) {
        case VE_ENTREPRISE: {
            if ((deltaT * temps) % 1440 < 18 * 60) {
                res = REPAS;
            } else {
                res = TRAVAIL;
            }
            break;
        }
            
        case VE_PARTICULIER: {
            double heure = ((temps * deltaT) % 1440) / 60.0;
            if (7.0 <= heure && heure < 11.0) {
                switch (positionActuelle) {
                    case TRAVAIL:
                        res = REPAS;
                        break;
                    
                    default:
                        res = TRAVAIL;
                }
            } else if (11.0 <= heure && heure < 14.0) {
                switch (positionActuelle) {
                    case REPAS:
                        res = TRAVAIL;
                        break;
                        
                    case MAISON:
                        res = ((boost::random::bernoulli_distribution<> (0.5))(gen)) ? REPAS : TRAVAIL;
                        break;
                        
                    case TRAVAIL:
                        res = ((boost::random::bernoulli_distribution<> (0.75))(gen)) ? REPAS : MAISON;
                        break;
                        
                    
                }

            } else {
                switch (positionActuelle) {
                    case MAISON:
                        res = REPAS;
                        break;
                        
                    default:
                        res = MAISON;
                        break;
                }
            }
            break;
        }
    
        case VE_PARTAGE: {
            int a = (boost::random::bernoulli_distribution<> (0.5))(gen) ? 1 : 0;
            res = 1 + a;
            break;
        }
    }
    
    return res;
}

Vehicule::Vehicule() {
    //
}

Vehicule::Vehicule(int deltaT) {
    soc = 100;
    etatMouvActuel = BRANCHE_EN_CHARGE; // BRANCHE_EN_CHARGE
    etatMouvSuivant = BRANCHE_EN_CHARGE; // BRANCHE_EN_CHARGE
    distanceParcourue = 0;
    nbTrajetsEffectues = 0;
    willToCharge = false;
    typeVehicule = initType();
    position = (typeVehicule == VE_PARTICULIER) ? MAISON : TRAVAIL; // HOME
    modele = initModele(typeVehicule);
    capacite = stats_capaciteVehicule[modele];
    vitesse = initVitesse(); // en km/h
    consommation = capacite / stats_autonomieVehicule[modele];
    longueurTrajet = 18.3; // A MODIFIER AVEC UNE LOI LOG-NORMALE
    puissanceCharge = 3.5;
    
    socMin = 0;
    nbTrajets = initNbTrajets();
    int dernierePosition(position);
    int dernierHoraire(0);
    for (int i(0); i < nbTrajets; i++) {
        int nouvelHoraire = initHoraireDepart(deltaT, dernierHoraire);
        int newDestination = giveDestination(typeVehicule, dernierePosition, nouvelHoraire, deltaT);
        destinations.push_back(newDestination);
        horaireDepart.push_back(nouvelHoraire);
        dernierePosition = newDestination;
        dernierHoraire = (nouvelHoraire + (int)std::ceil((longueurTrajet / vitesse) * 60 / deltaT)) % 1440;
    }
    
    initAccesBornes(accesBornes, typeVehicule);
    if (!accesBornes[destinations.back()]) { // pour éviter qu'un véhicule n'ait pas de borne en fin de journée
        nbTrajets++;
        horaireDepart.push_back(giveDestination(typeVehicule, destinations.back(), horaireDepart.back(), deltaT));
        switch (typeVehicule) {
            case VE_ENTREPRISE:
                destinations.push_back(TRAVAIL);
                break;
                
            case VE_PARTAGE:
                destinations.push_back(TRAVAIL);
                break;
                
            case VE_PARTICULIER:
                int p(0);
                while (!accesBornes[p]) { // on met le premier lieu où il y a une borne
                    p++;
                }
                destinations.push_back(p);
                break;
        }
    }
    
    computeSocMin(deltaT);
}

Vehicule::~Vehicule() {
    //
}




/************************
 * Algorithme du modèle *
 ************************/

/*
 * smartGrid() 
 * Définit le conmportement smartGrid choisi
 */
int smartGrid() {
    return BRANCHE_EN_CHARGE;
}

double puissanceDelivree() {
    return 3.5;
}

int Vehicule::transition(int temps, int deltaT) {
    int mouv(getEtatMouvActuel());
    
    if (mouv == EN_TRAIN_DE_ROULER) {
        if ((getDistanceRestante() <= 0) || (getSoc() <= 0)) {
            setEtatMouvSuivant(GARE_NON_BRANCHE); // on passe par l'état GARE_NON_BRANCHE avant l'état BRANCHE_* (car deltaT petit)
            computeSocMin(deltaT);
            return 0;
        } else {
            setEtatMouvSuivant(EN_TRAIN_DE_ROULER);
            return 0;
        }
    } else {
        if (getNbTrajets() > getNbTrajetsEffectues() && ((temps * deltaT) % 1440) >= deltaT * getHoraireDepart(getNbTrajetsEffectues())) {
            if (getSoc() >= getSocMin()) {
                setEtatMouvSuivant(EN_TRAIN_DE_ROULER);
                return 0;
            } else {
                setWillToCharge(true);
            }
        }
        
        if (getEtatMouvActuel() == GARE_NON_BRANCHE) {
            if (!getAccesBornes(getPosition()) || !getWillToCharge()) {
                std::cout << "J'ai pas de bornes !!! :'(" << std::endl;
                setEtatMouvSuivant(GARE_NON_BRANCHE);
                return 0;
            }
        }
        
        if (getEtatMouvActuel() == BRANCHE_EN_CHARGE) {
            if (getSoc() >= 100) {
                setEtatMouvSuivant(BRANCHE_PAS_EN_CHARGE);
                return 0;
            } else {
                setEtatMouvSuivant(smartGrid());
                return 0;
            }
        } else {
            setEtatMouvSuivant(smartGrid());
            return 0;
        }
    }
}

void Vehicule::simulation(int temps, int deltaT, std::vector<double>& puissanceReseau) {
    int mouv(getEtatMouvActuel());
    
    if (mouv == EN_TRAIN_DE_ROULER) {
        setSoc(std::max(0.0, getSoc() - 100.0 * (getVitesse() * deltaT/60.0) * (getConsommation() / getCapacite())));
        setDistanceParcourue(getDistanceParcourue() + std::min(getDistanceRestante(), getVitesse() * deltaT/60.0)); // usage de 'min' pour éviter d'avoir une distance parcourue supérieure à la distance à parcourir initialement
        setDistanceRestante(std::max(0.0, getDistanceRestante() - getVitesse() * deltaT/60.0)); // usage de 'max' pour éviter d'avoir une distance restante négative
    } else if (mouv == BRANCHE_EN_CHARGE && getSoc() < 100) {
        setSoc(std::min(100.0, getSoc() + 100.0 * (deltaT/60.0) * (getPuissanceCharge() / getCapacite()))); //puissanceCharge() fonction qui peut dépendre des paramètres qu'on veut, pour anticiper le smartgrid de ce coté là aussi. // usage de 'min' pour éviter d'avoir un SOC > 100
        puissanceReseau[temps] = puissanceReseau[temps] + getPuissanceCharge();
    }
    
    transition(temps, deltaT);
    
    int mouvSuiv(getEtatMouvSuivant());
    
    if (mouvSuiv == EN_TRAIN_DE_ROULER) {
        if (mouv != EN_TRAIN_DE_ROULER) {
            setPosition(getProchaineDestination());
            setDistanceRestante(getLongueurTrajet());
            incNbTrajetsEffectues();
        } else {
            setEtatMouvActuel(getEtatMouvSuivant());
        }
    }
    setEtatMouvActuel(getEtatMouvSuivant());
}



/**********************
 * Getters et setters *
 **********************/

double Vehicule::getSoc() {
    return soc;
}

double Vehicule::getCapacite() {
    return capacite;
}

void Vehicule::setSoc(double newSoc) {
    soc = newSoc;
}

double Vehicule::getSocMin() {
    return socMin;
}

void Vehicule::computeSocMin(int deltaT) {
    if (socMin <= 0) {
        int nbTrajets(getNbTrajetsEffectues());
        int nbTrajetsMax = getNbTrajets();
        double longueurTrajet(getLongueurTrajet());
        double distanceAvantBorne(longueurTrajet);
        while(!getAccesBornes(getDestination(nbTrajets % nbTrajetsMax)) && nbTrajets <= nbTrajetsMax){
            nbTrajets++;
            distanceAvantBorne += longueurTrajet;
        }
        socMin = 100 * distanceAvantBorne * getConsommation() / getCapacite();
    } else {
        socMin = socMin - 100 * (deltaT/60.0) * getVitesse() * getConsommation() / getCapacite();
    }
}


int Vehicule::getEtatMouvActuel() {
    return etatMouvActuel;
}

void Vehicule::setEtatMouvActuel(int mouv) {
    etatMouvActuel = mouv;
}

int Vehicule::getEtatMouvSuivant() {
    return etatMouvSuivant;
}

void Vehicule::setEtatMouvSuivant(int mouv) {
    etatMouvSuivant = mouv;
}

bool Vehicule::getWillToCharge() {
    return willToCharge;
}

void Vehicule::setWillToCharge(bool will) {
    willToCharge = will;
}

double Vehicule::getDistanceRestante() {
    return distanceRestante;
}

void Vehicule::setDistanceRestante(double distance) {
    distanceRestante = distance;
}

double Vehicule::getDistanceParcourue() {
    return distanceParcourue;
}

void Vehicule::setDistanceParcourue(double distance) {
    distanceParcourue = distance;
}

int Vehicule::getPosition() {
    return position;
}

void Vehicule::setPosition(int pos) {
    position = pos;
}

bool Vehicule::getAccesBornes(int borne) {
    return accesBornes[borne];
}

void Vehicule::setAccesBornes(int borne, bool acces) {
    accesBornes[borne] = acces;
}

int Vehicule::getNbTrajets() {
    return nbTrajets;
}

int Vehicule::getNbTrajetsEffectues() {
    return nbTrajetsEffectues;
}

void Vehicule::incNbTrajetsEffectues() {
    nbTrajetsEffectues += 1;
}

void Vehicule::resetNbTrajetsEffectues() {
    nbTrajetsEffectues = 0;
}

double Vehicule::getLongueurTrajet() {
    return longueurTrajet;
}

void Vehicule::setLongueurTrajet(double distance) {
    longueurTrajet = distance;
}

int Vehicule::getHoraireDepart(int nbTrajetEffectues) {
    return horaireDepart[nbTrajetEffectues];
}

int Vehicule::getTypeVehicule() {
    return typeVehicule;
}

int Vehicule::getModele() {
    return modele;
}

double Vehicule::getConsommation() {
	return consommation;
}

double Vehicule::getVitesse() {
	return vitesse;
}

double Vehicule::getPuissanceCharge() {
    return puissanceCharge;
}

int Vehicule::getDestination(int numTrajet) {
    return destinations[numTrajet];
}

int Vehicule::getProchaineDestination() {
    return destinations[getNbTrajetsEffectues()];
}

void Vehicule::addDestination(int position) {
    destinations.push_back(position);
}

void Vehicule::resetDestinations() {
    std::vector<int> newDest;
    destinations = newDest;
}

void Vehicule::reinitJour() {
    // Réinitialisation de certaines variables à minuit
    setDistanceParcourue(0); // Et si on a un trajet en cours à 00h00 ???
    resetNbTrajetsEffectues();
}

void Vehicule::printInfos(int deltaT) const {
    std::cout << "Type : " << listeTypeVehicule[typeVehicule] << std::endl;
    std::cout << "Modèle : " << listeModeleVehicule[modele] << std::endl;
    std::cout << "Consommation : " << consommation << " kWh/km" << std::endl;
    std::cout << "Vitesse : " << vitesse << " km/h" << std::endl;
    std::cout << "Distance d'un trajet : " << longueurTrajet << std::endl;
    std::cout << "SOC : " << soc << " %" << std::endl;
    std::cout << "SOC min : " << socMin << " %" << std::endl;
    std::cout << "Capacité : " << capacite << " kWh" << std::endl;
    std::cout << "Autonomie réelle : " << capacite / consommation << " km" << std::endl;
    std::cout << "Position : " << listePosition[position] << std::endl;
    std::cout << "État actuel : " << listeEtatMouvement[etatMouvActuel] << std::endl;
    std::cout << "Nombre de trajets journée : " << nbTrajets << std::endl;
    std::cout << "Nombre de trajets effectués : " << nbTrajetsEffectues << std::endl;
    std::cout << "Horaires de départ : " << std::endl;
    for (int i(0); i < nbTrajets; i++) {
        std::cout << "\t" << ((horaireDepart[i] * deltaT)/60) % 24 << "h" << (horaireDepart[i] * deltaT) % 60 << " --> " << listePosition[destinations[i]] << std::endl;
    }
    
}




