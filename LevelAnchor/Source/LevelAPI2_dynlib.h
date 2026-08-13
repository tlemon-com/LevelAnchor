#pragma once

//#include <iostream> // used for printf()

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif	// __cplusplus

#ifdef UNICODE
#define tstring LPCWSTR
#else
#define tstring LPCSTR
#endif

// stdio.h only used for printf error messages
#include <stdio.h>

enum DataRanges
{
	max_channels = 16,
	max_frames = (16 * 1024) // samples per channel
};

enum SurroundOrders
{
	SMTPE_ITU_AC3,
	FilmDolbyDigital,
	DTS_ProControl
};

enum MeterTypes
{
	LU,
	LUFS,

	LU_UNGATED,
	LUFS_UNGATED,

	LU_Min,
	LUFS_Min,

	LU_Dialog,
	LUFS_Dialog,

	LU_Dialog_UNGATED,
	LUFS_Dialog_UNGATED,

	Dialog_percentage,

	LU_Max_M,
	LUFS_Max_M,

	LU_Max_S, // 3 sec
	LUFS_Max_S,

	LRA, // (LRA_High - LRA_Low)
	LRA_Min,
	LRA_Max,
	LRA_Range_Low,
	LRA_Range_High,

	LU_BS1771, // realtime
	LU_BS1771_Filtered,
	LU_BS1771_Shortterm, // 3s

	LUFS_BS1771,
	LUFS_BS1771_Filtered,
	LUFS_BS1771_Shortterm, // 3s

	PPM,
	PPM_Max,

	RMS_Max,

	Peak,
	TruePeak,

	PSR,
	PeakLoudness,
	Limited_dB,

	AdjustLevel,
	AdjustLevel_NoPeak,
};

typedef void* LevelAPI2Ptr;
typedef void* LevelAPI2_RealtimeAdjusterPtr;

// Typedefs
typedef LevelAPI2Ptr(*LevelAPI2_Create_LevelAPI2_Ptr) (const unsigned int channels, const unsigned int samplerate, const unsigned int bitdepth);
typedef void (*LevelAPI2_Destroy_LevelAPI2_Ptr) (LevelAPI2Ptr obj);

typedef bool (*LevelAPI2_SetCertificate_Ptr) (const char* certificate_string, size_t bufferSize);
typedef bool (*LevelAPI2_GetErrorString_Ptr) (char* error_string, size_t bufferSize);

typedef int (*LevelAPI2_GetRevision_Ptr) ();

typedef void (*LevelAPI2_Init_Ptr) (LevelAPI2Ptr obj, const unsigned int channels, const unsigned int samplerate, const unsigned int bitdepth);

typedef unsigned int (*LevelAPI2_GetChannels_Ptr) (LevelAPI2Ptr obj);
typedef unsigned int (*LevelAPI2_GetSamplerate_Ptr) (LevelAPI2Ptr obj);
typedef unsigned int (*LevelAPI2_GetBitrate_Ptr) (LevelAPI2Ptr obj);

typedef bool (*LevelAPI2_AnalyseInterleaved_Ptr) (LevelAPI2Ptr obj, const float* samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_Analyse_Ptr) (LevelAPI2Ptr obj, const float** samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_AnalyseFile_Ptr) (LevelAPI2Ptr obj, const char* infilename);

typedef bool (*LevelAPI2_AdjustInterleaved_Ptr) (LevelAPI2Ptr obj, float* samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_Adjust_Ptr) (LevelAPI2Ptr obj, float** samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_AdjustFile_Ptr) (LevelAPI2Ptr obj, const char* infilename, const char* outfilename);

typedef float (*LevelAPI2_GetLevel_Ptr) (LevelAPI2Ptr obj, const enum MeterTypes metertype, const int channel);
typedef bool (*LevelAPI2_GetLimitingNeeded_Ptr) (LevelAPI2Ptr obj);

typedef float (*LevelAPI2_GetAdjustLevel_Ptr) (LevelAPI2Ptr obj, const bool ignore_peak_limit);
typedef bool (*LevelAPI2_SetAdjustLevel_Ptr) (LevelAPI2Ptr obj, const float dB);

typedef bool (*LevelAPI2_SetPresetEBU_R128_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetPresetATSC_Ptr) (LevelAPI2Ptr obj);

typedef enum SurroundOrders(*LevelAPI2_GetSurroundOrder_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetSurroundOrder_Ptr) (LevelAPI2Ptr obj, enum SurroundOrders type);

typedef int (*LevelAPI2_GetDialogChannels_Ptr) (LevelAPI2Ptr obj); // used for dialog detection when non surround (5 or 6 channels)
typedef bool (*LevelAPI2_GetDialogChannel_Ptr) (LevelAPI2Ptr obj, const int channel);
typedef bool (*LevelAPI2_SetDialogChannel_Ptr) (LevelAPI2Ptr obj, const int channel, const bool on);

typedef unsigned int (*LevelAPI2_GetAdjustLatency_Ptr) (LevelAPI2Ptr obj);

typedef float (*LevelAPI2_GetChannelWeight_Ptr) (LevelAPI2Ptr obj, const unsigned int channel);
typedef bool (*LevelAPI2_SetChannelWeight_Ptr) (LevelAPI2Ptr obj, const unsigned int channel, const float weight);

typedef float (*LevelAPI2_GetCalibration_Ptr) (LevelAPI2Ptr obj, const enum MeterTypes metertype);
typedef bool (*LevelAPI2_SetCalibration_Ptr) (LevelAPI2Ptr obj, const enum MeterTypes metertype, const float level);

