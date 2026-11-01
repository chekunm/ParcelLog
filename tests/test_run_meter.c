#include "run_meter.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    Release r={.period=100000,.deadline=120000,.due=0xffff0000};
    assert(release_ready(&r,0xffff0000)); release_finish(&r,0xffff0000+1800);
    assert(!release_ready(&r,0xffff0000+99999));
    assert(release_ready(&r,0xffff0000+300007));
    assert(r.skipped==2 && r.jitter_max==200007);
    release_finish(&r,0xffff0000+420001); assert(r.late==1);
    RunMeter m={0}; meter_add(&m,9);meter_add(&m,1);meter_add(&m,20);
    assert(m.n==3 && m.min==1 && m.max==20 && m.sum==30);
    puts("run_meter: wrap, anchored releases, exact skipped slots and completion deadlines passed");
    return 0;
}
