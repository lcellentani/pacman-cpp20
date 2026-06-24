module;
#include "imgui.h"
#include <string>
#include <string_view>

module game.console;

import engine.log;
import game.types;

namespace {

ImU32 color_for(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::Trace: return IM_COL32(150, 150, 150, 255);
    case LogLevel::Info:  return IM_COL32(220, 220, 220, 255);
    case LogLevel::Warn:  return IM_COL32(240, 200,  80, 255);
    case LogLevel::Error: return IM_COL32(240,  90,  90, 255);
    }
    return IM_COL32(220, 220, 220, 255);
}

const char* tag_for(LogLevel lvl) {
    switch (lvl) {
    case LogLevel::Trace: return "TRC";
    case LogLevel::Info:  return "INF";
    case LogLevel::Warn:  return "WRN";
    case LogLevel::Error: return "ERR";
    }
    return "   ";
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back()  == ' ' || s.back()  == '\t')) s.remove_suffix(1);
    return s;
}

} // namespace

void ConsoleView::draw(const GameState& game_state) {
    if (!visible_) return;

    ImGui::SetNextWindowPos({ (float)LAYOUT_CONSOLE_X, (float)LAYOUT_CONSOLE_Y }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ (float)LAYOUT_CONSOLE_W, (float)LAYOUT_CONSOLE_H }, ImGuiCond_Once);

    if (!ImGui::Begin("Console")) {
        ImGui::End();
        return;
    }

    // Top row: level filters + text filter + actions
    const char* labels[4] { "Trace", "Info", "Warn", "Error" };
    for (int i = 0; i < 4; ++i) {
        ImGui::Checkbox(labels[i], &show_level_[i]);
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(180.f);
    ImGui::InputTextWithHint("##filter", "filter...", filter_, sizeof(filter_));
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        Log::instance().clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);

    ImGui::Separator();

    // Scroll region — leave room for the command input line below
    const float input_h = ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("scroll", { 0, -input_h }, true,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        const auto entries = Log::instance().entries();
        const bool has_filter = filter_[0] != '\0';
        ImDrawList* dl = ImGui::GetWindowDrawList();
        (void)dl;
        for (const auto& e : entries) {
            const int li = static_cast<int>(e.level);
            if (li < 0 || li > 3 || !show_level_[li]) continue;
            if (has_filter && e.message.find(filter_) == std::string::npos) continue;

            ImGui::PushStyleColor(ImGuiCol_Text, color_for(e.level));
            ImGui::Text("[%7.2f] %s  %s", e.time_s, tag_for(e.level), e.message.c_str());
            ImGui::PopStyleColor();
        }

        if (scroll_to_bottom_ ||
            (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)) {
            ImGui::SetScrollHereY(1.0f);
        }
        scroll_to_bottom_ = false;
    }
    ImGui::EndChild();

    // Command input
    ImGui::SetNextItemWidth(-1);
    const bool entered = ImGui::InputText("##cmd", input_, sizeof(input_),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
    if (entered) {
        std::string_view cmd = trim(input_);
        if (!cmd.empty()) {
            log_info(std::string{ "> " } + std::string{ cmd });
            dispatch_command(cmd, game_state);
            scroll_to_bottom_ = true;
        }
        input_[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void ConsoleView::dispatch_command(std::string_view cmd, const GameState& game_state) {
    cmd = trim(cmd);
    // First whitespace-delimited token
    auto sp = cmd.find_first_of(" \t");
    std::string_view head = (sp == std::string_view::npos) ? cmd : cmd.substr(0, sp);

    if (head == "clear") {
        Log::instance().clear();
        return;
    }
    if (head == "dots_eaten") {
        log_info("dots_eaten = " + std::to_string(game_state.dots_eaten));
        return;
    }
    if (head == "dot_timer") {
        log_info("dot_timer = " + std::to_string(game_state.dot_timer));
        return;
    }
    if (head == "next_force_release") {
        if (game_state.next_force_release.has_value()) {
            log_info("next_force_release = " + std::to_string(static_cast<int>(game_state.next_force_release.value())));
        } else {
            log_info("next_force_release = <none>");
        }
        return;
    }
    log_warn(std::string{ "unknown command: " } + std::string{ head });
}