typedef float (*LevelAPI2_GetRelativeGate_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetRelativeGate_Ptr) (LevelAPI2Ptr obj, const float threshold);

typedef bool (*LevelAPI2_GetDialogGate_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetDialogGate_Ptr) (LevelAPI2Ptr obj, const bool on);

typedef bool (*LevelAPI2_GetLimiterEnable_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetLimiterEnable_Ptr) (LevelAPI2Ptr obj, const bool on);

typedef bool (*LevelAPI2_GetPPMEnable_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetPPMEnable_Ptr) (LevelAPI2Ptr obj, const bool on);

typedef bool (*LevelAPI2_GetRMSEnable_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetRMSEnable_Ptr) (LevelAPI2Ptr obj, const bool on);

typedef unsigned int (*LevelAPI2_GetTimeWindow_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetTimeWindow_Ptr) (LevelAPI2Ptr obj, const unsigned int seconds);

typedef float (*LevelAPI2_GetLimiterThreshold_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetLimiterThreshold_Ptr) (LevelAPI2Ptr obj, const float dB);

typedef float (*LevelAPI2_GetLimiterThresholdAdjust_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetLimiterThresholdAdjust_Ptr) (LevelAPI2Ptr obj, const float dB);

typedef enum MeterTypes(*LevelAPI2_GetAdjustTargetType_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetAdjustTargetType_Ptr) (LevelAPI2Ptr obj, enum MeterTypes metertype);

typedef float (*LevelAPI2_GetAdjustTargetLevel_Ptr) (LevelAPI2Ptr obj);
typedef bool (*LevelAPI2_SetAdjustTargetLevel_Ptr) (LevelAPI2Ptr obj, const float level);

typedef int* (*LevelAPI2_GetHistogramLUFS_Ptr) (LevelAPI2Ptr obj, int* size);
typedef int* (*LevelAPI2_GetHistogramLUFS_Dialog_Ptr) (LevelAPI2Ptr obj, int* size);
typedef int* (*LevelAPI2_GetHistogramLUFS_S_Ptr) (LevelAPI2Ptr obj, int* size);
typedef int* (*LevelAPI2_GetHistogramPSR_Ptr) (LevelAPI2Ptr obj, int* size);

typedef float (*LevelAPI2_dB2float_Ptr) (LevelAPI2Ptr obj, const float db);
typedef float (*LevelAPI2_float2dB_Ptr) (LevelAPI2Ptr obj, const float flt);
typedef float (*LevelAPI2_float2LUFS_Ptr) (LevelAPI2Ptr obj, const float flt);
typedef float (*LevelAPI2_LUFS2float_Ptr) (LevelAPI2Ptr obj, const float db);
typedef int (*LevelAPI2_PSR2tablePos_Ptr) (LevelAPI2Ptr obj, const float psr);
typedef float (*LevelAPI2_tablePos2PSR_Ptr) (LevelAPI2Ptr obj, const int pos);
typedef int (*LevelAPI2_LUFS2tablePos_Ptr) (LevelAPI2Ptr obj, const float lufs);
typedef float (*LevelAPI2_tablePos2LUFS_Ptr) (LevelAPI2Ptr obj, const int pos);

