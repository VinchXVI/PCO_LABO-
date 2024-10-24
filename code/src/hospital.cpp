#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcothread.h>

IWindowInterface* Hospital::interface = nullptr;

Hospital::Hospital(int uniqueId, int fund, int maxBeds)
    : Seller(fund, uniqueId), maxBeds(maxBeds), currentBeds(0), nbHospitalised(0), nbFree(0)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Hospital Created with " + QString::number(maxBeds) + " beds");
    
    std::vector<ItemType> initialStocks = { ItemType::PatientHealed, ItemType::PatientSick };

    for(const auto& item : initialStocks) {
        stocks[item] = 0;
    }
}

int Hospital::request(ItemType what, int qty){ //TODO
    int cost;
    switch (what) {
    case ItemType::PatientSick :
        if(this->getNumberPatients() < this->maxBeds){ // achat de patient un par un
            cost = getCostPerUnit(ItemType::PatientSick);
            this->money -= cost;
            this->stocks[ItemType::PatientSick]++;
            this->nbHospitalised++;
            break;
        } else {
            cost = 0;
            break;
        }
    default:
        cost = 0;
    }
    return cost;
}

void Hospital::freeHealedPatient() {
    if(this->stocks[ItemType::PatientHealed] != 0){
        this->stocks[ItemType::PatientHealed]--;
        this->money += getCostPerUnit(ItemType::PatientHealed);
        this->nbFree++;
    }
}

void Hospital::transferPatientsFromClinic() {
    int cost;
    if(this->getNumberPatients() < this->maxBeds){
      cost = chooseRandomSeller(clinics)->request(ItemType::PatientHealed, 1);
      if(cost != 0){
          this->money -= cost;
          this->stocks[ItemType::PatientHealed]++;
          this->nbHospitalised++;

      }
    }
}

int Hospital::send(ItemType it, int qty, int bill) {
    // TODO
    return 0;
}

void Hospital::run()
{
    if (clinics.empty()) {
        std::cerr << "You have to give clinics to a hospital before launching is routine" << std::endl;
        return;
    }

    interface->consoleAppendText(uniqueId, "[START] Hospital routine");

    while (this->money > 0) { // Tant qu'il a l'argent
        transferPatientsFromClinic();

        freeHealedPatient();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
        interface->simulateWork(); // Temps d'attente
    }

    interface->consoleAppendText(uniqueId, "[STOP] Hospital routine");
}

int Hospital::getAmountPaidToWorkers() {
    return nbHospitalised * getEmployeeSalary(EmployeeType::Nurse);
}

int Hospital::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed] + nbFree;
}

std::map<ItemType, int> Hospital::getItemsForSale()
{
    return stocks;
}

void Hospital::setClinics(std::vector<Seller*> clinics){
    this->clinics = clinics;

    for (Seller* clinic : clinics) {
        interface->setLink(uniqueId, clinic->getUniqueId());
    }
}

void Hospital::setInterface(IWindowInterface* windowInterface){
    interface = windowInterface;
}
