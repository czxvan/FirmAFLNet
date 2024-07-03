/*
 * QEMU TCG Multi Threaded vCPUs implementation
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "sysemu/tcg.h"
#include "sysemu/replay.h"
#include "sysemu/cpu-timers.h"
#include "qemu/main-loop.h"
#include "qemu/notify.h"
#include "qemu/guest-random.h"
#include "qemu/log.h"
#include "exec/tb-flush.h"
#include "exec/cpu_ldst.h"
#include "exec/exec-all.h"
#include "hw/boards.h"
#include "tcg/startup.h"
#include "tcg-accel-ops.h"
#include "tcg-accel-ops-mttcg.h"
#include "plugin_spy/aflspy.h"

typedef struct MttcgForceRcuNotifier {
    Notifier notifier;
    CPUState *cpu;
} MttcgForceRcuNotifier;

static void do_nothing(CPUState *cpu, run_on_cpu_data d)
{
}

static void mttcg_force_rcu(Notifier *notify, void *data)
{
    CPUState *cpu = container_of(notify, MttcgForceRcuNotifier, notifier)->cpu;

    /*
     * Called with rcu_registry_lock held, using async_run_on_cpu() ensures
     * that there are no deadlocks.
     */
    async_run_on_cpu(cpu, do_nothing, RUN_ON_CPU_NULL);
}

/*
 * In the multi-threaded case each vCPU has its own thread. The TLS
 * variable current_cpu can be used deep in the code to find the
 * current CPUState for a given thread.
 */
static void *mttcg_cpu_thread_fn(void *arg)
{
// again:
    MttcgForceRcuNotifier force_rcu;
    CPUState *cpu = arg;

    assert(tcg_enabled());
    g_assert(!icount_enabled());

    rcu_register_thread();
    force_rcu.notifier.notify = mttcg_force_rcu;
    force_rcu.cpu = cpu;
    rcu_add_force_rcu_notifier(&force_rcu.notifier);
    tcg_register_thread();
    
    bql_lock();
    qemu_thread_get_self(cpu->thread);

    cpu->thread_id = qemu_get_thread_id();
    cpu->neg.can_do_io = true;
    current_cpu = cpu;
    cpu_thread_signal_created(cpu);
    qemu_guest_random_seed_thread_part2(cpu->random_seed);

    /* process any pending work */
    cpu->exit_request = 1;

    do {
        if (cpu_can_run(cpu)) {
            int r;
            bql_unlock();
            r = tcg_cpu_exec(cpu);
            bql_lock();
            switch (r) {
            case EXCP_DEBUG:
                cpu_handle_guest_debug(cpu);
                break;
            case EXCP_HALTED:
                /*
                 * Usually cpu->halted is set, but may have already been
                 * reset by another thread by the time we arrive here.
                 */
                break;
            case EXCP_ATOMIC:
                bql_unlock();
                cpu_exec_step_atomic(cpu);
                bql_lock();
            default:
                /* Ignore everything else? */
                break;
            }
        }

        qatomic_set_mb(&cpu->exit_request, 0);
        qemu_wait_io_event(cpu);
    } while (!cpu->unplug || cpu_can_run(cpu));

    tcg_cpu_destroy(cpu);
    bql_unlock();
    rcu_remove_force_rcu_notifier(&force_rcu.notifier);
    rcu_unregister_thread();
    return NULL;
}

void mttcg_kick_vcpu_thread(CPUState *cpu)
{
    cpu_exit(cpu);
}

void mttcg_start_vcpu_thread(CPUState *cpu)
{
    char thread_name[VCPU_THREAD_NAME_SIZE];

    g_assert(tcg_enabled());
    tcg_cpu_init_cflags(cpu, current_machine->smp.max_cpus > 1);

    cpu->thread = g_new0(QemuThread, 1);
    cpu->halt_cond = g_malloc0(sizeof(QemuCond));
    qemu_cond_init(cpu->halt_cond);

    /* create a thread per vCPU with TCG (MTTCG) */
    snprintf(thread_name, VCPU_THREAD_NAME_SIZE, "CPU %d/TCG",
             cpu->cpu_index);

    qemu_thread_create(cpu->thread, thread_name, mttcg_cpu_thread_fn,
                       cpu, QEMU_THREAD_JOINABLE);
}

