#define EXIT    1
#define FORK    2
#define READ    3
#define WRITE   4
#define OPEN    5
#define CLOSE   6
#define EXECVE  11
#define CLONE   120

#define SOCKET  281
#define BIND    282
#define LISTEN  284
#define ACCEPT  285

#define SEND    289
#define SENDTO  290
#define SENDMSG 296
#define RECV    291
#define RECVFROM    292

#define CTL_READ_FD 198
#define STATE_WRITE_FD 199

#define MEM_BARRIER() \
  __asm__ volatile("" ::: "memory")

#define LOG_ERROR(...) \
    { \
        g_autofree gchar *log = g_strdup_printf("[ERROR] " __VA_ARGS__); \
        qemu_plugin_outs(log); \
    }

#define LOG_STATEMENT(...) \
    { \
        g_autofree gchar *log = g_strdup_printf(__VA_ARGS__); \
        qemu_plugin_outs(log); \
    }

#define LOG_STATEMENT_WITH_CTX(...) \
    g_autofree gchar *log = g_strdup_printf("ctx: %08x  " __VA_ARGS__, target_ctx); \
    qemu_plugin_outs(log);

#define LOG_MASK(cond) ((cond) && system_started && info->ctx == target_ctx)
#define QEMU_USER_MODE (g_strcmp0(QEMU_MODE, "USER") == 0)
#define QEMU_SYSTEM_MODE (g_strcmp0(QEMU_MODE, "USER") != 0)

typedef struct {
    uint32_t error_code;
} ExitParams;

typedef struct {
    uint32_t fd;
    const char *buf;
    uint32_t count;
} ReadParams;

typedef struct {
    uint32_t fd;
    const char *buf;
    uint32_t count;
} WriteParams;

typedef struct {
    const char *filename;
} OpenParams;

typedef struct {
    uint32_t fd;
} CloseParams;

typedef struct {
    const char *filename;
} ExecveParams;

typedef struct
{
    uint32_t    sockfd;
    const void  *buf;
    uint32_t    len;
    uint32_t    flags;
} SendParams;

typedef struct
{
    uint32_t    sockfd;
    const void  *buf;
    uint32_t    len;
    uint32_t    flags;
    uint32_t    dest_addr;
    uint32_t    dest_len;
} SendtoParams;

typedef struct
{
    uint32_t    sockfd;
    struct user_msghdr *msg;
    uint32_t    flags;
} SendmsgParams;

typedef struct
{
    uint32_t    sockfd;
    void        *buf;
    uint32_t    len;
    uint32_t    flags;
} RecvParams;

typedef struct
{
    uint32_t    sockfd;
    void        *buf;
    uint32_t    len;
    uint32_t    flags;
    uint32_t    src_addr;
    uint32_t    src_len;
} RecvfromParams;

typedef struct
{
    uint32_t    domain;
    uint32_t    type;
    uint32_t    protocol;
} SocketParams;

typedef struct
{
    uint32_t    sockfd;
    uint32_t    sock_addr;
    uint32_t    addr_len;
} BindParams;

typedef struct
{
    uint32_t    sockfd;
    uint32_t    backlog;
} ListenParams;

typedef struct
{
    uint32_t    sockfd;
    uint32_t    sock_addr;
    uint32_t    addr_len;
} AcceptParams;

typedef struct {
    uint32_t num;
    uint32_t ctx;
    union {
        ExitParams      *exit_params;
        ReadParams      *read_params;
        WriteParams     *write_params;
        OpenParams      *open_params;
        CloseParams     *close_params;
        ExecveParams    *execve_params;
        SendParams      *send_params;
        SendtoParams    *sendto_params;
        SendmsgParams    *sendmsg_params;
        RecvParams      *recv_params;
        RecvfromParams  *recvfrom_params;
        SocketParams    *socket_params;
        ListenParams    *listen_params;
        BindParams      *bind_params;
        AcceptParams    *accept_params;
    } params;
} SyscallInfo;

typedef struct {
    uint32_t ctx;
    uint32_t pc;
} TBInfo;

typedef struct {
    uint32_t ctx;
    uint32_t addr;
    uint32_t paddr;
    uint32_t prot;
    uint32_t mmu_idx;
} TLBInfo;

typedef struct {
    uint32_t ctx;
    uint32_t excp;
    uint32_t syndrome;
    uint32_t target_el;
    uint32_t exception_class;
} ExceptionInfo;

#define SYSTEM_STARTED_INDICATOR_PROCESS "/usr/bin/phosphor-host-state-manager"

// Only used in qemu internal part
#ifndef __AFL_SPY_C__
extern gboolean system_started;
extern gboolean forkserver_started;
extern gboolean afl_wants_cpu_to_stop;
extern int afl_qemuloop_pipe[2];
extern CPUState *snapshot_cpu;
extern CPUState *backup_cpu;
extern CPUArchState *backup_env;
extern uint32_t vcpu_thread_count;
void gotPipeNotification(void *data);
void afl_setup(void);
void afl_forkserver(CPUArchState *env);

