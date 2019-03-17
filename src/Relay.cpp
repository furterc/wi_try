/*
 * Relay.cpp
 *
 *  Created on: 17 Mar 2019
 *      Author: christo
 */

#include <Relay.h>

Relay::Relay(PinName pin) : DigitalInOut(pin)
{
    output();
    mode(OpenDrainNoPull);
    write(1);
}

Relay::~Relay()
{
    // TODO Auto-generated destructor stub
}

void Relay::latch(int state)
{
    if(state == 1)
        write(0);
    else
        write(1);
}

int Relay::get()
{
    if(!read())
        return 1;

    return 0;
}
