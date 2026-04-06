#!/usr/bin/env python3
"""Disassemble the extracted libdragon IPL3 binary."""
import sys, os

path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "ipl3_libdragon.bin")
data = open(path, "rb").read()

REGS = ["zero","at","v0","v1","a0","a1","a2","a3",
        "t0","t1","t2","t3","t4","t5","t6","t7",
        "s0","s1","s2","s3","s4","s5","s6","s7",
        "t8","t9","k0","k1","gp","sp","s8","ra"]

def dis(w, pc):
    op=(w>>26)&63; rs=(w>>21)&31; rt=(w>>16)&31; rd=(w>>11)&31
    sa=(w>>6)&31; fn=w&63; imm=w&0xFFFF
    simm=imm if imm<0x8000 else imm-0x10000
    tgt=(((pc+4)&0xF0000000)|((w&0x3FFFFFF)<<2))
    if op==0:
        if fn==0:    return "sll     $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==2:    return "srl     $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==3:    return "sra     $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==4:    return "sllv    $%s,$%s,$%s" % (REGS[rd],REGS[rt],REGS[rs])
        if fn==6:    return "srlv    $%s,$%s,$%s" % (REGS[rd],REGS[rt],REGS[rs])
        if fn==8:    return "jr      $%s" % REGS[rs]
        if fn==9:    return "jalr    $%s,$%s" % (REGS[rd],REGS[rs])
        if fn==0x10: return "mfhi    $%s" % REGS[rd]
        if fn==0x12: return "mflo    $%s" % REGS[rd]
        if fn==0x18: return "mult    $%s,$%s" % (REGS[rs],REGS[rt])
        if fn==0x19: return "multu   $%s,$%s" % (REGS[rs],REGS[rt])
        if fn==0x1A: return "div     $%s,$%s" % (REGS[rs],REGS[rt])
        if fn==0x1B: return "divu    $%s,$%s" % (REGS[rs],REGS[rt])
        if fn==0x20: return "add     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x21: return "addu    $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x22: return "sub     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x23: return "subu    $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x24: return "and     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x25: return "or      $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x26: return "xor     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x27: return "nor     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x2A: return "slt     $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x2B: return "sltu    $%s,$%s,$%s" % (REGS[rd],REGS[rs],REGS[rt])
        if fn==0x38: return "dsll    $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==0x3C: return "dsll32  $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==0x3A: return "dsrl    $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        if fn==0x3E: return "dsrl32  $%s,$%s,%d" % (REGS[rd],REGS[rt],sa)
        return "SPEC    fn=0x%02x  %08x" % (fn,w)
    if op==1:
        nm = {0:"bltz",1:"bgez",0x10:"bltzal",0x11:"bgezal"}.get(rt,"regimm%d"%rt)
        return "%s  $%s,0x%08x" % (nm,REGS[rs],pc+4+simm*4)
    if op==2:  return "j       0x%08x" % tgt
    if op==3:  return "jal     0x%08x" % tgt
    if op==4:  return "beq     $%s,$%s,0x%08x" % (REGS[rs],REGS[rt],pc+4+simm*4)
    if op==5:  return "bne     $%s,$%s,0x%08x" % (REGS[rs],REGS[rt],pc+4+simm*4)
    if op==6:  return "blez    $%s,0x%08x" % (REGS[rs],pc+4+simm*4)
    if op==7:  return "bgtz    $%s,0x%08x" % (REGS[rs],pc+4+simm*4)
    if op==8:  return "addi    $%s,$%s,%d" % (REGS[rt],REGS[rs],simm)
    if op==9:  return "addiu   $%s,$%s,0x%04x" % (REGS[rt],REGS[rs],imm)
    if op==0xA: return "slti    $%s,$%s,%d" % (REGS[rt],REGS[rs],simm)
    if op==0xB: return "sltiu   $%s,$%s,%d" % (REGS[rt],REGS[rs],simm)
    if op==0xC: return "andi    $%s,$%s,0x%04x" % (REGS[rt],REGS[rs],imm)
    if op==0xD: return "ori     $%s,$%s,0x%04x" % (REGS[rt],REGS[rs],imm)
    if op==0xE: return "xori    $%s,$%s,0x%04x" % (REGS[rt],REGS[rs],imm)
    if op==0xF: return "lui     $%s,0x%04x" % (REGS[rt],imm)
    if op==0x10: return "cop0    %08x" % w
    if op==0x12: return "cop2    %08x" % w
    if op==0x14: return "beql    $%s,$%s,0x%08x" % (REGS[rs],REGS[rt],pc+4+simm*4)
    if op==0x15: return "bnel    $%s,$%s,0x%08x" % (REGS[rs],REGS[rt],pc+4+simm*4)
    if op==0x16: return "blezl   $%s,0x%08x" % (REGS[rs],pc+4+simm*4)
    if op==0x17: return "bgtzl   $%s,0x%08x" % (REGS[rs],pc+4+simm*4)
    if op==0x19: return "daddiu  $%s,$%s,%d" % (REGS[rt],REGS[rs],simm)
    if op==0x1F: return "rdhwr   %08x" % w
    if op==0x20: return "lb      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x21: return "lh      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x23: return "lw      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x24: return "lbu     $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x25: return "lhu     $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x27: return "lwu     $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x28: return "sb      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x29: return "sh      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x2B: return "sw      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x2F: return "cache   %d,%d($%s)" % (rt,simm,REGS[rs])
    if op==0x32: return "lwc2    %08x" % w
    if op==0x37: return "ld      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    if op==0x3F: return "sd      $%s,%d($%s)" % (REGS[rt],simm,REGS[rs])
    return "???     op=0x%02x  %08x" % (op,w)

BASE = 0xBFC00040
print("IPL3 disassembly (%d bytes, running at 0xBFC00040):" % len(data))
for i in range(0, len(data), 4):
    w = int.from_bytes(data[i:i+4], "big")
    pc = BASE + i
    print("  %08x: %08x  %s" % (pc, w, dis(w, pc)))
