#!/usr/bin/env python3

# All function timings (in nanoseconds)
timings = {
    'blinkLED': 107.37,
    'simulateCpuWorkload': 36.69,
    'IfxScuCcu_getSourceFrequency': 95.84,
    'IfxScuCcu_getPllFrequency': 124.11,
    'IfxScuCcu_getPerPllFrequency1': 133.99,
    'IfxScuCcu_getPerPllFrequency2': 154.97,
    'waitTime': 106.44
}

print("=" * 70)
print("FULLY INCLUSIVE EXECUTION TIME ANALYSIS FOR blinkLED()")
print("=" * 70)

print("\n📊 Call Tree Structure:")
print("""
blinkLED (107.37 ns)
├─ simulateCpuWorkload (36.69 ns) [LEAF]
├─ IfxScuCcu_getSourceFrequency (95.84 ns)
│  ├─ IfxScuCcu_getPllFrequency (124.11 ns) [LEAF] OR
│  ├─ IfxScuCcu_getPerPllFrequency1 (133.99 ns) [LEAF] OR
│  └─ IfxScuCcu_getPerPllFrequency2 (154.97 ns) [LEAF]
└─ waitTime (106.44 ns) [LEAF]
""")

print("\n⚠️  NOTE: IfxScuCcu_getSourceFrequency() uses conditional logic")
print("   and calls ONLY ONE of the three PLL functions at runtime.\n")

# Calculate different scenarios
scenarios = {
    'PllFrequency': timings['IfxScuCcu_getPllFrequency'],
    'PerPllFrequency1': timings['IfxScuCcu_getPerPllFrequency1'],
    'PerPllFrequency2': timings['IfxScuCcu_getPerPllFrequency2']
}

print("\n" + "=" * 70)
print("SCENARIO CALCULATIONS")
print("=" * 70)

for scenario_name, pll_time in scenarios.items():
    print(f"\n🔹 Scenario: IfxScuCcu_getSourceFrequency calls {scenario_name}")
    print(f"   {'-' * 60}")

    blinkLED_self = timings['blinkLED']
    simulateCpu = timings['simulateCpuWorkload']
    getSourceFreq_self = timings['IfxScuCcu_getSourceFrequency']
    pll_func = pll_time
    waitTime_func = timings['waitTime']

    # IfxScuCcu_getSourceFrequency inclusive = self + one PLL function
    getSourceFreq_inclusive = getSourceFreq_self + pll_func

    # blinkLED inclusive = self + all called functions (inclusive)
    blinkLED_inclusive = blinkLED_self + simulateCpu + getSourceFreq_inclusive + waitTime_func

    print(f"   blinkLED (self):                      {blinkLED_self:8.2f} ns")
    print(f"   + simulateCpuWorkload:                {simulateCpu:8.2f} ns")
    print(f"   + IfxScuCcu_getSourceFrequency:")
    print(f"       - self:                           {getSourceFreq_self:8.2f} ns")
    print(f"       - {scenario_name:30s} {pll_func:8.2f} ns")
    print(f"       = inclusive:                      {getSourceFreq_inclusive:8.2f} ns")
    print(f"   + waitTime:                           {waitTime_func:8.2f} ns")
    print(f"   {'=' * 60}")
    print(f"   TOTAL INCLUSIVE TIME:                 {blinkLED_inclusive:8.2f} ns")

# Calculate worst case
worst_pll = max(scenarios.values())
worst_name = [k for k, v in scenarios.items() if v == worst_pll][0]
worst_getSourceFreq = timings['IfxScuCcu_getSourceFrequency'] + worst_pll
worst_total = timings['blinkLED'] + timings['simulateCpuWorkload'] + worst_getSourceFreq + timings['waitTime']

print("\n" + "=" * 70)
print("SUMMARY")
print("=" * 70)
print(f"\n✅ Best Case:  {timings['blinkLED'] + timings['simulateCpuWorkload'] + timings['IfxScuCcu_getSourceFrequency'] + timings['IfxScuCcu_getPllFrequency'] + timings['waitTime']:.2f} ns")
print(f"❌ Worst Case: {worst_total:.2f} ns (when calling {worst_name})")
print(f"\n📈 Difference: {worst_total - (timings['blinkLED'] + timings['simulateCpuWorkload'] + timings['IfxScuCcu_getSourceFrequency'] + timings['IfxScuCcu_getPllFrequency'] + timings['waitTime']):.2f} ns\n")

print("=" * 70)
print("BREAKDOWN OF COMPONENTS")
print("=" * 70)
print(f"\nDirect operations (blinkLED self):       {timings['blinkLED']:8.2f} ns ({timings['blinkLED']/worst_total*100:5.1f}%)")
print(f"New workload (simulateCpuWorkload):      {timings['simulateCpuWorkload']:8.2f} ns ({timings['simulateCpuWorkload']/worst_total*100:5.1f}%)")
print(f"Clock frequency query (inclusive):       {worst_getSourceFreq:8.2f} ns ({worst_getSourceFreq/worst_total*100:5.1f}%)")
print(f"Wait timer setup (waitTime):             {timings['waitTime']:8.2f} ns ({timings['waitTime']/worst_total*100:5.1f}%)")
print(f"\n{'=' * 70}\n")
