/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_CFD_H_
#define _MTK_PQ_CFD_H_

#ifdef _MTK_PQ_CFD_C_
#ifndef INTERFACE
#define INTERFACE
#endif
#else
#ifndef INTERFACE
#define INTERFACE	extern
#endif
#endif

//------------------------------------------------------------------------------
//  Macro and Define
//------------------------------------------------------------------------------
#define CFD_IS_HDMI(x)		((x) == E_CFD_INPUT_SOURCE_HDMI)
#define CFD_IS_DVI(x)		((x) == E_CFD_INPUT_SOURCE_DVI)
#define CFD_IS_MM(x)		(((x) == E_CFD_INPUT_SOURCE_STORAGE)\
				|| ((x) == E_CFD_INPUT_SOURCE_JPEG))
#define CFD_IS_DTV(x)		((x) == E_CFD_INPUT_SOURCE_DTV)
#define CFD_IS_B2R(x)		(CFD_IS_MM(x) || CFD_IS_DTV(x))
#define CFD_IS_VGA(x)		((x) == E_CFD_INPUT_SOURCE_VGA)
#define CFD_IS_COMPONENT(x)	((x) == E_CFD_INPUT_SOURCE_YPBPR)
#define CFD_IS_ATV(x)		((x) == E_CFD_INPUT_SOURCE_TV)
#define CFD_IS_CVBS(x)		((x) == E_CFD_INPUT_SOURCE_CVBS)
#define CFD_IS_SCART(x)		((x) == E_CFD_INPUT_SOURCE_SCART)
#define CFD_IS_VD(x)		(CFD_IS_ATV(x)\
				|| CFD_IS_CVBS(x)\
				|| CFD_IS_SCART(x))
#define CFD_IS_ANALOG(X)	(((x) == E_CFD_INPUT_SOURCE_VGA)\
				|| ((x) == E_CFD_INPUT_SOURCE_TV)\
				|| ((x) == E_CFD_INPUT_SOURCE_CVBS)\
				|| ((x) == E_CFD_INPUT_SOURCE_SVIDEO)\
				|| ((x) == E_CFD_INPUT_SOURCE_YPBPR)\
				|| ((x) == E_CFD_INPUT_SOURCE_SCART)\
				|| ((x) == E_CFD_INPUT_SOURCE_DVI))
#define CFD_IS_NONE(x)		((x) == E_CFD_INPUT_SOURCE_NONE)

//------------------------------------------------------------------------------
//  Type and Structure
//------------------------------------------------------------------------------
struct st_hdmi_avi_packet {
	__u8 u8InfoFrameVersion;
	__u8 u8PixelFormat;
	__u8 u8Colorimetry;
	__u8 u8ExtendedColorimetry;
	__u8 u8RgbQuantizationRange;
	__u8 u8YccQuantizationRange;
	__u8 u8AdditionalColorimetry;
	__u8 u8VideoInformationCode;
} __packed;

struct st_hdmi_hdr_packet {
	__u8 u8InfoFrameVersion;
	__u8 u8EOTF;
	__u8 u8StaticMetadataID;
	__u16 u16DisplayPrimariesX[3];
	__u16 u16DisplayPrimariesY[3];
	__u16 u16WhitePointX;
	__u16 u16WhitePointY;
	__u16 u16MaxDisplayMasteringLuminance;
	__u16 u16MinDisplayMasteringLuminance;
	__u16 u16MaximumContentLightLevel;
	__u16 u16MaximumFrameAverageLightLevel;
} __packed;

struct st_cfd_hdmi_info {
	__u8 u8IsGCInfoValid;
	__u8 u8ColorDepth; //get from GC info

	__u8 u8IsAVIValid;
	struct st_hdmi_avi_packet stAVI;

	__u8 u8IsHDRInfoValid;
	struct st_hdmi_hdr_packet stHDRInfo;