void dump_CPUState(CPUState *cpu);

#endif

#ifdef __AFL_SPY_C__

#include <sys/shm.h>

#define MAP_SIZE_POW2       16
#define MAP_SIZE            (1 << MAP_SIZE_POW2)

#define SHM_ENV_VAR         "__AFL_SHM_ID"
#define SHM_ENV_VAR_SPY       "__AFL_SHM_ID_SPY"

struct SpySignal {
    uint8_t trace_enabled;
    uint8_t next_step;
};

static uint32_t target_ctx = 0;
static uint32_t agent_ctx = 0;
static gboolean system_started = false;

static unsigned int afl_inst_rms = MAP_SIZE;
static unsigned char *trace_bits = NULL;
static struct SpySignal *spy_signal = NULL;

static uint8_t coverage_info[MAP_SIZE] = {0}; // debug coverage

static void afl_setup(void) {
    char *id_str = getenv(SHM_ENV_VAR),
         *id_str_spy = getenv(SHM_ENV_VAR_SPY),
         *inst_r = getenv("AFL_INST_RATIO");

    int shm_id, shm_id_spy;
    if (id_str && id_str_spy) {
        shm_id = atoi(id_str);
        shm_id_spy = atoi(id_str_spy);
        trace_bits = shmat(shm_id, NULL, 0);
        spy_signal = shmat(shm_id_spy, NULL, 0);
        if (trace_bits == (void *)-1 || spy_signal == (void *)-1) {
            LOG_ERROR("Failed to attach to shared memory\n");
        } else {
            LOG_STATEMENT("Attached to shared memory "
                          "trace_bits: %p, spy_signal: %p\n"
                          , trace_bits, spy_signal);
        }
    } else {
        LOG_ERROR("SHM_ENV_VAR or SHM_ENV_VAR_SPY not set "
                  "id_str: %s, id_str_spy: %s\n", id_str, id_str_spy);
    }

    if(inst_r && trace_bits) {
        unsigned int r = atoi(inst_r);
        if (r > 100) r = 100;
        if (!r) r = 1;
        afl_inst_rms = MAP_SIZE * r / 100;

        /* With AFL_INST_RATIO set to a low value, we want to touch the bitmap
           so that the parent doesn't give up on us. */
        trace_bits[0] = 1;
    }

}

static inline void afl_maybe_log(uint32_t cur_loc) {
    if (!trace_bits || !spy_signal || !(spy_signal->trace_enabled))
        return;

    // debug coverage
    // if (cur_loc > 0x00400000 && cur_loc < 0x00800000) {
        coverage_info[(cur_loc - 0x00400000) / 0x40] = 1;
        // trace_bits[(cur_loc - 0x00400000) / 0x40] = 1;

        // {
        //     cur_loc = (cur_loc >> 4) ^ (cur_loc << 8);
        //     cur_loc &= MAP_SIZE - 1;

        //     if (cur_loc >= afl_inst_rms) return;

        //     trace_bits[cur_loc] = 1;
        // }
    

        {
            static __thread uint32_t prev_loc = 0;

            cur_loc = (cur_loc >> 4) ^ (cur_loc << 8);
            cur_loc &= MAP_SIZE - 1;

            /* Implement probabilistic instrumentation by looking at scrambled block
            address. This keeps the instrumented locations stable across runs. */

            if (cur_loc >= afl_inst_rms) return;

            trace_bits[cur_loc ^ prev_loc]++;
            prev_loc = cur_loc >> 1;
        }

        
    // }

}

static inline int is_trace_enabled(void) {
    MEM_BARRIER();
    return spy_signal && spy_signal->trace_enabled;
}

static inline int is_afl_spying(void) {
    return spy_signal;
}

static inline int is_nextstep_zero() {
    return spy_signal && (spy_signal->next_step == 0);
}

static inline void set_nextstep(uint8_t value) {
    if (spy_signal) {
        spy_signal->next_step = value;
    }
}

static void set_agent_ctx(uint32_t ctx) {
    agent_ctx = ctx;
    LOG_STATEMENT("Agent Context set to %08x\n", agent_ctx);
    if (write(STATE_WRITE_FD, &agent_ctx, 4) != 4) {
        LOG_ERROR("Failed to write agent_ctx to STATE_WRITE_FD\n");
    } else {
        LOG_STATEMENT("Wrote agent_ctx to STATE_WRITE_FD\n");
    }
}

static void set_target_ctx(uint32_t ctx) {
    target_ctx = ctx;
    LOG_STATEMENT("Target Context set to %08x\n", target_ctx);
    if (write(STATE_WRITE_FD, &target_ctx, 4) != 4) {
        LOG_ERROR("Failed to write target_ctx to STATE_WRITE_FD\n");
    } else {
        LOG_STATEMENT("Wrote target_ctx to STATE_WRITE_FD\n");
    }
}

// static int is_empty(unsigned char *buf, int len) {
//     for (int i = 0; i < len; i++) {
//         if (buf[i] != 0) return 0;
//     }
//     return 1;
// }

#endif