void dump_CPUState(CPUState *cpu) {
    static int count = 0;
    count ++;
    FILE *log = qemu_log_trylock();
    if (log == NULL) {
        return;
    }
    fprintf(log, "--------------------------------\n");
    fprintf(log, "dump_CPUState %d:\n", count);
    fprintf(log, "\tnr_cores = %d\n", cpu->nr_cores);
    fprintf(log, "\tnr_threads = %d\n", cpu->nr_threads);
    fprintf(log, "\tthread_id = %d\n", cpu->thread_id);
    fprintf(log, "\trunning = %d\n", cpu->running);
    fprintf(log, "\thas_waiter = %d\n", cpu->has_waiter);
    fprintf(log, "\tthread_kicked = %d\n", cpu->thread_kicked);
    fprintf(log, "\tcreated = %d\n", cpu->created);
    fprintf(log, "\tstop = %d\n", cpu->stop);
    fprintf(log, "\tstopped = %d\n", cpu->stopped);
    fprintf(log, "\tstart_powered_off = %d\n", cpu->start_powered_off);
    fprintf(log, "\tunplug = %d\n", cpu->unplug);
    fprintf(log, "\tcrash_occurred = %d\n", cpu->crash_occurred);
    fprintf(log, "\texit_request = %d\n", cpu->exit_request);
    fprintf(log, "\texclusive_context_count = %d\n", cpu->exclusive_context_count);
    fprintf(log, "\tcflags_next_tb = %u\n", cpu->cflags_next_tb);
    fprintf(log, "\tinterrupt_request = %u\n", cpu->interrupt_request);
    fprintf(log, "\tsinglestep_enabled = %d\n", cpu->singlestep_enabled);
    fprintf(log, "\ticount_budget = %ld\n", cpu->icount_budget);
    fprintf(log, "\ticount_extra = %ld\n", cpu->icount_extra);
    fprintf(log, "\trandom_seed = %lu\n", cpu->random_seed);
    fprintf(log, "\tcpu_index = %d\n", cpu->cpu_index);
    fprintf(log, "\tcluster_index = %d\n", cpu->cluster_index);
    fprintf(log, "\ttcg_cflags = %u\n", cpu->tcg_cflags);
    fprintf(log, "\thalted = %u\n", cpu->halted);
    fprintf(log, "\texception_index = %d\n", cpu->exception_index);
    fprintf(log, "\tvcpu_dirty = %d\n", cpu->vcpu_dirty);
    fprintf(log, "\tthrottle_thread_scheduled = %d\n", cpu->throttle_thread_scheduled);
    fprintf(log, "\tthrottle_us_per_full = %ld\n", cpu->throttle_us_per_full);
    fprintf(log, "\tignore_memory_transaction_failures = %d\n", cpu->ignore_memory_transaction_failures);
    fprintf(log, "\tprctl_unalign_sigbus = %d\n", cpu->prctl_unalign_sigbus);
    qemu_log_unlock(log);

    CPUArchState *env = cpu_env(cpu);
    FILE *logfile = qemu_log_trylock();
    if(logfile) {
        cpu_dump_state(cpu, logfile, CPU_DUMP_CODE | CPU_DUMP_VPU);
        fprintf(logfile, "pgdir = %08x %08x %08x %08x\n"
                , (uint32_t)env->cp15.ttbr0_el[1]
                , (uint32_t)env->cp15.ttbr0_el[3]
                , (uint32_t)env->cp15.ttbr1_el[1]
                , (uint32_t)env->cp15.ttbr1_el[3]
                );
        fprintf(logfile, "--------------------------------\n");
        qemu_log_unlock(logfile);
    }
}