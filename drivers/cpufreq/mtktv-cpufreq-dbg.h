/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

extern int get_cpu_temperature_bysar(void);
extern u32 temperature_offset;
extern int auto_measurement;
extern unsigned int mtktv_cpufreq_get_clk(unsigned int cpu);
extern int mtktv_cpufreq_set_voltage(struct mtktv_cpu_dvfs_info *info, int vproc);
extern struct mtktv_cpu_dvfs_info *mtktv_cpu_dvfs_info_lookup(int cpu);
extern int mtktv_cpufreq_set_clk(unsigned int u32Current, unsigned int u32Target);