	__u8 u8IsVSIF10PlusDataValid;
	__u16 u16VSIF10PlusDataLen;
	__u8 u8IsVSIFDVMeatdataValid;
	__u16 u16VSIDVMeatdataLen;
	__u8 u8IsEMP10PlusDataValid;
	__u16 u16EMP10PlusDataLen;
	__u8 u8IsEMPDVMeatdataValid;
	__u16 u16EMPDVMeatdataLen;

	__u8 au8Reserved[12];	//reserved for 16 bytes alignment
} __packed;

struct st_vsif_10plus_info {
	__u8 u8VSIFTypeCode;
	__u8 u8VSIFVersion;
	__u8 u8Length;
	__u32 u32IEEECode;
	__u8 u8ApplicationVersion;
	__u8 u8TargetSystemDisplayMaxLuminance;
	__u8 u8AverageMaxRGB;
	__u8 au8DistributionValues[9];
	__u16 u16KneePointX;
	__u16 u16KneePointY;
	__u8 u8NumBezierCurveAnchors;
	__u8 au8BezierCurveAnchors[9];
	__u8 u8GraphicsOverlayFlag;
	__u8 u8NoDelayFlag;

	__u8 au8Reserved[13];	//reserved for 16 bytes alignment
} __packed;

struct st_emp_10plus_info {
	__u8 u8ItuTT35CountryCode;
	__u16 u16ItuTT35TerminalProviderCode;
	__u16 u16ItuTT35TerminalProviderOrientedCode;
	__u8 u8ApplicationIdentifier;
	__u8 u8ApplicationVersion;
	__u8 u8NumWindows;
	__u32 u32TargetedSystemDisplayMaximumLuminance;
	__u8 bTargetSystemDisplayActualPeakLuminanceFlag;
	__u32 au32Maxscl[3];
	__u32 au32Average_maxrgb[3];
	__u8 u8NumDistributions;
	__u8 au8DistributionIndex[15];
	__u32 au32DistributionValues[15];
	__u16 u16FractionBrightPixels;
	__u8 u8MasteringDisplayActualPeakLuminanceFlag;
	__u8 bToneMappingFlag;
	__u16 u16KneePointX;
	__u16 u16KneePointY;
	__u8 u8NumBezierCurveAnchors;
	__u16 au16BezierCurveAnchors[15];
	__u8 u8ColorSaturationMappingFlag;

	__u8 au8Reserved[11];	//reserved for 16 bytes alignment
} __packed;

struct st_mm_general_info {
	__u8 u8InputFormat;
	__u8 u8InputDataFormat;
	__u8 u8VideoFullRangeFlag;
	__u8 u8BitDepth;
	__u8 u8HdrType;
} __packed;

struct st_vui_info {
	__u8 u8ColourPrimaries;
	__u8 u8TransferCharacteristics;
	__u8 u8MatrixCoeffs;
} __packed;

struct st_sei_info {
	__u16 u16DisplayPrimariesX[3];
	__u16 u16DisplayPrimariesY[3];
	__u16 u16WhitePointX;
	__u16 u16WhitePointY;
	__u32 u32MasterPanelMaxLuminance;
	__u32 u32MasterPanelMinLuminance;
} __packed;

struct st_cll_info {
	__u16 u16MaxContentLightLevel;
	__u16 u16MaxPicAverageLightLevel;
} __packed;

struct st_cfd_mm_info {
	__u8 u8IsGeneralInfoValid;
	struct st_mm_general_info stGeneral;

	__u8 u8IsVUIValid;
	struct st_vui_info stUVI;

	__u8 u8IsSEIValid;
	struct st_sei_info stSEI;

	__u8 u8IsCLLValid;
	struct st_cll_info stCLL;

	__u8 u8IsFilmGrainValid;
	__u16 u16FilmGrainLen;
	__u8 u8IsDVMetadataValid;
	__u16 u16DVMetadataLen;
	__u8 u8IsTCHMetadataValid;
	__u16 u16TCHMetadataLen;
	__u8 u8IsHDR10PlusSEIValid;
	__u16 u16HDR10PlusSEILen;

