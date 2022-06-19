;; Simple Nintendo 64 sample without RSP code.
;;
;; Copyright 2021 - By Michael Kohn
;; https://www.mikekohn.net/
;; mike@mikekohn.net
;;
;; Sets up the Nintendo 64 display, clear parts of the screen with two colors,
;; and draws some squares and triangles using the RDP hardware without using
;; the RSP MIPS co-processor.

.mips

.include "nintendo64/rdp.inc"
.include "nintendo64/rsp.inc"
.include "nintendo64/system.inc"
.include "nintendo64/video_interface.inc"

.high_address 0x8010_ffff

.define COLOR(r,g,b) (r << 24) | (g << 16) | (b << 8) | 0xff

.org 0x8000_0000
cartridge_header:
  ;; PI_BSB_DOM1_LAT_REG
  ;; PI_BSB_DOM1_PGS_REG
  ;; PI_BSB_DOM1_PWD_REG
  ;; PI_BSB_DOM1_PGS_REG
  .db 0x80
  .db 0x37
  .db 0x12
  .db 0x40

  ;; Clock rate.
  .dc32 0x000f

  ;; Vector
  .dc32 start
  .dc32 0x1444

  ;; Complement checksum and checksum.
  .db "CRC1"
  .db "CRC2"

  ;; Unused.
  .dc32 0, 0

  ;; Program title.
  .db "NAKEN_ASM SAMPLE           "

  ;; Developer ID code.
  .db 0

  ;; Cartridge ID code.
  .dc16 0x0000

  ;; Country code (D=Germany, E=USA, J=Japan, P=Europe, U=Australia)
  .db 'E'

  ;; Version (00 = 1.0).
  .db 0

bootcode:
  ;; This was downloaded from Peter Lemon's git repo. It seems like it's
  ;; needed to initialize the hardware.
.binfile "bootcode.bin"

start:
  ;; Exception vector location in 32 bit mode is 0xbfc0_0000.
  ;; 0x1fc0_0000 to 0x1fc0_07bf is PIF Boot ROM.
  li $a0, PIF_BASE
  li $t0, 8
  sw $t0, PIF_RAM+0x3c($a0)

  ;; Enable interrupts.
  mfc0 $t0, CP0_STATUS
  ori $t0, $t0, 1
  mtc0 $t0, CP0_STATUS

  ;; Color the first 100 scan lines blue.
  ;; Color Bits: rrrr_rggg_ggbb_bbba (0000_0000_0011_1110).
  li $a0, KSEG1 | 0x0010_0000
  li $t0, 0x003e
  li $t1, 320 * 100
fill_loop_top:
  sh $t0, 0($a0)
  addiu $a0, $a0, 2
  addiu $t1, $t1, -1
  bne $t1, $0, fill_loop_top
  nop

  ;; Color the last 140 scan lines brown.
  ;; Color Bits: rrrr_rggg_ggbb_bbba (0011_1000_1100_0010).
  li $t0, 0x38c2
  li $t1, 320 * 140
fill_loop_bottom:
  sh $t0, 0($a0)
  addiu $a0, $a0, 2
  addiu $t1, $t1, -1
  bne $t1, $0, fill_loop_bottom
  nop

  ;; This reads reads in a set of [ 32 bit address, 32 bit value ]
  ;; from ROM memory. For each 32 bit value, it is written to the
  ;; 32 bit address which sets up the video display.
setup_video:
  li $t0, ntsc_320x240x16
  li $t1, (ntsc_320x240x16_end - ntsc_320x240x16) / 8
setup_video_loop:
  lw $a0, 0($t0)
  lw $t2, 4($t0)
  sw $t2, 0($a0)
  addiu $t0, $t0, 8
  addiu $t1, $t1, -1
  bne $t1, $0, setup_video_loop
  nop

  ;; Wait for vertical blank before drawing.
  jal wait_for_vblank
  nop

  ;; Wait until End/Start Valid are cleared.
  li $a0, KSEG1 | DP_BASE
wait_end_start_valid:
  lw $t0, DP_STATUS_REG($a0)
  andi $t0, $t0, 0x600
  bne $t0, $0, wait_end_start_valid
  nop

  ;; Setup RDP to execute instructions from RSP DMEM.
  ;; When DP_END_REG is written to, if it doesn't equal to DP_START_REG
  ;; it will start the RDP executing commands.
  li $a0, KSEG1 | DP_BASE
  li $t0, 0x000a
  sw $t0, DP_STATUS_REG($a0)
  sw $0, DP_START_REG($a0)
  sw $0, DP_END_REG($a0)

  ;; Copy RDP instructions from ROM to RSP data memory.
  ;; Must be done 32 bits at a time, not 64 bit.
setup_rdp:
  li $a0, dp_setup
  li $t1, (dp_list_end - dp_setup) / 4
  li $a1, KSEG1 | RSP_DMEM
setup_rdp_loop:
  lw $t2, 0($a0)
  sw $t2, 0($a1)
  addiu $a0, $a0, 4
  addiu $a1, $a1, 4
  addiu $t1, $t1, -1
  bne $t1, $0, setup_rdp_loop
  nop

  ;; Start RDP executing instructions. This should draw the triangles
  ;; defined in the ROM area below.
  li $a0, KSEG1 | DP_BASE
  li $t0, 4
  sw $t0, DP_STATUS_REG($a0)
  ;sw $0,  DP_START_REG($a0)
  li $t0, dp_list_end - dp_setup
  sw $t0, DP_END_REG($a0)

  ;; Infinite loop at end of program.
