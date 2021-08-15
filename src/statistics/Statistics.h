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

#ifndef _FOGFN_SRC_STATISTICS_H_
#define _FOGFN_SRC_STATISTICS_H_

#include "inet/common/INETDefs.h"

namespace fogfn {
using namespace inet;

//class GA;

class Statistics : public cSimpleModule
{

public:
    Statistics();

    ~Statistics();

protected:

    virtual void initialize(int stage);

    virtual void saveStatistics();

    virtual void handleMessage(cMessage* msg);

public:

};

} // namespace fogfn

#endif // ifndef _FOGFN_SRC_STATISTICS_H_
