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
#include <boost/lexical_cast.hpp>

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

// Type de véhicule (VEP, VEE, VAP)
boost::random::discrete_distribution<> distType{70, 20, 10};
int initType() {
    return distType(gen);
}

// Modèle du véhicule
boost::random::discrete_distribution<> distModele{0.6389, 0.18, 0, 0.0573, 0.0161};
const double stats_puissanceVehicule[NB_MODELE] = {65, 80, 50, 55, 222}; // kW
const double stats_capaciteVehicule[NB_MODELE] = {22, 24, 30, 17.6, 60}; // kWh
const double stats_autonomieVehicule[NB_MODELE] = {125, 150, 200, 100, 370}; // km
const double stats_tempsRechargeVehicule[NB_MODELE] = {6, 8, 6, 7, 3}; // h
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

// Disponibilité des bornes de recharge
boost::random::bernoulli_distribution<> distAccesBorneHOME(0.96); // HOME
boost::random::bernoulli_distribution<> distAccesBorneWORK(0.33); // WORK
boost::random::bernoulli_distribution<> distAccesBorneLUNCH(0.14); // LUNCH
void initAccesBornes(bool accesBornes[], int typeVE) {
    switch (typeVE) {
        case VE_PARTICULIER: {
            do { // pour éviter qu'un véhicule n'ait pas de borne du tout
                accesBornes[MAISON] = distAccesBorneHOME(gen);
                accesBornes[TRAVAIL] = distAccesBorneWORK(gen);
                accesBornes[REPAS] = distAccesBorneLUNCH(gen);
            } while (!(accesBornes[MAISON] || accesBornes[TRAVAIL] || accesBornes[REPAS]));
            break;
        }
            
        default:
            accesBornes[MAISON] = false; // VEE et VAP n'ont pas de maison
            accesBornes[TRAVAIL] = true; // Ils sont toujours des bornes de recharge (AutoLib' ou locaux de l'entreprise)
            accesBornes[REPAS] = (boost::random::bernoulli_distribution<> (0.5))(gen); // 1 chance sur 2 d'avoir une borne lors d'un déplacement
            break;
    }
}

// Vitesse moyenne
boost::random::normal_distribution<> distVitesse(38.75, 10.0); // km/h
double initVitesse() {
    return distVitesse(gen);
}

// Nombre de trajets par jour
boost::random::normal_distribution<> distNbTrajets(2.5, 1);
int initNbTrajets() {
    int nbTrajets = 0;
    while (nbTrajets <= 0) {
        nbTrajets = std::floor(distNbTrajets(gen));
    }
    
    return nbTrajets;
}

// Distance d'un trajet
boost::random::discrete_distribution<> distDistanceTrajet{0.13, 0.162, 0.206, 0.242, 0.19, 0.06, 0.011};
const double stats_distanceTrajet[7] = {2, 5, 10, 20, 40, 80, 150};
double initLongueurTrajet() {
    int classe = distDistanceTrajet(gen);
    int min_i = 0;
    if (classe > 0)
        min_i =  stats_distanceTrajet[classe-1];
    
    boost::random::uniform_real_distribution<> longueur(min_i, stats_distanceTrajet[classe]);
    
    return longueur(gen);
}

// Horaires de départ
const int classeHoraire[8] = {5, 7, 8, 9, 12, 16, 20, 24}; // max de la classe horaire
boost::random::discrete_distribution<> distClasseHoraireDepart{0.01, 0.15, 0.31, 0.34, 0.13, 0.3, 0.2, 0.01};
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

// SmartGrid
boost::random::bernoulli_distribution<> dist_acceptSmartGrid(0.75);
bool initAcceptSmartGrid() {
    return dist_acceptSmartGrid(gen);
}
boost::random::uniform_real_distribution<> dist_debutSmartGrid(0, 3);
double initDebutSmartGrid() {
    return (18.0 + boost::lexical_cast<double>(dist_debutSmartGrid(gen)));
}

// Vehicle2Grid
boost::random::bernoulli_distribution<> dist_acceptV2G(0.5);
bool initAcceptV2G() {
    return dist_acceptV2G(gen);
}


