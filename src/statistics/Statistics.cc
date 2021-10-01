
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

#include <vector>
#include <algorithm>    // std::sort
#include "src/statistics/Statistics.h"


namespace fogfn {
using namespace inet;

Define_Module(Statistics);

Statistics::Statistics(){

}

void Statistics::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){

    }

}


void Statistics::handleMessage(cMessage* msg)
{
    error("This module doesn't handle any message!");
}

void Statistics::collectGaParameters(double serviceTimeValue, double serviceCostValue, double energyConsumptionValue, double fitnessValue)
{
    //Time statistics
    FILE *serviceTimeValueFp, *serviceCostValueFp, *energyConsumptionValueFp, *fitnessValueFp;

    fitnessValueFp = fopen("1_fitnessValueGa.txt", "a");
    fprintf(fitnessValueFp, "%f\n", fitnessValue);
    fclose(fitnessValueFp);

    serviceTimeValueFp = fopen("2_serviceTimeValueGa.txt", "a");
    fprintf(serviceTimeValueFp, "%f\n", serviceTimeValue);
    fclose(serviceTimeValueFp);

    serviceCostValueFp = fopen("3_serviceCostValueGa.txt", "a");
    fprintf(serviceCostValueFp, "%f\n", serviceCostValue);
    fclose(serviceCostValueFp);

    energyConsumptionValueFp = fopen("4_energyConsumptionValueGa.txt", "a");
    fprintf(energyConsumptionValueFp, "%f\n", energyConsumptionValue);
    fclose(energyConsumptionValueFp);

}

void Statistics::collectGrayWolfParameters(double serviceTimeValue, double serviceCostValue, double energyConsumptionValue, double fitnessValue)
{
    //Time statistics
    FILE *serviceTimeValueFp, *serviceCostValueFp, *energyConsumptionValueFp, *fitnessValueFp;

    fitnessValueFp = fopen("5_fitnessValueGrayWolf.txt", "a");
    fprintf(fitnessValueFp, "%f\n", fitnessValue);
    fclose(fitnessValueFp);

    serviceTimeValueFp = fopen("6_serviceTimeValueGrayWolf.txt", "a");
    fprintf(serviceTimeValueFp, "%f\n", serviceTimeValue);
    fclose(serviceTimeValueFp);

    serviceCostValueFp = fopen("7_serviceCostValueGrayWolf.txt", "a");
    fprintf(serviceCostValueFp, "%f\n", serviceCostValue);
    fclose(serviceCostValueFp);

    energyConsumptionValueFp = fopen("8_energyConsumptionValueGrayWolf.txt", "a");
    fprintf(energyConsumptionValueFp, "%f\n", energyConsumptionValue);
    fclose(energyConsumptionValueFp);

}

Statistics::~Statistics()
{
    //cancelAndDelete(...);
}

} // namespace fogfn
