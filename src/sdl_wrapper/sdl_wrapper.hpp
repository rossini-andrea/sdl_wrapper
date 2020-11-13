#pragma once
#include <memory>
#include <functional>
#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace sdl_wrapper {
    using std::string_literals::operator""s;

    /**
     * @brief SDL init/quit wrapper class. Declare a variable of this type
     * in the scope that needs to work with SDL.
     */
    class SDL {
    private:
        SDL(const SDL&);
        SDL& operator=(const SDL&);
    public:
        SDL() {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                throw std::runtime_error("SDL uninitialized"s);
            }
        }
        ~SDL() {
            SDL_Quit();
        }

        /**
         * @brief Sets an SDL hint. (https://wiki.libsdl.org/CategoryHints#Hints)
         */
        bool set_hint(const char* name,
                     const char* value) const {
            return SDL_SetHint(name, value);
        }
    };

    /**
     * @brief SDL_IMG init/quit wrapper class. Declare a variable of this type
     * in the scope that needs to work with SDL.
     */
    class SDL_IMG {
    private:
        SDL_IMG(const SDL_IMG&);
        SDL_IMG& operator=(const SDL_IMG&);
    public:
        SDL_IMG() {
            int flags = IMG_INIT_PNG;

            if (!(IMG_Init(flags) & flags)) {
                throw std::runtime_error("IMG_Init failed! IMG_GetError: "s + IMG_GetError());
            }
        }
        ~SDL_IMG() {
            IMG_Quit();
        }
    };

    /**
     * @brief A software accessible image, stored in RAM.
     */
    class SDLSurface {
    private:
        friend class SDLRenderer;
        class Deleter {
        public:
            typedef SDL_Surface *pointer;
            void operator()(pointer t) {
                if (t == nullptr) {
                    return;
                }

                SDL_FreeSurface(t);
            }
        };
        std::unique_ptr<SDL_Surface, Deleter> m_surface;

        SDLSurface(const SDLSurface&);
        SDLSurface& operator=(const SDLSurface&);
    public:
        SDLSurface(SDL_Surface* surface) :
            m_surface(surface)
        {
        }

        SDLSurface(Uint32 flags,
            int    width,
            int    height,
            int    depth,
            Uint32 r_mask,
            Uint32 g_mask,
            Uint32 b_mask,
            Uint32 a_mask)
        {
            m_surface.reset(SDL_CreateRGBSurface(flags, width, height, depth,
                                    r_mask, g_mask, b_mask, a_mask));

            if (m_surface == nullptr) {
                throw std::runtime_error("SDL_CreateRGBSurface failed! SDL_Error: "s + SDL_GetError());
            }
        }

        ~SDLSurface() = default;

        /**
         * @brief Loads a surface from a Windows BMP.
         */
        static SDLSurface from_bmp(const char* file) {
            auto bmp = SDL_LoadBMP(file);

            if (bmp == nullptr) {
                throw std::runtime_error("SDL_LoadBMP failed! SDL_Error: "s + SDL_GetError());
            }

            return SDLSurface(bmp);
        }

        /**
         * @brief Loads a surface from any image format.
         */
        static SDLSurface from_image(const char* file) {
            auto img = IMG_Load(file);

            if (img == nullptr) {
                throw std::runtime_error("IMG_Load failed! IMG_Error: "s + IMG_GetError());
            }

            return SDLSurface(img);
        }

        /**
         * @brief Copy pixel data to a destination surface.
         */
        void blit_to(const SDLSurface &destination) const {
            SDL_BlitSurface(m_surface.get(), nullptr, destination.m_surface.get(), nullptr);
        }

        /**
         * @brief Locks this image. Can be used with `std` locking functions
         * and classes.
         */
        void lock() {
            if (SDL_LockSurface(m_surface.get()) != 0) {
                throw std::runtime_error("SDL_LockSurface failed! SDL_Error: "s + SDL_GetError());
            }
        }

        /**
         * @brief Unlock this image. Can be used with `std` locking functions
         * and classes.
         */
        void unlock() {
            SDL_UnlockSurface(m_surface.get());
        }

        /**
         * @brief Attempts to lock this image and returns the result. Can be
         * used with `std` locking functions and classes.
         */
        bool try_lock() {
            return SDL_LockSurface(m_surface.get()) == 0;
        }

        /**
         * @brief Retrieves the raw pointer to the underlying SDL object.
         */
        const SDL_Surface* get() const {
            return m_surface.get();
        }
    };

    /**
     * @brief A GPU accessible image, stored on graphics hardware.
     */
    class SDLTexture {
        friend class SDLRenderer;
        class Deleter {
        public:
            typedef SDL_Texture *pointer;
            void operator()(SDL_Texture *t) {
                if (t == nullptr) {
                    return;
                }

                SDL_DestroyTexture(t);
            }
        };
    private:
        std::unique_ptr<SDL_Texture, Deleter> m_texture;

    public:
        SDLTexture(SDL_Texture* texture) :
            m_texture(texture)
        {
        }
        SDLTexture(SDLTexture&&) = default;
        SDLTexture& operator=(SDLTexture&&) = default;
        ~SDLTexture() = default;

        /**
         * @brief Retrieves info about this texture.
         */
        void query(Uint32 *format, int *access, int *w, int *h) {
            SDL_QueryTexture(m_texture.get(), format, access, w, h);
        }
    };

    /**
     * @brief An SDL renderer.
     */
    class SDLRenderer {
        class Deleter {
        public:
            typedef SDL_Renderer *pointer;
            void operator()(SDL_Renderer *r) {
                if (r == nullptr) {
                    return;
                }

                SDL_DestroyRenderer(r);
            }
        };

        std::unique_ptr<SDL_Renderer, Deleter> m_renderer;

    public:
        SDLRenderer(SDL_Renderer* renderer) :
            m_renderer(renderer)
        {
        }

        ~SDLRenderer() = default;
        SDLRenderer(SDLRenderer&& other) = default;
        SDLRenderer& operator=(SDLRenderer&&) = default;

        /**
         * @brief Clears the render buffer.
         */
        void clear() const {
            SDL_RenderClear(m_renderer.get());
        }

        /**
         * @brief Flips render buffer to on screen buffer.
         */
        void present() const {
            SDL_RenderPresent(m_renderer.get());
        }

        /**
         * @brief Creates an empty texture.
         */
        SDLTexture create_texture(Uint32 format, int access, int w, int h) const {
            return SDLTexture(SDL_CreateTexture(m_renderer.get(), format, access, w, h));
        }

        /**
         * @brief Creates a texture from a RAM stored surface.
         */
        SDLTexture create_texture_from_surface(const SDLSurface &surface) const {
            return SDLTexture(SDL_CreateTextureFromSurface(m_renderer.get(), surface.m_surface.get()));
        }

        /**
         * @brief Set the virtual size of the screen buffer, useful for faking
         * low resulutions.
         */
        void set_logical_size(int w, int h) const {
            SDL_RenderSetLogicalSize(m_renderer.get(), w, h);
        }

        /**
         * @brief Sets color for direct draw operations.
         */
        void set_draw_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
            SDL_SetRenderDrawColor(m_renderer.get(), r, g, b, a);
        }

        /**
         * @brief Draws a line.
         */
        void draw_line(int x1, int y1, int x2, int y2) const {
            SDL_RenderDrawLine(m_renderer.get(), x1, y1, x2, y2);
        }

        /**
         * @brief Draws a rectoangle.
         */
        void draw_rect(const SDL_Rect* rect) const {
            SDL_RenderDrawRect(m_renderer.get(), rect);
        }

        /**
         * @brief Sets the new vieport.
         */
        void set_viewport(const SDL_Rect* rect) const {
            if (SDL_RenderSetViewport(m_renderer.get(), rect) < 0) {
                throw std::runtime_error("Could'nt set viewport! SDL_Error: "s + SDL_GetError());
            }
        }

        /**
         * @brief Redirect render operations to a texture.
         */
        void set_render_target(const SDLTexture &texture) const {
            SDL_SetRenderTarget(m_renderer.get(), texture.m_texture.get());
        }

        /**
         * @brief Resets the render target to the renderer itself.
         */
        void reset_render_target() const {
            SDL_SetRenderTarget(m_renderer.get(), nullptr);
        }

        /**
         * @brief Renders a texture on the current render target (wraps `SDL_RenderCopy`)
         */
        void draw_texture(const SDLTexture &texture,
            const SDL_Rect* srcrect, const SDL_Rect* dstrect) const {
            SDL_RenderCopy(m_renderer.get(), texture.m_texture.get(),
                srcrect, dstrect);
        }
    };

    /**
     * @brief An SDL window.
     */
    class SDLWindow {
    private:
        class Deleter {
        public:
            typedef SDL_Window *pointer;
            void operator()(SDL_Window *w) {
                if (w == nullptr) {
                    return;
                }

                SDL_DestroyWindow(w);
            }
        };

        std::unique_ptr<SDL_Window, Deleter> m_window;

        SDLWindow(const SDLWindow&);
        SDLWindow& operator=(const SDLWindow&);
    public:
        SDLWindow(const std::string &title, int width, int height, Uint32 flags) {
            m_window.reset(SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags));

            if (m_window == nullptr) {
                throw std::runtime_error("Window could not be created! SDL_Error: "s + SDL_GetError());
            }
        }
        ~SDLWindow() = default;

        SDLWindow(SDLWindow&&) = default;
        SDLWindow& operator=(SDLWindow&&) = default;

        /**
         * @brief Gets the software renderable surface of this window (don't use
         * if you want to use HW acceleration).
         */
        SDLSurface get_surface() const {
            return SDLSurface(SDL_GetWindowSurface(m_window.get()));
        }

        /**
         * @brief Signals the end of software render operations (don't use
         * if you want to use HW acceleration).
         */
        void update_surface() const {
            SDL_UpdateWindowSurface(m_window.get());
        }

        /**
         * @brief Creates the hardware accelerated (whenever possible) renderer
         * associated with this window.
         */
        SDLRenderer create_renderer(int index, Uint32 flags) const {
            return SDLRenderer(SDL_CreateRenderer(m_window.get(), index, flags));
        }
    };
}