	__u8 au8Reserved[13];	//reserved for 16 bytes alignment
} __packed;

struct st_fg_info {
	__u8 au8FilmGrain[256];
} __packed;

struct st_hdr10plus_sei {
	__u8 u8CountryCode;
	__u16 u16TerminalProviderCode;
	__u16 u16TerminalProviderOrientedCode;
	__u8 u8ApplicationIdentifier;
	__u8 u8ApplicationVersion;
	__u8 u8NumWindows;
	__u32 u32TargetSystemDisplayMaxLuminance;
	__u8 u8TargetSystemDisplayActualPeakLuminanceFlag;
	__u32 au32MaxSCL[3];
	__u32 u32AverageMaxRGB;
	__u8 u8NumDistributions;
	__u8 au8DistributionIndex[9];
	__u32 au32DistributionValues[9];
	__u16 u16FractionBrightPixels;
	__u8 u8MasterDisplayActualPeakLuminanceFlag;
	__u8 u8ToneMappingFlag;
	__u16 u16KneePointX;
	__u16 u16KneePointY;
	__u8 u8NumBezierCurveAnchors;
	__u16 au16BezierCurveAnchors[9];
	__u8 u8ColorSaturationMappingFlag;

	__u8 au8Reserved[9];	//reserved for 16 bytes alignment
} __packed;

struct st_main_color_fmt {
	__u8 u8Fmt;
	/// general format, ref enum EN_CFD_CFIO
	__u8 u8DataFmt;
	/// data format, ref enum EN_CFD_MC_FORMAT

	__u8 u8CP;
	/// Color primaries, ref enum EN_CFD_CFIO_CP
	__u8 u8TC;
	/// Transfer characteristics, ref enum EN_CFD_CFIO_TR
	__u8 u8MC;
	/// Matrix coefficients, ref enum EN_CFD_CFIO_MC

	__u8 u8BitDepth;
	/// pixel depth: 8,10,12,16...
	bool bFullRange;
	/// pixel data range: full-0~255, limit-16~235
} __packed;

struct st_color_primaries {
	/// Display primaries x, data *0.00002 0xC350 = 1
	__u16 u16Display_Primaries_x[3];
	/// Display primaries y, data *0.00002 0xC350 = 1
	__u16 u16Display_Primaries_y[3];
	/// White point x, data *0.00002 0xC350 = 1
	__u16 u16White_point_x;
	/// White point y, data *0.00002 0xC350 = 1
	__u16 u16White_point_y;
} __packed;

struct st_cfd_input_info {
	__u8 u8InputSRC;
	// enum EN_CFD_INPUT_SOURCE

	__u8 u8SRCDataFmt;
	/// data format, ref enum EN_CFD_MC_FORMAT
	struct st_main_color_fmt stMainColorFmt;

	__u8 u8HDRType;	// enum v4l2_hdr_type

	// extern info
	struct st_cfd_hdmi_info stHDMI;
	struct st_vsif_10plus_info stVSIF10Plus;
	struct st_emp_10plus_info stEMP10Plus;

	struct st_cfd_mm_info stVDEC;
	struct st_fg_info stFG;
	struct st_hdr10plus_sei stSEI10Plus;
} __packed;

struct st_cfd_output_info {
	struct st_main_color_fmt stMainColorFmt;
	struct st_color_primaries stCP;

	/// Max luminance, data * 1 nits
	__u16 u16MaxLuminance;
	/// Med luminance, data * 1 nits
	__u16 u16MedLuminance;
	/// Min luminance, data * 0.0001 nits
	__u16 u16MinLuminance;

	/// Linear RGB
	bool bLinearRgb;

	/// Customer color primaries
	bool bCusCP;
	/// Source white x
	__u16 u16SourceWx;
	/// Source white y
	__u16 u16SourceWy;
} __packed;