// Fonctions de vérification
bool passeParUneBorne(std::vector<int> &destinations, bool accesBornes[]) {
    bool res = false;
    for (int i = 0; i < destinations.size(); i++) {
        res = res || accesBornes[destinations[i]];
    }
    return res;
}

bool checkOrdreHorairesDepart(std::vector<int> &horairesDepart) {
    bool res = true;
    for (int i = 1; i < horairesDepart.size(); i++)
        res = res && (horairesDepart[i] > horairesDepart[i-1]);
    
    return res;
}


// Constructeur
Vehicule::Vehicule() {
    //
}

Vehicule::Vehicule(int deltaT, bool debug) {
    if (debug)
        std::cout << "Init VE" << std::endl;
    soc = 100;
    etatMouvActuel = BRANCHE_PAS_EN_CHARGE;
    etatMouvSuivant = BRANCHE_PAS_EN_CHARGE;
    nbTrajetsEffectues = 0;
    willToCharge = true;
    typeVehicule = initType();
    position = (typeVehicule == VE_PARTICULIER) ? MAISON : TRAVAIL;
    modele = initModele(typeVehicule);
    capacite = stats_capaciteVehicule[modele];
    vitesse = initVitesse(); // en km/h
    consommation = capacite / stats_autonomieVehicule[modele];
    longueurTrajet = initLongueurTrajet();
    puissanceCharge = 3.5;
    acceptSmartGrid = initAcceptSmartGrid();
    debutSmartGrid = initDebutSmartGrid();
    acceptV2G = (acceptSmartGrid) ? initAcceptV2G() : false;
    puissanceV2G = (acceptV2G) ? 2.0 : 0.0;
    socV2G = 40;
    vehiculeToGrid = false; // indique si on fait du V2G actuellement (flag)
    if (debug)
        std::cout << "\t" << "Constantes terminées : VE de type " << typeVehicule << std::endl;
    
    socMin = 0;
    nbTrajets = initNbTrajets();
    if (debug)
        std::cout << "\t" << "Nombre trajets. OK" << std::endl;
    
    for (int h = 0; h < 3; h++) {
        accesBornes[h] = false;
    }
    initAccesBornes(accesBornes, typeVehicule);
    if (debug)
        std::cout << "\t" << "Accès bornes. OK" << std::endl;
    
    int dernierePosition(position);
    int dernierHoraire(0);
    if (debug)
        std::cout << "\t" << "Calcul des horaires et destinations" << std::endl;
    do {
        destinations.clear();
        horaireDepart.clear();
        for (int i(0); i < nbTrajets; i++) {
            if (debug)
                std::cout << "\t\t" << "Couple n°" << (i+1) << " sur " << nbTrajets << std::endl;
            int nouvelHoraire = initHoraireDepart(deltaT, dernierHoraire);
            if (debug)
                std::cout << "\t\t\t" << "Horaire. OK" << std::endl;
            int newDestination = giveDestination(typeVehicule, dernierePosition, nouvelHoraire, deltaT);
            if (debug)
                std::cout << "\t\t\t" << "Destination. OK" << std::endl;
            destinations.push_back(newDestination);
            horaireDepart.push_back(nouvelHoraire);
            dernierePosition = newDestination;
            dernierHoraire = (nouvelHoraire + (int)std::ceil((longueurTrajet / vitesse) * 60 / deltaT)) % (1440/deltaT);
        }
    } while (!passeParUneBorne(destinations, accesBornes));
    
    if (debug && !checkOrdreHorairesDepart(horaireDepart))
        std::cout << "INCOHERENCE DANS LES HORAIRES" << std::endl;
    
    if (debug)
        std::cout << "\t" << "Destinations et horaires départ. OK" << std::endl;
    
    computeSocMin(deltaT);
    if (debug) {
        std::cout << "\t" << "SOC min. OK" << std::endl;
        std::cout << "  VE OK !" << std::endl << std::endl;
    }
}

