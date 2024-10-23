#ifndef AMBULANCE_H
#define AMBULANCE_H

#include <QTimer>
#include <pcosynchro/pcomutex.h>

#include "iwindowinterface.h"
#include "costs.h"
#include "seller.h"

/**
 * @brief La classe Ambulance représente une ambulance capable de transporter des patients
 *        et de gérer les ressources nécessaires à leur traitement.
 *        Hérite de la classe Seller pour gérer le trading de patients.
 */
class Ambulance : public Seller {
public:
    /**
     * @brief Constructeur de la classe Ambulance
     * @param uniqueId Identifiant unique de l'ambulance
     * @param fund Argent initial alloué à l'ambulance
     * @param resourcesSupplied Liste des ressources que cette ambulance peut fournir
     * @param initialStocks Stocks initiaux de ressources disponibles dans l'ambulance
     */
    Ambulance(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied, std::map<ItemType, int> initialStocks);

    /**
     * @brief getItemsForSale
     * @return Les items disponibles dans les stocks de l'ambulance pour vente ou transfert
     */
    std::map<ItemType, int> getItemsForSale() override;

    /**
     * @brief Fonction permettant de proposer des ressources au vendeur
     * @param what Le type de resource
     * @param qty Nombre de ressources
     * @param bill Le coût de la transaction
     * @return La quantité acceptée ou la facture (peu dépendre de votre logique) et 0 si la transaction n'est pas acceptée.
     */
    int send(ItemType it, int qty, int bill) override;

    /**
     * @brief Fonction permettant d'acheter des ressources au vendeur
     * @param what Le type de resource à acheter
     * @param qty Nombre de ressources voulant être achetées
     * @return La facture : côut de la resource * le nombre, 0 si indisponible
     */
    int request(ItemType what, int qty) override;

    /**
     * @brief run
     * La boucle principale de l'ambulance, qui gère l'augmentation des stocks et le coût des employés.
     * Cette méthode est exécutée dans un thread pour permettre un fonctionnement asynchrone.
     */
    void run();

    /**
     * @brief getMaterialCost
     * @return Le coût des matériaux nécessaires pour le fonctionnement de l'ambulance.
     */
    int getMaterialCost();

    /**
     * @brief getAmountPaidToWorkers
     * @return Le montant total payé aux travailleurs de l'ambulance.
     */
    int getAmountPaidToWorkers();

    int getNumberPatients();

    /**
     * @brief setHospitals
     * @param hospitals Une liste d'hôpitaux avec lesquels l'ambulance va interagir
     * Cette fonction configure les hôpitaux pour lesquels l'ambulance effectuera des transferts de patients.
     */
    void setHospitals(std::vector<Seller*> hospitals);

    /**
     * @brief setInterface
     * @param windowInterface Pointeur vers l'interface graphique utilisée pour afficher les logs et mises à jour
     * Configure l'interface pour afficher les actions de l'ambulance.
     */
    static void setInterface(IWindowInterface* windowInterface);

    /**
     * @brief getResourcesSupplied
     * @return Les ressources fournies par cette ambulance
     */
    std::vector<ItemType> getResourcesSupplied() const;

protected:
    /**
     * @brief sendPatient
     * Fonction responsable de l'envoi d'un patient à l'hôpital ou à la clinique.
     * Cette méthode gère les détails logistiques de la transmission d'un patient.
     */
    void sendPatient();

    std::vector<ItemType> resourcesSupplied;  // Liste des items que ce fournisseur gère (ressources de l'ambulance)
    int nbTransfer;  // Nombre total d'items (patients) transférés par l'ambulance
    static IWindowInterface* interface;  // Interface pour les logs et mises à jour
    std::vector<Seller*> hospitals;  // Liste des hôpitaux associés à cette ambulance
};

#endif // AMBULANCE_H