struct st_cfd_ui_info {
	/// 0:off
	/// 1:on
	/// default on , not in the document
	__u8 u8OSDUIEn;

	/// Mode 0: update matrix by OSD and color format driver
	/// Mode 1: only update matrix by OSD controls
	/// for mode1 : the configures of matrix keep the same as
	/// the values by calling CFD last time
	__u8 u8OSDUIMode;

	//0:auto depends on STB rule
	//1:always do HDR2SDR for HDR input
	//2:always not do HDR2SDR for HDR input
	__u8 u8HDR_UI_H2SMode;

	/// 0: not skip, 1: skip Hue, Saturation
	bool bSkipPictureSetting;

	/// Hue
	__u16 u16Hue;
	/// Saturation
	__u16 u16Saturation;
	/// Contrast
	__u16 u16Contrast;
	/// brightness R/G/B
	__u16 au16Brightness[3];
	/// RGB gain R/G/B
	__u16 au16RGBGain[3];

	// customer color range, enum v4l2_cus_color_range_type
	__u8 u8ColorRangeType;
	// 0~64
	__u8 u8UltraBlackLevel;
	// 0~64
	__u8 u8UltraWhiteLevel;

	bool bColorCorrectionEnable;
	__s16 as16CorrectionMatrix[32];

	// enable Linear RGB in SDR case
	bool bLinearEnable;

	/// open HDR UI
	// hdr enable mode, ref enum v4l2_hdr_enable_mode
	__u8 u8HDREnableMode;
	// tmo level, ref enum v4l2_tmo_level_type
	__u8 u8TmoLevel;
	/// open HDR UI end
} __packed;

struct st_xc_status_info {
	bool bIsInputImode;	// is input I mode
	bool bPreVSDEnable;	// is pre v scaling down enable
	bool bHVSPEnable;	// is HVSP enable
	bool u8MADIDramFormat;	// enum EN_CFD_MC_FORMAT
} __packed;

struct st_pq_status_info {
	bool bPQBypass;	// is PQ bypass
} __packed;

struct st_dv_status_info {
	bool bIsDolby;	// is dolby enable
	bool bSkipUI;	// is dolby SDK/IDK, skip UI
} __packed;

struct st_tch_status_info {
	bool bIsTCH;	// is tch enable
} __packed;

struct st_cfd_status_info {
	__u8 u8HdrStatus;	// enum EN_CFD_HDR_STATUS
	__u8 u8HdrPosition;	// enum EN_HW_CFD_HDR_POSITION

	bool bCFDFired;	// is CFD fired
} __packed;

struct st_cfd_info {
	struct st_cfd_input_info stIn;
	struct st_cfd_output_info stOut;
	struct st_cfd_ui_info stUI;

	struct st_xc_status_info stXC;
	struct st_pq_status_info stPQ;
	struct st_dv_status_info stDV;
	struct st_tch_status_info stTCH;
	struct st_cfd_status_info stCFD;

	__u32 u32UpdataFlag;

	void *pALGInfo;
} __packed;

//------------------------------------------------------------------------------
//  Variable
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
bool mtk_cfd_ResetCfdInfo(struct st_cfd_info *pstCfdInfo);
bool mtk_cfd_SetInputSource(struct st_cfd_info *pstCFDInfo,
	__u8 u8Input);
bool mtk_cfd_SetSRCDataFmt(struct st_cfd_info *pstCFDInfo,
	__u8 u8SRCDataFmt);
bool mtk_cfd_SetInputMainFmt(struct st_cfd_info *pstCFDInfo,
	struct st_main_color_fmt *pstFmt);
bool mtk_cfd_SetHDMIMetaData(struct st_cfd_info *pstCFDInfo,
	void __user *FD);
bool mtk_cfd_SetMMMetaData(struct st_cfd_info *pstCFDInfo,
	void __user *FD);
bool mtk_cfd_SetOutputMainFmt(struct st_cfd_info *pstCFDInfo,
	struct st_main_color_fmt *pstFmt);
