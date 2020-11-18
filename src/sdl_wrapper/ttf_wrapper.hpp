#pragma once

#ifndef __TTF_WRAPPER_HPP
#define __TTF_WRAPPER_HPP

#include <memory>
#include <stdexcept>
#include <string>

#include "sdl_wrapper.h"
#include "SDL_ttf.h"

namespace sdl_wrapper {
    using std::string_literals::operator""s;

    /**
     * @brief TTF init/quit wrapper class. Declare a variable of this type
     * in the scope that needs to work with SDL TTF.
     */
    class SDLTTF {
    private:
        SDLTTF(const SDLTTF&);
        SDLTTF& operator=(const SDLTTF&);
    public:
        SDLTTF() {
            if (TTF_Init() < 0) {
                throw std::runtime_error("Can't init TTF! TTF_Error: "s + TTF_GetError());
            }
        }
        ~SDLTTF() {
            TTF_Quit();
        }
    };
/*
TTF_Font *font;
font=TTF_OpenFont("font.ttf", 16);
if(!font) {
    printf("TTF_OpenFont: %s\n", TTF_GetError());
    // handle error
}
*/
    /**
     * @brief Wraps a TTF font object.
     */
    class TTFFont {
    private:
        friend class TTFFont;
        class Deleter {
        public:
            typedef TTFFont *pointer;
            void operator()(pointer t) {
                if (t == nullptr) {
                    return;
                }

                TTF_CloseFont(t);
            }
        };
        std::unique_ptr<TTF_Font, Deleter> m_font;

        TTFFont(const TTFFont&);
        TTFFont& operator=(const TTFFont&);
    public:
        TTFFont(TTF_Font* font) :
            m_font(font)
        {
        }

        ~TTFFont() = default;

        /**
         * @brief Loads font from file.
         *
         * @param file The true-type font file.
         * @param ptsize The point size.
         *
         * @return An instance of font object.
         */
        static TTFFont open(const char *file, int ptsize) {
            auto font = TTF_OpenFont(file, ptsize);

            if(font == nullptr) {
                throw std::runtime_error("Could not load font! TTF_Error: "s + TTF_GetError());
            }

            return TTFFont(font);
        }

        struct GlyphMetrics {
            int minx, maxx, miny, maxy, advance
        };

        /**
         * @brief Gets metrics for a glyph.
         *
         * @param ch Glyph to measure.
         *
         * @return Measures.
         */
        GlyphMetrics glyph_metrics(Uint16 ch) {
            GlyphMetrics metrics;

            if (TTF_GlyphMetrics(m_font, ch,
                    &metrics.minx,
                    &metrics.maxx,
                    &metrics.miny,
                    &metrics.maxy,
                    &metrics.advance) < 0)
                throw std::runtime_error("Could not get metrics! TTF_Error: "s + TTF_GetError());
            }

            return metrics;
        }

        /**
         * @brief Renders a single glyph in solid color.
         *
         * @param ch The glyph to render.
         * @param fg The font color.
         *
         * @return The rendered surface.
        */
        SDLSurface render_glyph_solid(Uint16 ch, SDL_Color fg) {
            auto surface = TTF_RenderGlyph_Solid(m_font, ch, fg);

            if(surface == nullptr) {
                throw std::runtime_error("Failure in glyph render! TTF_Error: "s + TTF_GetError());
            }

            return SDLSurface(surface);
        }

    };

}

#endif
