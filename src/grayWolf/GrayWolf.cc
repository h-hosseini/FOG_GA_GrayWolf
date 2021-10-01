
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "src/grayWolf/GrayWolf.h"
#include "src/ga/GA.h"


#include <vector>
#include <algorithm>    // std::sort
#include <math.h>       // ceil

namespace fogfn {
using namespace inet;

Define_Module(GrayWolf);

void GrayWolf::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){
        statistics = check_and_cast<Statistics *>(getSimulation()->getSystemModule()->getSubmodule("statistics"));
        //selfMsg = new ClockEvent("sendTimer");
        selfMsg = new cMessage("sendTimer");
        numberOfFogNodes = par("numberOfFogNodes");
        numberOfIterations = par("numberOfIterations");
        numberOfServices = numberOfFogNodes; // Each service per each fog node //par("numberOfServices");
        wolfSize = numberOfServices;
        numberOfPopulation = par("numberOfPopulation");

        dataSizePerMip = par("dataSizePerMip");
        unitCostProcessing = par("unitCostProcessing"); //unit cost in $ for processing resource in mips at fog node
        unitCostStorage = par("unitCostStorage"); //unit cost in $ for storage resource at fog node (per byte per second)
        unitCostEnergy = par("unitCostEnergy"); //unit cost in $ for energy consumption per joule in fog environment

        fogNodes.resize(numberOfFogNodes);
        services.resize(numberOfServices); //Each Fog node one service
        WATCH(numberOfFogNodes);
        WATCH(numberOfIterations);
        WATCH(numberOfServices);
        WATCH(numberOfPopulation);

        WATCH(dataSizePerMip);
        WATCH(unitCostProcessing);
        WATCH(unitCostStorage);
        WATCH(unitCostEnergy);

        WATCH_VECTOR(fogNodes);
        WATCH_VECTOR(services);

        WATCH_VECTOR(population);

        startTime = par("startTime");
        //scheduleClockEventAt(startTime, selfMsg);
        scheduleAt(startTime, selfMsg);
    }

}

void GrayWolf::registFogNodesInfo(cModule *host, L3Address ipAddress, int fogIndex){
    fogNodes.at(fogIndex).host = host;
    fogNodes.at(fogIndex).ipAddress = ipAddress;
    EV_INFO << "GrayWolf::registFogNodesInfo(): Fog node name: " << host->getFullName() << ", Fog node #: " << fogIndex << ", Ip address: " << ipAddress << " is registered." << endl;
}

void GrayWolf::registFogServiceInfo(L3Address srcAddr, double processingCapacity, double linkCapacity, double powerIdle, double powerTransmission, double powerProcessing, double dataSize, double requestSize, double responseSize, double serviceDeadline){
    int fogIndex = -1;
    for (int i=0; i<numberOfFogNodes; i++){
        if (fogNodes.at(i).ipAddress == srcAddr){
            fogIndex = i;
        }
    }
    if (fogIndex == -1){
        error("GrayWolf::registFogServiceInfo(): The Fog node has not been registered before!");
        //throw cRuntimeError("GrayWolf::registFogServiceInfo(): The Fog node has not been registered before!");
    }
    fogNodes.at(fogIndex).processingCapacity = processingCapacity;
    fogNodes.at(fogIndex).linkCapacity = linkCapacity;
    fogNodes.at(fogIndex).powerIdle = powerIdle;
    fogNodes.at(fogIndex).powerTransmission = powerTransmission;
    fogNodes.at(fogIndex).powerProcessing = powerProcessing;
    services.at(fogIndex).dataSize = dataSize;
    services.at(fogIndex).requestSize = requestSize;
    services.at(fogIndex).responseSize = responseSize;
    services.at(fogIndex).serviceDeadline = serviceDeadline;

    EV_INFO << "GrayWolf::registFogServiceInfo(): service is registered. Fog info: " << fogNodes.at(fogIndex) << ", Service info: " << services.at(fogIndex) << endl;
}

/*
 * The service time is defined as the sum of processing, communication,
 * and service availability time for the services in the fog computing
 * environment given by Eq. (3).
 */