// Destructeur
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
int Vehicule::smartGrid(int temps, int deltaT, int useCase) {
    double heure = ((temps * deltaT) % 1440) / 60.0;
    switch (useCase) {
        case 0: {
            if (getSoc() >= 100)
                return BRANCHE_PAS_EN_CHARGE;
            else
                return BRANCHE_EN_CHARGE;
        }
            
        case 1: {
            if (getAcceptSmartGrid()) {
                if ((getDebutSmartGrid() <= heure && heure < (getDebutSmartGrid() + 3.0)) || getSoc() >= 100)
                    return BRANCHE_PAS_EN_CHARGE;
                else
                    return BRANCHE_EN_CHARGE;
            } else
                return smartGrid(temps, deltaT, 0);
        }
            
        case 2: {
            if (getAcceptSmartGrid()) {
                if (getAcceptV2G()) {
                    setVehiculeToGrid(false);
                    if (getDebutSmartGrid() <= heure && heure < (getDebutSmartGrid() + 3.0)) {
                        if (getSoc() >= getSocMin() && getSoc() > getSocV2G())
                            setVehiculeToGrid(true);
                        return BRANCHE_PAS_EN_CHARGE;
                    }
                    else
                        return BRANCHE_EN_CHARGE;
                } else {
                    return smartGrid(temps, deltaT, 1);
                }
            } else {
                return smartGrid(temps, deltaT, 0);
            }
            
        }
            
        default:
            return -1;
    }	
}

int Vehicule::transition(int temps, int deltaT, int useCase) {
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
                if (!getWillToCharge())
                    std::cout << "J'ai pas de bornes !!! :'(" << std::endl;
                setEtatMouvSuivant(GARE_NON_BRANCHE);
                return 0;
            }
        }
        
        setEtatMouvSuivant(smartGrid(temps, deltaT, useCase));
        return 0;
    }
}

double Vehicule::simulation(int temps, int deltaT, int useCase) {
    double result = 0.0;
    //std::cout << "Simulation " << temps << ". ";
    if (temps > 0 && (temps * deltaT) % 1440 == 0) {
        setNeedToReset(true);
    }
    
    if (getEtatMouvActuel() != EN_TRAIN_DE_ROULER && getNeedToReset()) {
        reinitJour(); // on réinitialise certaines données à 00h00 chaque jour
    }
    
    int mouv(getEtatMouvActuel());
    
    if (mouv == EN_TRAIN_DE_ROULER) {
        setSoc(std::max(0.0, getSoc() - 100.0 * (getVitesse() * deltaT/60.0) * (getConsommation() / getCapacite())));
        setDistanceRestante(std::max(0.0, getDistanceRestante() - getVitesse() * deltaT/60.0)); // usage de 'max' pour éviter d'avoir une distance restante négative
    } else if (mouv == BRANCHE_EN_CHARGE && getSoc() < 100) {
        setSoc(std::min(100.0, getSoc() + 100.0 * (deltaT/60.0) * (getPuissanceCharge() / getCapacite()))); //puissanceCharge() fonction qui peut dépendre des paramètres qu'on veut, pour anticiper le smartgrid de ce coté là aussi. // usage de 'min' pour éviter d'avoir un SOC > 100
        result = getPuissanceCharge();
    } else if(mouv == BRANCHE_PAS_EN_CHARGE) {
        if (getVehiculeToGrid()) {
            setSoc(getSoc() - getPuissanceV2G() * deltaT / 60.0);
            result = -getPuissanceV2G();
        } else {
            result = 0.0;
        }
    }
    
    transition(temps, deltaT, useCase);
    
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
    
    return result; // renvoie la puissance demandé à l'instant t par le VE
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

double Vehicule::getSocV2G() {
    return socV2G;
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

double Vehicule::getPuissanceV2G() {
    return puissanceV2G;
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

bool Vehicule::getVehiculeToGrid() {
    return vehiculeToGrid;
}

void Vehicule::setVehiculeToGrid(bool v2g) {
    vehiculeToGrid = v2g;
}

bool Vehicule::getAcceptSmartGrid() {
    return acceptSmartGrid;
}

double Vehicule::getDebutSmartGrid() {
    return debutSmartGrid;
}

bool Vehicule::getAcceptV2G() {
    return acceptV2G;
}

bool Vehicule::getNeedToReset() {
    return needToReset;
}

void Vehicule::setNeedToReset(bool need) {
    needToReset = need;
}

void Vehicule::reinitJour() {
    // Réinitialisation de certaines variables à minuit
    resetNbTrajetsEffectues();
    setNeedToReset(false);
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




