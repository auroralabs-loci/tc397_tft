#!/usr/bin/env python3
import csv

# Read the assembly functions
with open('temp_getSourceFreq.txt', 'r') as f:
    getSourceFreq_asm = f.read()

with open('temp_waitTime.txt', 'r') as f:
    waitTime_asm = f.read()

# Already have these from before
simulateCpuWorkload_asm = """<simulateCpuWorkload>:
	mov.aa %a14,%sp
	sub.a %sp,8
	mov %d2,0
	st.w [%a14]-8,%d2
	mov %d2,0
	st.w [%a14]-4,%d2
	j <simulateCpuWorkload+0x3c>
	ld.w %d2,[%a14]-8
	sh %d2,-3
	ld.w %d3,[%a14]-4
	xor %d2,%d3
	addi %d2,%d2,31161
	addih %d2,%d2,40503
	ld.w %d3,[%a14]-8
	add %d2,%d3
	st.w [%a14]-8,%d2
	ld.w %d2,[%a14]-4
	add %d2,1
	st.w [%a14]-4,%d2
	ld.w %d2,[%a14]-4
	movh %d3,8
	addi %d3,%d3,-24288
	jlt.u %d2,%d3,<simulateCpuWorkload+0x14>
	ld.w %d2,[%a14]-8
	nop
	ret"""

blinkLED_asm = """<blinkLED>:
	mov.aa %a14,%sp
	sub.a %sp,40
	movh %d2,61444
	addi %d2,%d2,-21248
	st.w [%a14]-24,%d2
	mov %d2,0
	st.b [%a14]-25,%d2
	ld.bu %d2,[%a14]-25
	ld.w %d3,[%a14]-24
	st.w [%a14]-32,%d3
	st.b [%a14]-33,%d2
	movh %d2,1
	add %d2,1
	st.w [%a14]-40,%d2
	ld.bu %d2,[%a14]-33
	ld.w %d3,[%a14]-40
	sh %d2,%d3,%d2
	ld.a %a2,[%a14]-32
	add.a %a2,4
	st.w [%a2],%d2
	nop
	nop
	call <simulateCpuWorkload>
	movh %d2,61440
	addi %d2,%d2,4096
	st.w [%a14]-4,%d2
	mov %d2,500
	st.w [%a14]-8,%d2
	ld.w %d2,[%a14]-4
	st.w [%a14]-12,%d2
	mov %d4,0
	call <IfxScuCcu_getSourceFrequency>
	mov %d3,%d2
	movh.a %a2,61443
	lea %a2,[%a2]24624 <<bmhd_3_copy+0x40c34a30>>
	ld.w %d2,[%a2]
	extr.u %d2,%d2,0,4
	and %d2,%d2,255
	itof %d2,%d2
	div.f %d2,%d3,%d2
	st.w [%a14]-16,%d2
	ld.w %d2,[%a14]-16
	ftoiz %d2,%d2
	st.w [%a14]-20,%d2
	ld.w %d4,[%a14]-20
	movh %d2,4194
	addi %d2,%d2,19923
	mul %e2,%d4,%d2
	sha %d3,-6
	sha %d2,%d4,-31
	sub %d2,%d3,%d2
	mov %d3,%d2
	ld.w %d2,[%a14]-8
	mul %d2,%d3
	mov %e2,%d2
	mov %e4,%d3,%d2
	call <waitTime>
	nop
	ret"""

# Write complete CSV
with open('complete_functions.csv', 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['function_name', 'assembly_code'])

    writer.writerow(['simulateCpuWorkload', simulateCpuWorkload_asm])
    writer.writerow(['blinkLED', blinkLED_asm])
    writer.writerow(['IfxScuCcu_getSourceFrequency', getSourceFreq_asm])
    writer.writerow(['waitTime', waitTime_asm])

print("Complete CSV created with all functions")
