#include <openssl/ssl.h>

#define DEFAULT_TEST_ALIVE_SCRIPT "scripts/test_alive.sh"
#define DEFAULT_TEST_AGENT_SCRIPT "scripts/test_agent.sh"
#define DEFAULT_RESTART_TARGET_SCRIPT "scripts/restart_target.sh"

#define SHM_ENV_VAR_SPY         "__AFL_SHM_ID_SPY"

#define ST_TEST_ALIVE   0   // target is alive
#define ST_TEST_FAILED  7   // connection failed which should never happen
#define ST_TEST_TIMEOUT 28  // target blocked or crashed
#define ST_RESTART_SUCCESS   0


// Though it's not recommended to define a global variable in a header file,
// it's fine for this project.
static uint32_t target_ctx = 0;
static uint32_t agent_ctx = 0;
static volatile u8 test_timeout = 0;

static SSL_CTX *ssl_ctx = NULL;
SSL *ssl = NULL;
u8 ssl_enabled = 0;

#define SPY_SIGNAL_SIZE 3
struct SpySignal {
    u8 detect_started;
    u8 trace_enabled;
    u8 next_step;
};

static struct SpySignal* spy_signal;

static s32 shm_id_spy;                    /* ID of the SHM region used as trace_enable   */

static char* log_dir = NULL;

static void stop_qemu_system() {
    if (qemu_mode == 2) {
        system("pgrep qemu-system && (pgrep qemu-system | xargs kill -9)");
        OKF("Stopped qemu-system");
    }
}

static void remove_shm_spy(void) {
    shmctl(shm_id_spy, IPC_RMID, NULL);
}

EXP_ST void setup_shm_spy(void) {
    u8* shm_str_spy;
    shm_id_spy = shmget(IPC_PRIVATE, sizeof(struct SpySignal), IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id_spy < 0) PFATAL("shmget() failed");

    atexit(remove_shm_spy);
    atexit(stop_qemu_system);

    shm_str_spy = alloc_printf("%d", shm_id_spy);
    setenv(SHM_ENV_VAR_SPY, shm_str_spy, 1);
    ck_free(shm_str_spy);

    spy_signal = shmat(shm_id_spy, NULL, 0);

    if (spy_signal == (void *)-1) PFATAL("shmat() failed");
    memset(spy_signal, 0, SPY_SIGNAL_SIZE);

    log_dir = getenv("AFL_LOG_DIR");
    if (log_dir == NULL) {
        log_dir = "afl-spy-logs/";
    }

    char cmd[1024];

    memset(cmd, 0, 1024);
    snprintf(cmd, 1024, "rm -rf %s", log_dir);
    if (WEXITSTATUS(system(cmd)) != 0) {
        FATAL("Unable to remove log directory");
    }

    memset(cmd, 0, 1024);
    snprintf(cmd, 1024, "mkdir -p %s", log_dir);
    if (WEXITSTATUS(system(cmd)) != 0) {
        FATAL("Unable to create log directory");
    }
}

static void free_ssl_resource(void) {
    if (ssl) {
        SSL_free(ssl);
        SSL_CTX_free(SSL_get_SSL_CTX(ssl));
    }
}

EXP_ST void setup_ssl(void) {
    ssl_enabled = 1;
    SSL_library_init();
    ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (ssl_ctx == NULL) {
        FATAL("SSL_CTX_new failed");
    }
    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        FATAL("SSL_new failed");
    }
    atexit(free_ssl_resource);
}

int send_test_alive_request() {
    static char* test_alive_script = NULL;
    if (test_alive_script == NULL) {
        test_alive_script = getenv("TEST_ALIVE_SCRIPT");
        if (test_alive_script == NULL) {
            test_alive_script = DEFAULT_TEST_ALIVE_SCRIPT;
        }
        OKF("Test Alive script is %s", test_alive_script);
    }
    static char* cmd = NULL;
    if (cmd == NULL) {
        cmd = (char*)malloc(1024);
        memset(cmd, 0, 1024);
        if (cmd == NULL) {
            FATAL("Unable to allocate memory");
        }
        snprintf(cmd, 1024, "bash %s;", test_alive_script);
    }
    int res = WEXITSTATUS(system(cmd));
    static int count = 0;
    count++;
    if (res != 0) {
      OKF("Test Alive timed out, res = %d, count = %d", res, count);
    }
    return res;
}

