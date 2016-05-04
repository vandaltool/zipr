bits 64
global main
section .text
main:

; 3 regsiter  pd+ps

vfmadd132pd xmm0, xmm1, xmm2
vfmadd132ps xmm0, xmm1, xmm2
vfmadd213pd xmm0, xmm1, xmm2
vfmadd213ps xmm0, xmm1, xmm2
vfmadd231pd xmm0, xmm1, xmm2
vfmadd231ps xmm0, xmm1, xmm2
vfmaddsub132pd xmm0, xmm1, xmm2
vfmaddsub132ps xmm0, xmm1, xmm2
vfmaddsub213pd xmm0, xmm1, xmm2
vfmaddsub213ps xmm0, xmm1, xmm2
vfmaddsub231pd xmm0, xmm1, xmm2
vfmaddsub231ps xmm0, xmm1, xmm2
vfmsub132pd xmm0, xmm1, xmm2
vfmsub132ps xmm0, xmm1, xmm2
vfmsub213pd xmm0, xmm1, xmm2
vfmsub213ps xmm0, xmm1, xmm2
vfmsub231pd xmm0, xmm1, xmm2
vfmsub231ps xmm0, xmm1, xmm2
vfmsubadd132pd xmm0, xmm1, xmm2
vfmsubadd132ps xmm0, xmm1, xmm2
vfmsubadd213pd xmm0, xmm1, xmm2
vfmsubadd213ps xmm0, xmm1, xmm2
vfmsubadd231pd xmm0, xmm1, xmm2
vfmsubadd231ps xmm0, xmm1, xmm2
vfnmadd132pd xmm0, xmm1, xmm2
vfnmadd132ps xmm0, xmm1, xmm2
vfnmadd213pd xmm0, xmm1, xmm2
vfnmadd213ps xmm0, xmm1, xmm2
vfnmadd231pd xmm0, xmm1, xmm2
vfnmadd231ps xmm0, xmm1, xmm2
vfnmsub132pd xmm0, xmm1, xmm2
vfnmsub132ps xmm0, xmm1, xmm2
vfnmsub213pd xmm0, xmm1, xmm2
vfnmsub213ps xmm0, xmm1, xmm2
vfnmsub231pd xmm0, xmm1, xmm2
vfnmsub231ps xmm0, xmm1, xmm2



; 2 regsiter+mem  pd+ps

vfmadd132pd xmm0, xmm1, [rsp+1024]
vfmadd132ps xmm0, xmm1, [rsp+1024]
vfmadd213pd xmm0, xmm1, [rsp+1024]
vfmadd213ps xmm0, xmm1, [rsp+1024]
vfmadd231pd xmm0, xmm1, [rsp+1024]
vfmadd231ps xmm0, xmm1, [rsp+1024]
vfmaddsub132pd xmm0, xmm1, [rsp+1024]
vfmaddsub132ps xmm0, xmm1, [rsp+1024]
vfmaddsub213pd xmm0, xmm1, [rsp+1024]
vfmaddsub213ps xmm0, xmm1, [rsp+1024]
vfmaddsub231pd xmm0, xmm1, [rsp+1024]
vfmaddsub231ps xmm0, xmm1, [rsp+1024]
vfmsub132pd xmm0, xmm1, [rsp+1024]
vfmsub132ps xmm0, xmm1, [rsp+1024]
vfmsub213pd xmm0, xmm1, [rsp+1024]
vfmsub213ps xmm0, xmm1, [rsp+1024]
vfmsub231pd xmm0, xmm1, [rsp+1024]
vfmsub231ps xmm0, xmm1, [rsp+1024]
vfmsubadd132pd xmm0, xmm1, [rsp+1024]
vfmsubadd132ps xmm0, xmm1, [rsp+1024]
vfmsubadd213pd xmm0, xmm1, [rsp+1024]
vfmsubadd213ps xmm0, xmm1, [rsp+1024]
vfmsubadd231pd xmm0, xmm1, [rsp+1024]
vfmsubadd231ps xmm0, xmm1, [rsp+1024]
vfnmadd132pd xmm0, xmm1, [rsp+1024]
vfnmadd132ps xmm0, xmm1, [rsp+1024]
vfnmadd213pd xmm0, xmm1, [rsp+1024]
vfnmadd213ps xmm0, xmm1, [rsp+1024]
vfnmadd231pd xmm0, xmm1, [rsp+1024]
vfnmadd231ps xmm0, xmm1, [rsp+1024]
vfnmsub132pd xmm0, xmm1, [rsp+1024]
vfnmsub132ps xmm0, xmm1, [rsp+1024]
vfnmsub213pd xmm0, xmm1, [rsp+1024]
vfnmsub213ps xmm0, xmm1, [rsp+1024]
vfnmsub231pd xmm0, xmm1, [rsp+1024]
vfnmsub231ps xmm0, xmm1, [rsp+1024]