double GrayWolf::serviceTime(Wolf wolf){

    double processingTime = 0;  // The processing time tpro depends on the service size (data size) ssize i and the processing capacity of the fog node yj given by Eq. (4).
    double communicationTime = 0; // The communication time tcom is defined as the total time involved for transferring the service request to fog nodes and response back to the actuators. tcom depends on the service size (data size) and the link capacity Bf connected between the nodes as shown by Eq. (5).
    double serviceAvailabilityTime = 0; // The service availability time tav involves the time taken for selecting the fog node to deploy the services and data. Also, it depends on the total time of the service request in the waiting queue to get the selected fog nodes’ computational resources. Thus tav depends on the amount of time it takes for completing the current service request.

    std::vector<double> waitingQueueDelay;
    waitingQueueDelay.resize(numberOfFogNodes);
    for (int j=0; j<numberOfFogNodes ; j++){
        waitingQueueDelay.at(j) = 0;
    }

    for (int i=0; i<numberOfServices ; i++){
        //processingTime += services.at(i).dataSize / fogNodes.at(wolf.at(i)).processingCapacity;
        double processingTimeServicei = services.at(i).dataSize / fogNodes.at(wolf.at(i)).processingCapacity;
        processingTime += processingTimeServicei;
        communicationTime += (services.at(i).requestSize + services.at(i).responseSize) / (fogNodes.at(wolf.at(i)).linkCapacity * 1000000); // * 1000000 because the units of the requestSize and responseSize is Byte, but linkCapacity's is Mbps.
        waitingQueueDelay.at(wolf.at(i)) += processingTime;
        serviceAvailabilityTime += waitingQueueDelay.at(wolf.at(i)) + processingTimeServicei;
    }

    EV << "Service Time: " << processingTime + communicationTime + serviceAvailabilityTime << " => Processing Time: " << processingTime << ", Communication Time: " << communicationTime << ", Service Availability Time:" << serviceAvailabilityTime << endl;

    return processingTime + communicationTime + serviceAvailabilityTime;
}

/*
 * This method calculates consumed energy for all nodes.
 * If a fog node does not participate in the optimization(or wolf),
 * its idle energy is calculate for the cost function.
 */
double GrayWolf::energyConsumption(Wolf wolf){

    double t0 = 0; // start time
    double t1 = 0; // processing start time
    double tEndDelta = 0; // end time
    std::vector<double> processingTimeEndDelta;
    double energyProcessing = 0;
    double energyTransmission = 0;

    //calculate tEndDelta
    processingTimeEndDelta.resize(numberOfFogNodes);
    for(int i=0; i<numberOfFogNodes; i++){
        double tEndDeltai = 0;
        for(int j=0; j<numberOfServices; j++){
            if(wolf.at(j) == i){
                tEndDeltai += services.at(j).dataSize / fogNodes.at(i).processingCapacity;
            }
        }
        processingTimeEndDelta.at(i) = tEndDelta;
        if(tEndDeltai > tEndDelta){
            tEndDelta = tEndDeltai;
        }
    }

    //calculate t1
    for(int i=0; i<numberOfFogNodes; i++){
        double transmissionTime = services.at(i).requestSize / fogNodes.at(i).linkCapacity;
        if(transmissionTime > t1){
            t1 = transmissionTime;
        }
    }

    //Energy calculation
    for(int i=0; i<numberOfFogNodes ; i++){
        double transmissionTimeEnd = services.at(i).requestSize / fogNodes.at(i).linkCapacity;
        double energyTransmissionIdle = (t1 - transmissionTimeEnd) * fogNodes.at(i).powerIdle;
        double energyTransmissionActive = (transmissionTimeEnd - t0) * fogNodes.at(i).powerTransmission;
        energyTransmission += energyTransmissionIdle + energyTransmissionActive;

        double energyProcessingIdle = (tEndDelta - processingTimeEndDelta.at(i)) * fogNodes.at(i).powerIdle;
        double energyProcessingActive = processingTimeEndDelta.at(i) * fogNodes.at(i).powerProcessing;
        energyProcessing += energyProcessingIdle + energyProcessingActive;

    }
    EV << "Energy Consumption: " << energyProcessing + energyTransmission << " => Energy Processing: " << energyProcessing << ", Energy Transmission: " << energyTransmission << endl;

    return energyProcessing + energyTransmission;
}

double GrayWolf::serviceCost(Wolf wolf){

    double processingCost = 0;
    double storageCost = 0;
    double energyCost = 0;

    for (int i=0; i<numberOfServices ; i++){
        processingCost += services.at(i).dataSize * dataSizePerMip * unitCostProcessing;
        storageCost += services.at(i).dataSize / 8 * unitCostStorage;
    }

    energyCost += energyConsumption(wolf) * unitCostEnergy;

    EV << "Service Cost: " << processingCost + storageCost + energyCost << " => Processing Cost: " << processingCost << ", Storage Cost: " << storageCost << ", Energy Cost:" << energyCost << endl;
    return processingCost + storageCost + energyCost;
}

