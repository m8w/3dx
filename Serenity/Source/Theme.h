#pragma once
#include <juce_graphics/juce_graphics.h>
#include <array>

// Theme spectrum: dark monochrome house style -> high-contrast funhouse,
// mirrored from original-html/*.html's THEMES array (see e.g. shift.html).
struct Palette
{
    const char* name;
    juce::Colour bg, panel, line, fg, dim, faint, acc, acc2, acc3;
};

inline const std::array<Palette, 5>& themes()
{
    static const std::array<Palette, 5> t { {
        { "VOID",     juce::Colour(0xff0a0a0c), juce::Colour(0xff101014), juce::Colour(0xff1d1d24),
          juce::Colour(0xffc9c9ce), juce::Colour(0xff5c5c66), juce::Colour(0xff33333c),
          juce::Colour(0xffffb347), juce::Colour(0xff47d7ff), juce::Colour(0xffffe66d) },
        { "DUSK",     juce::Colour(0xff0d0a14), juce::Colour(0xff171226), juce::Colour(0xff2a1f3d),
          juce::Colour(0xffe8e2f5), juce::Colour(0xff8a7ba3), juce::Colour(0xff4a3d63),
          juce::Colour(0xffff6ec7), juce::Colour(0xff34e5c8), juce::Colour(0xffffd23f) },
        { "NEON",     juce::Colour(0xff0a0014), juce::Colour(0xff150726), juce::Colour(0xff2d0f4d),
          juce::Colour(0xfff5eaff), juce::Colour(0xffa684c9), juce::Colour(0xff5c2e8a),
          juce::Colour(0xffff2e88), juce::Colour(0xff00f0ff), juce::Colour(0xffc6ff00) },
        { "CANDY",    juce::Colour(0xfffff4fb), juce::Colour(0xffffffff), juce::Colour(0xffffd1ec),
          juce::Colour(0xff3a1249), juce::Colour(0xffa15fb3), juce::Colour(0xffffe3f5),
          juce::Colour(0xffff3d81), juce::Colour(0xff00c2ff), juce::Colour(0xffffbe0b) },
        { "FUNHOUSE", juce::Colour(0xfffff9e6), juce::Colour(0xffffffff), juce::Colour(0xffff006e),
          juce::Colour(0xff14003d), juce::Colour(0xffff5400), juce::Colour(0xffffd400),
          juce::Colour(0xffff006e), juce::Colour(0xff3a86ff), juce::Colour(0xff06ffa5) },
    } };
    return t;
}

// Implemented by any editor component whose colours depend on the active
// theme, so the editor can push a new Palette down the whole tree on
// selection without recreating components.
struct Themeable
{
    virtual ~Themeable() = default;
    virtual void setPalette(const Palette&) = 0;
};
