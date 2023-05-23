/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2015 MediaTek Inc.
 */

	/*
	 * DF: for single variable
	 * DF_A: for array variable
	 * DF_S: for segmented single byte
	 * NOTE: fmt string must be less than FMT_MAX_LEN
	 */
	DF(fiq_step, " fiq step: %u "),
	DF(exp_type, " exception type: %u\n"),
	DF(kaslr_offset, "Kernel Offset: 0x%llx\n"),
	DF(oops_in_progress_addr, "&oops_in_progress: 0x%llx\n"),
	/* ensure info related to HWT always be bottom and keep their order*/
	DF(mcdi_wfi, "mcdi_wfi: 0x%x\n"),
	DF(mcdi_r15, "mcdi_r15: 0x%x\n"),
	DF(deepidle_data, "deepidle: 0x%x\n"),
	DF(sodi3_data, "sodi3: 0x%x\n"),
	DF(sodi_data, "sodi: 0x%x\n"),
	DF(mcsodi_data, "mcsodi: 0x%x\n"),
	DF(spm_suspend_data, "spm_suspend: 0x%x\n"),
	DF(spm_common_scenario_data, "spm_common_scenario: 0x%x\n"),
	DF_A(clk_data, "clk_data: 0x%x\n", 8),
	DF(suspend_debug_flag, NULL/* not enabled */),
	DF(fiq_cache_step, "fiq_cache_step: %d\n"),
	DF(vcore_dvfs_opp, "vcore_dvfs_opp: 0x%x\n"),
	DF(vcore_dvfs_status, "vcore_dvfs_status: 0x%x\n"),
	DF_A(ppm_cluster_limit, "ppm_cluster_limit: 0x%08x\n", 8),
	DF(ppm_step, "ppm_step: 0x%x\n"),
	DF(ppm_cur_state, "ppm_cur_state: 0x%x\n"),
	DF(ppm_min_pwr_bgt, "ppm_min_pwr_bgt: %d\n"),
	DF(ppm_policy_mask, "ppm_policy_mask: 0x%x\n"),
	DF(ppm_waiting_for_pbm, "ppm_waiting_for_pbm: 0x%x\n"),
	DF(cpu_dvfs_vproc_big, "cpu_dvfs_vproc_big: 0x%x\n"),
	DF(cpu_dvfs_vproc_little, "cpu_dvfs_vproc_little: 0x%x\n"),
	DF(cpu_dvfs_oppidx, "cpu_dvfs_oppidx: 0x%x\n"),
	DF(cpu_dvfs_cci_oppidx, "cpu_dvfs_oppidx: cci = 0x%x\n"),
	DF(cpu_dvfs_status, "cpu_dvfs_status: 0x%x\n"),
	DF(cpu_dvfs_step, "cpu_dvfs_step: 0x%x\n"),
	DF(cpu_dvfs_pbm_step, "cpu_dvfs_pbm_step: 0x%x\n"),
	DF(cpu_dvfs_cb, "cpu_dvfs_cb: 0x%x\n"),
	DF(cpufreq_cb, "cpufreq_cb: 0x%x\n"),
	DF(gpu_dvfs_vgpu, "gpu_dvfs_vgpu: 0x%x\n"),
	DF(gpu_dvfs_oppidx, "gpu_dvfs_oppidx: 0x%x\n"),
	DF(gpu_dvfs_status, "gpu_dvfs_status: 0x%x\n"),
	DF(gpu_dvfs_power_count, "gpu_dvfs_power_count: %d\n"),
	DF(drcc_0, "drcc_0 = 0x%X\n"),
	DF(drcc_1, "drcc_1 = 0x%X\n"),
	DF(drcc_2, "drcc_2 = 0x%X\n"),
	DF(drcc_3, "drcc_3 = 0x%X\n"),
	DF(drcc_dbg_ret, "DRCC dbg info result: 0x%x\n"),
	DF(drcc_dbg_off, "DRCC dbg info offset: 0x%x\n"),
	DF(drcc_dbg_ts, "DRCC dbg info timestamp: 0x%llx\n"),
	DF(ptp_devinfo_0, "EEM devinfo0 = 0x%X\n"),
	DF(ptp_devinfo_1, "EEM devinfo1 = 0x%X\n"),
	DF(ptp_devinfo_2, "EEM devinfo2 = 0x%X\n"),
	DF(ptp_devinfo_3, "EEM devinfo3 = 0x%X\n"),
	DF(ptp_devinfo_4, "EEM devinfo4 = 0x%X\n"),
	DF(ptp_devinfo_5, "EEM devinfo5 = 0x%X\n"),
	DF(ptp_devinfo_6, "EEM devinfo6 = 0x%X\n"),
	DF(ptp_devinfo_7, "EEM devinfo7 = 0x%X\n"),
	DF(ptp_e0, "M_HW_RES0 = 0x%X\n"),
	DF(ptp_e1, "M_HW_RES1 = 0x%X\n"),
	DF(ptp_e2, "M_HW_RES2 = 0x%X\n"),
	DF(ptp_e3, "M_HW_RES3 = 0x%X\n"),
	DF(ptp_e4, "M_HW_RES4 = 0x%X\n"),
	DF(ptp_e5, "M_HW_RES5 = 0x%X\n"),
	DF(ptp_e6, "M_HW_RES6 = 0x%X\n"),
	DF(ptp_e7, "M_HW_RES7 = 0x%X\n"),
	DF(ptp_e8, "M_HW_RES8 = 0x%X\n"),
	DF(ptp_e9, "M_HW_RES9 = 0x%X\n"),
	DF(ptp_e10, "M_HW_RESA = 0x%X\n"),
	DF(ptp_e11, "M_HW_RESB = 0x%X\n"),
	DF_S(ptp_vboot, "ptp_bank_[%d]_vboot = 0x%llx\n"),
	DF_S(ptp_cpu_big_volt, "ptp_cpu_big_volt[%d] = %llx\n"),
	DF_S(ptp_cpu_big_volt_1, "ptp_cpu_big_volt_1[%d] = %llx\n"),
	DF_S(ptp_cpu_big_volt_2, "ptp_cpu_big_volt_2[%d] = %llx\n"),
	DF_S(ptp_cpu_big_volt_3, "ptp_cpu_big_volt_3[%d] = %llx\n"),
	DF_S(ptp_cpu_2_little_volt, "ptp_cpu_2_little_volt[%d] = %llx\n"),
	DF_S(ptp_cpu_2_little_volt_1, "ptp_cpu_2_little_volt_1[%d] = %llx\n"),
	DF_S(ptp_cpu_2_little_volt_2, "ptp_cpu_2_little_volt_2[%d] = %llx\n"),
	DF_S(ptp_cpu_2_little_volt_3, "ptp_cpu_2_little_volt_3[%d] = %llx\n"),
	DF_S(ptp_cpu_little_volt, "ptp_cpu_little_volt[%d] = %llx\n"),
	DF_S(ptp_cpu_little_volt_1, "ptp_cpu_little_volt_1[%d] = %llx\n"),
	DF_S(ptp_cpu_little_volt_2, "ptp_cpu_little_volt_2[%d] = %llx\n"),
	DF_S(ptp_cpu_little_volt_3, "ptp_cpu_little_volt_3[%d] = %llx\n"),
	DF_S(ptp_cpu_cci_volt, "ptp_cpu_cci_volt[%d] = %llx\n"),
	DF_S(ptp_cpu_cci_volt_1, "ptp_cpu_cci_volt_1[%d] = %llx\n"),
	DF_S(ptp_cpu_cci_volt_2, "ptp_cpu_cci_volt_2[%d] = %llx\n"),
	DF_S(ptp_cpu_cci_volt_3, "ptp_cpu_cci_volt_3[%d] = %llx\n"),
	DF_S(ptp_gpu_volt, "ptp_gpu_volt[%d] = %llx\n"),
	DF_S(ptp_gpu_volt_1, "ptp_gpu_volt_1[%d] = %llx\n"),
	DF_S(ptp_gpu_volt_2, "ptp_gpu_volt_2[%d] = %llx\n"),
	DF_S(ptp_gpu_volt_3, "ptp_gpu_volt_3[%d] = %llx\n"),
	DF_S(ptp_temp, "ptp_temp[%d] = %llx\n"),
	DF(ptp_status, "ptp_status: 0x%x\n"),
	DF(eem_pi_offset, "eem_pi_offset : 0x%x\n"),
	DF(etc_status, "etc_status : 0x%x\n"),
	DF(etc_mode, "etc_mode : 0x%x\n"),
	DF_A(thermal_temp, "thermal_temp = %d\n", THERMAL_RESERVED_TZS),
	DF(thermal_status, "thermal_status: %d\n"),
	DF(thermal_ATM_status, "thermal_ATM_status: %d\n"),
	DF(thermal_ktime, "thermal_ktime: %lld\n"),
	DF(idvfs_ctrl_reg, "idvfs_ctrl_reg = 0x%x\n"),
	DF(idvfs_enable_cnt, "idvfs_enable_cnt = %u\n"),
	DF(idvfs_swreq_cnt, "idvfs_swreq_cnt = %u\n"),
	DF(idvfs_curr_volt, "idvfs_curr_volt(raw) = 0x%x\n"),
	DF(idvfs_sram_ldo, "idvfs_sram_ldo = %umv\n"),
	DF(idvfs_swavg_curr_pct_x100, "idvfs_swavg_curr_pct_x100 = %u\n"),
	DF(idvfs_swreq_curr_pct_x100, "idvfs_swreq_curr_pct_x100 = %u\n"),
	DF(idvfs_swreq_next_pct_x100, "idvfs_swreq_next_pct_x100 = %u\n"),
	DF(idvfs_state_manchine, "idvfs_state_manchine = 0x%x\n"),
	DF_A(ocp_target_limit, "ocp_target_limit: %d\n", 4),
	DF(ocp_enable, "ocp_enable = 0x%x\n"),
	DF(scp_pc, "scp_pc: 0x%x\n"),
	DF(scp_lr, "scp_lr: 0x%x\n"),
	DF(hang_detect_timeout_count, "hang detect time out: 0x%x\n"),
	DF(last_sync_func, "last sync function: 0x%lx\n"),
	DF(last_async_func, "last async function: 0x%lx\n"),
	DF(gz_irq, "GZ IRQ: 0x%x\n"),
	/* kparams (unused) */

	/* ensure info related to HWT always be bottom and keep their order*/
	DF(last_init_func, "last init function: 0x%lx\n"),
	DF(pmic_ext_buck, "pmic & external buck: 0x%x\n"),
	DF(hps_cb_enter_times, "CPU HPS footprint: %llu, "),
	DF(hps_cb_cpu_bitmask, "0x%x, "),
	DF(hps_cb_footprint, "%d, "),
	DF(hps_cb_fp_times, "%llu\n"),
	DF(hotplug_cpu_event, "CPU notifier status: %d, "),
	DF(hotplug_cb_index, "%d, "),
	DF(hotplug_cb_fp, "0x%llx, "),
	DF(hotplug_cb_times, "%llu\n"),
	DF(cpu_caller, "CPU Hotplug: caller CPU%d, "),
	DF(cpu_callee, "callee CPU%d\n"),
	DF(cpu_up_prepare_ktime, "CPU_UP_PREPARE: %lld\n"),
	DF(cpu_starting_ktime, "CPU_STARTING: %lld\n"),
	DF(cpu_online_ktime, "CPU_ONLINE: %lld\n"),
	DF(cpu_down_prepare_ktime, "CPU_DOWN_PREPARE: %lld\n"),
	DF(cpu_dying_ktime, "CPU_DYING: %lld\n"),
	DF(cpu_dead_ktime, "CPU_DEAD: %lld\n"),
	DF(cpu_post_dead_ktime, "CPU_POST_DEAD: %lld\n"),
