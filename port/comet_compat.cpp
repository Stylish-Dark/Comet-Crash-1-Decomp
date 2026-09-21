#include "comet_compat.h"
#include "ppu_recomp.h"

#include <cstdint>
#include <cstdio>

extern "C" {
void ps3_hle_register_ctx(uint32_t nid, const char* name, void (*fn)(ppu_context*));
void vm_write8(uint64_t addr, uint8_t value);
void vm_write32(uint64_t addr, uint32_t value);
void cellGcmSetWaitFlip(void);
}

namespace {
/* Exact NPEB00142 imports that the pinned ps3recomp runtime either does not
 * implement or deliberately leaves to title-specific policy.  These overrides
 * are installed after ppu_hle_init(), so a port never falls through to the
 * dispatcher's dangerous "unresolved import == CELL_OK" compatibility path. */
constexpr uint32_t NID_CELL_GCM_FUNC15                       = 0x3A33C1FDu; // _cellGcmFunc15
constexpr uint32_t NID_CELL_RESC_SET_WAIT_FLIP               = 0x0D3C22CEu; // cellRescSetWaitFlip
constexpr uint32_t NID_SYS_NET_FREE_THREAD_CONTEXT           = 0xFDB8F926u; // sys_net_free_thread_context
constexpr uint32_t NID_CELL_GAME_BOOT_CHECK                   = 0xF52639EAu; // cellGameBootCheck
constexpr uint32_t NID_CELL_GAME_CONTENT_PERMIT               = 0x70ACEC67u; // cellGameContentPermit

constexpr uint32_t NID_NP_MANAGER_GET_CONTENT_RATING_FLAG   = 0x6EE62ED2u; // sceNpManagerGetContentRatingFlag
constexpr uint32_t NID_NP_MANAGER_GET_ACCOUNT_REGION         = 0xB1E0718Bu; // sceNpManagerGetAccountRegion
constexpr uint32_t NID_NP_SCORE_CREATE_TITLE_CTX             = 0xB9F93BBBu; // sceNpScoreCreateTitleCtx
constexpr uint32_t NID_NP_SCORE_DESTROY_TITLE_CTX            = 0x259113B8u; // sceNpScoreDestroyTitleCtx
constexpr uint32_t NID_NP_SCORE_CREATE_TRANSACTION_CTX       = 0x6F5E8143u; // sceNpScoreCreateTransactionCtx
constexpr uint32_t NID_NP_SCORE_DESTROY_TRANSACTION_CTX      = 0xC5F4CF82u; // sceNpScoreDestroyTransactionCtx
constexpr uint32_t NID_NP_SCORE_RECORD_SCORE                 = 0x1672170Eu; // sceNpScoreRecordScore
constexpr uint32_t NID_NP_SCORE_GET_RANKING_BY_NPID          = 0x05D65DFFu; // sceNpScoreGetRankingByNpId
constexpr uint32_t NID_NP_SCORE_GET_RANKING_BY_RANGE         = 0xFBC82301u; // sceNpScoreGetRankingByRange
constexpr uint32_t NID_NP_SCORE_ABORT_TRANSACTION            = 0xEE5B20D9u; // sceNpScoreAbortTransaction
constexpr uint32_t NID_NP_TROPHY_ABORT_HANDLE                = 0x48BD97C7u; // sceNpTrophyAbortHandle

constexpr uint32_t NID_VIDEO_EXPORT_INITIALIZE2              = 0x2F457571u; // cellVideoExportInitialize2
constexpr uint32_t NID_VIDEO_EXPORT_FROM_FILE                = 0x81296524u; // cellVideoExportFromFile
constexpr uint32_t NID_VIDEO_EXPORT_FINALIZE                 = 0xC15BE817u; // cellVideoExportFinalize
constexpr uint32_t NID_VIDEO_UPLOAD_INITIALIZE               = 0x122E0D0Fu; // cellVideoUploadInitialize
constexpr uint32_t NID_CELL_REC_OPEN                         = 0x39651E01u; // cellRecOpen
constexpr uint32_t NID_CELL_REC_CLOSE                        = 0x4AC76585u; // cellRecClose
constexpr uint32_t NID_CELL_REC_STOP                         = 0x5B45439Du; // cellRecStop
constexpr uint32_t NID_CELL_REC_START                        = 0x964CD1B8u; // cellRecStart
constexpr uint32_t NID_CELL_REC_QUERY_MEM_SIZE               = 0xDBF22BD1u; // cellRecQueryMemSize

constexpr int32_t CELL_OK                                     = 0;
constexpr uint32_t CELL_GAME_GAMETYPE_HDD                     = 2u;
constexpr uint32_t CELL_GAME_SIZEKB_NOTCALC                    = 0xFFFFFFFFu;
constexpr int32_t SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID    = static_cast<int32_t>(0x8002A1A4u);
constexpr int32_t CELL_VIDEO_EXPORT_UTIL_ERROR_INITIALIZE      = static_cast<int32_t>(0x8002CA0Au);
constexpr int32_t CELL_VIDEO_EXPORT_UTIL_ERROR_ACCESS_ERROR    = static_cast<int32_t>(0x8002CA04u);
constexpr int32_t CELL_VIDEO_UPLOAD_ERROR_SERVICE_UNAVAILABLE  = static_cast<int32_t>(0x8002D004u);
constexpr int32_t CELL_REC_ERROR_FATAL                         = static_cast<int32_t>(0x8002C502u);
constexpr uint32_t CELL_REC_FALLBACK_MEM_SIZE                  = 0x00900000u;

inline void ret(ppu_context* ctx, int32_t value) {
    ctx->gpr[3] = static_cast<uint64_t>(static_cast<int64_t>(value));
}
inline void ret_u32(ppu_context* ctx, uint32_t value) {
    ctx->gpr[3] = value;
}

void log_once(const char* name, const char* disposition);

void guest_cstr(uint32_t ea, const char* text) {
    if (!ea || !text) return;
    for (uint32_t i=0;;++i) {
        const uint8_t c=static_cast<uint8_t>(text[i]);
        vm_write8(ea+i,c);
        if (!c) break;
    }
}

void hle_cellGameBootCheck(ppu_context* ctx) {
    const uint32_t type_ea=static_cast<uint32_t>(ctx->gpr[3]);
    const uint32_t attr_ea=static_cast<uint32_t>(ctx->gpr[4]);
    const uint32_t size_ea=static_cast<uint32_t>(ctx->gpr[5]);
    const uint32_t dir_ea =static_cast<uint32_t>(ctx->gpr[6]);
    if (type_ea) vm_write32(type_ea,CELL_GAME_GAMETYPE_HDD);
    if (attr_ea) vm_write32(attr_ea,0);
    if (size_ea) {
        vm_write32(size_ea+0,40u*1024u*1024u-256u);
        vm_write32(size_ea+4,CELL_GAME_SIZEKB_NOTCALC);
        vm_write32(size_ea+8,4u);
    }
    guest_cstr(dir_ea,"NPEB00142");
    static bool once=false; if(!once){once=true;log_once("cellGameBootCheck","HG/HDD title NPEB00142");}
    ret(ctx,CELL_OK);
}

void hle_cellGameContentPermit(ppu_context* ctx) {
    guest_cstr(static_cast<uint32_t>(ctx->gpr[3]),"/dev_hdd0/game/NPEB00142");
    guest_cstr(static_cast<uint32_t>(ctx->gpr[4]),"/dev_hdd0/game/NPEB00142/USRDIR");
    static bool once=false; if(!once){once=true;log_once("cellGameContentPermit","HDD content paths");}
    ret(ctx,CELL_OK);
}

void log_once(const char* name, const char* disposition) {
    /* The individual handlers use distinct static guards below; this helper
     * keeps the message format consistent without building a logging system. */
    std::fprintf(stderr, "[comet-compat] %s -> %s\n", name, disposition);
}

void hle_cellGcmFunc15(ppu_context* ctx) {
    static bool once=false; if(!once){once=true;log_once("_cellGcmFunc15","no-op (matches RPCS3 bring-up behavior)");}
    ret(ctx, CELL_OK);
}

void hle_cellRescSetWaitFlip(ppu_context* ctx) {
    static bool once=false; if(!once){once=true;log_once("cellRescSetWaitFlip","cellGcmSetWaitFlip");}
    cellGcmSetWaitFlip();
    ret(ctx, CELL_OK);
}

void hle_sysNetFreeThreadContext(ppu_context* ctx) {
    static bool once=false; if(!once){once=true;log_once("sys_net_free_thread_context","host cleanup success");}
    ret(ctx, CELL_OK);
}

/* Comet Crash's PSN bootstrap reads these two values before creating score
 * contexts.  Supplying sane offline identity metadata lets the title finish
 * its local setup; actual leaderboard operations below still fail explicitly
 * as offline instead of pretending a network transaction succeeded. */
void hle_npContentRating(ppu_context* ctx) {
    const uint32_t restricted_ea = static_cast<uint32_t>(ctx->gpr[3]);
    const uint32_t age_ea        = static_cast<uint32_t>(ctx->gpr[4]);
    if (restricted_ea) vm_write32(restricted_ea, 0);
    if (age_ea)        vm_write32(age_ea, 18);
    ret(ctx, CELL_OK);
}

void hle_npAccountRegion(ppu_context* ctx) {
    const uint32_t country_ea  = static_cast<uint32_t>(ctx->gpr[3]);
    const uint32_t language_ea = static_cast<uint32_t>(ctx->gpr[4]);
    if (country_ea) {
        vm_write8(country_ea + 0, 'U');
        vm_write8(country_ea + 1, 'S');
    }
    if (language_ea) vm_write32(language_ea, 1); /* CELL_SYSUTIL_LANG_ENGLISH_US */
    ret(ctx, CELL_OK);
}

void hle_npScoreCreateTitleCtx(ppu_context* ctx) {
    /* Positive IDs are success values for the NP score API.  They are only
     * local placeholders: every operation that would contact PSN returns the
     * official offline error below. */
    ret(ctx, 1);
}
void hle_npScoreDestroyTitleCtx(ppu_context* ctx) { ret(ctx, CELL_OK); }
void hle_npScoreCreateTransactionCtx(ppu_context* ctx) { ret(ctx, 1); }
void hle_npScoreDestroyTransactionCtx(ppu_context* ctx) { ret(ctx, CELL_OK); }
void hle_npScoreOffline(ppu_context* ctx) { ret(ctx, SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID); }
void hle_npScoreAbortTransaction(ppu_context* ctx) { ret(ctx, CELL_OK); }
void hle_npTrophyAbortHandle(ppu_context* ctx) { ret(ctx, CELL_OK); }

/* PS3's XMB recording/export/upload services have no meaningful native-PC
 * equivalent for the port.  Crucially, fail them *synchronously* so Comet's
 * existing error paths run; unresolved imports used to return fake success and
 * leave the game waiting for callbacks that would never arrive. */
void hle_videoExportInitialize2(ppu_context* ctx) { ret(ctx, CELL_VIDEO_EXPORT_UTIL_ERROR_INITIALIZE); }
void hle_videoExportFromFile(ppu_context* ctx)    { ret(ctx, CELL_VIDEO_EXPORT_UTIL_ERROR_ACCESS_ERROR); }
void hle_videoExportFinalize(ppu_context* ctx)    { ret(ctx, CELL_OK); }
void hle_videoUploadInitialize(ppu_context* ctx)  { ret(ctx, CELL_VIDEO_UPLOAD_ERROR_SERVICE_UNAVAILABLE); }

void hle_cellRecQueryMemSize(ppu_context* ctx) { ret_u32(ctx, CELL_REC_FALLBACK_MEM_SIZE); }
void hle_cellRecUnsupported(ppu_context* ctx)  { ret(ctx, CELL_REC_ERROR_FATAL); }

void reg(uint32_t nid, const char* name, void (*fn)(ppu_context*)) {
    ps3_hle_register_ctx(nid, name, fn);
}
}

