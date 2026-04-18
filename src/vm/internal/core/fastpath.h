/* SPDX-License-Identifier: MIT */

#ifndef GRAPHION_VM_FASTPATH_H
#define GRAPHION_VM_FASTPATH_H

#include "vm/vm.h"

void refresh_value_move_validation(graphion_vm *vm);
void refresh_global_print_validation(graphion_vm *vm);
int graphion_vm_try_run_fastpath(graphion_vm *vm, int *handled);
#endif


