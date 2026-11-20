#pragma once
#include <string>
#include <vector>

namespace shop {
namespace ui {

// Frame-based console animations. Each one degrades to a single static
// frame when animations are disabled (--no-anim or a non-VT terminal).
namespace anim {

void pause(int ms);
// Prints text one character at a time (ANSI codes are emitted atomically).
void typewriter(const std::string& text, int msPerChar = 14, int indent = 2);
// "●∙∙ label" spinner for durationMs, then a green check mark.
void spinner(const std::string& label, int durationMs, int indent = 2);
// Runs a spinner for each step in turn (used while a payment "processes").
void steps(const std::vector<std::string>& labels, int msEach, int indent = 2);
// Filling gradient progress bar with a percentage.
void progressBar(const std::string& label, int durationMs, int width = 36, int indent = 2);
// Big check mark drawn stroke by stroke, then a success banner.
void successBurst(const std::string& title, const std::string& subtitle);
// Big cross that shakes, then a failure banner.
void failureShake(const std::string& title, const std::string& subtitle);
// Short colour wipe used between screens.
void wipe();

}  // namespace anim
}  // namespace ui
}  // namespace shop
