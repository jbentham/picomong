// Mongoose Web server on Pi PicoW, see https://iosoft.blog/picomong
//
// Copyright (c) 2024, Jeremy P Bentham
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// v0.01 JPB 7/1/24   Adapted from pedla.c v0.22
// v0.02 JPB 8/1/24   Updated mongoose, moved to separate directory
// v0.03 JPB 20/8/24  Updated mongoose again
// v0.04 JPB 20/8/24  Removed dynamic file test
// v0.05 JPB 20/8/24  Updated mongoose, restored device dashboard
//                    Changed Web port from 8000 to 80 in CMakeLists

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "picowi/picowi_defs.h"
#include "picowi/picowi_auth.h"

#include "mongoose/mongoose.h"
#include "mongoose/net.h"
#include "mg_wifi.h"

#define SW_VERSION  "0.05"
char version[] = "Picomong v" SW_VERSION;

// Timeout values in msec
#define LINK_UP_BLINK       500
#define LINK_DOWN_BLINK     100
#define JOIN_DOWN_MS        3000   

bool force_down;
uint ready_ticks, led_ticks;

void listener(struct mg_connection *c, int ev, void *ev_data);
int link_check(void);
int mstimeout(uint *tickp, int msec);

int main(void) 
{
    bool ledon = 0;
    stdio_init_all();

    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    uint8_t *id = board_id.id;
    struct mg_mgr mgr;
    struct mg_tcpip_if mif = {.driver = &mg_tcpip_driver_wifi, .mgr = &mgr};

    mg_mgr_init(&mgr);
    mg_log_set(MG_LL_NONE);
    web_init(&mgr);
    //mg_log_set(MG_LL_DEBUG);
    xprintf("\n%s\n", version);
    mg_tcpip_init(&mgr, &mif);
    mg_http_listen(&mgr, "http://0.0.0.0:8000", listener, &mgr);
    
    // Main polling loop
    for (;;) 
    {
        mg_mgr_poll(&mgr, 50);
        if (mstimeout(&led_ticks, link_check() > 0 ? LINK_UP_BLINK : LINK_DOWN_BLINK))
            wifi_set_led(ledon = !ledon);
        if (!mif.driver->up(0))
            wifi_poll(0, 0);
    }
    return 0;
}

// Return non-zero if timeout
int mstimeout(uint *tickp, int msec)
{
    uint t = mg_millis();
    uint dt = t - *tickp;

    if (msec == 0 || dt >= msec)
    {
        *tickp = t;
        return (1);
    }
    return (0);
}

// Check for network down/up/ready change of state
// Return descriptive string if state has changed
char *state_change(int state)
{
    static int last_state = 0;
    static char *states[] = { "DOWN", "UP", "REQ", "READY" };

    if (state != last_state && state >= MG_TCPIP_STATE_DOWN && state <= MG_TCPIP_STATE_READY)
    {
        last_state = state;
        return states[state];
    }
    return 0;
}

// Connection callback
void listener(struct mg_connection *c, int ev, void *ev_data)
{
    static int num = 0;
    struct mg_mgr *mgrp = c->mgr;
    struct mg_tcpip_if *ifp = mgrp->priv;

    if (ev != MG_EV_HTTP_MSG) 
    {
        char *s = state_change(ifp->state);
        if (s) 
        {
            if (ifp->state == MG_TCPIP_STATE_READY)
                xprintf("IP state: %s, IP: %M, GW: %M\n", s, mg_print_ip4, &ifp->ip, mg_print_ip4, &ifp->gw);
            else
                xprintf("IP state: %s\n", s);
            mstimeout(&ready_ticks, 0);
        }
        else if (ifp->state != MG_TCPIP_STATE_READY && mstimeout(&ready_ticks, JOIN_DOWN_MS))
            force_down = !force_down;
    }
}

// Dummy function to keep Mongoose happy
int mkdir(const char *s, mode_t m)
{
    return 0;
}

// EOF
