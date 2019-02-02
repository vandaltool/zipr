#!/bin/bash

configs=(do_cfi 
	do_cfi_2_byte 
	do_cfi_4_byte 
	do_cfi_8_byte 
	do_coloring_cfi 
	do_coloring_cfi_2_byte
        do_coloring_cfi_4_byte 
	do_coloring_cfi_8_byte 
	do_cfi_exe_nonces 
	do_cfi_exe_nonces_1_byte
	do_cfi_exe_nonces_2_byte 
	do_cfi_exe_nonces_8_byte 
	do_cfi_exe_nonces_2_byte_non_exe
  	do_cfi_exe_nonces_4_byte_non_exe 
	do_cfi_exe_nonces_8_byte_non_exe 
	do_cfi_exe_nonces_1_byte_2_byte_non_exe 
	do_cfi_exe_nonces_1_byte_4_byte_non_exe
  	do_cfi_exe_nonces_1_byte_8_byte_non_exe 
	do_cfi_exe_nonces_2_byte_2_byte_non_exe
	do_cfi_exe_nonces_2_byte_4_byte_non_exe 
	do_cfi_exe_nonces_2_byte_8_byte_non_exe
	do_cfi_exe_nonces_8_byte_2_byte_non_exe 
	do_cfi_exe_nonces_8_byte_4_byte_non_exe
	do_cfi_exe_nonces_8_byte_8_byte_non_exe 
	do_cfi_exe_nonces_color_non_exe 
	do_cfi_exe_nonces_1_byte_color_non_exe 
	do_cfi_exe_nonces_2_byte_color_non_exe 
	do_cfi_exe_nonces_8_byte_color_non_exe 
	do_cfi_exe_nonces_2_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_4_byte_non_exe_color_non_exe 
	do_cfi_exe_nonces_8_byte_non_exe_color_non_exe	 		
	do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_non_exe 			
	do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_non_exe
	do_cfi_exe_nonces_color_exe
	do_cfi_exe_nonces_1_byte_color_exe
	do_cfi_exe_nonces_2_byte_color_exe
	do_cfi_exe_nonces_8_byte_color_exe
	do_cfi_exe_nonces_2_byte_non_exe_color_exe
	do_cfi_exe_nonces_4_byte_non_exe_color_exe
	do_cfi_exe_nonces_8_byte_non_exe_color_exe
	do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_exe
	do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_exe
	do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_exe
	do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_exe
	do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_exe
	do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_exe
	do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_exe
	do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_exe
	do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_exe
	do_cfi_exe_nonces_color_both
	do_cfi_exe_nonces_1_byte_color_both
	do_cfi_exe_nonces_2_byte_color_both
	do_cfi_exe_nonces_8_byte_color_both
	do_cfi_exe_nonces_2_byte_non_exe_color_both
	do_cfi_exe_nonces_4_byte_non_exe_color_both
	do_cfi_exe_nonces_8_byte_non_exe_color_both
	do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_both
	do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_both
	do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_both
	do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_both
	do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_both
	do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_both
	do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_both
	do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_both
	do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_both)

do_cfi()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false")
}

do_cfi_2_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2")
}

do_cfi_4_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4")
}

do_cfi_8_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8")
}

###############################################################################

do_coloring_cfi()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option selective_cfi:--color --step-option zipr:"--add-sections false")
}

do_coloring_cfi_2_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option selective_cfi:--color --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2")
}

do_coloring_cfi_4_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option selective_cfi:--color --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4")
}

do_coloring_cfi_8_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option selective_cfi:--color --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8")
}

###############################################################################

do_cfi_exe_nonces()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false")
}

do_cfi_exe_nonces_1_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1")
}

do_cfi_exe_nonces_2_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2")
}

do_cfi_exe_nonces_8_byte()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8")
}

do_cfi_exe_nonces_2_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2")
}

do_cfi_exe_nonces_4_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4")
}

do_cfi_exe_nonces_8_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8")
}

do_cfi_exe_nonces_1_byte_2_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 2")
}

do_cfi_exe_nonces_1_byte_4_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 4")
}

do_cfi_exe_nonces_1_byte_8_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 8")
}

do_cfi_exe_nonces_2_byte_2_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 2")
}

do_cfi_exe_nonces_2_byte_4_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 4")
}

do_cfi_exe_nonces_2_byte_8_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 8")
}

do_cfi_exe_nonces_8_byte_2_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 2")
}

do_cfi_exe_nonces_8_byte_4_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 4")
}

do_cfi_exe_nonces_8_byte_8_byte_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 8")
}

###############################################################################

do_cfi_exe_nonces_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_4_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_non_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color)
}

###############################################################################

do_cfi_exe_nonces_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_1_byte_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_2_byte_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_8_byte_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_2_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_4_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_8_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces)
}

do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_exe()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces)
}

###############################################################################

do_cfi_exe_nonces_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_4_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_2_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_4_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_1_byte_8_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 1" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_2_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_4_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_2_byte_8_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 2" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_2_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 2" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_4_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 4" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
}

do_cfi_exe_nonces_8_byte_8_byte_non_exe_color_both()
{
        (set -x; $PS $1 $2 --backend zipr --critical-step move_globals=on --critical-step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false" --step-option selective_cfi:"--exe-nonce-size 8" --step-option selective_cfi:"--nonce-size 8" --step-option selective_cfi:--color-exe-nonces --step-option selective_cfi:--color)
} 
