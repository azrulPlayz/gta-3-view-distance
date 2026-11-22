// GTA III (Android 1.8, ARM32, libR1.so) - View Distance + Streaming mod untuk AML
// Simbol diverifikasi dari libR1.so milik user.
#include <stdint.h>
#include <mod/amlmod.h>
#include <mod/logger.h>

MYMOD(net.kamu.gta3viewdist, GTA3 View Distance, 1.0, Kamu)

// ====== ATUR DI SINI (1.0 = normal) ======
static const float LOD_SCALE    = 50.0f;  // jarak model detail (bawaan game: 1.2, ini nilai langsung)
static const float FARCLIP_MULT = 100.5f;  // pengali jarak render maksimum
static const float FOG_MULT     = 5.5f;  // pengali awal kabut (samakan dengan FARCLIP_MULT)
static const float STREAM_MULT  = 50.0f;  // pengali memori streaming map
// =========================================

static float   *pLodScale  = nullptr;
static int     *pMemAvail  = nullptr;
static float   *pFarClip   = nullptr;   // float[96]
static float   *pFogStart  = nullptr;   // float[96]

// CStreaming::Init(): setelah game set memori streaming, kita kalikan
static void (*Streaming_Init_orig)() = nullptr;
static void Streaming_Init_hook() {
    Streaming_Init_orig();
    if (pMemAvail && *pMemAvail > 0) {
        int before = *pMemAvail;
        *pMemAvail = (int)((float)before * STREAM_MULT);
        logger->Info("streaming memory: %d -> %d", before, *pMemAvail);
    }
}

// CTimeCycle::Initialise(): setelah timecyc.dat dibaca, kalikan jarak render dan kabut
static void (*TimeCycle_Init_orig)() = nullptr;
static void TimeCycle_Init_hook() {
    TimeCycle_Init_orig();
    for (int i = 0; i < 96; i++) {
        if (pFarClip)  pFarClip[i]  *= FARCLIP_MULT;
        if (pFogStart) pFogStart[i] *= FOG_MULT;
    }
    logger->Info("far clip dan fog sudah dikalikan");
}

// CTimeCycle::Update(): dipanggil tiap frame, jaga LOD scale tetap
static void (*TimeCycle_Update_orig)() = nullptr;
static void TimeCycle_Update_hook() {
    TimeCycle_Update_orig();
    if (pLodScale) *pLodScale = LOD_SCALE;
}

extern "C" void OnModLoad() {
    void* h = aml->GetLibHandle("libR1.so");
    if (!h) { logger->Error("libR1.so tidak ketemu"); return; }

    pLodScale = (float*)aml->GetSym(h, "_ZN9CRenderer15ms_lodDistScaleE");
    pMemAvail = (int*)  aml->GetSym(h, "_ZN10CStreaming18ms_memoryAvailableE");
    pFarClip  = (float*)aml->GetSym(h, "_ZN10CTimeCycle10m_fFarClipE");
    pFogStart = (float*)aml->GetSym(h, "_ZN10CTimeCycle11m_fFogStartE");

    void* sInit = (void*)aml->GetSym(h, "_ZN10CStreaming4InitEv");
    void* tInit = (void*)aml->GetSym(h, "_ZN10CTimeCycle10InitialiseEv");
    void* tUpd  = (void*)aml->GetSym(h, "_ZN10CTimeCycle6UpdateEv");

    logger->Info("lod=%p mem=%p far=%p fog=%p sInit=%p tInit=%p tUpd=%p",
                 pLodScale, pMemAvail, pFarClip, pFogStart, sInit, tInit, tUpd);

    if (sInit) aml->Hook(sInit, (void*)Streaming_Init_hook,  (void**)&Streaming_Init_orig);
    if (tInit) aml->Hook(tInit, (void*)TimeCycle_Init_hook,  (void**)&TimeCycle_Init_orig);
    if (tUpd)  aml->Hook(tUpd,  (void*)TimeCycle_Update_hook,(void**)&TimeCycle_Update_orig);
}
