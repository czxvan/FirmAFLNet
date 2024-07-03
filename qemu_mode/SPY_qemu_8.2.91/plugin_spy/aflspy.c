#define __AFL_SPY_C__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <qemu-plugin-spy.h>
#include "aflspy.h"

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static const char *QEMU_MODE = NULL;

void vcpu_insn_trans(qemu_plugin_id_t id,
                            CPUState *cpu, CPUArchState *env,
                            uint32_t insn)
{
    static uint64_t count = 0;
    count++;
    if (count % 100000 == 0) {
        g_autofree gchar *log = g_strdup_printf("count: %ld, insn: %08x\n", count, insn);
        qemu_plugin_outs(log);
    }
}

void vcpu_tb_exec_spy(qemu_plugin_id_t id, CPUState *cpu, CPUArchState *env, void *data)
{
    TBInfo *info = (TBInfo *)data;

    static uint64_t count = 0;
    gboolean log_tb_exec_spy = LOG_MASK(false);
    gboolean log_coverage_info = LOG_MASK(false);
    if(is_trace_enabled()) {
        if (info->ctx == target_ctx && info->pc < 0x01000000) {
            count++;
            afl_maybe_log(info->pc); // used to record code execution info
            if (log_tb_exec_spy) {
                LOG_STATEMENT("ctx: %08x  tb_exec pc: %08x\n"
                                , info->ctx
                                , info->pc);
            }
        }
    } else if (count > 0) {
        LOG_STATEMENT("tb count: %ld\n", count);
        count = 0;

        // print coverage info
        int count = 0;
        for (int i = 0; i < MAP_SIZE; i++) {
            if (coverage_info[i] > 0 || trace_bits[i] > 0) {
                count++;
            }
        }
        LOG_STATEMENT("coverage_info count: %d\n", count);
        if (log_coverage_info) {
            for (int i = 0; i < MAP_SIZE; i++) {
                if (coverage_info[i] > 0 || trace_bits[i] > 0) {
                    LOG_STATEMENT("coverage_info[%d]: %d trace_bits[%d]: %d \n", i, coverage_info[i], i, trace_bits[i]);
                }
            }
        }
        memset(coverage_info, 0, MAP_SIZE);
    }
}

void vcpu_exception_spy(qemu_plugin_id_t id,
                            CPUState *cpu, CPUArchState *env,
                            void *data)
{
    ExceptionInfo *info = (ExceptionInfo *)data;
    gboolean log_exception_spy = LOG_MASK(false);
    if (system_started && info->ctx == target_ctx) {
        if (log_exception_spy)
            LOG_STATEMENT("ctx: %08x  excp: %08x  syndrome: %08x  "
                            "target_el: %d  EC: %x\n"
                            , info->ctx
                            , info->excp
                            , info->syndrome
                            , info->target_el
                            , info->exception_class);
        if (info->exception_class == 0x25) {
            LOG_STATEMENT("ctx: %08x  Target Context reset from EXCEPTION 25\n"
                          , info->ctx);
        }
    }
}

void vcpu_syscall_spy(qemu_plugin_id_t id, CPUState *cpu, CPUArchState *env, void *data)
{
    SyscallInfo *info = (SyscallInfo *)data;

    gboolean log_exit = LOG_MASK(true);
    gboolean log_fork = LOG_MASK(false);
    gboolean log_read = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_write = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_open = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_close = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_execve = LOG_MASK(false);
    gboolean log_clone = LOG_MASK(false);
    gboolean log_send = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_sendto = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_sendmsg = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_recv = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_recvfrom = LOG_MASK(false) || QEMU_USER_MODE;

    gboolean log_socket = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_bind = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_listen = LOG_MASK(false) || QEMU_USER_MODE;
    gboolean log_accept = LOG_MASK(true) || QEMU_USER_MODE;
    gboolean log_default = LOG_MASK(false);

    switch (info->num) {
        case EXIT: {
            if (system_started && info->ctx == target_ctx) {
                if(log_exit) {
                    g_autofree gchar *log = g_strdup_printf(
                        "ctx: %08x  exit error_num: %d\n"
                        , info->ctx
                        , info->params.exit_params->error_code
                    );
                    qemu_plugin_outs(log);
                }
                LOG_STATEMENT("Target EXIT\n");
            }

        } break;
        case FORK: {
            if(log_fork) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  fork\n"
                    , info->ctx
                );
                qemu_plugin_outs(log);
            }  
        } break;
        case READ: {
            if(log_read) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  read  from %d %d bytes\n"
                    , info->ctx
                    , info->params.read_params->fd
                    , info->params.read_params->count
                );
                qemu_plugin_outs(log);
            }
        } break;
        case WRITE: {
            if (log_write) {
                if (info->params.write_params->count > 4) {
                    LOG_STATEMENT("ctx: %08x  write  into %d %d bytes\n"
                                  "\t%s\n"
                                  , info->ctx
                                  , info->params.write_params->fd
                                  , info->params.write_params->count
                                  , info->params.write_params->buf);
                }
            }
        } break;
        case OPEN: {
            if(log_open) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  open  %s\n"
                    , info->ctx
                    , info->params.open_params->filename
                );
                qemu_plugin_outs(log);
            }
        } break;
        case CLOSE: {
            if(log_close) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  close  fd: %d\n"
                    , info->ctx
                    , info->params.close_params->fd
                );
                qemu_plugin_outs(log);
            }
        } break;
        case EXECVE: {
            // Use strstr instead of g_strcmp0 to wildcard match
            if (!system_started &&
                strstr(info->params.execve_params->filename, SYSTEM_STARTED_INDICATOR_PROCESS) != NULL)
            {
                system_started = true;
                afl_setup();
                LOG_STATEMENT("System Started.\n");
                if (write(STATE_WRITE_FD, "RDY!", 4) != 4) {
                    LOG_ERROR("Failed to write RDY! to STATE_WRITE_FD\n");
                } else {
                    LOG_STATEMENT("Wrote RDY! to STATE_WRITE_FD\n");
                }
            }
            if (log_execve) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  execve  %s\n"
                    , info->ctx
                    , info->params.execve_params->filename
                );
                qemu_plugin_outs(log);
            }
        } break;
        case CLONE: {
            if (log_clone) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  clone\n"
                    , info->ctx
                );
                qemu_plugin_outs(log);
            }
        } break;
        case SEND: {
            if (system_started && info->ctx == target_ctx) {
                if (is_trace_enabled() && is_nextstep_zero()) {
                    set_nextstep(1);
                    LOG_STATEMENT("Target SEND\n");
                }
            }
            if (log_send) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  send %d bytes to %d, flags: %d\n"
                    "\t%s\n"
                    , info->ctx
                    , info->params.send_params->len
                    , info->params.send_params->sockfd
                    , info->params.send_params->flags
                    , (char*)info->params.send_params->buf
                );
                qemu_plugin_outs(log);

                const uint8_t *buf = info->params.send_params->buf;
                for (int i = 0; i < info->params.send_params->len; i++) {
                    if ((i+1) % 11 == 0) {
                        log = g_strdup_printf("%02x\n", buf[i]);
                    } else {
                        log = g_strdup_printf("%02x ", buf[i]);
                    }
                    qemu_plugin_outs(log);
                }
                qemu_plugin_outs("\n");
            }
        } break;
        case SENDTO: {
            if (system_started && info->ctx == target_ctx) {
                if (is_trace_enabled() && is_nextstep_zero()) {
                    set_nextstep(1);
                    LOG_STATEMENT("Target SENDTO\n");
                }
            }
            if (log_sendto) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  sendto %d bytes to %d, flags: %d, dest_addr: %08x, dest_len: %d\n"
                    "\t%s\n"
                    , info->ctx
                    , info->params.sendto_params->len
                    , info->params.sendto_params->sockfd
                    , info->params.sendto_params->flags
                    , info->params.sendto_params->dest_addr
                    , info->params.sendto_params->dest_len
                    , (char*)info->params.sendto_params->buf
                );
                qemu_plugin_outs(log);

                const uint8_t *buf = info->params.sendto_params->buf;
                for (int i = 0; i < info->params.sendto_params->len; i++) {
                    if ((i+1) % 11 == 0) {
                        log = g_strdup_printf("%02x\n", buf[i]);
                    } else {
                        log = g_strdup_printf("%02x ", buf[i]);
                    }
                    qemu_plugin_outs(log);
                }
                qemu_plugin_outs("\n");
            }
        } break;
        case SENDMSG: {
            if (system_started && info->ctx == target_ctx) {
                if (is_trace_enabled() && is_nextstep_zero()) {
                    set_nextstep(1);
                    LOG_STATEMENT("Target SENDMSG\n");
                }
            }
            if (log_sendmsg) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  sendmsg to %d flags: %x\n"
                    , info->ctx
                    , info->params.send_params->sockfd
                    , info->params.send_params->flags
                );
                qemu_plugin_outs(log);
            }
        } break;
        case RECV: {
            if (log_recv) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  recv %d bytes from %d, flags: %d\n"
                    , info->ctx
                    , info->params.recv_params->len
                    , info->params.recv_params->sockfd
                    , info->params.recv_params->flags
                );
                qemu_plugin_outs(log);
            }
        } break;
        case RECVFROM: {
            if (log_recvfrom) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  recvfrom %d bytes from %d, flags: %d, src_addr: %08x, src_len: %d\n"
                    , info->ctx
                    , info->params.recvfrom_params->len
                    , info->params.recvfrom_params->sockfd
                    , info->params.recvfrom_params->flags
                    , info->params.recvfrom_params->src_addr
                    , info->params.recvfrom_params->src_len
                );
                qemu_plugin_outs(log);
            }
        } break;
        case SOCKET: {
            if (log_socket) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  socket domain: %08x, type: %d, protocol: %d\n"
                    , info->ctx
                    , info->params.socket_params->domain
                    , info->params.socket_params->type
                    , info->params.socket_params->protocol
                );
                qemu_plugin_outs(log);
            }
        } break;
        case BIND: {
            if (log_bind) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  bind sockfd: %d, sock_addr: %08x, addr_len: %d\n"
                    , info->ctx
                    , info->params.bind_params->sockfd
                    , info->params.bind_params->sock_addr
                    , info->params.bind_params->addr_len
                );
                qemu_plugin_outs(log);
            }
        } break;
        case LISTEN: {
            if (log_listen) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  listen sockfd: %d, backlog: %d\n"
                    , info->ctx
                    , info->params.listen_params->sockfd
                    , info->params.listen_params->backlog
                );
                qemu_plugin_outs(log);
            }
        } break;
        case ACCEPT: {
            if (system_started) {
                if (!agent_ctx) {
                    set_agent_ctx(info->ctx);
                } else if (!target_ctx && info->ctx != agent_ctx) {
                    set_target_ctx(info->ctx);
                } else if (target_ctx && info->ctx == agent_ctx) {
                    target_ctx = 0;
                }
            }
            if (log_accept) {
                g_autofree gchar *log = g_strdup_printf(
                    "ctx: %08x  accept sockfd: %d, sock_addr: %08x, addr_len: %d\n"
                    , info->ctx
                    , info->params.accept_params->sockfd
                    , info->params.accept_params->sock_addr
                    , info->params.accept_params->addr_len
                );
                qemu_plugin_outs(log);
            }
        } break;
        default: {
            if (log_default) {
                g_autofree gchar *log = g_strdup_printf(
                        "ctx: %08x  unnamed syscall %d\n"
                        , info->ctx
                        , info->num
                    );
                    qemu_plugin_outs(log);
            }
        }
    }
}

QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id,
                                           const qemu_info_t *info, int argc,
                                           char **argv)
{
    qemu_plugin_register_vcpu_insn_trans_cb(id, vcpu_insn_trans); // used to count insns
    qemu_plugin_register_vcpu_syscall_spy_cb(id, vcpu_syscall_spy);
    qemu_plugin_register_vcpu_tb_exec_spy_cb(id, vcpu_tb_exec_spy);
    qemu_plugin_register_vcpu_exception_spy_cb(id, vcpu_exception_spy); // used to
    return 0;
}