typedef LevelAPI2_RealtimeAdjusterPtr(*LevelAPI2_RealtimeAdjuster_Create_Ptr) (const unsigned int channels, const unsigned int samplerate, const unsigned int bitdepth);
typedef void (*LevelAPI2_RealtimeAdjuster_Destroy_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef bool (*LevelAPI2_RealtimeAdjuster_Adjust_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, float** samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_RealtimeAdjuster_AdjustInterleaved_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, float* samples, const unsigned int sample_frames);
typedef bool (*LevelAPI2_RealtimeAdjuster_Reset_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, const unsigned int channels, const unsigned int samplerate, const unsigned int bitdepth);
typedef int (*LevelAPI2_RealtimeAdjuster_GetPreset_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef bool (*LevelAPI2_RealtimeAdjuster_SetPreset_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, const unsigned int p);
typedef float (*LevelAPI2_RealtimeAdjuster_GetTargetLUFS_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef bool (*LevelAPI2_RealtimeAdjuster_SetTargetLUFS_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, const float v);
typedef float (*LevelAPI2_RealtimeAdjuster_GetLastAppliedGainDb_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetWarmupSecondsLeft_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetWarmupSecondsMax_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsLeft_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsMax_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetLevel_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, const enum MeterTypes metertype, const int channel);
typedef float (*LevelAPI2_RealtimeAdjuster_GetLUFS_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetPSR_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetLRA_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef float (*LevelAPI2_RealtimeAdjuster_GetBS1771_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef bool (*LevelAPI2_RealtimeAdjuster_GetLimiterEnabled_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj);
typedef bool (*LevelAPI2_RealtimeAdjuster_SetLimiterEnabled_Ptr) (LevelAPI2_RealtimeAdjusterPtr obj, const bool enable);


struct LevelAPI2_FunctionHandles
{
	LevelAPI2_Create_LevelAPI2_Ptr Create_LevelAPI2;
	LevelAPI2_Destroy_LevelAPI2_Ptr Destroy_LevelAPI2;

	LevelAPI2_SetCertificate_Ptr SetCertificate;
	LevelAPI2_GetErrorString_Ptr GetErrorString;

	LevelAPI2_GetRevision_Ptr GetRevision;

	LevelAPI2_Init_Ptr Init;

	LevelAPI2_GetChannels_Ptr GetChannels;
	LevelAPI2_GetSamplerate_Ptr GetSamplerate;
	LevelAPI2_GetBitrate_Ptr GetBitrate;

	LevelAPI2_AnalyseInterleaved_Ptr AnalyseInterleaved;
	LevelAPI2_Analyse_Ptr Analyse;
	LevelAPI2_AnalyseFile_Ptr AnalyseFile;

	LevelAPI2_AdjustInterleaved_Ptr AdjustInterleaved;
	LevelAPI2_Adjust_Ptr Adjust;
	LevelAPI2_AdjustFile_Ptr AdjustFile;

	LevelAPI2_GetLevel_Ptr GetLevel;
	LevelAPI2_GetLimitingNeeded_Ptr GetLimitingNeeded;

	LevelAPI2_GetAdjustLevel_Ptr GetAdjustLevel;
	LevelAPI2_SetAdjustLevel_Ptr SetAdjustLevel;

	LevelAPI2_SetPresetEBU_R128_Ptr SetPresetEBU_R128;
	LevelAPI2_SetPresetATSC_Ptr SetPresetATSC;

	LevelAPI2_GetSurroundOrder_Ptr GetSurroundOrder;
	LevelAPI2_SetSurroundOrder_Ptr SetSurroundOrder;

	LevelAPI2_GetDialogChannels_Ptr GetDialogChannels;
	LevelAPI2_GetDialogChannel_Ptr GetDialogChannel;
	LevelAPI2_SetDialogChannel_Ptr SetDialogChannel;

	LevelAPI2_GetAdjustLatency_Ptr GetAdjustLatency;

	LevelAPI2_GetChannelWeight_Ptr GetChannelWeight;
	LevelAPI2_SetChannelWeight_Ptr SetChannelWeight;

	LevelAPI2_GetCalibration_Ptr GetCalibration;
	LevelAPI2_SetCalibration_Ptr SetCalibration;

	LevelAPI2_GetRelativeGate_Ptr GetRelativeGate;
	LevelAPI2_SetRelativeGate_Ptr SetRelativeGate;

	LevelAPI2_GetDialogGate_Ptr GetDialogGate;
	LevelAPI2_SetDialogGate_Ptr SetDialogGate;

	LevelAPI2_GetLimiterEnable_Ptr GetLimiterEnable;
	LevelAPI2_SetLimiterEnable_Ptr SetLimiterEnable;

	LevelAPI2_GetPPMEnable_Ptr GetPPMEnable;
	LevelAPI2_SetPPMEnable_Ptr SetPPMEnable;

	LevelAPI2_GetRMSEnable_Ptr GetRMSEnable;
	LevelAPI2_SetRMSEnable_Ptr SetRMSEnable;

	LevelAPI2_GetLimiterThreshold_Ptr GetLimiterThreshold;
	LevelAPI2_SetLimiterThreshold_Ptr SetLimiterThreshold;

	LevelAPI2_GetLimiterThresholdAdjust_Ptr GetLimiterThresholdAdjust;
	LevelAPI2_SetLimiterThresholdAdjust_Ptr SetLimiterThresholdAdjust;

	LevelAPI2_GetTimeWindow_Ptr GetTimeWindow;
	LevelAPI2_SetTimeWindow_Ptr SetTimeWindow;

	LevelAPI2_GetAdjustTargetType_Ptr GetAdjustTargetType;
	LevelAPI2_SetAdjustTargetType_Ptr SetAdjustTargetType;

	LevelAPI2_GetAdjustTargetLevel_Ptr GetAdjustTargetLevel;
	LevelAPI2_SetAdjustTargetLevel_Ptr SetAdjustTargetLevel;

	LevelAPI2_GetHistogramLUFS_Ptr GetHistogramLUFS;
	LevelAPI2_GetHistogramLUFS_Dialog_Ptr GetHistogramLUFS_Dialog;
	LevelAPI2_GetHistogramLUFS_S_Ptr GetHistogramLUFS_S;
	LevelAPI2_GetHistogramPSR_Ptr GetHistogramPSR;

	LevelAPI2_RealtimeAdjuster_Create_Ptr Create_RealtimeAdjuster;
	LevelAPI2_RealtimeAdjuster_Destroy_Ptr Destroy_RealtimeAdjuster; 
	LevelAPI2_RealtimeAdjuster_Adjust_Ptr RealtimeAdjuster_Adjust;
	LevelAPI2_RealtimeAdjuster_AdjustInterleaved_Ptr RealtimeAdjuster_AdjustInterleaved;
	LevelAPI2_RealtimeAdjuster_Reset_Ptr RealtimeAdjuster_Reset;
	LevelAPI2_RealtimeAdjuster_GetPreset_Ptr RealtimeAdjuster_GetPreset;
	LevelAPI2_RealtimeAdjuster_SetPreset_Ptr RealtimeAdjuster_SetPreset;
	LevelAPI2_RealtimeAdjuster_GetTargetLUFS_Ptr RealtimeAdjuster_GetTargetLUFS;
	LevelAPI2_RealtimeAdjuster_SetTargetLUFS_Ptr RealtimeAdjuster_SetTargetLUFS;
	LevelAPI2_RealtimeAdjuster_GetLevel_Ptr RealtimeAdjuster_GetLevel;
	LevelAPI2_RealtimeAdjuster_GetLastAppliedGainDb_Ptr RealtimeAdjuster_GetLastAppliedGainDb;
	LevelAPI2_RealtimeAdjuster_GetWarmupSecondsLeft_Ptr RealtimeAdjuster_GetWarmupSecondsLeft;
	LevelAPI2_RealtimeAdjuster_GetWarmupSecondsMax_Ptr RealtimeAdjuster_GetWarmupSecondsMax;
	LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsLeft_Ptr RealtimeAdjuster_GetCalibrationSecondsLeft;
	LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsMax_Ptr RealtimeAdjuster_GetCalibrationSecondsMax;
	LevelAPI2_RealtimeAdjuster_GetLUFS_Ptr RealtimeAdjuster_GetLUFS;
	LevelAPI2_RealtimeAdjuster_GetPSR_Ptr RealtimeAdjuster_GetPSR;
	LevelAPI2_RealtimeAdjuster_GetLRA_Ptr RealtimeAdjuster_GetLRA;
	LevelAPI2_RealtimeAdjuster_GetBS1771_Ptr RealtimeAdjuster_GetBS1771;

	LevelAPI2_RealtimeAdjuster_GetLimiterEnabled_Ptr RealtimeAdjuster_GetLimiterEnabled;
	LevelAPI2_RealtimeAdjuster_SetLimiterEnabled_Ptr RealtimeAdjuster_SetLimiterEnabled;


	LevelAPI2_dB2float_Ptr dB2float;
	LevelAPI2_float2dB_Ptr float2dB;
	LevelAPI2_float2LUFS_Ptr float2LUFS;
	LevelAPI2_LUFS2float_Ptr LUFS2float;

	LevelAPI2_PSR2tablePos_Ptr PSR2tablePos;
	LevelAPI2_tablePos2PSR_Ptr tablePos2PSR;
	LevelAPI2_LUFS2tablePos_Ptr LUFS2tablePos;
	LevelAPI2_tablePos2LUFS_Ptr tablePos2LUFS;

};

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define GETHANDLE_FUNC GetProcAddress
#elif defined(__APPLE__)
// .so library loader
#include <dlfcn.h>
#define GETHANDLE_FUNC CreateSOHandle

// helper function to create the pointers to the functions in the library
void* CreateSOHandle(void* lib_handle, const char* functionname)
{
	void* ambigious = (void*)dlsym(lib_handle, functionname);
	char* errorSO = 0;
	if ((errorSO = dlerror()) != 0)
	{
		//printf("%s\n", errorSO);
		return 0;
	}
	return ambigious;
}
#elif defined(__linux__)
// .so library loader
#include <dlfcn.h>
#define GETHANDLE_FUNC CreateSOHandle

// helper function to create the pointers to the functions in the library
void* CreateSOHandle(void* lib_handle, const char* functionname)
{
	void* ambigious = (void*)dlsym(lib_handle, functionname);
	char* errorSO = 0;
	if ((errorSO = dlerror()) != 0)
	{
		//printf("%s\n", errorSO);
		return 0;
	}
	return ambigious;
}
#endif

#ifdef UNICODE
tstring ConvertToTString(const char* text)
{
	// Get the length of the multibyte string
	size_t length = strlen(text);
	// Get the required buffer size for the wide character string
	size_t size = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0);
	if (size == 0)
	{
		// Handle error
		return NULL;
	}
	// Allocate memory for the wide character string
	wchar_t* buffer = (wchar_t*)malloc((size + 1) * sizeof(wchar_t));
	// Convert the multibyte string to wide character string
	MultiByteToWideChar(CP_UTF8, 0, text, (int)length, buffer, (int)size);
	// Null-terminate the wide character string
	buffer[size] = L'\0';
	// Return LPCWSTR or LPCSTR based on UNICODE definition
	return buffer;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
HMODULE InitFunctions(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
#elif defined(__APPLE__)
void* InitFunctions(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
#elif defined(__linux__)
void* InitFunctions(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
#endif
{
	levelAPI2->SetCertificate = NULL;
	levelAPI2->GetErrorString = NULL;

	levelAPI2->GetRevision = NULL;

	levelAPI2->Create_LevelAPI2 = NULL;
	levelAPI2->Destroy_LevelAPI2 = NULL;

	levelAPI2->Init = NULL;

	levelAPI2->GetChannels = NULL;
	levelAPI2->GetSamplerate = NULL;
	levelAPI2->GetBitrate = NULL;

	levelAPI2->AnalyseInterleaved = NULL;
	levelAPI2->Analyse = NULL;
	levelAPI2->AnalyseFile = NULL;

	levelAPI2->AdjustInterleaved = NULL;
	levelAPI2->Adjust = NULL;
	levelAPI2->AdjustFile = NULL;

	levelAPI2->GetLevel = NULL;
	levelAPI2->GetLimitingNeeded = NULL;

	levelAPI2->GetAdjustLevel = NULL;
	levelAPI2->SetAdjustLevel = NULL;

	levelAPI2->SetPresetEBU_R128 = NULL;
	levelAPI2->SetPresetATSC = NULL;

	levelAPI2->GetSurroundOrder = NULL;
	levelAPI2->SetSurroundOrder = NULL;

	levelAPI2->GetDialogChannels = NULL;
	levelAPI2->GetDialogChannel = NULL;
	levelAPI2->SetDialogChannel = NULL;

	levelAPI2->GetAdjustLatency = NULL;

	levelAPI2->GetChannelWeight = NULL;
	levelAPI2->SetChannelWeight = NULL;

	levelAPI2->GetCalibration = NULL;
	levelAPI2->SetCalibration = NULL;

	levelAPI2->GetRelativeGate = NULL;
	levelAPI2->SetRelativeGate = NULL;

	levelAPI2->GetDialogGate = NULL;
	levelAPI2->SetDialogGate = NULL;

	levelAPI2->GetLimiterEnable = NULL;
	levelAPI2->SetLimiterEnable = NULL;

	levelAPI2->GetPPMEnable = NULL;
	levelAPI2->SetPPMEnable = NULL;

	levelAPI2->GetRMSEnable = NULL;
	levelAPI2->SetRMSEnable = NULL;

	levelAPI2->GetLimiterThreshold = NULL;
	levelAPI2->SetLimiterThreshold = NULL;

	levelAPI2->GetLimiterThresholdAdjust = NULL;
	levelAPI2->SetLimiterThresholdAdjust = NULL;

	levelAPI2->GetTimeWindow = NULL;
	levelAPI2->SetTimeWindow = NULL;

	levelAPI2->GetAdjustTargetType = NULL;
	levelAPI2->SetAdjustTargetType = NULL;

	levelAPI2->GetAdjustTargetLevel = NULL;
	levelAPI2->SetAdjustTargetLevel = NULL;

	levelAPI2->GetHistogramLUFS = NULL;
	levelAPI2->GetHistogramLUFS_Dialog = NULL;
	levelAPI2->GetHistogramLUFS_S = NULL;
	levelAPI2->GetHistogramPSR = NULL;

	levelAPI2->dB2float = NULL;
	levelAPI2->float2dB = NULL;
	levelAPI2->float2LUFS = NULL;
	levelAPI2->LUFS2float = NULL;

	levelAPI2->PSR2tablePos = NULL;
	levelAPI2->tablePos2PSR = NULL;
	levelAPI2->LUFS2tablePos = NULL;
	levelAPI2->tablePos2LUFS = NULL;

	levelAPI2->Create_RealtimeAdjuster = NULL;
	levelAPI2->Destroy_RealtimeAdjuster = NULL;
	levelAPI2->RealtimeAdjuster_Adjust = NULL;
	levelAPI2->RealtimeAdjuster_AdjustInterleaved = NULL;
	levelAPI2->RealtimeAdjuster_Reset = NULL;
	levelAPI2->RealtimeAdjuster_GetPreset = NULL;
	levelAPI2->RealtimeAdjuster_SetPreset = NULL;
	levelAPI2->RealtimeAdjuster_GetTargetLUFS = NULL;
	levelAPI2->RealtimeAdjuster_SetTargetLUFS = NULL;
	levelAPI2->RealtimeAdjuster_GetLevel = NULL;
	levelAPI2->RealtimeAdjuster_GetLastAppliedGainDb = NULL;
	levelAPI2->RealtimeAdjuster_GetWarmupSecondsLeft = NULL;
	levelAPI2->RealtimeAdjuster_GetWarmupSecondsMax = NULL;
	levelAPI2->RealtimeAdjuster_GetCalibrationSecondsLeft = NULL;
	levelAPI2->RealtimeAdjuster_GetCalibrationSecondsMax = NULL;
	levelAPI2->RealtimeAdjuster_GetLUFS = NULL;
	levelAPI2->RealtimeAdjuster_GetPSR = NULL;
	levelAPI2->RealtimeAdjuster_GetLRA = NULL;
	levelAPI2->RealtimeAdjuster_GetBS1771 = NULL;
	levelAPI2->RealtimeAdjuster_GetLimiterEnabled = NULL;
	levelAPI2->RealtimeAdjuster_SetLimiterEnabled = NULL;


	// handle to loaded library module, returns a handle to the DLL, otherwise NULL
#if defined(_WIN32) || defined(_WIN64)
#ifdef UNICODE
	tstring buffer = ConvertToTString(libPath);
	HMODULE lib_handle = LoadLibrary(buffer);
	free((void*)buffer);
#else
	HMODULE lib_handle = LoadLibrary(libPath);
#endif
	if (!lib_handle)
	{
		DWORD dw = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		//printf("Error LoadLibrary %d\n", dw);
		return 0;
	}
#elif defined(__APPLE__)
		//void *lib_handle = dlmopen(LM_ID_NEWLM, libPath, RTLD_LAZY | RTLD_LOCAL);
		//void *lib_handle = dlopen(libPath, RTLD_LAZY | RTLD_LOCAL);
	void* lib_handle = dlopen(libPath, RTLD_LAZY);
	if (!lib_handle)
	{
		printf("%s\n", dlerror()); // #include <iostream>
		return 0;
	}
#elif defined(__linux__)
		//void *lib_handle = dlmopen(LM_ID_NEWLM, libPath, RTLD_LAZY | RTLD_LOCAL);
		//void *lib_handle = dlopen(libPath, RTLD_LAZY | RTLD_LOCAL);
	void* lib_handle = dlopen(libPath, RTLD_LAZY);
	if (!lib_handle)
	{
		printf("%s\n", dlerror()); // #include <iostream>
		return 0;
	}
#endif
	// returns the address of the DLL functions, otherwise NULL
	levelAPI2->SetCertificate = (LevelAPI2_SetCertificate_Ptr)GETHANDLE_FUNC(lib_handle, "SetCertificate");
	levelAPI2->GetErrorString = (LevelAPI2_GetErrorString_Ptr)GETHANDLE_FUNC(lib_handle, "GetErrorString");

	levelAPI2->GetRevision = (LevelAPI2_GetRevision_Ptr)GETHANDLE_FUNC(lib_handle, "GetRevision");

	levelAPI2->Create_LevelAPI2 = (LevelAPI2_Create_LevelAPI2_Ptr)GETHANDLE_FUNC(lib_handle, "Create_LevelAPI2");
	levelAPI2->Destroy_LevelAPI2 = (LevelAPI2_Destroy_LevelAPI2_Ptr)GETHANDLE_FUNC(lib_handle, "Destroy_LevelAPI2");

	levelAPI2->Init = (LevelAPI2_Init_Ptr)GETHANDLE_FUNC(lib_handle, "Init");

	levelAPI2->GetChannels = (LevelAPI2_GetChannels_Ptr)GETHANDLE_FUNC(lib_handle, "GetChannels");
	levelAPI2->GetSamplerate = (LevelAPI2_GetSamplerate_Ptr)GETHANDLE_FUNC(lib_handle, "GetSamplerate");
	levelAPI2->GetBitrate = (LevelAPI2_GetBitrate_Ptr)GETHANDLE_FUNC(lib_handle, "GetBitrate");

	levelAPI2->AnalyseInterleaved = (LevelAPI2_AnalyseInterleaved_Ptr)GETHANDLE_FUNC(lib_handle, "AnalyseInterleaved");
	levelAPI2->Analyse = (LevelAPI2_Analyse_Ptr)GETHANDLE_FUNC(lib_handle, "Analyse");
	levelAPI2->AnalyseFile = (LevelAPI2_AnalyseFile_Ptr)GETHANDLE_FUNC(lib_handle, "AnalyseFile");

	levelAPI2->AdjustInterleaved = (LevelAPI2_AdjustInterleaved_Ptr)GETHANDLE_FUNC(lib_handle, "AdjustInterleaved");
	levelAPI2->Adjust = (LevelAPI2_Adjust_Ptr)GETHANDLE_FUNC(lib_handle, "Adjust");
	levelAPI2->AdjustFile = (LevelAPI2_AdjustFile_Ptr)GETHANDLE_FUNC(lib_handle, "AdjustFile");

	levelAPI2->GetLevel = (LevelAPI2_GetLevel_Ptr)GETHANDLE_FUNC(lib_handle, "GetLevel");
	levelAPI2->GetLimitingNeeded = (LevelAPI2_GetLimitingNeeded_Ptr)GETHANDLE_FUNC(lib_handle, "GetLimitingNeeded");

	levelAPI2->GetAdjustLevel = (LevelAPI2_GetAdjustLevel_Ptr)GETHANDLE_FUNC(lib_handle, "GetAdjustLevel");
	levelAPI2->SetAdjustLevel = (LevelAPI2_SetAdjustLevel_Ptr)GETHANDLE_FUNC(lib_handle, "SetAdjustLevel");

	levelAPI2->SetPresetEBU_R128 = (LevelAPI2_SetPresetEBU_R128_Ptr)GETHANDLE_FUNC(lib_handle, "SetPresetEBU_R128");
	levelAPI2->SetPresetATSC = (LevelAPI2_SetPresetATSC_Ptr)GETHANDLE_FUNC(lib_handle, "SetPresetATSC");

	levelAPI2->GetSurroundOrder = (LevelAPI2_GetSurroundOrder_Ptr)GETHANDLE_FUNC(lib_handle, "GetSurroundOrder");
	levelAPI2->SetSurroundOrder = (LevelAPI2_SetSurroundOrder_Ptr)GETHANDLE_FUNC(lib_handle, "SetSurroundOrder");

	levelAPI2->GetDialogChannels = (LevelAPI2_GetDialogChannels_Ptr)GETHANDLE_FUNC(lib_handle, "GetDialogChannels");
	levelAPI2->GetDialogChannel = (LevelAPI2_GetDialogChannel_Ptr)GETHANDLE_FUNC(lib_handle, "GetDialogChannel");
	levelAPI2->SetDialogChannel = (LevelAPI2_SetDialogChannel_Ptr)GETHANDLE_FUNC(lib_handle, "SetDialogChannel");

	levelAPI2->GetAdjustLatency = (LevelAPI2_GetAdjustLatency_Ptr)GETHANDLE_FUNC(lib_handle, "GetAdjustLatency");

	levelAPI2->GetChannelWeight = (LevelAPI2_GetChannelWeight_Ptr)GETHANDLE_FUNC(lib_handle, "GetChannelWeight");
	levelAPI2->SetChannelWeight = (LevelAPI2_SetChannelWeight_Ptr)GETHANDLE_FUNC(lib_handle, "SetChannelWeight");

	levelAPI2->GetCalibration = (LevelAPI2_GetCalibration_Ptr)GETHANDLE_FUNC(lib_handle, "GetCalibration");
	levelAPI2->SetCalibration = (LevelAPI2_SetCalibration_Ptr)GETHANDLE_FUNC(lib_handle, "SetCalibration");

	levelAPI2->GetRelativeGate = (LevelAPI2_GetRelativeGate_Ptr)GETHANDLE_FUNC(lib_handle, "GetRelativeGate");
	levelAPI2->SetRelativeGate = (LevelAPI2_SetRelativeGate_Ptr)GETHANDLE_FUNC(lib_handle, "SetRelativeGate");

	levelAPI2->GetDialogGate = (LevelAPI2_GetDialogGate_Ptr)GETHANDLE_FUNC(lib_handle, "GetDialogGate");
	levelAPI2->SetDialogGate = (LevelAPI2_SetDialogGate_Ptr)GETHANDLE_FUNC(lib_handle, "SetDialogGate");

	levelAPI2->GetLimiterEnable = (LevelAPI2_GetLimiterEnable_Ptr)GETHANDLE_FUNC(lib_handle, "GetLimiterEnable");
	levelAPI2->SetLimiterEnable = (LevelAPI2_SetLimiterEnable_Ptr)GETHANDLE_FUNC(lib_handle, "SetLimiterEnable");

	levelAPI2->GetPPMEnable = (LevelAPI2_GetPPMEnable_Ptr)GETHANDLE_FUNC(lib_handle, "GetPPMEnable");
	levelAPI2->SetPPMEnable = (LevelAPI2_SetPPMEnable_Ptr)GETHANDLE_FUNC(lib_handle, "SetPPMEnable");

	levelAPI2->GetRMSEnable = (LevelAPI2_GetRMSEnable_Ptr)GETHANDLE_FUNC(lib_handle, "GetRMSEnable");
	levelAPI2->SetRMSEnable = (LevelAPI2_SetRMSEnable_Ptr)GETHANDLE_FUNC(lib_handle, "SetRMSEnable");

	levelAPI2->GetLimiterThreshold = (LevelAPI2_GetLimiterThreshold_Ptr)GETHANDLE_FUNC(lib_handle, "GetLimiterThreshold");
	levelAPI2->SetLimiterThreshold = (LevelAPI2_SetLimiterThreshold_Ptr)GETHANDLE_FUNC(lib_handle, "SetLimiterThreshold");

	levelAPI2->GetLimiterThresholdAdjust = (LevelAPI2_GetLimiterThresholdAdjust_Ptr)GETHANDLE_FUNC(lib_handle, "GetLimiterThresholdAdjust");
	levelAPI2->SetLimiterThresholdAdjust = (LevelAPI2_SetLimiterThresholdAdjust_Ptr)GETHANDLE_FUNC(lib_handle, "SetLimiterThresholdAdjust");

	levelAPI2->GetTimeWindow = (LevelAPI2_GetTimeWindow_Ptr)GETHANDLE_FUNC(lib_handle, "GetTimeWindow");
	levelAPI2->SetTimeWindow = (LevelAPI2_SetTimeWindow_Ptr)GETHANDLE_FUNC(lib_handle, "SetTimeWindow");

	levelAPI2->GetAdjustTargetType = (LevelAPI2_GetAdjustTargetType_Ptr)GETHANDLE_FUNC(lib_handle, "GetAdjustTargetType");
	levelAPI2->SetAdjustTargetType = (LevelAPI2_SetAdjustTargetType_Ptr)GETHANDLE_FUNC(lib_handle, "SetAdjustTargetType");

	levelAPI2->GetAdjustTargetLevel = (LevelAPI2_GetAdjustTargetLevel_Ptr)GETHANDLE_FUNC(lib_handle, "GetAdjustTargetLevel");
	levelAPI2->SetAdjustTargetLevel = (LevelAPI2_SetAdjustTargetLevel_Ptr)GETHANDLE_FUNC(lib_handle, "SetAdjustTargetLevel");

	levelAPI2->GetHistogramLUFS = (LevelAPI2_GetHistogramLUFS_Ptr)GETHANDLE_FUNC(lib_handle, "GetHistogramLUFS");
	levelAPI2->GetHistogramLUFS_Dialog = (LevelAPI2_GetHistogramLUFS_Dialog_Ptr)GETHANDLE_FUNC(lib_handle, "GetHistogramLUFS_Dialog");
	levelAPI2->GetHistogramLUFS_S = (LevelAPI2_GetHistogramLUFS_S_Ptr)GETHANDLE_FUNC(lib_handle, "GetHistogramLUFS_S");
	levelAPI2->GetHistogramPSR = (LevelAPI2_GetHistogramPSR_Ptr)GETHANDLE_FUNC(lib_handle, "GetHistogramPSR");

	levelAPI2->dB2float = (LevelAPI2_dB2float_Ptr)GETHANDLE_FUNC(lib_handle, "dB2float");
	levelAPI2->float2dB = (LevelAPI2_float2dB_Ptr)GETHANDLE_FUNC(lib_handle, "float2dB");
	levelAPI2->float2LUFS = (LevelAPI2_float2LUFS_Ptr)GETHANDLE_FUNC(lib_handle, "float2LUFS");
	levelAPI2->LUFS2float = (LevelAPI2_LUFS2float_Ptr)GETHANDLE_FUNC(lib_handle, "LUFS2float");

	levelAPI2->PSR2tablePos = (LevelAPI2_PSR2tablePos_Ptr)GETHANDLE_FUNC(lib_handle, "PSR2tablePos");
	levelAPI2->tablePos2PSR = (LevelAPI2_tablePos2PSR_Ptr)GETHANDLE_FUNC(lib_handle, "tablePos2PSR");
	levelAPI2->LUFS2tablePos = (LevelAPI2_LUFS2tablePos_Ptr)GETHANDLE_FUNC(lib_handle, "LUFS2tablePos");
	levelAPI2->tablePos2LUFS = (LevelAPI2_tablePos2LUFS_Ptr)GETHANDLE_FUNC(lib_handle, "tablePos2LUFS");

	levelAPI2->Create_RealtimeAdjuster = (LevelAPI2_RealtimeAdjuster_Create_Ptr)GETHANDLE_FUNC(lib_handle, "Create_RealtimeAdjuster");
	levelAPI2->Destroy_RealtimeAdjuster = (LevelAPI2_RealtimeAdjuster_Destroy_Ptr)GETHANDLE_FUNC(lib_handle, "Destroy_RealtimeAdjuster");
	levelAPI2->RealtimeAdjuster_Adjust = (LevelAPI2_RealtimeAdjuster_Adjust_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_Adjust");
	levelAPI2->RealtimeAdjuster_AdjustInterleaved = (LevelAPI2_RealtimeAdjuster_AdjustInterleaved_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_AdjustInterleaved");
	levelAPI2->RealtimeAdjuster_Reset = (LevelAPI2_RealtimeAdjuster_Reset_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_Reset");
	levelAPI2->RealtimeAdjuster_GetPreset = (LevelAPI2_RealtimeAdjuster_GetPreset_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetPreset");
	levelAPI2->RealtimeAdjuster_SetPreset = (LevelAPI2_RealtimeAdjuster_SetPreset_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_SetPreset");
	levelAPI2->RealtimeAdjuster_GetTargetLUFS = (LevelAPI2_RealtimeAdjuster_GetTargetLUFS_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetTargetLUFS");
	levelAPI2->RealtimeAdjuster_SetTargetLUFS = (LevelAPI2_RealtimeAdjuster_SetTargetLUFS_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_SetTargetLUFS");
	levelAPI2->RealtimeAdjuster_GetLevel = (LevelAPI2_RealtimeAdjuster_GetLevel_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetLevel");
	levelAPI2->RealtimeAdjuster_GetLastAppliedGainDb = (LevelAPI2_RealtimeAdjuster_GetLastAppliedGainDb_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetLastAppliedGainDb");
	levelAPI2->RealtimeAdjuster_GetWarmupSecondsLeft = (LevelAPI2_RealtimeAdjuster_GetWarmupSecondsLeft_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetWarmupSecondsLeft");
	levelAPI2->RealtimeAdjuster_GetWarmupSecondsMax = (LevelAPI2_RealtimeAdjuster_GetWarmupSecondsMax_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetWarmupSecondsMax");
	levelAPI2->RealtimeAdjuster_GetCalibrationSecondsLeft = (LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsLeft_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetCalibrationSecondsLeft");
	levelAPI2->RealtimeAdjuster_GetCalibrationSecondsMax = (LevelAPI2_RealtimeAdjuster_GetCalibrationSecondsMax_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetCalibrationSecondsMax");
	levelAPI2->RealtimeAdjuster_GetLUFS = (LevelAPI2_RealtimeAdjuster_GetLUFS_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetLUFS");
	levelAPI2->RealtimeAdjuster_GetPSR = (LevelAPI2_RealtimeAdjuster_GetPSR_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetPSR");
	levelAPI2->RealtimeAdjuster_GetLRA = (LevelAPI2_RealtimeAdjuster_GetLRA_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetLRA");
	levelAPI2->RealtimeAdjuster_GetBS1771 = (LevelAPI2_RealtimeAdjuster_GetBS1771_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetBS1771");
	levelAPI2->RealtimeAdjuster_GetLimiterEnabled = (LevelAPI2_RealtimeAdjuster_GetLimiterEnabled_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_GetLimiterEnabled");
	levelAPI2->RealtimeAdjuster_SetLimiterEnabled = (LevelAPI2_RealtimeAdjuster_SetLimiterEnabled_Ptr)GETHANDLE_FUNC(lib_handle, "RealtimeAdjuster_SetLimiterEnabled");

	return lib_handle;
};

#if defined(_WIN32) || defined(_WIN64)
void* LevelAPI2_LoadDLL(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
{
	void* dll_handle = (HMODULE*)malloc(sizeof(HMODULE));
	HMODULE temp_handle = InitFunctions(libPath, levelAPI2);
	if (temp_handle != NULL)
	{
#pragma warning(suppress : 6011)
		// suppress : 6011 we check the dereference for NULL already
		* ((HMODULE*)dll_handle) = temp_handle;
	}
	else printf("error loading handle\n");

	if (!(levelAPI2->Create_LevelAPI2))
	{// check if function pointer is not 0
		//printf("Failed to load the functions from the LevelAPI dynamic library. Library file not found?\n\n");
		free((HMODULE)dll_handle);
		return 0;
	}
	return dll_handle;
}
void LevelAPI2_UnloadDLL(void* dll_handle)
{   // close the dynamicly loaded library
	if (dll_handle)
	{
		HMODULE temp = *((HMODULE*)dll_handle);
		BOOL bRes = FreeLibrary(temp); // returns nonzero if successfull
		LPVOID lpMsgBuf;
		if (!bRes)
		{
			DWORD dw = GetLastError();

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				0, NULL);
		}
		free((HMODULE)dll_handle);
	}
}
#elif defined (__APPLE__)
void* LevelAPI2_LoadDLL(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
{
	void* dll_handle = InitFunctions(libPath, levelAPI2);
	if (!dll_handle)
	{
		//printf("Failed to load the functions from the LevelAPI dynamic library. Library file not found?\n\n");
		return 0;
	}
	return dll_handle;
}
void LevelAPI2_UnloadDLL(void* dll_handle)
{   // close the dynamicly loaded library
	if (dll_handle)
		dlclose(dll_handle);
}
#elif defined (__linux__)
void* LevelAPI2_LoadDLL(const char* libPath, struct LevelAPI2_FunctionHandles* levelAPI2)
{
	void* dll_handle = InitFunctions(libPath, levelAPI2);
	if (!dll_handle)
	{
		//printf("Failed to load the functions from the LevelAPI dynamic library. Library file not found?\n\n");
		return 0;
	}
	return dll_handle;
}
void LevelAPI2_UnloadDLL(void* dll_handle)
{   // close the dynamicly loaded library
	if (dll_handle)
		dlclose(dll_handle);
}
#endif

#ifdef __cplusplus
}		// extern "C"
#endif	// __cplusplus

