//
// Copyright (C) 2014 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

package fogfn.simulation;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.AdhocHost;
import inet.physicallayer.wireless.ieee80211.bitlevel.Ieee80211LayeredScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;

import fogfn.src.ga.GA;
import fogfn.src.grayWolf.GrayWolf;
import fogfn.src.statistics.Statistics;

network Sim1
{
    parameters:
        int numberOfFogNodes;
        string optimizerType = default("Genetic");
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            parameters:
                @display("p=35,39;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                @display("p=201,39;is=s");
        }
        radioMedium: Ieee80211LayeredScalarRadioMedium {
            parameters:
                @display("p=119,39;is=s");
        }
        gaOptimizer: GA if optimizerType == "Genetic"{
            @display("p=276,39;is=s");
        }
        grayWolfOptimizer: GrayWolf if optimizerType == "GrayWolf"{
            @display("p=373,39;is=s");
        }
        statistics: Statistics {
            @display("p=459,39;is=s");
        }
        admin: AdhocHost {
            parameters:
                @display("r=,,#707070;p=250,200;i=device/pc2");
        }
        fognode[numberOfFogNodes]: AdhocHost {
            parameters:
                @display("r=,,#707070;p=250,200;i=device/pc3");
        }
}
