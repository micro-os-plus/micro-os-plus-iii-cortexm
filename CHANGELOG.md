# Change & release log

Releases in reverse chronological order.

Please check
[GitHub](https://github.com/micro-os-plus/micro-os-plus-iii-cortexm/issues)
and close existing issues and pull requests.

## 2026-11-28

* v1.2.0

## 2026-11-25

* 78b6094 os-core.cpp: set PSPLIM for the running thread
* f65a7a5 os-inlines.h: set MSPLIM in set_stack_limit()
* 3541a3f Allow Armv8-M architecture builds

## 2026-11-24

* 88529c5 CODE-REVIEW-1.md: follow-up on the switch timestamp

## 2026-11-23

* c244fa1 os-core.cpp: do not set the switch timestamp again in start()
* d1d38d0 .npmignore: exclude CLAUDE.md, .claude/ and maintenance/
* 228dfd0 CODE-REVIEW-1.md: code review of the Cortex-M port
* 29a7ce7 port: Cortex-M7 r0p1 erratum 837070 workaround (#12)
* 3b8a92f port: ACLE architecture tests, central check (#11)
* 3a57a88 os-core.cpp: PendSV_Handler in basic asm (#10)
* 8905fd7 os-core.cpp: initial MSP from __stack (#9)
* e6062f2 os-core.cpp: enter the first thread directly (#8)
* 7c346b2 os-core.cpp: stop if SysTick_Config() fails (#7)
* b86a78b README.md, port headers: update the documentation
* e47269d os-inlines.h: is_priority_valid() false for NMI and HardFault (#6)
* a572be7 os-inlines.h: remove the unmatched diagnostic pop (#5)
* d5c30e4 os-core.cpp: stack size assertion in bytes (#4)
* 32662b5 os-core.cpp: no reschedule before the scheduler is started (#3)
* bcd0100 os-core.cpp: record a reschedule requested while locked (#2)
* ff9f21b os-inlines.h: hrclock pending tick from SCB->ICSR (#1)
* ea061a1 rename tests
* 088ee19 add maintenance

## 2025-09-24

* 687e975 README update
* 2da1b0f 1.1.0
* dbdb387 prepare v1.1.0
* 901e3d7 set & use Optimise is_reschedule_pending

## 2025-08-24

* a1dd28d 1.0.1
* c8e6702 prepare v1.0.1
* 9e595b4 rename xpack-development
* b81c95c add .clang-format and re-format
* b9b53eb update copyright notices

## 2023-07-22

* 430fc95 README updates

## 2023-07-21

* 82f1761 README updates
* 26196a4 README updates

## 2023-07-18

* 408db54 README update

## 2023-07-14

* cd612f3 README update
* 7384b56 README update
* 81fc1f7 .npmignore scripts
* 45e9a9c README update
* 8577143 README update
* e31e752 1.0.0
* a5e1192 prepare v1.0.0
* 6360a77 .npmignore update
* faa52c8 package.json: minXpm 0.16.2

## 2023-06-30

* a25f34b package.json: 1.0.0-pre.4
* 89a5b33 cosmetics

## 2023-06-26

* 518a6cf package.json; 1.0.0-pre.3
* d42ea12 use #if defined(OS_USE_OS_APP_CONFIG_H)

## 2023-06-22

* 6219584 update copyright notice

## 2023-06-21

* 1e3c8dd os-core.cpp: silence warning
* bbbde69 package.json: 1.0.0-pre.2
* fbbba64 add CMakeLists.txt
* 3fee5cc fix casts and silence warnings
* 01dceb9 os-inlines.h: remove trailing dot in messages

## 2023-06-17

* 5b774fc README update
* e0b843d README update
* 41f2e42 clean yotta & xcdl files
* 67e91d8 add package.json and make it an xPack

## 2023-05-25

* 9630923 os-core.cpp: remove register for C++17
