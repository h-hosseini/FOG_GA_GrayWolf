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

#ifndef _FOGFN_SRC_GRAYWOLF_H_
#define _FOGFN_SRC_GRAYWOLF_H_

#define NUMBER_OF_TASKS 5
#define NUMBER_OF_JOBS 5

#include "inet/common/INETDefs.h"
#include "src/statistics/Statistics.h"
#include "inet/networklayer/common/L3AddressResolver.h"
//#include "inet/clock/contract/ClockEvent.h"
//#include "inet/common/clock/ClockUserModuleMixin.h"


namespace fogfn {
using namespace inet;

class GrayWolf : public cSimpleModule
{
    int numberOfFogNodes;
    int numberOfIterations;
    int numberOfServices;
    int numberOfPopulation;
    int wolfSize;
    double dataSizePerMip;
    double unitCostProcessing; //unit cost in $ for processing resource in mips at fog node
    double unitCostStorage; //unit cost in $ for storage resource at fog node (per byte per second)
    double unitCostEnergy; //unit cost in $ for energy consumption per joule in fog environment

    Statistics *statistics;
    //ClockEvent *selfMsg;
    cMessage *selfMsg;
    simtime_t startTime;
    //clocktime_t startTime;

    struct FogNode{
        cModule *host;
        L3Address ipAddress;
        //int nodeIndex;

        double processingCapacity; //processing capability of a fog node j
        double linkCapacity; //bandwidth between nodes in Mbps

        double powerIdle; // power consumption of fog node in idle state in watts
        double powerTransmission; // maximum power consumption during transmission in watts
        double powerProcessing; // maximum power consumption during processing in watts

        FogNode()
            : host(nullptr)
            , ipAddress (Ipv4Address::UNSPECIFIED_ADDRESS)
            , processingCapacity (0)
            , linkCapacity(0)
            , powerIdle(0)
            , powerTransmission (0)
            , powerProcessing (0)
            {};

    };
    friend std::ostream& operator<<(std::ostream& os, const struct FogNode& fogNode);

    typedef std::vector<struct FogNode> FogNodes;
    FogNodes fogNodes;


    struct Service{
        double dataSize; // service size (in Bytes) to calculate the processing time
        double requestSize; // size of service request(in Bytes) to calculate the communication time
        double responseSize; // size of service response(in Bytes) to calculate the communication time

        double serviceDeadline;


        Service()
            : dataSize(0)
            , requestSize(0)
            , responseSize(0)
            , serviceDeadline(0)
            {};

    };
    friend std::ostream& operator<<(std::ostream& os, const struct Service& service);

    typedef std::vector<struct Service> Services;
    Services services;

    //A wolf data structure
    typedef std::vector <int> Wolf;
    //friend std::ostream& operator<<(std::ostream& os, const Wolf& wolf);

    //Wolf wolf;

    struct ValueFitnessCostFunctionsStruct{
        double serviceTimeValue;
        double serviceCostValue;
        double energyConsumptionValue;
        double fitnessValue;
    };
    typedef struct ValueFitnessCostFunctionsStruct ValueFitnessCostFunctions;

    struct Individual{
        Wolf wolf;
        ValueFitnessCostFunctions valueFitnessCostFunctions;

        public:
            static bool compareIndividuals(Individual &a, Individual &b) {
                return a.valueFitnessCostFunctions.fitnessValue > b.valueFitnessCostFunctions.fitnessValue;
        }
    };
    friend std::ostream& operator<<(std::ostream& os, const struct Individual& individual);

    typedef struct Individual Individualt;
    typedef std::vector <Individualt> Population;
    Population population;
    Population newPopulation;

public:
    GrayWolf()
        : wolfSize(0)
        , numberOfFogNodes(0)
        , numberOfIterations(0)
        , numberOfServices(0)
        , numberOfPopulation(0)
        , dataSizePerMip(0)
        , unitCostProcessing(0)
        , unitCostStorage(0)
        , unitCostEnergy(0)
        , statistics(nullptr)
        , selfMsg(nullptr)
        , startTime(SIMTIME_ZERO)
        //, startTime(CLOCKTIME_ZERO)
        {};

    ~GrayWolf();

protected:

    virtual void initialize(int stage);
    virtual double serviceTime(Wolf wolf);
    virtual double energyConsumption(Wolf wolf);
    virtual double serviceCost(Wolf wolf);
    virtual ValueFitnessCostFunctions fitnessFunction(Wolf wolf);
    virtual void generationOfInitialPopulation();
    virtual int crossOver(int x1, int x2, int x3);
    virtual void updateWolves(double a);
    virtual Individualt executeGrayWolf();

public:
    virtual void registFogNodesInfo(cModule *host, L3Address ipAddress, int fogIndex);
    virtual void registFogServiceInfo(L3Address srcAddr, double processingCapacity, double linkCapacity, double powerIdle, double powerTransmission, double powerProcessing, double dataSize, double requestSize, double responseSize, double serviceDeadline);
    virtual void handleMessage(cMessage *msg);


};

} // namespace fogfn

#endif // ifndef _FOGFN_SRC_GrayWolf_H_
