#include "clinic.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include "utils.h"

IWindowInterface* Clinic::interface = nullptr;

Clinic::Clinic(int uniqueId, int fund, std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId), nbTreated(0), resourcesNeeded(resourcesNeeded)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");

    for(const auto& item : resourcesNeeded) {
        stocks[item] = 0;
    }
}

bool Clinic::verifyResources() {
    for (auto item : resourcesNeeded) {
        if (stocks[item] == 0) {
            return false;
        }
    }
    return true;
}

int Clinic::request(ItemType what, int qty){ // what == PatientHealed (appelé que par hopital)
    int cost = 0;
    this->mutex.lock();
    if(this->stocks[what] > 0){
        this->stocks[what]--;
        cost = getCostPerUnit(what) * qty; // /!\ le coût est égal au prix du traitement pas celui de transfert (à changer en getCostPerUnit(ItemType::PatienSick) ?)
        this->money += cost;
    }
    this->mutex.unlock();
    return cost;
}

void Clinic::treatPatient() {
    int employeeCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));

    this->mutex.lock();
    if(this->money - employeeCost >= 0){
        this->money -= employeeCost;
    } else { // Si la clinique n'a plus les fonds pour traiter des patients, stop sa routine
        this->stopRoutine = true;
        this->mutex.unlock();
        return;
    }
    this->mutex.unlock();

    //Temps simulant un traitement
    interface->simulateWork();

    this->mutex.lock();
    for(ItemType item : resourcesNeeded){
        this->stocks[item]--;
    }
    this->stocks[ItemType::PatientHealed]++;
    this->nbTreated++;
    this->mutex.unlock();
    
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() {
    int cost, qty;
    bool suppliersStillRunning = false;

    //vérification si les fournisseurs sont toujours actifs
    for(Seller* supplier : suppliers){
        if(!supplier->isStopped())
            suppliersStillRunning = true;
    }
    if(!suppliersStillRunning){ // Si les que les fournisseur ne sont plus actifs, stopper la routine
        this->stopRoutine = true;
        return;
    }

    this->mutex.lock();
    for(ItemType item : this->resourcesNeeded) { // Une ressource après l'autre
        qty = 0;
        cost = 0;

        if(this->stocks[item] == 0) {
            switch (item) {
            // Tansfert de patient depuis les hôpitaux
            case ItemType::PatientSick:
                qty = 1;
                if(this->money - qty * getCostPerUnit(item) >= 0) { // Si fonds suffisants
                    cost = chooseRandomSeller(hospitals)->request(item, qty);
                }
                break;

            // Commande de matériel médical
            case ItemType::Pill:
            case ItemType::Scalpel:
            case ItemType::Stethoscope:
            case ItemType::Syringe:
            case ItemType::Thermometer:
            default:
                qty = 1; // commande par lot de X
                if(this->money - qty * getCostPerUnit(item) >= 0 and suppliersStillRunning) { // Si fonds suffisants et si les suppliers sont toujours actifs
                    for(Seller* supplier : suppliers) {
                        cost = supplier->request(item, qty);
                        if(cost != 0) {
                            break; // Arrête de chercher dès qu'on a trouvé un fournisseur
                        }
                    }
                }
                break;
            } // fin du switch
        } // fin de vérification des stocks d'un item

        // Mise à jour des stocks et des fonds si la transaction a réussi
        if(cost > 0) {
            this->stocks[item] += qty;
            this->money -= cost;
        }

    } // fin de boucle for chaque item
    this->mutex.unlock();
}


void Clinic::run() {
    if (hospitals.empty() || suppliers.empty()) {
        std::cerr << "You have to give to hospitals and suppliers to run a clinic" << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (this->money > 0 and !this->stopRoutine  and !stopProgram) { // Tant que la clinic a de l'argent pour s'approvisionner et traiter les patient
        if (verifyResources()) {
            treatPatient();
        } else {
            orderResources();
        }

        interface->simulateWork();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    this->stopRoutine = true; // double check si pas changer dans orderResources() ou treatPatient() ou si money == 0
    interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}


void Clinic::setHospitalsAndSuppliers(std::vector<Seller*> hospitals, std::vector<Seller*> suppliers) {
    this->hospitals = hospitals;
    this->suppliers = suppliers;

    for (Seller* hospital : hospitals) {
        interface->setLink(uniqueId, hospital->getUniqueId());
    }
    for (Seller* supplier : suppliers) {
        interface->setLink(uniqueId, supplier->getUniqueId());
    }
}

int Clinic::getTreatmentCost() {
    return 0;
}

int Clinic::getWaitingPatients() {
    return stocks[ItemType::PatientSick];
}

int Clinic::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed];
}

int Clinic::send(ItemType it, int qty, int bill){
    return 0;
}

int Clinic::getAmountPaidToWorkers() {
    return nbTreated * getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));
}

void Clinic::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}

std::map<ItemType, int> Clinic::getItemsForSale() {
    return stocks;
}


Pulmonology::Pulmonology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Thermometer}) {}

Cardiology::Cardiology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Syringe, ItemType::Stethoscope}) {}

Neurology::Neurology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Scalpel}) {}
