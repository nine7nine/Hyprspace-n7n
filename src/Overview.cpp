#include "Overview.hpp"
#include "Globals.hpp"

CHyprspaceWidget::CHyprspaceWidget(uint64_t inOwnerID) {
    ownerID = inOwnerID;

    curAnimationConfig = *g_pConfigManager->getAnimationPropertyConfig("windows");

    // the fuck is pValues???
    curAnimation = *curAnimationConfig.pValues;
    curAnimationConfig.pValues = &curAnimation;

    if (Config::overrideAnimSpeed > 0)
        curAnimation.internalSpeed = Config::overrideAnimSpeed;

    curYOffset.create(&curAnimationConfig, AVARDAMAGE_ENTIRE);
    workspaceScrollOffset.create(&curAnimationConfig, AVARDAMAGE_ENTIRE);
    curYOffset.setValueAndWarp(Config::panelHeight);
    workspaceScrollOffset.setValueAndWarp(0);
}

// TODO: implement deconstructor and delete widget on monitor unplug
CHyprspaceWidget::~CHyprspaceWidget() {}

CMonitor* CHyprspaceWidget::getOwner() {
    return g_pCompositor->getMonitorFromID(ownerID);
}

void CHyprspaceWidget::show() {
    auto owner = getOwner();
    if (!owner) return;

    if (prevFullscreen.empty()) {
        // unfullscreen all windows
        for (auto& ws : g_pCompositor->m_vWorkspaces) {
            if (ws->m_iMonitorID == ownerID) {
                const auto w = g_pCompositor->getFullscreenWindowOnWorkspace(ws->m_iID);
                if (w != nullptr && ws->m_efFullscreenMode != FULLSCREEN_INVALID) {
                    // use fakefullscreenstate to preserve client's internal state
                    // fixes youtube fullscreen not restoring properly
                    if (ws->m_efFullscreenMode == FULLSCREEN_FULL) w->m_bFakeFullscreenState = true;
                    // we use the getWindowFromHandle function to prevent dangling pointers
                    prevFullscreen.emplace_back(std::make_tuple((uint32_t)(((uint64_t)w.get()) & 0xFFFFFFFF), ws->m_efFullscreenMode));
                    g_pCompositor->setWindowFullscreen(w, false);
                }
            }
        }
    }

    active = true;

    // panel offset should be handled by swipe event when swiping
    if (!swiping) {
        curYOffset = 0;
        curSwipeOffset = (Config::panelHeight + Config::reservedArea) * owner->scale;
    }

    updateLayout();
    g_pHyprRenderer->damageMonitor(owner);
    g_pCompositor->scheduleFrameForMonitor(owner);
}

void CHyprspaceWidget::hide() {
    auto owner = getOwner();
    if (!owner) return;

    oLayerAlpha.clear();

    // restore fullscreen state
    for (auto& fs : prevFullscreen) {
        const auto w = g_pCompositor->getWindowFromHandle(std::get<0>(fs));
        const auto oFullscreenMode = std::get<1>(fs);
        g_pCompositor->setWindowFullscreen(w, true, oFullscreenMode); 
        if (oFullscreenMode == FULLSCREEN_FULL) w->m_bFakeFullscreenState = false;
    }
    prevFullscreen.clear();

    active = false;

    // panel offset should be handled by swipe event when swiping
    if (!swiping) {
        curYOffset = (Config::panelHeight + Config::reservedArea) * owner->scale;
        curSwipeOffset = -10.;
    }

    updateLayout();
    g_pCompositor->scheduleFrameForMonitor(owner);
}

void CHyprspaceWidget::updateConfig() {
    curAnimationConfig = *g_pConfigManager->getAnimationPropertyConfig("windows");

    curAnimation = *curAnimationConfig.pValues;
    curAnimationConfig.pValues = &curAnimation;

    if (Config::overrideAnimSpeed > 0)
        curAnimation.internalSpeed = Config::overrideAnimSpeed;

    curYOffset.create(&curAnimationConfig, AVARDAMAGE_ENTIRE);
    workspaceScrollOffset.create(&curAnimationConfig, AVARDAMAGE_ENTIRE);
}

bool CHyprspaceWidget::isActive() {
    return active;
}
