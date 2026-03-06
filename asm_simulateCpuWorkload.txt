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
ret