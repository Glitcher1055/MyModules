// Elytra <-> Chestplate swap button
// Minecraft Bedrock 1.21.30, arm64, LeviLauncher native .so mod
// Build: NDK r26+, -std=c++20, link with your mod loader's hook lib + ImGui.

#include <string>
#include <fstream>
#include <cstdint>
#include <cstring>

// ---------- Config (editable) ----------
// File: /sdcard/games/elytra_swap.cfg   (lines: key=value)
struct Config {
    float x = 20.f;      // button position, px from left
    float y = 200.f;     // px from top
    float size = 48.f;   // button size, px
    bool  enabled = true;
} g_cfg;

static void LoadConfig() {
    std::ifstream f("/sdcard/games/elytra_swap.cfg");
    std::string line;
    while (std::getline(f, line)) {
        auto p = line.find('=');
        if (p == std::string::npos) continue;
        std::string k = line.substr(0, p), v = line.substr(p + 1);
        if (k == "x") g_cfg.x = std::stof(v);
        else if (k == "y") g_cfg.y = std::stof(v);
        else if (k == "size") g_cfg.size = std::stof(v);
        else if (k == "enabled") g_cfg.enabled = (v == "1" || v == "true");
    }
}

// ---------- Game interface (TODO: fill from your loader / sig scan) ----------
// Minimal opaque types; real layouts come from 1.21.30 headers.
struct Player;
struct ItemStack;

enum ArmorSlot : int { Head = 0, Chest = 1, Legs = 2, Feet = 3 };

// TODO 1: Player::getArmor(ArmorSlot) -> const ItemStack&
const ItemStack* Game_GetArmor(Player* p, ArmorSlot s);
// TODO 2: Player inventory access: size, item at slot, item raw name ("minecraft:elytra")
int  Game_InventorySize(Player* p);
const ItemStack* Game_GetInventoryItem(Player* p, int slot);
std::string Game_ItemName(const ItemStack* it);   // empty string if air/null
// TODO 3: send ItemStackRequestPacket with Swap action:
//   src = (Inventory container, slot), dst = (Armor container, slot 1)
void Game_SendSwap(Player* p, int inventorySlot, ArmorSlot armorSlot);

// ---------- Logic ----------
static bool IsElytra(const std::string& n)     { return n == "minecraft:elytra"; }
static bool IsChestplate(const std::string& n) {
    static const char* suf = "_chestplate";
    return n.size() > strlen(suf) &&
           n.compare(n.size() - strlen(suf), strlen(suf), suf) == 0;
}

void DoSwap(Player* p) {
    if (!p || !g_cfg.enabled) return;

    std::string worn = Game_ItemName(Game_GetArmor(p, ArmorSlot::Chest));
    bool wearingElytra = IsElytra(worn);

    // Look for the opposite item in the inventory.
    for (int i = 0, n = Game_InventorySize(p); i < n; ++i) {
        std::string name = Game_ItemName(Game_GetInventoryItem(p, i));
        if (name.empty()) continue;
        bool match = wearingElytra ? IsChestplate(name) : IsElytra(name);
        if (match) {
            Game_SendSwap(p, i, ArmorSlot::Chest);
            return;
        }
    }
    // Nothing found -> do nothing.
}

// ---------- UI (ImGui overlay, called every frame from your render hook) ----------
#include "imgui.h"

extern Player* g_localPlayer;   // TODO: set from a Player tick hook

void RenderSwapButton() {
    if (!g_cfg.enabled || !g_localPlayer) return;
    ImGui::SetNextWindowPos({g_cfg.x, g_cfg.y});
    ImGui::SetNextWindowSize({g_cfg.size, g_cfg.size});
    ImGui::Begin("##elytra_swap", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button("E<>C", {g_cfg.size, g_cfg.size}))
        DoSwap(g_localPlayer);
    ImGui::End();
}

// ---------- Entry ----------
extern "C" __attribute__((visibility("default"))) void mod_init() {
    LoadConfig();
    // TODO: install hooks: render -> RenderSwapButton(), Player tick -> g_localPlayer
}