; 3 regsiter  pd+ps as y-regs

vfmadd132pd ymm0, ymm1, ymm2
vfmadd132ps ymm0, ymm1, ymm2
vfmadd213pd ymm0, ymm1, ymm2
vfmadd213ps ymm0, ymm1, ymm2
vfmadd231pd ymm0, ymm1, ymm2
vfmadd231ps ymm0, ymm1, ymm2
vfmaddsub132pd ymm0, ymm1, ymm2
vfmaddsub132ps ymm0, ymm1, ymm2
vfmaddsub213pd ymm0, ymm1, ymm2
vfmaddsub213ps ymm0, ymm1, ymm2
vfmaddsub231pd ymm0, ymm1, ymm2
vfmaddsub231ps ymm0, ymm1, ymm2
vfmsub132pd ymm0, ymm1, ymm2
vfmsub132ps ymm0, ymm1, ymm2
vfmsub213pd ymm0, ymm1, ymm2
vfmsub213ps ymm0, ymm1, ymm2
vfmsub231pd ymm0, ymm1, ymm2
vfmsub231ps ymm0, ymm1, ymm2
vfmsubadd132pd ymm0, ymm1, ymm2
vfmsubadd132ps ymm0, ymm1, ymm2
vfmsubadd213pd ymm0, ymm1, ymm2
vfmsubadd213ps ymm0, ymm1, ymm2
vfmsubadd231pd ymm0, ymm1, ymm2
vfmsubadd231ps ymm0, ymm1, ymm2
vfnmadd132pd ymm0, ymm1, ymm2
vfnmadd132ps ymm0, ymm1, ymm2
vfnmadd213pd ymm0, ymm1, ymm2
vfnmadd213ps ymm0, ymm1, ymm2
vfnmadd231pd ymm0, ymm1, ymm2
vfnmadd231ps ymm0, ymm1, ymm2
vfnmsub132pd ymm0, ymm1, ymm2
vfnmsub132ps ymm0, ymm1, ymm2
vfnmsub213pd ymm0, ymm1, ymm2
vfnmsub213ps ymm0, ymm1, ymm2
vfnmsub231pd ymm0, ymm1, ymm2
vfnmsub231ps ymm0, ymm1, ymm2

; 2 regsiter+mem  pd+ps as y-regs

vfmadd132pd ymm0, ymm1, [rsp+1024]
vfmadd132ps ymm0, ymm1, [rsp+1024]
vfmadd213pd ymm0, ymm1, [rsp+1024]
vfmadd213ps ymm0, ymm1, [rsp+1024]
vfmadd231pd ymm0, ymm1, [rsp+1024]
vfmadd231ps ymm0, ymm1, [rsp+1024]
vfmaddsub132pd ymm0, ymm1, [rsp+1024]
vfmaddsub132ps ymm0, ymm1, [rsp+1024]
vfmaddsub213pd ymm0, ymm1, [rsp+1024]
vfmaddsub213ps ymm0, ymm1, [rsp+1024]
vfmaddsub231pd ymm0, ymm1, [rsp+1024]
vfmaddsub231ps ymm0, ymm1, [rsp+1024]
vfmsub132pd ymm0, ymm1, [rsp+1024]
vfmsub132ps ymm0, ymm1, [rsp+1024]
vfmsub213pd ymm0, ymm1, [rsp+1024]
vfmsub213ps ymm0, ymm1, [rsp+1024]
vfmsub231pd ymm0, ymm1, [rsp+1024]
vfmsub231ps ymm0, ymm1, [rsp+1024]
vfmsubadd132pd ymm0, ymm1, [rsp+1024]
vfmsubadd132ps ymm0, ymm1, [rsp+1024]
vfmsubadd213pd ymm0, ymm1, [rsp+1024]
vfmsubadd213ps ymm0, ymm1, [rsp+1024]
vfmsubadd231pd ymm0, ymm1, [rsp+1024]
vfmsubadd231ps ymm0, ymm1, [rsp+1024]
vfnmadd132pd ymm0, ymm1, [rsp+1024]
vfnmadd132ps ymm0, ymm1, [rsp+1024]
vfnmadd213pd ymm0, ymm1, [rsp+1024]
vfnmadd213ps ymm0, ymm1, [rsp+1024]
vfnmadd231pd ymm0, ymm1, [rsp+1024]
vfnmadd231ps ymm0, ymm1, [rsp+1024]
vfnmsub132pd ymm0, ymm1, [rsp+1024]
vfnmsub132ps ymm0, ymm1, [rsp+1024]
vfnmsub213pd ymm0, ymm1, [rsp+1024]
vfnmsub213ps ymm0, ymm1, [rsp+1024]
vfnmsub231pd ymm0, ymm1, [rsp+1024]
vfnmsub231ps ymm0, ymm1, [rsp+1024]



















