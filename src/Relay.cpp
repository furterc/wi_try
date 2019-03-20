/*
 * Relay.cpp
 *
 *  Created on: 17 Mar 2019
 *      Author: christo
 */

#include <Relay.h>

Relay::Relay(PinName pin, eRelayState state) : DigitalInOut(pin), mState(state)
{
    output();
    mode(OpenDrainNoPull);

    write(state);
}

Relay::~Relay()
{
    // TODO Auto-generated destructor stub
}

void Relay::latch(int state)
{
    if(state == 1)
        write(!mState);
    else
        write(mState);
}

int Relay::get()
{
    if(!read() && mState == RELAY_NO)
        return 1;

    if(read() && mState == RELAY_NC)
        return 1;

    return 0;
}