GrayWolf::ValueFitnessCostFunctions GrayWolf::fitnessFunction(Wolf wolf){

    ValueFitnessCostFunctions valueFitnessCostFunctions;
    valueFitnessCostFunctions.serviceTimeValue = serviceTime(wolf);
    valueFitnessCostFunctions.serviceCostValue = serviceCost(wolf);
    valueFitnessCostFunctions.energyConsumptionValue = energyConsumption(wolf);
    valueFitnessCostFunctions.fitnessValue = 3 / (valueFitnessCostFunctions.serviceTimeValue + valueFitnessCostFunctions.serviceCostValue + valueFitnessCostFunctions.energyConsumptionValue);

    EV << "Fitness Function Value" << valueFitnessCostFunctions.fitnessValue << ", Sevice Time: " << valueFitnessCostFunctions.serviceTimeValue << ", Service Cost: " << valueFitnessCostFunctions.serviceCostValue << ", Energy Consumtion: " << valueFitnessCostFunctions.energyConsumptionValue << endl;

    return valueFitnessCostFunctions;

}

void GrayWolf::generationOfInitialPopulation(){
    EV << "Generates initial population." << endl;

    if(population.size() < numberOfPopulation)
                population.resize(numberOfPopulation);

    for(int n=0; n<numberOfPopulation; n++){
        if(population.at(n).wolf.size() < wolfSize){
            population.at(n).wolf.resize(wolfSize);
        }
        for(int i=0; i<wolfSize; i++){
            population.at(n).wolf.at(i) = intuniform(0, numberOfFogNodes - 1);
        }
        population.at(n).valueFitnessCostFunctions = fitnessFunction(population.at(n).wolf);
        //EV << "Initial population[" << n << "]: " << population.at(n) << endl;
    }
    EV << "Generates initial population." << endl;
}

int GrayWolf::crossOver(int x1, int x2, int x3){

    int selectedWolf = intuniform(0, 2);

    if(selectedWolf == 0){
        return x1;
    }else if(selectedWolf == 1){
        return x2;
    }

    return x3;
}

/*
void GrayWolf::updateWolves(double a){

    Wolf alpha = population.at(0).wolf;
    Wolf beta = population.at(1).wolf;
    Wolf delta = population.at(2).wolf;

    for(int i=0; i<numberOfPopulation; i++){
        for(int j=0; j<wolfSize; j++){
            // Alpha
            double r1 = uniform(0, 1);
            double r2 = uniform(0, 1);
            double A1 = 2.0 * a * r1 - a;
            double C1 = 2.0 * r2;
            double dAlpha = std::abs(C1 * alpha.at(j) - population.at(i).wolf.at(j));
            double X1 = alpha.at(j) - A1 * dAlpha;
            // Beta
            r1 = uniform(0, 1);
            r2 = uniform(0, 1);
            A1 = 2.0 * a * r1 - a;
            C1 = 2.0 * r2;
            double dBeta = std::abs(C1 * beta.at(j) - population.at(i).wolf.at(j));
            double X2 = beta.at(j) - A1 * dBeta;
            // Delta
            r1 = uniform(0, 1);
            r2 = uniform(0, 1);
            A1 = 2.0 * a * r1 - a;
            C1 = 2.0 * r2;
            double dDelta = std::abs(C1 * delta.at(j) - population.at(i).wolf.at(j));
            double X3 = delta.at(j) - A1 * dDelta;

            population.at(i).wolf.at(j) = (X1 + X2 + X3) / 3.0;


        }
        population.at(i).valueFitnessCostFunctions = fitnessFunction(population.at(i).wolf);
        EV << "Updated Wolf#" << i << ": " << population.at(i) << endl;
    }
}
*/

