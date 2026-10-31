#include <src/plugins/PluginAPI.hpp>
#include <src/protocols/SessionLock.hpp>
#include <src/protocols/core/Output.hpp>
#include <src/output/Monitor.hpp>
#include <src/debug/log/Logger.hpp>

#include <algorithm>
#include <vector>

// Hyprland 0.56.2 drops the new_id of get_lock_surface when the output's monitor
// is already gone. The lock client's later destroy() then hits invalid_object and
// the compositor kills it (hyprwm/Hyprland#16385). This hook creates an inert
// object for that id so destroy() stays valid.

using GetLockSurfaceFn = void (*)(CSessionLockProtocol*, CExtSessionLockV1*, uint32_t, wl_resource*, wl_resource*);

static HANDLE                                    handle;
static CFunctionHook*                            hook;
static std::vector<SP<CExtSessionLockSurfaceV1>> inertSurfaces;

static void forget(CExtSessionLockSurfaceV1* resource) {
    std::erase_if(inertSurfaces, [resource](const auto& other) { return other.get() == resource; });
}

static void createInertLockSurface(CExtSessionLockV1* lock, uint32_t id) {
    const auto resource = inertSurfaces.emplace_back(makeShared<CExtSessionLockSurfaceV1>(lock->client(), lock->version(), id));

    if (!resource->resource()) {
        lock->noMemory();
        inertSurfaces.pop_back();
        return;
    }

    resource->setDestroy(forget);
    resource->setOnDestroy(forget);
    Log::logger->log(Log::WARN, "[lockfix] get_lock_surface for a gone output: created inert lock surface {}", id);
}

static void onGetLockSurface(CSessionLockProtocol* self, CExtSessionLockV1* lock, uint32_t id, wl_resource* surface, wl_resource* output) {
    const auto outputResource = CWLOutputResource::fromResource(output);
    const auto monitor        = outputResource ? outputResource->m_monitor.lock() : PHLMONITOR{};
    if (!monitor) {
        createInertLockSurface(lock, id);
        return;
    }

    reinterpret_cast<GetLockSurfaceFn>(hook->m_original)(self, lock, id, surface, output);
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE h) {
    handle = h;

    if (std::string{__hyprland_api_get_hash()} != __hyprland_api_get_client_hash())
        throw std::runtime_error("[lockfix] built against another Hyprland version; rebuild it");

    for (const auto& match : HyprlandAPI::findFunctionsByName(handle, "onGetLockSurface")) {
        if (match.demangled.starts_with("CSessionLockProtocol::onGetLockSurface(")) {
            hook = HyprlandAPI::createFunctionHook(handle, match.address, reinterpret_cast<void*>(&onGetLockSurface));
            break;
        }
    }

    if (!hook || !hook->hook())
        throw std::runtime_error("[lockfix] could not hook CSessionLockProtocol::onGetLockSurface");

    return {"lockfix", "Keep the lock client alive when its output disappears (hyprwm/Hyprland#16385)", "p", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    inertSurfaces.clear();
}