int send_test_agent_request() {
    static char* test_agent_script = NULL;
    if (test_agent_script == NULL) {
        test_agent_script = getenv("TEST_AGENT_SCRIPT");
        if (test_agent_script == NULL) {
            test_agent_script = DEFAULT_TEST_AGENT_SCRIPT;
        }
        OKF("Test Agent script is %s", test_agent_script);
    }
    static char* cmd = NULL;
    if (cmd == NULL) {
        cmd = (char*)malloc(1024);
        memset(cmd, 0, 1024);
        if (cmd == NULL) {
            FATAL("Unable to allocate memory");
        }
        snprintf(cmd, 1024, "bash %s;", test_agent_script);
    }
    printf("cmd: %s\n", cmd);
    int res = WEXITSTATUS(system(cmd));
    if (res != 0) {
        OKF("Test Agent timed out, status:%d", res);
    } else {
        OKF("Test Agent success, status:%d", res);
    }
    return res;
}

int send_restart_target_request() {
    static char* restart_target_script = NULL;
    if (restart_target_script == NULL) {
        restart_target_script = getenv("RESTART_TARGET_SCRIPT");
        if (restart_target_script == NULL) {
            restart_target_script = DEFAULT_RESTART_TARGET_SCRIPT;
        }
        OKF("Restart Target script is %s", restart_target_script);
    }
    static char* cmd = NULL;
    if (cmd == NULL) {
        cmd = (char*)malloc(1024);
        memset(cmd, 0, 1024);
        if (cmd == NULL) {
            FATAL("Unable to allocate memory");
        }
        snprintf(cmd, 1024, "bash %s;", restart_target_script);
    }
    int res = WEXITSTATUS(system(cmd));
    if (res != 0) {
      OKF("Fail to send restart target request to spy-agent, res = %d", res);
    }
    return res;
}

static inline int test_alive() {
    return send_test_alive_request();
}

static inline int test_agent() {
    return send_test_agent_request();
}

void clear_pipe_content(fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    char buffer[256];
    int bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
    }

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

static void read_target_ctx() {
    int res = 0;
    if ((res = read(fsrv_st_fd, &target_ctx, 4)) != 4) {
      if (stop_soon) return;
      RPFATAL(res, "Unable to communicate with fork server (OOM?)");
    } else {
      OKF("target context is %08x", target_ctx);
      return;
    }
}

static void read_agent_ctx() {
    int res = 0;
    if ((res = read(fsrv_st_fd, &agent_ctx, 4)) != 4) {
      if (stop_soon) return;
      RPFATAL(res, "Unable to communicate with fork server (OOM?)");
    } else {
      OKF("agent context is %08x", agent_ctx);
      return;
    }
}

static void detect_target() {
    clear_pipe_content(fsrv_st_fd);
    int res = 0;
    int count = 0;
    while((res = send_test_alive_request()) != 0) {
        count++;
        sleep(3);
        if (count == 10) {
            FATAL("Fail to send test alive request to target: %d\n", res);
        }
    }
    read_target_ctx();
    sleep(2);
}

static void detect_agent() {
    int res = 0;
    int count = 0;
    while((res = send_test_agent_request()) != 0) {
        count++;
        sleep(3);
        if (count == 10) {
            FATAL("Fail to send test agent request to spy-agent: %d\n", res);
        }
    }
    read_agent_ctx();
}

static void restart_target() {
    int res = 0;
    int count = 0;
    while((res = send_restart_target_request()) != 0) {
        count++;
        sleep(5);
        if (count == 10) {
            FATAL("Fail to send restart target request to spy-agent: %d\n", res);
        }
    }
    detect_target();
}

static void log_trace_bits(int count) {
    // if(log_dir) {
    //   static int count1 = 0;
    //   count1++;
    // //   if (count1 < 500) {
    //     char *log_file = alloc_printf("%s/trace_bits_%d.txt", log_dir, count1);
    //     FILE* fp = fopen(log_file, "w");
    //     if (fp == NULL) {
    //       FATAL("Cannot open log file");
    //     }
        
    //     int count2 = 0;
    //     for (int i = 0; i < MAP_SIZE; i++) {
    //       if (trace_bits[i] > 0) {
    //           count2++;
    //       }
    //     }
    //     fprintf(fp, "wait_count: %d test_num: %d non0bytes_num: %d\n", count, count1, count2);
    //     for (int i = 0; i < MAP_SIZE; i++) {
    //       if (trace_bits[i] > 0) {
    //           fprintf(fp, "trace_bits[%d]: %d\n", i, trace_bits[i]);
    //       }
    //     }
    //     fclose(fp);
    // //   }
      

    //   if (count1 == 100) {
    //     OKF("100 trace bits logs have been generated");
    //     exit(0);
    //   }
    // }
}