void GrayWolf::updateWolves(double a){

    Wolf alpha = population.at(0).wolf;
    Wolf beta = population.at(1).wolf;
    Wolf delta = population.at(2).wolf;

    for(int i=0; i<numberOfPopulation; i++){
        for(int j=0; j<wolfSize; j++){
            // Alpha
            double r1 = uniform(0, 1);
            double r2 = uniform(0, 1);
            double A1 = 2.0 * a * r1 - a;
            double C1 = 2.0 * r2;
            double dAlpha = std::abs(C1 * alpha.at(j) - population.at(i).wolf.at(j));
            double X1 = alpha.at(j) - A1 * dAlpha;
            // Beta
            r1 = uniform(0, 1);
            r2 = uniform(0, 1);
            A1 = 2.0 * a * r1 - a;
            C1 = 2.0 * r2;
            double dBeta = std::abs(C1 * beta.at(j) - population.at(i).wolf.at(j));
            double X2 = beta.at(j) - A1 * dBeta;
            // Delta
            r1 = uniform(0, 1);
            r2 = uniform(0, 1);
            A1 = 2.0 * a * r1 - a;
            C1 = 2.0 * r2;
            double dDelta = std::abs(C1 * delta.at(j) - population.at(i).wolf.at(j));
            double X3 = delta.at(j) - A1 * dDelta;

            int x1 = std::abs((int) X1) % numberOfFogNodes;
            int x2 = std::abs((int) X2) % numberOfFogNodes;
            int x3 = std::abs((int) X3) % numberOfFogNodes;
            population.at(i).wolf.at(j) = crossOver(x1, x2, x3);
            EV << "X1, X2, X3: " << X1 << ", " << X2 << ", " << X3 << ", " << " and x1, x2, x3: " << x1 << ", " << x2 << ", " << x3 << " and crossover:" << population.at(i).wolf.at(j) << endl;
        }
        population.at(i).valueFitnessCostFunctions = fitnessFunction(population.at(i).wolf);
        EV << "Updated Wolf#" << i << ": " << population.at(i) << endl;
    }
}

GrayWolf::Individualt GrayWolf::executeGrayWolf(){

    generationOfInitialPopulation();
    sort(population.begin(), population.end(), Individual::compareIndividuals);
    Individualt bestIndividual = population.at(0);
    for(int i=0; i<numberOfIterations; i++){
        double a = 2 * (1 - (i * i) / (numberOfIterations * numberOfIterations));
        updateWolves(a);
        sort(population.begin(), population.end(), Individual::compareIndividuals);
        bestIndividual = population.at(0);
    }
    statistics->collectGrayWolfParameters(bestIndividual.valueFitnessCostFunctions.serviceTimeValue, bestIndividual.valueFitnessCostFunctions.serviceCostValue, bestIndividual.valueFitnessCostFunctions.energyConsumptionValue, bestIndividual.valueFitnessCostFunctions.fitnessValue);
    return bestIndividual;

}

void GrayWolf::handleMessage(cMessage *msg)
{
    Individual bestIndividual;
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        bestIndividual = executeGrayWolf();
    }
    EV << "The best individual is " << bestIndividual << endl;

}

GrayWolf::~GrayWolf()
{
    cancelAndDelete(selfMsg);
}

std::ostream& operator<<(std::ostream& os, const struct GrayWolf::Service& service)
{
     os << "{dataSize: " << service.dataSize;
     os << ", requestSize: " << service.requestSize;
     os << ", responseSize: " << service.responseSize;
     os << ", serviceDeadline: " << service.serviceDeadline << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct GrayWolf::FogNode& fogNode)
{
     os << "{hostName: " << fogNode.host->getFullName();
     os << ", ipAddress: " << fogNode.ipAddress;
     os << ", processingCapacity: " << fogNode.processingCapacity;
     os << ", linkCapacity: " << fogNode.linkCapacity;
     os << ", powerIdle: " << fogNode.powerIdle;
     os << ", powerTransmission: " << fogNode.powerTransmission;
     os << ", powerProcessing: " << fogNode.powerProcessing << "}";
    return os;
}
/*
std::ostream& operator<<(std::ostream& os, const GrayWolf::Wolf& wolf)
{
    os << "{";
    for(int i=0; i<wolf.size(); i++){
        os << "wolf[" << i << "]: " << wolf.at(i);
        if(i!=wolf.size()-1)
            os << ", ";
    }
    os << "}";
}
*/
std::ostream& operator<<(std::ostream& os, const struct GrayWolf::Individual& individual)
{
     os << "{wolf: " << individual.wolf;
     os << ", fitnessValue: " << individual.valueFitnessCostFunctions.fitnessValue;
     os << ", serviceTimeValue: " << individual.valueFitnessCostFunctions.serviceTimeValue;
     os << ", serviceCostValue: " << individual.valueFitnessCostFunctions.serviceCostValue;
     os << ", energyConsumptionValue: " << individual.valueFitnessCostFunctions.energyConsumptionValue << "}";
}
} // namespace fogfn