bool mtk_cfd_SetOutputCP(struct st_cfd_info *pstCFDInfo,
	struct st_color_primaries *pstCP);
bool mtk_cfd_SetOutputluma(struct st_cfd_info *pstCFDInfo,
	__u16 u16MaxLuminance, __u16 u16MedLuminance, __u16 u16MinLuminance);
bool mtk_cfd_SetOutputLinear(struct st_cfd_info *pstCFDInfo,
	bool bLinearRGB);
bool mtk_cfd_SetOutputCusCP(struct st_cfd_info *pstCFDInfo,
	bool bCusCP, __u16 u16WhitePointX, __u16 u16WhitePointY);
bool mtk_cfd_SetUISkipPicSetting(struct st_cfd_info *pstCFDInfo,
	bool bSkip);
bool mtk_cfd_SetUIHue(struct st_cfd_info *pstCFDInfo,
	__u16 u16Hue);
bool mtk_cfd_SetUISaturation(struct st_cfd_info *pstCFDInfo,
	__u16 u16Saturation);
bool mtk_cfd_SetUIContrast(struct st_cfd_info *pstCFDInfo,
	__u16 u16Contrast);
bool mtk_cfd_SetUIBrightness(struct st_cfd_info *pstCFDInfo,
	__u16 u16BrightnessR, __u16 u16BrightnessG, __u16 u16BrightnessB);
bool mtk_cfd_SetUIRGBGain(struct st_cfd_info *pstCFDInfo,
	__u16 u16GainR, __u16 u16GainG, __u16 u16GainB);
bool mtk_cfd_SetUIColorRange(struct st_cfd_info *pstCFDInfo,
	__u8 u8ColorRangeType, __u8 u8WhiteLevel, __u8 u8BlackLevel);
bool mtk_cfd_SetUIColorCorrection(struct st_cfd_info *pstCFDInfo,
	bool bEnable, __s16 *ps16Matries, __u16 u16MatriesNum);
bool mtk_cfd_SetUILinear(struct st_cfd_info *pstCFDInfo,
	bool bLinear);
bool mtk_cfd_SetUIHDR(struct st_cfd_info *pstCFDInfo,
	__u8 u8HDREnableMode);
bool mtk_cfd_SetUITMOLevel(struct st_cfd_info *pstCFDInfo,
	__u8 u8TMOLevel);
bool mtk_cfd_SetXCStatusIMode(struct st_cfd_info *pstCFDInfo,
	bool bIMode);
bool mtk_cfd_SetXCStatusPreVSD(struct st_cfd_info *pstCFDInfo,
	bool bPreVSD);
bool mtk_cfd_SetXCStatusMADIDataFmt(
	struct st_cfd_info *pstCFDInfo, __u8 u8Fmt);
bool mtk_cfd_SetXCStatusHVSP(struct st_cfd_info *pstCFDInfo,
	bool bHVSP);
bool mtk_cfd_SetPQStatusBypass(struct st_cfd_info *pstCFDInfo,
	bool bPQBypass);
bool mtk_cfd_SetDVStatusFlag(struct st_cfd_info *pstCFDInfo,
	bool bIsDolby);
bool mtk_cfd_SetDVStatusSkipUI(struct st_cfd_info *pstCFDInfo,
	bool bSkipUI);
bool mtk_cfd_SetTCHStatusFlag(struct st_cfd_info *pstCFDInfo,
	bool bIsTCH);
bool mtk_cfd_SetMainParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetHDMIParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetMMParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetTMOParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetOutputParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetUIParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetBlackAndWhite(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetDolbyHdrParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_Set420CtrlParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetFileGrainParam(struct st_cfd_info *pstCFDInfo);
bool mtk_cfd_SetFire(struct st_cfd_info *pstCFDInfo, bool bForce);

#undef INTERFACE
#endif		// _MTK_PQ_CFD_H_
