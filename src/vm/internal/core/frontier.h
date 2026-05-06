/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_INTERNAL_CORE_FRONTIER_H
#define GRAPHION_VM_INTERNAL_CORE_FRONTIER_H

#include "vm/vm.h"

static inline int vm_frontier_is_bound(const graphion_vm *vm) {
  return vm != NULL && vm->frontier_input != NULL && vm->frontier_output != NULL &&
         vm->frontier_input_len <= vm->frontier_capacity;
}

#endif