; 3 regsiter  sd+ss

vfmadd132sd xmm0, xmm1, xmm2
vfmadd132ss xmm0, xmm1, xmm2
vfmadd213sd xmm0, xmm1, xmm2
vfmadd213ss xmm0, xmm1, xmm2
vfmadd231sd xmm0, xmm1, xmm2
vfmadd231ss xmm0, xmm1, xmm2
vfmsub132sd xmm0, xmm1, xmm2
vfmsub132ss xmm0, xmm1, xmm2
vfmsub213sd xmm0, xmm1, xmm2
vfmsub213ss xmm0, xmm1, xmm2
vfmsub231sd xmm0, xmm1, xmm2
vfmsub231ss xmm0, xmm1, xmm2
vfnmadd132sd xmm0, xmm1, xmm2
vfnmadd132ss xmm0, xmm1, xmm2
vfnmadd213sd xmm0, xmm1, xmm2
vfnmadd213ss xmm0, xmm1, xmm2
vfnmadd231sd xmm0, xmm1, xmm2
vfnmadd231ss xmm0, xmm1, xmm2
vfnmsub132sd xmm0, xmm1, xmm2
vfnmsub132ss xmm0, xmm1, xmm2
vfnmsub213sd xmm0, xmm1, xmm2
vfnmsub213ss xmm0, xmm1, xmm2
vfnmsub231sd xmm0, xmm1, xmm2
vfnmsub231ss xmm0, xmm1, xmm2



; 2 regsiter+mem  sd+ss

vfmadd132sd xmm0, xmm1, [rsp+1024]
vfmadd132ss xmm0, xmm1, [rsp+1024]
vfmadd213sd xmm0, xmm1, [rsp+1024]
vfmadd213ss xmm0, xmm1, [rsp+1024]
vfmadd231sd xmm0, xmm1, [rsp+1024]
vfmadd231ss xmm0, xmm1, [rsp+1024]
vfmsub132sd xmm0, xmm1, [rsp+1024]
vfmsub132ss xmm0, xmm1, [rsp+1024]
vfmsub213sd xmm0, xmm1, [rsp+1024]
vfmsub213ss xmm0, xmm1, [rsp+1024]
vfmsub231sd xmm0, xmm1, [rsp+1024]
vfmsub231ss xmm0, xmm1, [rsp+1024]
vfnmadd132sd xmm0, xmm1, [rsp+1024]
vfnmadd132ss xmm0, xmm1, [rsp+1024]
vfnmadd213sd xmm0, xmm1, [rsp+1024]
vfnmadd213ss xmm0, xmm1, [rsp+1024]
vfnmadd231sd xmm0, xmm1, [rsp+1024]
vfnmadd231ss xmm0, xmm1, [rsp+1024]
vfnmsub132sd xmm0, xmm1, [rsp+1024]
vfnmsub132ss xmm0, xmm1, [rsp+1024]
vfnmsub213sd xmm0, xmm1, [rsp+1024]
vfnmsub213ss xmm0, xmm1, [rsp+1024]
vfnmsub231sd xmm0, xmm1, [rsp+1024]
vfnmsub231ss xmm0, xmm1, [rsp+1024]