void comet_compat_install_hle_overrides(void) {
    reg(NID_CELL_GCM_FUNC15, "_cellGcmFunc15 [Comet compat]", hle_cellGcmFunc15);
    reg(NID_CELL_RESC_SET_WAIT_FLIP, "cellRescSetWaitFlip [Comet compat]", hle_cellRescSetWaitFlip);
    reg(NID_SYS_NET_FREE_THREAD_CONTEXT, "sys_net_free_thread_context [Comet compat]", hle_sysNetFreeThreadContext);
    reg(NID_CELL_GAME_BOOT_CHECK, "cellGameBootCheck [Comet HG]", hle_cellGameBootCheck);
    reg(NID_CELL_GAME_CONTENT_PERMIT, "cellGameContentPermit [Comet HG]", hle_cellGameContentPermit);

    reg(NID_NP_MANAGER_GET_CONTENT_RATING_FLAG, "sceNpManagerGetContentRatingFlag [Comet offline]", hle_npContentRating);
    reg(NID_NP_MANAGER_GET_ACCOUNT_REGION, "sceNpManagerGetAccountRegion [Comet offline]", hle_npAccountRegion);
    reg(NID_NP_SCORE_CREATE_TITLE_CTX, "sceNpScoreCreateTitleCtx [Comet offline]", hle_npScoreCreateTitleCtx);
    reg(NID_NP_SCORE_DESTROY_TITLE_CTX, "sceNpScoreDestroyTitleCtx [Comet offline]", hle_npScoreDestroyTitleCtx);
    reg(NID_NP_SCORE_CREATE_TRANSACTION_CTX, "sceNpScoreCreateTransactionCtx [Comet offline]", hle_npScoreCreateTransactionCtx);
    reg(NID_NP_SCORE_DESTROY_TRANSACTION_CTX, "sceNpScoreDestroyTransactionCtx [Comet offline]", hle_npScoreDestroyTransactionCtx);
    reg(NID_NP_SCORE_RECORD_SCORE, "sceNpScoreRecordScore [Comet offline]", hle_npScoreOffline);
    reg(NID_NP_SCORE_GET_RANKING_BY_NPID, "sceNpScoreGetRankingByNpId [Comet offline]", hle_npScoreOffline);
    reg(NID_NP_SCORE_GET_RANKING_BY_RANGE, "sceNpScoreGetRankingByRange [Comet offline]", hle_npScoreOffline);
    reg(NID_NP_SCORE_ABORT_TRANSACTION, "sceNpScoreAbortTransaction [Comet offline]", hle_npScoreAbortTransaction);
    reg(NID_NP_TROPHY_ABORT_HANDLE, "sceNpTrophyAbortHandle [Comet compat]", hle_npTrophyAbortHandle);

    reg(NID_VIDEO_EXPORT_INITIALIZE2, "cellVideoExportInitialize2 [Comet unsupported]", hle_videoExportInitialize2);
    reg(NID_VIDEO_EXPORT_FROM_FILE, "cellVideoExportFromFile [Comet unsupported]", hle_videoExportFromFile);
    reg(NID_VIDEO_EXPORT_FINALIZE, "cellVideoExportFinalize [Comet unsupported]", hle_videoExportFinalize);
    reg(NID_VIDEO_UPLOAD_INITIALIZE, "cellVideoUploadInitialize [Comet unsupported]", hle_videoUploadInitialize);
    reg(NID_CELL_REC_OPEN, "cellRecOpen [Comet unsupported]", hle_cellRecUnsupported);
    reg(NID_CELL_REC_CLOSE, "cellRecClose [Comet unsupported]", hle_cellRecUnsupported);
    reg(NID_CELL_REC_STOP, "cellRecStop [Comet unsupported]", hle_cellRecUnsupported);
    reg(NID_CELL_REC_START, "cellRecStart [Comet unsupported]", hle_cellRecUnsupported);
    reg(NID_CELL_REC_QUERY_MEM_SIZE, "cellRecQueryMemSize [Comet unsupported]", hle_cellRecQueryMemSize);

    std::fprintf(stderr, "[comet-compat] installed Comet-specific HLE compatibility overrides\n");
}
