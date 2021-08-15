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

package fogfn.src.ga;

simple GA
{
    parameters:
        @class(fogfn::GA);
        @display("i=showcases/marsrover");
        @labels(optimizer);
        //Network parameter
        int numberOfFogNodes = default(5);
    	//Population parameters
    	int numberOfIterations = default(10);
    	int numberOfPopulation = default(100);
    	double elitismRate = default(0.08);
        double crossoverProbability = default(0.5);
        double mutationRate = default(0.3);
        //Processing parameters
        double dataSizePerMip = default(0);
        //Cost parameters
        double unitCostProcessing = default(0); //unit cost in $ for processing resource in mips at fog node
        double unitCostStorage = default(0); //unit cost in $ for storage resource at fog node (per byte per second)
        double unitCostEnergy = default(0); //unit cost in $ for energy consumption per joule in fog environment       
}