while_1:
  b while_1
  nop

wait_for_vblank:
  li $a0, KSEG1 | VI_BASE
  li $t0, 512
wait_for_vblank_loop:
  lw $t1, VI_V_CURRENT_LINE_REG($a0)
  bne $t0, $t1, wait_for_vblank_loop
  nop
  jr $ra
  nop

;; NTSC values found in the Reality Coprocessor.pdf
;; VI_CONTROL_REG:                   0 0011 0010 0000 1110
;; VI_TIMING_REG:  0000 0011 1110 0101 0010 0010 0011 1001
;; VI_BASE is the video interface base in KSEG1 (no TLB, no cache).
.align_bits 32
ntsc_320x240x16:
  .dc32 KSEG1 | VI_BASE | VI_CONTROL_REG,     0x0000_320e
  .dc32 KSEG1 | VI_BASE | VI_DRAM_ADDR_REG,   0xa010_0000
  .dc32 KSEG1 | VI_BASE | VI_H_WIDTH_REG,     0x0000_0140
  .dc32 KSEG1 | VI_BASE | VI_V_INTR_REG,      0x0000_0200
  .dc32 KSEG1 | VI_BASE | VI_TIMING_REG,      0x03e5_2239
  .dc32 KSEG1 | VI_BASE | VI_V_SYNC_REG,      0x0000_020d
  .dc32 KSEG1 | VI_BASE | VI_H_SYNC_REG,      0x0000_0c15
  .dc32 KSEG1 | VI_BASE | VI_H_SYNC_LEAP_REG, 0x0c15_0c15
  .dc32 KSEG1 | VI_BASE | VI_H_VIDEO_REG,     0x006c_02ec
  .dc32 KSEG1 | VI_BASE | VI_V_VIDEO_REG,     0x0025_01ff
  .dc32 KSEG1 | VI_BASE | VI_V_BURST_REG,     0x000e_0204
  .dc32 KSEG1 | VI_BASE | VI_X_SCALE_REG,     0x0000_0200
  .dc32 KSEG1 | VI_BASE | VI_Y_SCALE_REG,     0x0000_0400
ntsc_320x240x16_end:

;; Here are some DP (Display Processor) hardware commands that
;; set up the drawing modes and draw some different colored shapes.
;; The code to calculate the slopes and such for the triangles are
;; done with the calc_triangle.py Python script.
.align_bits 64
dp_setup:
  .dc64 (DP_OP_SET_COLOR_IMAGE << 56) | (2 << 51) | (319 << 32) | 0x10_0000
  .dc64 (DP_OP_SET_Z_IMAGE << 56) | (0x10_0000 + (320 * 240 * 2))
  .dc64 (DP_OP_SET_SCISSOR << 56) | ((320 << 2) << 12) | (240 << 2)
  .dc64 (DP_OP_SET_OTHER_MODES << 56) | (1 << 55) | (3 << 52)
dp_setup_end:

dp_draw_squares:
  ;; Red square.
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_FILL_COLOR << 56) | (0xf80e << 16) | (0xf80e)
  .dc64 (DP_OP_FILL_RECTANGLE << 56) | ((100 << 2) << 44) | ((100 << 2) << 32) | ((50 << 2) << 12) | (50 << 2)

  ;; Dark purple square.
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_FILL_COLOR << 56) | (0x080f << 16) | (0x080f)
  .dc64 (DP_OP_FILL_RECTANGLE << 56) | ((150 << 2) << 44) | ((150 << 2) << 32) | ((100 << 2) << 12) | (100 << 2)
dp_draw_squares_end:

dp_draw_triangles:
  ;; Left major triangle (green)
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_OTHER_MODES << 56) | (1 << 55) | (1 << 31)
  .dc64 (DP_OP_SET_BLEND_COLOR << 56) | COLOR(0x00, 0xff, 0x00)
  .dc64 0x088002fb02aa01e0
  .dc64 0x00aa0000fffd097b
  .dc64 0x009615c1ffff6ef5
  .dc64 0x0095f0bf000065b0
  ;; Right major triangle (purple)
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_BLEND_COLOR << 56) | COLOR(0xff, 0x00, 0xff)
  .dc64 0x080002fb02aa01e0
  .dc64 0x00e600000002f684
  .dc64 0x00f9ea3e0000910a
  .dc64 0x00fa0f40ffff9a4f
  ;; Isosceles triangle (white)
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_BLEND_COLOR << 56) | COLOR(0xff, 0xff, 0xff)
  .dc64 0x08000168016800c8
  .dc64 0x00e6000017d77bff
  .dc64 0x00f9eccc00007fff
  .dc64 0x00fa1333ffff8000
  ;; Isosceles triangle (yellow, upside-down)
  .dc64 (DP_OP_SYNC_PIPE << 56)
  .dc64 (DP_OP_SET_BLEND_COLOR << 56) | COLOR(0xff, 0xff, 0x00)
  .dc64 0x0880016800c900c8
  .dc64 0x00aa0000ffff7f84
  .dc64 0x0081eccc00007fff
  .dc64 0x005a0000010aaaaa
dp_draw_triangles_end:

  .dc64 (DP_OP_SYNC_FULL << 56)
dp_list_end:

