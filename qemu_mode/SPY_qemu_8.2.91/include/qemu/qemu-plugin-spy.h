#ifndef QEMU_QEMU_PLUGIN_SPY_H
#define QEMU_QEMU_PLUGIN_SPY_H

#include "osdep.h"
#include "qemu-plugin.h"

typedef void (*qemu_plugin_vcpu_insn_trans_cb_t)(qemu_plugin_id_t id,
                                               CPUState *cpu,
                                               CPUArchState *env,
                                               uint32_t insn);

typedef void (*qemu_plugin_vcpu_syscall_spy_cb_t)(qemu_plugin_id_t id,
                                               CPUState *cpu,
                                               CPUArchState *env,
                                               void *data);

typedef void (*qemu_plugin_vcpu_tb_exec_spy_cb_t)(qemu_plugin_id_t id,
                                               CPUState *cpu,
                                               CPUArchState *env,
                                               void *data);

typedef void (*qemu_plugin_vcpu_tlb_set_cb_t)(qemu_plugin_id_t id,
                                               CPUState *cpu,
                                               CPUArchState *env,
                                               void *data);

typedef void (*qemu_plugin_vcpu_exception_spy_cb_t)(qemu_plugin_id_t id,
                                               CPUState *cpu,
                                               CPUArchState *env,
                                               void *data);
QEMU_PLUGIN_API
void qemu_plugin_register_vcpu_insn_trans_cb(qemu_plugin_id_t id,
                                           qemu_plugin_vcpu_insn_trans_cb_t cb);
QEMU_PLUGIN_API
void qemu_plugin_register_vcpu_syscall_spy_cb(qemu_plugin_id_t id,
                                           qemu_plugin_vcpu_syscall_spy_cb_t cb);
QEMU_PLUGIN_API
void qemu_plugin_register_vcpu_tb_exec_spy_cb(qemu_plugin_id_t id,
                                           qemu_plugin_vcpu_tb_exec_spy_cb_t cb);
QEMU_PLUGIN_API
void qemu_plugin_register_vcpu_tlb_set_cb(qemu_plugin_id_t id,
                                           qemu_plugin_vcpu_tlb_set_cb_t cb);
QEMU_PLUGIN_API
void qemu_plugin_register_vcpu_exception_spy_cb(qemu_plugin_id_t id,
                                           qemu_plugin_vcpu_exception_spy_cb_t cb);

#endif /* QEMU_QEMU_PLUGIN_SPY_H */