//
// Created by barkerm on 19/04/17.
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "fixparser.h"
#include "utils.h"

void start(void* a, int b, int c) {}

void tag(void* a, int b, const char* c, int d) {}

void end(void* a) {}

int main(int argc, char** argv)
{
    const char* fix_message =
        "8=FIX.4.2|9=224|35=8|49=REUTERS|56=BROKER04|34=4|52=20170412-23:44:20.150|60=20170412-23:44:20.117|20=0|"
        "22=8|6=0|11=ORD10001|17=QLAKAAAAAAAAAAAC|48=180385|55=HPQ|1=1|37=QLAKAAAAAAAAAAAB|151=10.0|14=0.0|38=10.0|"
        "54=1|44=100.0|59=0|150=0|39=0|10=010|";

    char* buf = fixify(fix_message);

    const int i = parse_fix_message(buf, strlen(buf), NULL, start, tag, end);
    assert(i > 0);

}